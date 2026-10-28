# SPDX-License-Identifier: MIT
"""Generate synthetic parser test data, never copied from a firmware image.

These small HII structures exercise the project's identity and boundary checks.
They are host-test vectors, not installable or renderable firmware packages.
"""
from pathlib import Path
import struct
import uuid

BASE = Path(__file__).resolve().parent / 'fixtures'
GUIDS = {
    'Setup': ('7b59104a-c00d-4158-87ff-f04d6396a915', 0x2712),
    'CbsSetupDxeRPL': ('c03209ed-5f4b-4631-bd03-8ed627cd9daf', 0x7000),
    'AmdPbsSetupDxe': ('b863b959-0ec6-4033-99c1-8fd89f040222', 0x000B),
}

def opcode(kind, payload=b'', scope=False):
    size = len(payload) + 2
    assert size < 128
    return bytes((kind, size | (0x80 if scope else 0))) + payload

def package(kind, payload):
    return struct.pack('<I', (kind << 24) | (len(payload) + 4)) + payload

def make_list(name, formset, form_id):
    class_guid = uuid.UUID('93039971-8545-4b04-b45e-32eb8326040e').bytes_le
    forms = opcode(0x0E, uuid.UUID(formset).bytes_le + struct.pack('<HHB', 0, 0, 1) + class_guid, True)
    if name == 'Setup':
        variable = uuid.UUID('ec87d643-eba4-4bb5-a1e5-3f3e36b20da9').bytes_le
        forms += opcode(0x24, variable + struct.pack('<HH', 0xF101, 255) + b'Setup\0')
    forms += opcode(0x01, struct.pack('<HH', form_id, 0), True)
    if name == 'Setup':
        for _ in range(28):
            forms += opcode(0x0A, scope=True)
            forms += opcode(0x12, struct.pack('<HH', 0xA6, 0))
            forms += opcode(0x29)
        question = struct.pack('<HHHHHB', 0x03A6, 0x03A7, 0xA6, 0xF101, 0x8A, 0x10)
        forms += opcode(0x05, question + bytes((0x10, 0, 1, 0)), True)
        forms += opcode(0x29)
    forms += opcode(0x29) + opcode(0x29)
    language = b'en-US\0'
    header_size = 46 + len(language)
    string_header = struct.pack('<II', header_size, header_size) + bytes(32) + struct.pack('<H', 0) + language
    strings = package(4, string_header + b'\0')
    payload = strings + package(2, forms) + package(0xDF, b'')
    list_guid = uuid.uuid5(uuid.NAMESPACE_URL, 'urn:meow-menu:synthetic:' + name).bytes_le
    return list_guid + struct.pack('<I', len(payload) + 20) + payload

if __name__ == '__main__':
    BASE.mkdir(exist_ok=True)
    for name, (guid, form_id) in GUIDS.items():
        (BASE / (name + '.fixture.hii')).write_bytes(make_list(name, guid, form_id))
    print('Generated three synthetic HII test fixtures; no BIOS image required.')
