"""Verify possession's panel field reads against the original DKII paging callback."""
import re
import struct
import unittest

import network_hands_native_tests as native


class NativePossessionContracts(unittest.TestCase):
    """Keep the UI adapter connected to the native page counter and page capacity."""

    def test_local_movement_boundary_uses_ecx_and_no_stack_arguments(self):
        """Generated cdecl metadata must not lose the Controller carried in ECX."""
        native.NativeHandContracts.setUpClass()
        data = native.NativeHandContracts.data
        self.assertEqual(data(0x406777, 7), bytes.fromhex("8bcee892810000"))
        self.assertEqual(data(0x40E910, 10), bytes.fromhex("558bec6aff68d8926400"))
        self.assertEqual(data(0x40E92C, 2), bytes.fromhex("8bf1"))
        self.assertEqual(data(0x40F67C, 1), b"\xc3")
        self.assertEqual(data(0x40677E, 5), bytes.fromhex("e90e060000"))
        callers = {ins.address for address in native.NativeHandContracts.functions
                   for ins in native.NativeHandContracts.instructions(address)
                   if ins.mnemonic == "call" and ins.op_str == "0x40e910"}
        self.assertEqual(callers, {0x406779})
        replacements = (native.ROOT / "src/replace_globals.txt").read_text()
        self.assertIn("0040E910 void * __cdecl CDefaultPlayerInterface_sub_40E910()", replacements)

    def test_local_probe_sets_capability_inherited_by_ai_terrain_gate(self):
        """Pin the actual leaked context and terrain consumer, avoiding guessed creature flags."""
        native.NativeHandContracts.setUpClass()
        instructions = native.NativeHandContracts.instructions
        probe = {i.address: (i.mnemonic, i.op_str) for i in instructions(0x40F6B0)}
        self.assertEqual(probe[0x40F720], ("mov", "dword ptr [0x6ec9e4], edi"))
        self.assertEqual(probe[0x40F728], ("mov", "dword ptr [0x6ec9e4], 1"))
        ai_writes = {i.op_str.split(",")[0] for i in instructions(0x4D5A40)
                     if i.mnemonic == "mov" and i.op_str.startswith("dword ptr [0x6ec9")}
        self.assertEqual(ai_writes, {f"dword ptr [0x{address:x}]"
                                    for address in (0x6EC9D0, 0x6EC9D4, 0x6EC9D8, 0x6EC9DC, 0x6EC9E0)})
        callback = struct.unpack("<I", native.NativeHandContracts.data(0x6A11D0, 4))[0]
        self.assertEqual(callback, 0x4C8BD0)
        terrain = {i.address: (i.mnemonic, i.op_str) for i in instructions(callback)}
        self.assertEqual(terrain[0x4C8CBE], ("mov", "edx, dword ptr [0x6ec9e4]"))
        self.assertEqual(terrain[0x4C8CC6], ("jne", "0x4c8ce0"))

    def test_spell_page_fields_match_native_scroll(self):
        native.NativeHandContracts.setUpClass()
        instructions = {i.address: (i.mnemonic, i.op_str)
                        for i in native.NativeHandContracts.instructions(0x417730)}
        # Spell arrows pass category 2. Native scrolling increments AD+category*4,
        # and divides the remaining entries by DD+category*4 to bound that counter.
        self.assertEqual(instructions[0x41773E], ("mov", "esi, dword ptr [eax + ecx*4 + 0xad]"))
        self.assertEqual(instructions[0x41774D], ("add", "esi, edx"))
        self.assertEqual(instructions[0x41775A], ("mov", "esi, dword ptr [eax + ecx*4 + 0xdd]"))
        self.assertEqual(instructions[0x417771], ("div", "esi"))
        source = (native.ROOT / "src/dk2/gui/game/active_panel/win_keeper_spells.cpp").read_text()
        for name, offset in (("page", 0xAD + 2 * 4), ("scrollStep", 0xDD + 2 * 4),
                             ("visibleSlots", 0x7D + 2 * 4)):
            match = re.search(rf"memcpy\(&{name}, bytes \+ (0x[\dA-Fa-f]+),", source)
            self.assertIsNotNone(match, name)
            self.assertEqual(int(match[1], 16), offset, f"incorrect native spell {name} field")

    def test_visible_panel_capacity_is_distinct_from_scroll_step(self):
        """A multi-column panel must lock possession beyond its first column too."""
        native.NativeHandContracts.setUpClass()
        layout = {i.address: (i.mnemonic, i.op_str)
                  for i in native.NativeHandContracts.instructions(0x417470)}
        self.assertEqual(layout[0x417666], ("imul", "ebx, edi"))
        self.assertEqual(layout[0x417670], ("mov", "dword ptr [ebp + eax*4 + 0xdd], edx"))
        self.assertEqual(layout[0x417677], ("mov", "dword ptr [ebp + eax*4 + 0x7d], ebx"))
        refresh = {i.address: (i.mnemonic, i.op_str)
                   for i in native.NativeHandContracts.instructions(0x4113B0)}
        self.assertEqual(refresh[0x4113F1], ("mov", "edx, dword ptr [esi + 0x85]"))
        self.assertEqual(refresh[0x4113F9], ("cmp", "ebx, edx"))


if __name__ == "__main__":
    unittest.main()
