# Public repository contents

The public Git history contains project-authored source, scripts, documentation, generated-test logic, an independently rebuildable application binary and an attributed subset of EDK II headers.

The following stay local and are excluded by `.gitignore`:

- Original or extracted firmware images and capsule payloads.
- Real HII packages, current or previous NVRAM variables and runtime logs.
- Device-specific analysis exports, raw paths and hardware-state inventories.
- Historical tools and binaries whose redistribution provenance was not established.
- The separately supplied UEFI Shell executable.
- Build products, generated test fixtures and test-result files.

This separation preserves the complete local research workspace without making its backups or device state public. The public tests generate their own synthetic samples.

Do not force-add ignored dumps or backups when contributing. Review `git diff --cached --stat` and the staged file list before publishing changes.
