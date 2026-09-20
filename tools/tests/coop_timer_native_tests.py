"""Keep campaign countdown events while bypassing native multiplayer timeout defeat."""
import unittest

import network_hands_native_tests as native


class CampaignTimerContracts(unittest.TestCase):
    """Verify the hook is after countdown/expiry updates and before the defeat branch."""

    def test_expiry_preserves_script_event_and_scopes_defeat(self):
        native.NativeHandContracts.setUpClass()
        instructions = {i.address: (i.mnemonic, i.op_str)
                        for i in native.NativeHandContracts.instructions(0x50A450)}
        expected = {
            0x50A65C: ("dec", "eax"),
            0x50A65F: ("mov", "dword ptr [esi + 0x72aa], eax"),
            0x50A671: ("or", "dl, 8"),
            0x50A674: ("mov", "byte ptr [esi + 0x72a1], dl"),
            0x50A67A: ("mov", "eax, dword ptr [0x75ad4b]"),
            0x50A67F: ("cmp", "eax, 2"),
            0x50A684: ("cmp", "eax, 3"),
            0x50A687: ("jne", "0x50a745"),
            0x50A6AF: ("push", "3"),
            0x50A6B3: ("call", "0x4be630"),
        }
        for address, instruction in expected.items():
            self.assertEqual(instructions[address], instruction, hex(address))
        source = (native.ROOT / "src/patches/coop_campaign_init.cpp").read_text()
        self.assertRegex(source, r"\{0x0050A67A, eaxRead, sizeof\(eaxRead\), modeForInitialization\}",
                         "campaign countdown still enters native multiplayer defeat")


if __name__ == "__main__":
    unittest.main()
