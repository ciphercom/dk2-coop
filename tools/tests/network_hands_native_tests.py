"""Check the DKII 1.70 binary seams used by Controller-owned Hands without launching the game."""
from pathlib import Path
import re
import struct
import unittest

import capstone

ROOT = Path(__file__).resolve().parents[2]


class NativeHandContracts(unittest.TestCase):
    """Fail if a native reader or queued pickup layout no longer matches the hooks."""

    @classmethod
    def setUpClass(cls):
        cls.binary = (ROOT / "libs/dkii_exe/DKII.EXE").read_bytes()
        pe = struct.unpack_from("<I", cls.binary, 60)[0]
        cls.base = struct.unpack_from("<I", cls.binary, pe + 52)[0]
        count = struct.unpack_from("<H", cls.binary, pe + 6)[0]
        start = pe + 24 + struct.unpack_from("<H", cls.binary, pe + 20)[0]
        cls.sections = [struct.unpack_from("<IIII", cls.binary, start + i * 40 + 8) for i in range(count)]
        cls.decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        cls.functions = {}
        for line in (ROOT / "mapping/DKII_EXE_v170.sgmap").read_text().splitlines():
            match = re.match(r"global: va=([0-9A-F]+),name=([^,]+),size=(\d+)", line)
            if match:
                cls.functions[int(match[1], 16)] = (match[2], int(match[3]))

    @classmethod
    def data(cls, address, size):
        rva = address - cls.base
        for _, section_rva, raw_size, offset in cls.sections:
            if section_rva <= rva < section_rva + raw_size:
                begin = offset + rva - section_rva
                return cls.binary[begin:begin + size]
        return b""

    @classmethod
    def instructions(cls, address):
        return list(cls.decoder.disasm(cls.data(address, cls.functions[address][1]), address))

    def test_original_entry_guards(self):
        for address, expected in {
            0x4BC500: "83ec0c33c0", 0x4C09F0: "538b5c2408",
            0x4B8D40: "83ec108d442400", 0x4B9250: "83ec108d442400",
        }.items():
            self.assertEqual(self.data(address, len(bytes.fromhex(expected))), bytes.fromhex(expected), hex(address))

    def test_network_pickups_hook_the_handlers_the_tick_calls_directly(self):
        # Both peers crashed at N39+2B with EAX=0x21C6: origin 2 was still in creature tag 0x1C6.
        # GameActionHandler_handle138 calls the handler table directly, bypassing CWorld::callActionHandler.
        replacements = (ROOT / "src/replace_globals.txt").read_text()
        for address, name in {
            0x513480: "GameActionHandler_N39", 0x5134E0: "GameActionHandler_N3A",
            0x5135B0: "GameActionHandler_N3B", 0x513600: "GameActionHandler_N3C",
        }.items():
            self.assertTrue(re.search(rf"(?m)^{address:08X} int __stdcall {name}\(GameAction \*\)$", replacements),
                            f"Native pickup handler {name} at {address:08X} is not replaced")

    def test_every_native_ui_world_hand_reader_is_filtered(self):
        observed = set()
        for address in self.functions:
            # handleRightClick is recompiled and uses an explicit local query.
            if not 0x400000 <= address < 0x440000 or address == 0x408EE0:
                continue
            for ins in self.instructions(address):
                if ins.mnemonic == "call" and re.search(r"\+ 0x(26c|270|274)\]", ins.op_str):
                    observed.add(ins.address + ins.size)
        source = (ROOT / "src/patches/network_hands.cpp").read_text()
        filtered = {int(value, 16) for value in re.findall(r"case (0x[0-9A-F]+):", source)}
        self.assertEqual(filtered, observed)

    def test_cursor_bypasses_are_accounted_for(self):
        callers = set()
        for address in self.functions:
            if not 0x400000 <= address < 0x440000 or address == 0x40E050:
                continue
            for ins in self.instructions(address):
                if ins.mnemonic == "call" and ins.op_str == "0x4bcab0":
                    callers.add(ins.address + ins.size)
        self.assertEqual(callers, {0x40AF59, 0x40E4B3})

    def test_3d_preview_passes_only_a_16_bit_keeper_tag(self):
        # The preview loads AX/SI, leaving high bits intact; the native tag lookup ignores them.
        preview = {ins.address: (ins.mnemonic, ins.op_str) for ins in self.instructions(0x40D670)}
        self.assertEqual(preview[0x40D694], ("mov", "ax, word ptr [edi + 8]"))
        self.assertEqual(preview[0x40D69F], ("push", "eax"))
        self.assertEqual(preview[0x40D6B3], ("mov", "si, word ptr [edi + 8]"))
        lookup = self.instructions(0x508C40)
        self.assertTrue(any(ins.mnemonic == "and" and ins.op_str == "eax, 0xffff" for ins in lookup))

    def test_delayed_pickup_handlers_leave_origin_word_unused(self):
        for address in (0x4C0BE0, 0x4C0CB0, 0x4C0E40, 0x4C0EA0):
            for ins in self.instructions(address):
                self.assertNotRegex(ins.op_str, r"\+ 0x19[26]\]", hex(ins.address))
        # All three pickup action records finish through act3, which calls the hooked insertion.
        setup = self.instructions(0x4C06F0)
        loads = [ins for ins in setup if ins.mnemonic == "mov" and ins.op_str == "esi, 0x4c0ea0"]
        self.assertEqual(len(loads), 3)
        self.assertTrue(any(ins.mnemonic == "call" and ins.op_str == "0x4bc500" for ins in self.instructions(0x4C0EA0)))


if __name__ == "__main__":
    unittest.main()
