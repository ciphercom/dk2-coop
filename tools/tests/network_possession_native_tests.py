"""Verify possession's panel field reads against the original DKII paging callback."""
import re
import unittest

import network_hands_native_tests as native


class NativePossessionContracts(unittest.TestCase):
    """Keep the UI adapter connected to the native page counter and page capacity."""

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
