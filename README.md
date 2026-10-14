# Tikun — תיקון

**Fix text typed in the wrong keyboard layout (Hebrew ↔ English) in any Windows app, with one shortcut.**

> תיקון — מתקן טקסט שהוקלד בפריסת מקלדת שגויה (עברית ↔ אנגלית), בכל תוכנה, בלחיצת קיצור אחת.

You typed a whole sentence and the keyboard was on the wrong language: `akuo guko` instead of `שלום עולם`.
Select it, press **Win + Shift + Z**, and it is replaced by what you meant to type.
This is **not a translation**: each key is simply re-read on the other layout.

## Install

1. Download `tikun.exe` from the [Releases](../../releases) page. Check it against `SHA256SUMS.txt` if you like.
2. Double-click it. A teal **ת** icon appears next to the clock. No installer, no admin rights.

## Use

- Select the text, press **Win + Shift + Z**.
- Click the tray icon for the menu: **Change shortcut…**, **Start with Windows**, About, Quit.
- Direction is detected automatically (more Hebrew letters → to English, otherwise → to Hebrew).
- The menu follows your Windows language (English, French, Hebrew).

## How it works

Tikun copies your selection (Ctrl+C), remaps every character by physical key position using the standard
Israeli keyboard layout (SI 1452), pastes it back (Ctrl+V), and restores your previous clipboard.

## Privacy

- No network access, no telemetry, no keystroke logging. The binary imports no networking libraries.
- Settings are stored in `%APPDATA%\Tikun\config.ini` (just the shortcut).
- What Tikun itself writes to the clipboard is flagged to stay out of Windows clipboard history and cloud sync.
  **However**, the selection is first copied by the *target application* with a normal Ctrl+C, and Windows
  may record that copy in clipboard history. If you convert sensitive text, turn clipboard history off.

## Limitations

- Standard Israeli layout only (SI 1452). Other Hebrew layouts are not supported.
- Hebrew → English cannot recover capital letters (Hebrew has none): `שלום` becomes `akuo`, never `Akuo`.
- Characters produced with Shift (`<`, `>`, `?`, `:`, `"`, `{`, `}`…) are not remapped.
- Classic console windows treat Ctrl+C as "interrupt": don't use it there.
- Apps running as administrator don't accept simulated keystrokes from a normal-privilege program.
- Some apps copy the *current line* when nothing is selected; Tikun will convert that line.
- Very large clipboard contents (> 256 MB) and image-handle-only formats may not be restored.

## Build

Requires `mingw-w64` (cross-compiles from Linux, macOS or WSL) and a native `gcc` for the tests.

```
make test     # unit tests for the remapping logic
make          # builds build/tikun.exe
```

Release binaries are built by GitHub Actions from the tagged source (`.github/workflows/build.yml`).

## License

MIT — see [LICENSE](LICENSE).

## See also

- [AutoSwitch](https://github.com/Zvi-Yafi/AutoSwitch) by Zvi Yafi: a similar idea for macOS, which corrects words automatically as you type.
