/* Tikun - Hebrew <-> English keyboard layout fixer for Windows.
 *
 * Select text that was typed with the wrong layout, press the hotkey
 * (default Win+Shift+Z): the selection is replaced by the same keystrokes
 * interpreted on the other layout. No translation, no network, no logging.
 *
 * MIT License. */

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include <string.h>

#include "convert.h"

#define APP_NAME        L"Tikun"
#define APP_VERSION     L"2.0.0"
#define HOTKEY_ID       1
#define WM_TRAYICON     (WM_APP + 1)
#define ID_CHANGE       1001
#define ID_AUTOSTART    1002
#define ID_ABOUT        1003
#define ID_EXIT         1004
#define RES_ICON_ID     1

#define DEFAULT_MODS    (MOD_WIN | MOD_SHIFT)
#define DEFAULT_VK      'Z'

#define COPY_TIMEOUT_MS   1000            /* wait for the app to answer Ctrl+C */
#define PASTE_SETTLE_MS   300             /* let the app consume Ctrl+V before restoring */
#define MAX_BACKUP_BYTES  (256u * 1024u * 1024u)

/* ------------------------------------------------------------------ */
/* UI strings (English / French / Hebrew, chosen from the Windows UI language) */

typedef struct {
    const wchar_t *noShortcut, *change, *autostart, *about, *quit;
    const wchar_t *capTitle, *capPrompt, *capNeedMod, *capBusy;
    const wchar_t *aboutText, *alreadyRunning, *regFailed;
} Strings;

static const Strings STR_EN = {
    L"(no shortcut)", L"Change shortcut\u2026", L"Start with Windows", L"About Tikun", L"Quit",
    L"Tikun \u2014 change shortcut",
    L"Press the new key combination.\n(Esc to cancel)",
    L"Add Ctrl, Alt, Shift or Win to the key.",
    L"This shortcut is already used by Windows or another program.\nTry a different one.",
    L"Tikun " APP_VERSION L"\n\nSelect text typed with the wrong keyboard layout (Hebrew \u2194 English) "
    L"and press %s to fix it. No translation: each key is simply remapped.\n\n"
    L"Everything stays local: no network access, no keystroke logging.\nMIT License.",
    L"Tikun is already running (look for its icon near the clock).",
    L"The shortcut %s is already used by another program.\nTikun keeps running: right-click its icon and choose \"Change shortcut\u2026\"."
};

static const Strings STR_FR = {
    L"(aucun raccourci)", L"Changer le raccourci\u2026", L"Lancer au d\u00e9marrage de Windows", L"\u00c0 propos de Tikun", L"Quitter",
    L"Tikun \u2014 changer le raccourci",
    L"Appuie sur la nouvelle combinaison de touches.\n(\u00c9chap pour annuler)",
    L"Ajoute Ctrl, Alt, Maj ou Win \u00e0 la touche.",
    L"Ce raccourci est d\u00e9j\u00e0 utilis\u00e9 par Windows ou un autre programme.\nEssaie-en un autre.",
    L"Tikun " APP_VERSION L"\n\nS\u00e9lectionne du texte tap\u00e9 avec le mauvais clavier (h\u00e9breu \u2194 anglais) "
    L"puis appuie sur %s pour le corriger. Pas de traduction : chaque touche est simplement remapp\u00e9e.\n\n"
    L"Tout reste local : aucun acc\u00e8s r\u00e9seau, aucun enregistrement des frappes.\nLicence MIT.",
    L"Tikun est d\u00e9j\u00e0 lanc\u00e9 (cherche son ic\u00f4ne pr\u00e8s de l'horloge).",
    L"Le raccourci %s est d\u00e9j\u00e0 utilis\u00e9 par un autre programme.\nTikun reste actif : clic droit sur son ic\u00f4ne, puis \u00ab Changer le raccourci\u2026 \u00bb."
};

