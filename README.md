# Perspective Awakens — PSP Homebrew Starter

This is a starter prototype project, not a finished game. It provides a PSP-native code base and a GitHub Actions workflow intended to build an `EBOOT.PBP` with the PSPDEV toolchain.

## Build
In a PSPDEV environment with the PSP SDK installed, run `make`.
If successful, copy `EBOOT.PBP` to `/PSP/GAME/PerspectiveAwakens/EBOOT.PBP` on your PSP memory stick.

## Prototype controls
- D-pad: move the player marker
- Cross: resize the object
- Start: reset
- Triangle: toggle help

## Important
This source has not been compiled or tested on PSP hardware in this package. Treat it as a prototype starter, not a guaranteed working EBOOT.
