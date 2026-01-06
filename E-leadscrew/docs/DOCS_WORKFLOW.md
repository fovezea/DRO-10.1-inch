# Documentation Workflow & Standards

## Single Source of Truth
To prevent information fragmentation, the following files are designated as the authoritative sources for specific domains:

1.  **Hardware & Pinouts:** `HARDWARE_SPARTAN_EDGE.md`
    *   *Do not* hardcode pin tables in individual project READMEs. Link to this file.
2.  **Communication Protocols:** `PROTOCOL.md`
    *   Shared protocols should be defined here.
3.  **Project specifics:** `../README.md`
    *   Contains build instructions, architecture overviews, and usage examples specific to this codebase.

## Update Workflow
When making changes to the codebase that affect external interfaces:

1.  **Hardware Changes:** If you change `src/constraints.xdc`, verify if it matches `HARDWARE_SPARTAN_EDGE.md`. If you use a new pin, update the reference doc.
2.  **Protocol Changes:** If you modify the UART command parser, update `PROTOCOL.md` immediately.
3.  **Project README:** Update `../README.md` only if build steps or feature sets change.

## Completeness Checklist
Before marking documentation as complete, verify:
- [ ] Hardware reference covers all currently used pins.
- [ ] Protocol docs cover all implemented opcodes.
- [ ] Build instructions are tested and working.
- [ ] Legacy information is clearly marked or removed.