static const Strings STR_HE = {
    L"(\u05DC\u05DC\u05D0 \u05E7\u05D9\u05E6\u05D5\u05E8)",
    L"\u05E9\u05D9\u05E0\u05D5\u05D9 \u05E7\u05D9\u05E6\u05D5\u05E8 \u05DE\u05E7\u05E9\u05D9\u05DD\u2026",
    L"\u05D4\u05E4\u05E2\u05DC\u05D4 \u05E2\u05DD Windows",
    L"\u05D0\u05D5\u05D3\u05D5\u05EA Tikun",
    L"\u05D9\u05E6\u05D9\u05D0\u05D4",
    L"Tikun \u2014 \u05E9\u05D9\u05E0\u05D5\u05D9 \u05E7\u05D9\u05E6\u05D5\u05E8",
    L"\u05D4\u05E7\u05E9 \u05D0\u05EA \u05E6\u05D9\u05E8\u05D5\u05E3 \u05D4\u05DE\u05E7\u05E9\u05D9\u05DD \u05D4\u05D7\u05D3\u05E9.\n(Esc \u05DC\u05D1\u05D9\u05D8\u05D5\u05DC)",
    L"\u05D4\u05D5\u05E1\u05E3 Ctrl, Alt, Shift \u05D0\u05D5 Win \u05DC\u05DE\u05E7\u05E9.",
    L"\u05D4\u05E7\u05D9\u05E6\u05D5\u05E8 \u05D4\u05D6\u05D4 \u05DB\u05D1\u05E8 \u05D1\u05E9\u05D9\u05DE\u05D5\u05E9 \u05E2\u05DC \u05D9\u05D3\u05D9 Windows \u05D0\u05D5 \u05EA\u05D5\u05DB\u05E0\u05D4 \u05D0\u05D7\u05E8\u05EA.\n\u05E0\u05E1\u05D4 \u05E6\u05D9\u05E8\u05D5\u05E3 \u05D0\u05D7\u05E8.",
    L"Tikun " APP_VERSION L"\n\n\u05E1\u05DE\u05DF \u05D8\u05E7\u05E1\u05D8 \u05E9\u05D4\u05D5\u05E7\u05DC\u05D3 \u05D1\u05E4\u05E8\u05D9\u05E1\u05EA \u05DE\u05E7\u05DC\u05D3\u05EA \u05E9\u05D2\u05D5\u05D9\u05D4 "
    L"(\u05E2\u05D1\u05E8\u05D9\u05EA \u2194 \u05D0\u05E0\u05D2\u05DC\u05D9\u05EA) \u05D5\u05D4\u05E7\u05E9 %s \u05DB\u05D3\u05D9 \u05DC\u05EA\u05E7\u05DF. "
    L"\u05DC\u05DC\u05D0 \u05EA\u05E8\u05D2\u05D5\u05DD \u2014 \u05DB\u05DC \u05DE\u05E7\u05E9 \u05DE\u05DE\u05D5\u05E4\u05D4 \u05DE\u05D7\u05D3\u05E9.\n\n"
    L"\u05D4\u05DB\u05DC \u05E4\u05D5\u05E2\u05DC \u05DE\u05E7\u05D5\u05DE\u05D9\u05EA: \u05DC\u05DC\u05D0 \u05D2\u05D9\u05E9\u05D4 \u05DC\u05E8\u05E9\u05EA \u05D5\u05DC\u05DC\u05D0 \u05EA\u05D9\u05E2\u05D5\u05D3 \u05D4\u05E7\u05E9\u05D5\u05EA.\n\u05E8\u05D9\u05E9\u05D9\u05D5\u05DF MIT.",
    L"Tikun \u05DB\u05D1\u05E8 \u05E4\u05D5\u05E2\u05DC (\u05D7\u05E4\u05E9 \u05D0\u05EA \u05D4\u05E1\u05DE\u05DC \u05DC\u05D9\u05D3 \u05D4\u05E9\u05E2\u05D5\u05DF).",
    L"\u05D4\u05E7\u05D9\u05E6\u05D5\u05E8 %s \u05DB\u05D1\u05E8 \u05D1\u05E9\u05D9\u05DE\u05D5\u05E9 \u05E2\u05DC \u05D9\u05D3\u05D9 \u05EA\u05D5\u05DB\u05E0\u05D4 \u05D0\u05D7\u05E8\u05EA.\nTikun \u05DE\u05DE\u05E9\u05D9\u05DA \u05DC\u05E4\u05E2\u05D5\u05DC: \u05DC\u05D7\u05E5 \u05DC\u05D7\u05D9\u05E6\u05D4 \u05D9\u05DE\u05E0\u05D9\u05EA \u05E2\u05DC \u05D4\u05E1\u05DE\u05DC \u05D5\u05D1\u05D7\u05E8 \u05D1\u05E9\u05D9\u05E0\u05D5\u05D9 \u05E7\u05D9\u05E6\u05D5\u05E8."
};

