import argparse
import io
import pathlib
import sgmap
from dk2cxx import *
from sgmap import PtrType


def guess_struct_name(name: str, struct_names: set[str]):
  name = name.replace('::', '_')
  while True:
    spl = name.rsplit('_', 1)
    if len(spl) != 2:
      break
    name, _ = spl
    if name in struct_names:
      return name
  return None

def get_struct_name(fun_t: sgmap.FunctionType):
  this = fun_t.args[0]
  if this.kind == sgmap.TypeKind.Ptr:
    ptr_t: sgmap.PtrType = this
    if ptr_t.type.kind == sgmap.TypeKind.Struct:
      struct_t: sgmap.StructType = ptr_t.type
      return struct_t.struct.name
  return None


def main(sgmap_file: pathlib.Path, replace_globals_file: pathlib.Path):
  structs, globals = sgmap.parse_file(sgmap_file)
  globals_map: dict[int, sgmap.Global] = {g.va: g for g in globals}
  members_map: dict[int, sgmap.Struct] = {m.va: s for s in structs for m in s.functions}
  struct_names: set[str] = {s.name for s in structs}
  with open(replace_globals_file, 'r') as f:
    lines = f.readlines()

  def visit(va: int, desc: str) -> tuple[str, int, str]:
    glob: sgmap.Global = globals_map[va]
    if glob.type.kind is sgmap.TypeKind.Function:
      fun_t = glob.type  # type: sgmap.FunctionType
      struct_name = None
      if fun_t.declspec == sgmap.Declspec.Thiscall:
        structs = members_map.get(va)
        if structs is not None:
          struct_name = structs.name
        if not struct_name:
          struct_name = get_struct_name(fun_t)
      if struct_name is not None:
        return struct_name, va, format_function(fun_t, f'{struct_name}::{glob.name}')
      if not struct_name:
        struct_name = guess_struct_name(glob.name, struct_names)
      if struct_name is not None:
        return struct_name, va, format_function(fun_t, glob.name)
      return 'functions', va, format_function(fun_t, glob.name)
    return 'globals', va, format_type(glob.type, glob.name)

  to_replace: dict[str, list[tuple[int, str, str]]] = {}
  for ln in lines:
    line = ln.rstrip()
    if not line or line.startswith('#') or line.startswith('//'):
      if line.startswith('#00'):
        va, desc = line[1:].split(' ', 1)
        va = int(va, 16)
        group, va, desc = visit(va, desc)
        to_replace.setdefault(group, []).append((va, desc, 'd'))
      continue
    va, desc = line.split(' ', 1)
    va = int(va, 16)
    group, va, desc = visit(va, desc)
    to_replace.setdefault(group, []).append((va, desc, ''))

  to_replace_list = [(group, records) for group, records in to_replace.items()]
  for group, records in to_replace_list:
    records.sort(key=lambda r: r[0])
  to_replace_list.sort(key=lambda t: t[1][0][0])

  with io.StringIO() as o:
    for group, records in to_replace_list:
      o.write(f'# {group}\n')
      for va, desc, tag in records:
        if tag == 'd':
          o.write(f'#{va:08X} {desc}\n')
        else:
          o.write(f'{va:08X} {desc}\n')
      o.write(f'\n')
    content = o.getvalue()
  # print(content)
  replace_globals_file.write_text(content)


def start():
  parser = argparse.ArgumentParser(description='Optional app description')
  parser.add_argument('-sgmap_file', type=str, required=True)
  parser.add_argument('-replace_globals', type=str, required=True)
  args = parser.parse_args()
  main(
    pathlib.Path(args.sgmap_file),
    pathlib.Path(args.replace_globals),
  )


if __name__ == '__main__':
  start()
