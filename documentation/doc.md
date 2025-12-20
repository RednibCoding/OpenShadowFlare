# ShadowFlare Documentation

This folder contains reverse-engineering documentation for ShadowFlare (2003).

## Documents

| File | Description |
|------|-------------|
| [exe-analysis.md](exe-analysis.md) | ShadowFlare.exe reverse engineering - functions, addresses, structures |
| [data-files.md](data-files.md) | Game file formats - NJP, RCLIB-L, save files, etc. |
| [game-mechanics.md](game-mechanics.md) | Game mechanics - classes, spells, stats, controls |

## Quick Reference

### Character Classes
- Hunter, Warrior, Wizard

### Elements
- Fire, Water, Earth, Thunder, Holy, Dark, Gel, Metal

### Key Exe Addresses
- WinMain: 0x004022e0
- WndProc: 0x00402b30
- InitGame: 0x004016b0
- State Machine: 0x004023d0
- Script Interpreter: 0x00430f80

### File Formats
- `.njp` - Sprites (RCLIB-L compressed)
- `.caf` - Character animation
- `.Ssv` - Save data
- `.Voc` - Audio
- `.Scs` - Scripts