static const Strings *g_s = &STR_EN;

static void InitStrings(void) {
    switch (PRIMARYLANGID(GetUserDefaultUILanguage())) {
        case LANG_FRENCH: g_s = &STR_FR; break;
        case LANG_HEBREW: g_s = &STR_HE; break;
        default: break;
    }
}

/* ------------------------------------------------------------------ */
/* Globals */

static HINSTANCE g_hInst;
static HWND g_hwnd;
static UINT g_msgTaskbar;
static NOTIFYICONDATAW g_nid;
static HICON g_icon;
static UINT g_mods = DEFAULT_MODS, g_vk = DEFAULT_VK;
static BOOL g_hkOk = FALSE, g_busy = FALSE;
static int g_dpi = 96;

static int S(int v) { return MulDiv(v, g_dpi, 96); }

/* ------------------------------------------------------------------ */
/* Hotkey helpers */

static BOOL IsModifierVk(UINT vk) {
    return vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_CONTROL || vk == VK_LCONTROL ||
           vk == VK_RCONTROL || vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU || vk == VK_LWIN || vk == VK_RWIN;
}

static BOOL AllowedAlone(UINT vk) {
    return (vk >= VK_F1 && vk <= VK_F24) || vk == VK_PAUSE || vk == VK_SCROLL;
}

static UINT CurrentMods(void) {
    UINT m = 0;
    if (GetKeyState(VK_CONTROL) & 0x8000) m |= MOD_CONTROL;
    if (GetKeyState(VK_MENU) & 0x8000) m |= MOD_ALT;
    if (GetKeyState(VK_SHIFT) & 0x8000) m |= MOD_SHIFT;
    if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000)) m |= MOD_WIN;
    return m;
}

static void ModsToString(UINT mods, wchar_t *out) {
    out[0] = 0;
    if (mods & MOD_CONTROL) lstrcatW(out, L"Ctrl + ");
    if (mods & MOD_ALT) lstrcatW(out, L"Alt + ");
    if (mods & MOD_SHIFT) lstrcatW(out, L"Shift + ");
    if (mods & MOD_WIN) lstrcatW(out, L"Win + ");
}

static void VkName(UINT vk, wchar_t *out, int cap) {
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) { out[0] = (wchar_t)vk; out[1] = 0; return; }
    if (vk >= VK_F1 && vk <= VK_F24) { wsprintfW(out, L"F%d", (int)(vk - VK_F1 + 1)); return; }
    switch (vk) {
        case VK_PAUSE:  lstrcpynW(out, L"Pause", cap); return;
        case VK_SCROLL: lstrcpynW(out, L"Scroll Lock", cap); return;
        case VK_SPACE:  lstrcpynW(out, L"Space", cap); return;
        default: break;
    }
    LONG lp = (LONG)(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC) << 16);
    switch (vk) {
        case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
        case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: case VK_NUMLOCK: case VK_DIVIDE:
            lp |= (1 << 24); break;
        default: break;
    }
    if (!GetKeyNameTextW(lp, out, cap)) wsprintfW(out, L"0x%02X", vk);
}

static void HotkeyToString(UINT mods, UINT vk, wchar_t *out, int cap) {
    wchar_t m[64], k[48];
    ModsToString(mods, m);
    VkName(vk, k, 48);
    lstrcpynW(out, m, cap);
    lstrcatW(out, k);
}

