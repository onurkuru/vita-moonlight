# Vita Moonlight

Vita Moonlight is a PlayStation Vita port of Moonlight, with major improvements for usability, pairing, device management, and now advanced touch, multitouch, and DS4 touchpad emulation.

## Highlights (0.13.2)

- **Touchscreen Modes Unified:** Select between three touchscreen modes from the settings menu:
  - **DS4 Touchpad:** Emulates a DualShock 4-style multitouch touchpad, compatible with gestures and Steam Input advanced controls.
  - **Absolute Mouse:** Use the Vita screen as a true absolute mouse (with improved gesture support).
  - **Tablet (Sunshine):** Native multitouch for Sunshine streaming, with true multi-finger gestures.
- **DS4 Touchpad Sensitivity:** DS4 mode is now more sensitive and precise for a smoother experience.
- **PS Button Capture:** Optional PS button capture implemented — double press opens the menu, single press forwarded to Sunshine as PS/Xbox button mapping (requires SceShell permission in the VPK).
- **Touch Zones Independent of Mode:** Touch zone support allows using front-screen touch zones independent of the selected touch input mode; when a zone is active, touch input in that area is ignored.
- **Wake-on-LAN (WOL) Integration:** Power on your remote PC directly from the Vita, with robust MAC address handling and cross-platform compatibility.
- **Host MAC Management:** MAC addresses are now saved/loaded correctly, with all ARP/legacy logic removed.
- **WOL Packet Debugging:** Includes a Python script (`tools/wol_sniffer.py`) to verify WOL packets on your network.
- **UI Improvements:**
  - Host Management menu: improved navigation (O/cancel returns), clearer visual feedback, and unified status display.
  - Special button overlays no longer interfere with absolute touch input.
- **Combo Fixes:** L1+L2 and R1+R2 combos now work correctly, even with touchscreen or backtouch enabled.
- **Robustness:** Many bugfixes, code cleanups, and internal refactors for stability and maintainability.
- **L1/R1 and L2/R2 Swap:** Swap the functions of the L1/R1 and L2/R2 buttons from the settings menu for greater comfort and customization.
- **Gamepad Type Selection:** Choose Xbox or PlayStation controller layout directly from the settings menu.


## Keyboard mode — play from a **macOS** host (this fork, v0.13.4)

