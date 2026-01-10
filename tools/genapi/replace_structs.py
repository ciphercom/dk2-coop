import pathlib

TO_REPLACE: dict[str, str] = {}
BY_NAMESPACE: dict[str, dict[str, str]] = {}

def init(replace_structs_file: pathlib.Path):
  TO_REPLACE.clear()
  BY_NAMESPACE.clear()
  with open(replace_structs_file, 'r') as f:
    for line in f.readlines():
      line = line.rstrip()
      if not line or line.startswith('#'):
        continue
      fr, to = line.split(' ', 1)
      namespace, name = to.rsplit('::', 1)
      TO_REPLACE[fr] = to
      BY_NAMESPACE.setdefault(namespace, {})[fr] = name


def gen_struct_h_header(empty_line: str, ref_types: set[str]):
  for ns, ns_replaces in BY_NAMESPACE.items():
    if any(filter(lambda n: n in ns_replaces, ref_types)):
      yield empty_line
      yield f"namespace {ns} {{"
      for fr, to in ns_replaces.items():
        if fr not in ref_types:
          continue
        yield f"  class {to};"
      yield f"}}  // namespace {ns}"


def gen_struct_h_is_replaced(name):
  return name in TO_REPLACE


def get_replace_name(struct_name):
  return TO_REPLACE.get(struct_name)


def is_replace_name(struct_name):
  return struct_name in TO_REPLACE


def gen_struct_h_gen_include(empty_line: str, struct_name: str, inc_path: str):
  rep: str = TO_REPLACE.get(struct_name)
  if rep is None:
    return
  rep.replace('::', '/')
  yield empty_line
  yield f"#include <{inc_path}>"