static BOOL RegisterCurrentHotkey(void) {
    g_hkOk = RegisterHotKey(g_hwnd, HOTKEY_ID, g_mods | MOD_NOREPEAT, g_vk);
    return g_hkOk;
}

/* ------------------------------------------------------------------ */
/* Config (%APPDATA%\Tikun\config.ini) and autostart (HKCU\...\Run) */

static BOOL ConfigPath(wchar_t *out, BOOL create) {
    wchar_t base[MAX_PATH], dir[MAX_PATH + 16];
    DWORD n = GetEnvironmentVariableW(L"APPDATA", base, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return FALSE;
    wsprintfW(dir, L"%s\\Tikun", base);
    if (create) CreateDirectoryW(dir, NULL);
    wsprintfW(out, L"%s\\config.ini", dir);
    return TRUE;
}

static void LoadConfig(void) {
    wchar_t p[MAX_PATH + 32];
    g_mods = DEFAULT_MODS;
    g_vk = DEFAULT_VK;
    if (!ConfigPath(p, FALSE)) return;
    UINT m = GetPrivateProfileIntW(L"hotkey", L"modifiers", DEFAULT_MODS, p);
    UINT v = GetPrivateProfileIntW(L"hotkey", L"vk", DEFAULT_VK, p);
    if ((m & ~0x0Fu) == 0 && v > 0 && v < 256 && (m != 0 || AllowedAlone(v))) { g_mods = m; g_vk = v; }
}

static void SaveConfig(void) {
    wchar_t p[MAX_PATH + 32], num[16];
    if (!ConfigPath(p, TRUE)) return;
    wsprintfW(num, L"%u", g_mods);
    WritePrivateProfileStringW(L"hotkey", L"modifiers", num, p);
    wsprintfW(num, L"%u", g_vk);
    WritePrivateProfileStringW(L"hotkey", L"vk", num, p);
}

#define RUN_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"

static BOOL IsAutostart(void) {
    HKEY k;
    BOOL on = FALSE;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_READ, &k) == ERROR_SUCCESS) {
        on = RegQueryValueExW(k, APP_NAME, NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
        RegCloseKey(k);
    }
    return on;
}

static void SetAutostart(BOOL on) {
    HKEY k;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_WRITE, &k) != ERROR_SUCCESS) return;
    if (on) {
        wchar_t exe[MAX_PATH], val[MAX_PATH + 4];
        if (GetModuleFileNameW(NULL, exe, MAX_PATH)) {
            wsprintfW(val, L"\"%s\"", exe);
            RegSetValueExW(k, APP_NAME, 0, REG_SZ, (const BYTE *)val, (DWORD)((lstrlenW(val) + 1) * sizeof(wchar_t)));
        }
    } else {
        RegDeleteValueW(k, APP_NAME);
    }
    RegCloseKey(k);
}

/* ------------------------------------------------------------------ */
/* Clipboard: full backup/restore of memory-based formats, and privacy flags
 * so that what Tikun writes is kept out of Windows clipboard history / cloud sync. */

typedef struct { UINT fmt; SIZE_T size; BYTE *data; } ClipItem;
typedef struct { ClipItem *items; int count, cap; } ClipBackup;

static BOOL OpenClipboardRetry(void) {
    for (int i = 0; i < 25; i++) {
        if (OpenClipboard(NULL)) return TRUE;
        Sleep(10);
    }
    return FALSE;
}

static BOOL FormatIsMemoryBased(UINT f) {
    if (f == CF_BITMAP || f == CF_METAFILEPICT || f == CF_PALETTE || f == CF_ENHMETAFILE || f == CF_OWNERDISPLAY) return FALSE;
    if (f >= CF_DSPTEXT && f <= CF_DSPMETAFILEPICT) return FALSE;
    if (f == CF_DSPENHMETAFILE) return FALSE;
    if (f >= CF_PRIVATEFIRST && f <= CF_PRIVATELAST) return FALSE;
    if (f >= CF_GDIOBJFIRST && f <= CF_GDIOBJLAST) return FALSE;
    return TRUE;
}