> Download the ready-built VPK from the [Releases](https://github.com/onurkuru/vita-moonlight/releases) page.

### The problem

Sunshine on macOS is experimental and **gamepads do not work**: macOS has no API for injecting a virtual game controller, so any pad input Moonlight sends to a Mac is silently dropped. Video and audio stream fine, but you cannot control anything. Upstream Vita Moonlight only ever sends the Vita's buttons as gamepad input, so a Mac host was unusable.

### The solution

This fork adds **Settings → "Keyboard mode (for macOS host)"** (right below *Swap R1/L1 <-> R2/L2*). When it is `yes`, every button, trigger and stick direction is sent as a **keyboard key** instead — and keyboard input *is* supported by Sunshine on macOS. You then map those keys inside your emulator or game on the Mac.

It is off by default; Windows/Linux users lose nothing.

### Key map

| Vita | Key | Vita | Key |
|---|---|---|---|
| ✕ Cross | `Space` | D-pad | `↑ ↓ ← →` |
| ○ Circle | `E` | Left stick | `W A S D` |
| □ Square | `Z` | Right stick | `I J K L` |
| △ Triangle | `C` | L1 / R1 | `Q` / `R` |
| Start | `Enter` | L2 / R2 (rear touch) | `1` / `3` |
| Select | `Backspace` | L3 / R3 | `F` / `H` |

Sticks are quantised to 8 directions with a 50 % threshold (no analog speed). Fine for action games, less so for racing.

### Mac host setup (Sunshine)

1. Install Sunshine — `brew install lizardbyte/homebrew/sunshine`, then `brew services start lizardbyte/homebrew/sunshine`. Grant **Screen Recording** when macOS asks (otherwise the log says *Unable to find display or encoder*).
2. Open `https://localhost:47990`, create a user, pair the Vita with the PIN it shows.
3. **Required** in `~/.config/sunshine/sunshine.conf`:

   ```ini
   encoder = software
   sw_preset = ultrafast
   sw_tune = zerolatency
   fec_percentage = 30
   hevc_mode = 1
   av1_mode = 1
   ```

   Apple's VideoToolbox encoder **does not honour on-demand IDR (key-frame) requests**. The Vita is 2.4 GHz-only, so it drops packets, asks for an IDR, never gets one, and freezes for seconds. x264 with `zerolatency` answers immediately and costs only a few percent of one core at 960×544. The log line to look for is `Encoder did not produce IDR frame when requested!` — if you see it, you are still on VideoToolbox.
4. Restart Sunshine (`brew services restart lizardbyte/homebrew/sunshine`).

### Vita settings that work

| Setting | Value | Why |
|---|---|---|
| Resolution | **960x544** | The Vita's native panel; anything larger is wasted bandwidth |
| FPS | **30** | Most PS3/console games run at 30; 60 doubles bitrate for nothing |
| Bitrate | **4000 Kbps** | 2.4 GHz Wi-Fi realistic ceiling |
| Enable reference frame invalidation | yes | Recovers from packet loss without a full key frame |
| Enable VITA vblank / frame pacer | no | Both add latency |
| Keyboard mode | **yes** | This feature |

The on-screen counter should read `30 / 30`. `0 / 60` means the stream target is too high.

### Example: RPCS3 on the Mac

Create `~/Library/Application Support/rpcs3/input_configs/global/VitaKeyboard.yml` by copying `Default.yml` and setting:

```yaml
Player 1 Input:
  Handler: Keyboard
  Device: Keyboard
  Config:
    Left Stick Left: A
    Left Stick Down: S
    Left Stick Right: D
    Left Stick Up: W
    Right Stick Left: J
    Right Stick Down: K
    Right Stick Right: L
    Right Stick Up: I
    Start: Return
    Select: Backspace
    Square: Z
    Cross: Space
    Circle: E
    Triangle: C
    Left: Left
    Down: Down
    Right: Right
    Up: Up
    R1: R
    R2: 3
    R3: H
    L1: Q
    L2: 1
    L3: F
```

Select it in **Pads → Profile → VitaKeyboard**. Add RPCS3 to Sunshine's `apps.json` so it launches from the Vita:

```json
{
  "name": "RPCS3 - My Game",
  "detached": ["open -a RPCS3 --args --no-gui /path/to/GAME/PS3_GAME"],
  "prep-cmd": [{ "do": "", "undo": "pkill -x rpcs3" }]
}
```

Keep the game window focused on the Mac — keyboard events go to whatever is in front.

Tested on a MacBook Pro M2 (2022) with *X-Men Origins: Wolverine* at a locked 30 fps.

### Other fixes in this fork

- Builds with GCC 15 / current VitaSDK (`-std=gnu99`, `-lzstd`, missing includes) and on macOS hosts (`sed -i.bak`).
- Settings menu stack overflow: `menu[32]` was exactly full; adding **any** entry corrupted the stack and crashed with `C2-12828-1`. Grown to 48 to match the existing `assert(idx < 48)`.
- `tests/kbm_test.c` — host-side unit test of the key translation (`cc -std=gnu99 tests/kbm_test.c && ./a.out`).

Build instructions for macOS are in [`BUILD-MAC.md`](BUILD-MAC.md).

## Documentation

More information can find [moonlight-docs][1], [moonlight-embedded][2], and our [wiki][3].
If you need more help, join the #vita-help channel in [discord][4].

[1]: https://github.com/moonlight-stream/moonlight-docs/wiki
[2]: https://github.com/irtimmer/moonlight-embedded/wiki
[3]: https://github.com/xyzz/vita-moonlight/wiki
[4]: https://discord.gg/atkmxxT

## Upcoming Features

- **Artemis/Apollo compatibility:** Planned support for Artemis/Apollo (a modified Sunshine host), to allow streaming from more sources and custom servers.

Stay tuned for more improvements!

## How to open the Pause Menu

> **To open the pause menu at any time (even in any touch mode), press:**
> 
> **START + L + R**
>
> This shortcut works regardless of the selected touch mode (Absolute Mouse or Touchscreen). Use it to access the in-game pause/options menu quickly.

---

## How to open the Floating Keyboard

> **To open the elevated floating keyboard at any time, press:**
>
> **START + LEFT**
>
> This shortcut will always open the virtual keyboard in elevated mode, never covering the main screen. Works in all touch modes.

---


 
## What's New in 0.13.2

- **PS button capture:** When enabled, double-pressing the PS button will open the menu; a single press is passed through as a PS/Xbox button press to Sunshine.
  - Note: Implementing PS button capture required removing the `-s` (safe) flag from `vita-make-fself` and adding the `SceShell` permission (0x2800000000000001) to the VPK.
- **Touch Zones Independent of Mode:** Added a setting to enable/disable touch zones independently of the touch input mode.
  - Allows using front screen touch zones regardless of touch input mode. When a touch zone is pressed, the touch input in that area is ignored; unused touch zones work like normal touch areas.

### Bug fixes (0.13.2)

- Fixed front touch zone L2/R2 special keys not working.

### Refactor & maintenance (0.13.2)

- Updated subproject commits for `enet`, `inih`, and `moonlight-common-c`.
- Split `vitainput_process` into multiple smaller functions for readability and maintainability.

---

## Screenshots

<p align="center">
  <img src="docs/keyboard.jpg" alt="Floating keyboard in Steam app" width="400"/>
  <br><b>Floating keyboard in a Steam app using Moonlight</b>
</p>

<p align="center">
  <img src="docs/ip1.png" alt="Host waiting for IP update (yellow)" width="400"/>
  <br><b>Host waiting for IP update (yellow)</b>
</p>

<p align="center">
  <img src="docs/ip2.png" alt="Host online (green)" width="400"/>
  <br><b>Host online (green)</b>
</p>

<p align="center">
  <img src="docs/ip3.png" alt="Host offline/disconnected (red)" width="400"/>
  <br><b>Host offline/disconnected (red)</b>
</p>

<p align="center">
  <img src="docs/ip4.png" alt="IP change confirmation dialog" width="400"/>
  <br><b>IP change confirmation dialog (shows old/new IP)</b>
</p>

<p align="center">
  <img src="docs/ip5.png" alt="Search device function" width="400"/>
  <br><b>Search device function showing a found local device</b>
</p>

---


## Build Requirements

- **VitaSDK** installed and configured on your system ([guide here](https://vitasdk.org/)).
- **Submodules updated:**
  ```sh
  git submodule update --init
  ```

---

## Quick Build

1. Install dependencies with [vdpm](https://github.com/vitasdk/vdpm) if you haven't already.
2. Make sure VitaSDK is installed and in your $PATH.
3. Run:
   ```sh
   ./makepsv
   ```
   This will generate a VPK file ready to install on your PS Vita.

---

## Manual Build (optional)

If you prefer to build manually:

```sh
# If you do git pull, make sure to update submodules first
 git submodule update --init
 mkdir build && cd build
 cmake ..
 make
```

---

## Note about colors in vita2d

> **Important:** The vita2d library interprets colors in BGRA format (not RGBA). For example:
> - 0xFF00FFFF will appear **yellow** (not cyan)
> - 0xFFFFFF00 will appear **blue** (not yellow)
> - 0xFF00FF00 will appear **green** (correct)
> - 0xFFFF0000 will appear **red** (correct)
>
> If the color does not look as expected, swap the byte order (use BGR instead of RGB).


Thanks to all contributors and the Moonlight community!

