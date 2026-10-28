# Third-party notices

Project-authored application code, scripts, generated synthetic test data and documentation are provided under the [MIT License](LICENSE).

## EDK II headers

`源码/include/` contains an explicitly selected subset of headers from the [TianoCore EDK II project](https://github.com/tianocore/edk2), commit `931973afa488b9bc3debee3aa9390e184a460870`.

The headers retain their original copyright and SPDX notices, including **BSD-2-Clause-Patent**. The original EDK II license notice is included in [`源码/EDK2-LICENSE.txt`](源码/EDK2-LICENSE.txt). File provenance and SHA256 hashes are recorded in [`源码/headers-provenance.json`](源码/headers-provenance.json). The project MIT license does not replace these third-party terms.

## Not distributed

This repository does not distribute COLORFIRE/AMI/AMD firmware images, extracted real HII packages, SPI or NVRAM dumps, SREP, UefiVarTool, or the locally used UEFI Shell executable. Model identifiers, protocol GUIDs and field definitions are documented for interoperability; they are not a license to redistribute a vendor firmware image.

Supply an appropriately licensed x64 UEFI Shell separately. The upstream Shell sources are in [TianoCore ShellPkg](https://github.com/tianocore/edk2/tree/master/ShellPkg). The bundled `MeowMenu.efi` is compiled from this repository's project-authored sources, not extracted from a vendor BIOS.