static void BackupClipboard(ClipBackup *b) {
    memset(b, 0, sizeof(*b));
    if (!OpenClipboardRetry()) return;
    SIZE_T total = 0;
    for (UINT f = EnumClipboardFormats(0); f; f = EnumClipboardFormats(f)) {
        if (!FormatIsMemoryBased(f)) continue;
        HANDLE h = GetClipboardData(f);
        if (!h) continue;
        SIZE_T sz = GlobalSize(h);
        if (sz == 0) continue;
        if (total + sz > MAX_BACKUP_BYTES) break;
        void *p = GlobalLock(h);
        if (!p) continue;
        BYTE *copy = (BYTE *)malloc(sz);
        if (!copy) { GlobalUnlock(h); continue; }
        memcpy(copy, p, sz);
        GlobalUnlock(h);
        if (b->count == b->cap) {
            int ncap = b->cap ? b->cap * 2 : 16;
            ClipItem *ni = (ClipItem *)realloc(b->items, (size_t)ncap * sizeof(ClipItem));
            if (!ni) { free(copy); break; }
            b->items = ni;
            b->cap = ncap;
        }
        b->items[b->count].fmt = f;
        b->items[b->count].size = sz;
        b->items[b->count].data = copy;
        b->count++;
        total += sz;
    }
    CloseClipboard();
}

static void FreeBackup(ClipBackup *b) {
    for (int i = 0; i < b->count; i++) free(b->items[i].data);
    free(b->items);
    memset(b, 0, sizeof(*b));
}

static void SetDwordFormat(const wchar_t *name, DWORD value) {
    UINT f = RegisterClipboardFormatW(name);
    if (!f) return;
    HGLOBAL g = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
    if (!g) return;
    DWORD *p = (DWORD *)GlobalLock(g);
    if (!p) { GlobalFree(g); return; }
    *p = value;
    GlobalUnlock(g);
    if (!SetClipboardData(f, g)) GlobalFree(g);
}

/* Clipboard must be open. */
static void SetPrivacyFlags(void) {
    SetDwordFormat(L"ExcludeClipboardContentFromMonitorProcessing", 0);
    SetDwordFormat(L"CanIncludeInClipboardHistory", 0);
    SetDwordFormat(L"CanUploadToCloudClipboard", 0);
}

static void RestoreClipboard(ClipBackup *b) {
    if (!OpenClipboardRetry()) { FreeBackup(b); return; }
    EmptyClipboard();
    for (int i = 0; i < b->count; i++) {
        HGLOBAL g = GlobalAlloc(GMEM_MOVEABLE, b->items[i].size);
        if (!g) continue;
        void *p = GlobalLock(g);
        if (!p) { GlobalFree(g); continue; }
        memcpy(p, b->items[i].data, b->items[i].size);
        GlobalUnlock(g);
        if (!SetClipboardData(b->items[i].fmt, g)) GlobalFree(g);
    }
    if (b->count) SetPrivacyFlags(); /* the original was already in history; don't duplicate it */
    CloseClipboard();
    FreeBackup(b);
}

static wchar_t *ReadClipboardText(void) {
    wchar_t *copy = NULL;
    if (!OpenClipboardRetry()) return NULL;
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (h) {
        const wchar_t *p = (const wchar_t *)GlobalLock(h);
        if (p) {
            size_t n = wcslen(p);
            copy = (wchar_t *)malloc((n + 1) * sizeof(wchar_t));
            if (copy) memcpy(copy, p, (n + 1) * sizeof(wchar_t));
            GlobalUnlock(h);
        }
    }
    CloseClipboard();
    return copy;
}

static BOOL WriteClipboardText(const wchar_t *s) {
    size_t bytes = (wcslen(s) + 1) * sizeof(wchar_t);
    BOOL ok = FALSE;
    if (!OpenClipboardRetry()) return FALSE;
    EmptyClipboard();
    HGLOBAL g = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (g) {
        void *p = GlobalLock(g);
        if (p) {
            memcpy(p, s, bytes);
            GlobalUnlock(g);
            ok = SetClipboardData(CF_UNICODETEXT, g) != NULL;
        }
        if (!ok) GlobalFree(g);
    }
    SetPrivacyFlags();
    CloseClipboard();
    return ok;
}

