# Humbug-NX
A Switch wrapper for the 32-bit Android version of Humbug 2.2.4 (Unity 2019.2, IL2CPP). It runs the game's own libraries directly on the Switch CPU. The `.nro` holds no game files: you add your own APK.
## Install

You need Atmosphere and the sphaira homebrew menu.

1. Download humbug_nx-v0.1.0.zip from the Releases page and extract it. Copy the humbug_nx folder to sdmc:/switch/humbug_nx/
2. Put your APK in that folder. Its file name doesn't matter.
3. In sphaira, open Homebrew, select Humbug and choose Install Forwarder.
4. Start Humbug from its HOME menu icon. The game won't start without the forwarder.

The first start unpacks the APK and takes a minute or two.

## Controls

| Input | Action |
| --- | --- |
| Left stick | Move the blue cursor |
| Right stick | Move the cursor slowly |
| D-pad | Flick that way from the cursor, one flick per press |
| A, ZR or ZL | Click or tap; hold to drag |
| B | Go back or close a menu |
| X | Undo |
| Y | Restart the level |
| L | Turn the picture left, for a console held upright; press again to turn it back |
| R | Turn the picture right, for a console held upright; press again to turn it back |
| Plus | Pause: leaves for the main menu, where Resume continues the level |
| Minus | Change the picture size: normal, then three wider sizes. It is remembered |
| Touchscreen | Touch directly |

X, Y and Plus only work during a level. While the picture is turned, the sticks, D-pad and touchscreen
turn with it, and Minus does nothing. A flick pressed during another flick follows when it ends.

## If something goes wrong

`debug.log` and `crash.log` are written to `sdmc:/switch/humbug_nx/`. Send them with a description of
what happened. `config.ini` in the same folder has the display and debug options.

## Limits

- It only works with the exact libraries of this APK. Another copy is reported as a guard mismatch in
  `debug.log`.
- Store, ads, analytics and online features have no Android services behind them. Video hints won't play.
- The game is portrait, so it shows with black bars at the sides. Set `screen = wide` under `[display]`
  in `config.ini` to make it draw across the whole screen, in a layout it wasn't designed for.

## Build

Needs Docker with `devkitpro/devkita64` and `ghcr.io/vita2hos/devcontainer/vita2hos`, and `libnx32/`
built with its `build_libnx32.sh`.

```bash
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

The port is in `port/`. `port/tools/gen_offsets.py` rebuilds the game addresses and their guards.

## Credits
[ChanseyIsTheBest](https://github.com/ChanseyIsTheBest/sa2-nx): his porting guide and tools.
Built on [teenytitans_nx](https://github.com/iqbalriz/teenytitans_nx) and
[dcr_sea_nx](https://github.com/aks796/dcr_sea_nx), with the
[android32](https://github.com/aks796/android32), [libnx32](https://github.com/aks796/libnx32) and
[mesa32](https://github.com/aks796/mesa32) projects. See `port/LICENSE`. Humbug is by Dunderbit; this
project isn't affiliated with Dunderbit or Nintendo.
