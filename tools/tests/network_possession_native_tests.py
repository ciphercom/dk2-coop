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
        for name, offset in (("page", 0xAD + 2 * 4), ("pageSize", 0xDD + 2 * 4)):
            match = re.search(rf"memcpy\(&{name}, bytes \+ (0x[\dA-Fa-f]+),", source)
            self.assertIsNotNone(match, name)
            self.assertEqual(int(match[1], 16), offset, f"incorrect native spell {name} field")


if __name__ == "__main__":
    unittest.main()