/* ------------------------------------------------------------------ */
/* Simulated keystrokes */

static void SendCtrl(WORD key) {
    INPUT in[4];
    ZeroMemory(in, sizeof(in));
    WORD vks[4] = { VK_CONTROL, key, key, VK_CONTROL };
    for (int i = 0; i < 4; i++) {
        in[i].type = INPUT_KEYBOARD;
        in[i].ki.wVk = vks[i];
        in[i].ki.wScan = (WORD)MapVirtualKeyW(vks[i], MAPVK_VK_TO_VSC);
        if (i >= 2) in[i].ki.dwFlags = KEYEVENTF_KEYUP;
    }
    SendInput(4, in, sizeof(INPUT));
}

/* Unassigned key tap: stops Windows treating a lone Win/Alt release as a Start-menu / menu-bar activation. */
static void MaskTap(void) {
    INPUT in[2];
    ZeroMemory(in, sizeof(in));
    in[0].type = in[1].type = INPUT_KEYBOARD;
    in[0].ki.wVk = in[1].ki.wVk = 0xE8;
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, in, sizeof(INPUT));
}

static BOOL KeyDown(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

/* The hotkey keys are still physically held when the hotkey fires; wait for
 * their release, otherwise our Ctrl+C would arrive as e.g. Win+Shift+Ctrl+C. */
static void WaitForRelease(void) {
    DWORD start = GetTickCount();
    while (GetTickCount() - start < 2000) {
        BOOL down = KeyDown((int)g_vk);
        if (g_mods & MOD_CONTROL) down |= KeyDown(VK_CONTROL);
        if (g_mods & MOD_SHIFT) down |= KeyDown(VK_SHIFT);
        if (g_mods & MOD_ALT) down |= KeyDown(VK_MENU);
        if (g_mods & MOD_WIN) down |= (KeyDown(VK_LWIN) || KeyDown(VK_RWIN));
        if (!down) return;
        Sleep(10);
    }
}

/* ------------------------------------------------------------------ */
/* The actual job */

static void DoConversion(void) {
    if (g_busy) return;
    g_busy = TRUE;

    if (g_mods & (MOD_WIN | MOD_ALT)) MaskTap();
    WaitForRelease();

    ClipBackup bk;
    BackupClipboard(&bk);

    if (OpenClipboardRetry()) { EmptyClipboard(); CloseClipboard(); }
    DWORD seq = GetClipboardSequenceNumber();
    SendCtrl('C');

    BOOL copied = FALSE;
    for (DWORD t = 0; t < COPY_TIMEOUT_MS; t += 10) {
        Sleep(10);
        if (GetClipboardSequenceNumber() != seq && IsClipboardFormatAvailable(CF_UNICODETEXT)) { copied = TRUE; break; }
    }

    BOOL pasted = FALSE;
    if (copied) {
        Sleep(30); /* let the source app finish writing all its formats */
        wchar_t *text = ReadClipboardText();
        if (text) {
            wchar_t *conv = tikun_convert(text);
            if (conv) {
                if (WriteClipboardText(conv)) { SendCtrl('V'); pasted = TRUE; }
                free(conv);
            }
            free(text);
        }
    }

    if (pasted) Sleep(PASTE_SETTLE_MS);
    else MessageBeep(MB_ICONEXCLAMATION); /* nothing selected / nothing copied */

    RestoreClipboard(&bk);

    MSG m;
    while (PeekMessageW(&m, g_hwnd, WM_HOTKEY, WM_HOTKEY, PM_REMOVE)) { /* drop queued presses */ }
    g_busy = FALSE;
}

/* ------------------------------------------------------------------ */
/* Tray icon */

static void TrayFillTip(void) {
    wchar_t hk[96];
    if (g_hkOk) HotkeyToString(g_mods, g_vk, hk, 96);
    else lstrcpynW(hk, g_s->noShortcut, 96);
    wsprintfW(g_nid.szTip, L"Tikun \u2014 %s", hk);
}

static void TrayAdd(void) {
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_icon;
    TrayFillTip();
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

static void TrayUpdateTip(void) {
    TrayFillTip();
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

/* ------------------------------------------------------------------ */
/* "Change shortcut" capture window */

static HWND g_capWnd, g_capLabel;
static BOOL g_capOpen, g_capOk;
static HFONT g_capFont;

static void SetCapText(const wchar_t *t) { if (g_capLabel) SetWindowTextW(g_capLabel, t); }

static LRESULT CALLBACK CapProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        RECT rc;
        GetClientRect(h, &rc);
        g_capLabel = CreateWindowExW(0, L"STATIC", g_s->capPrompt, WS_CHILD | WS_VISIBLE | SS_CENTER,
                                     S(16), S(24), rc.right - S(32), rc.bottom - S(40), h, NULL, g_hInst, NULL);
        NONCLIENTMETRICSW ncm;
        ZeroMemory(&ncm, sizeof(ncm));
        ncm.cbSize = sizeof(ncm);
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
            ncm.lfMessageFont.lfHeight = ncm.lfMessageFont.lfHeight * 12 / 10;
            g_capFont = CreateFontIndirectW(&ncm.lfMessageFont);
            if (g_capFont) SendMessageW(g_capLabel, WM_SETFONT, (WPARAM)g_capFont, TRUE);
        }
        return 0;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        UINT vk = (UINT)w, mods = CurrentMods();
        if (IsModifierVk(vk)) {
            wchar_t t[80];
            ModsToString(mods, t);
            lstrcatW(t, L"\u2026");
            SetCapText(t);
            return 0;
        }
        if (vk == VK_ESCAPE && mods == 0) { DestroyWindow(h); return 0; }
        if (mods == 0 && !AllowedAlone(vk)) { SetCapText(g_s->capNeedMod); return 0; }
        if (RegisterHotKey(g_hwnd, HOTKEY_ID, mods | MOD_NOREPEAT, vk)) {
            g_mods = mods;
            g_vk = vk;
            g_hkOk = TRUE;
            SaveConfig();
            g_capOk = TRUE;
            DestroyWindow(h);
        } else {
            SetCapText(g_s->capBusy);
        }
        return 0;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (CurrentMods() == 0) SetCapText(g_s->capPrompt);
        return 0;
    case WM_CHAR:
    case WM_SYSCHAR:
        return 0;
    case WM_CLOSE:
        DestroyWindow(h);
        return 0;
    case WM_DESTROY:
        if (g_capFont) { DeleteObject(g_capFont); g_capFont = NULL; }
        g_capLabel = NULL;
        g_capWnd = NULL;
        g_capOpen = FALSE;
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static void ShowCapture(void) {
    if (g_capOpen) { SetForegroundWindow(g_capWnd); return; }

    static BOOL registered = FALSE;
    if (!registered) {
        WNDCLASSEXW wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = CapProc;
        wc.hInstance = g_hInst;
        wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"TikunCapture";
        wc.hIcon = g_icon;
        RegisterClassExW(&wc);
        registered = TRUE;
    }

    /* free the current combination so that it can be re-captured too */
    UnregisterHotKey(g_hwnd, HOTKEY_ID);
    UINT oldMods = g_mods, oldVk = g_vk;
    BOOL oldOk = g_hkOk;
    g_capOk = FALSE;
    g_capOpen = TRUE;

    DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    RECT r = { 0, 0, S(380), S(120) };
    AdjustWindowRectEx(&r, style, FALSE, WS_EX_TOPMOST);
    int w = r.right - r.left, hgt = r.bottom - r.top;
    RECT wa;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    int x = wa.left + ((wa.right - wa.left) - w) / 2;
    int y = wa.top + ((wa.bottom - wa.top) - hgt) / 2;

    g_capWnd = CreateWindowExW(WS_EX_TOPMOST, L"TikunCapture", g_s->capTitle, style, x, y, w, hgt, NULL, NULL, g_hInst, NULL);
    if (!g_capWnd) {
        g_capOpen = FALSE;
    } else {
        ShowWindow(g_capWnd, SW_SHOW);
        SetForegroundWindow(g_capWnd);
        SetFocus(g_capWnd);
        MSG msg;
        while (g_capOpen) {
            BOOL got = GetMessageW(&msg, NULL, 0, 0);
            if (got <= 0) { if (got == 0) PostQuitMessage((int)msg.wParam); break; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (!g_capOk) { /* cancelled: put the previous shortcut back */
        g_mods = oldMods;
        g_vk = oldVk;
        g_hkOk = oldOk && RegisterCurrentHotkey();
    }
    TrayUpdateTip();
}

/* ------------------------------------------------------------------ */
/* Main window (hidden) */

static void ShowMenu(HWND h) {
    POINT pt;
    GetCursorPos(&pt);
    wchar_t hk[96], title[160];
    if (g_hkOk) HotkeyToString(g_mods, g_vk, hk, 96);
    else lstrcpynW(hk, g_s->noShortcut, 96);
    wsprintfW(title, L"Tikun  \u2014  %s", hk);

    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING | MF_DISABLED, 0, title);
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, ID_CHANGE, g_s->change);
    AppendMenuW(m, MF_STRING | (IsAutostart() ? MF_CHECKED : 0), ID_AUTOSTART, g_s->autostart);
    AppendMenuW(m, MF_STRING, ID_ABOUT, g_s->about);
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING, ID_EXIT, g_s->quit);
    SetForegroundWindow(h);
    TrackPopupMenu(m, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, h, NULL);
    PostMessageW(h, WM_NULL, 0, 0);
    DestroyMenu(m);
}

static void ShowAbout(void) {
    wchar_t hk[96], text[1024];
    if (g_hkOk) HotkeyToString(g_mods, g_vk, hk, 96);
    else lstrcpynW(hk, g_s->noShortcut, 96);
    wsprintfW(text, g_s->aboutText, hk);
    MessageBoxW(NULL, text, L"Tikun", MB_OK | MB_ICONINFORMATION);
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == g_msgTaskbar && g_msgTaskbar) { TrayAdd(); return 0; } /* Explorer restarted */
    switch (m) {
    case WM_CREATE:
        g_hwnd = h;
        RegisterCurrentHotkey();
        TrayAdd();
        return 0;
    case WM_HOTKEY:
        if (w == HOTKEY_ID) DoConversion();
        return 0;
    case WM_TRAYICON:
        if (l == WM_RBUTTONUP || l == WM_LBUTTONUP) ShowMenu(h);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case ID_CHANGE: ShowCapture(); break;
        case ID_AUTOSTART: SetAutostart(!IsAutostart()); break;
        case ID_ABOUT: ShowAbout(); break;
        case ID_EXIT: DestroyWindow(h); break;
        }
        return 0;
    case WM_DESTROY:
        UnregisterHotKey(h, HOTKEY_ID);
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR cmd, int show) {
    (void)hPrev; (void)cmd; (void)show;
    g_hInst = hInst;
    InitStrings();

    HANDLE mtx = CreateMutexW(NULL, FALSE, L"Local\\Tikun.SingleInstance");
    if (mtx && GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(NULL, g_s->alreadyRunning, L"Tikun", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    HDC dc = GetDC(NULL);
    if (dc) { g_dpi = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(NULL, dc); }
    if (g_dpi < 96) g_dpi = 96;

    g_icon = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(RES_ICON_ID), IMAGE_ICON,
                               GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    if (!g_icon) g_icon = LoadIconW(NULL, IDI_APPLICATION);

    LoadConfig();
    g_msgTaskbar = RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"TikunMain";
    wc.hIcon = g_icon;
    RegisterClassExW(&wc);

    HWND hw = CreateWindowExW(0, L"TikunMain", APP_NAME, WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, hInst, NULL);
    if (!hw) return 1;

    if (!g_hkOk) {
        wchar_t hk[96], text[512];
        HotkeyToString(g_mods, g_vk, hk, 96);
        wsprintfW(text, g_s->regFailed, hk);
        MessageBoxW(NULL, text, L"Tikun", MB_OK | MB_ICONWARNING);
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (mtx) CloseHandle(mtx);
    return (int)msg.wParam;
}
