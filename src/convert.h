/* convert.h - pure key-position remapping between the US-English layout and
 * the standard Israeli Hebrew layout (SI 1452). No Windows dependency, so it
 * can be unit-tested on any platform. */
#ifndef TIKUN_CONVERT_H
#define TIKUN_CONVERT_H

#include <stdlib.h>
#include <wchar.h>

typedef struct { unsigned en; unsigned he; } TikunPair;

/* en = character produced by the key on the English layout (unshifted),
 * he = character produced by the same physical key on the Hebrew layout. */
static const TikunPair TIKUN_MAP[] = {
    {'q', '/'},    {'w', '\''},   {'e', 0x05E7}, {'r', 0x05E8}, {'t', 0x05D0},
    {'y', 0x05D8}, {'u', 0x05D5}, {'i', 0x05DF}, {'o', 0x05DD}, {'p', 0x05E4},
    {'[', ']'},    {']', '['},
    {'a', 0x05E9}, {'s', 0x05D3}, {'d', 0x05D2}, {'f', 0x05DB}, {'g', 0x05E2},
    {'h', 0x05D9}, {'j', 0x05D7}, {'k', 0x05DC}, {'l', 0x05DA}, {';', 0x05E3},
    {'\'', ','},
    {'z', 0x05D6}, {'x', 0x05E1}, {'c', 0x05D1}, {'v', 0x05D4}, {'b', 0x05E0},
    {'n', 0x05DE}, {'m', 0x05E6}, {',', 0x05EA}, {'.', 0x05E5}, {'/', '.'},
};
#define TIKUN_MAP_LEN ((int)(sizeof(TIKUN_MAP) / sizeof(TIKUN_MAP[0])))

static inline unsigned tikun_en_to_he(unsigned c) {
    for (int i = 0; i < TIKUN_MAP_LEN; i++)
        if (TIKUN_MAP[i].en == c) return TIKUN_MAP[i].he;
    return 0;
}

static inline unsigned tikun_he_to_en(unsigned c) {
    for (int i = 0; i < TIKUN_MAP_LEN; i++)
        if (TIKUN_MAP[i].he == c) return TIKUN_MAP[i].en;
    return 0;
}

/* Returns 1 if the text should be converted English->Hebrew, 0 for
 * Hebrew->English. Decided once for the whole selection by counting letters,
 * so ambiguous punctuation (. , / ' ; [ ]) is always mapped consistently. */
static inline int tikun_direction_to_hebrew(const wchar_t *s) {
    size_t he = 0, la = 0;
    for (; *s; s++) {
        unsigned c = (unsigned)*s;
        if (c >= 0x05D0 && c <= 0x05EA) he++;
        else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) la++;
    }
    return he > la ? 0 : 1;
}

/* Returns a newly malloc'ed converted copy (caller frees), or NULL. */
static inline wchar_t *tikun_convert(const wchar_t *src) {
    size_t n = wcslen(src);
    wchar_t *out = (wchar_t *)malloc((n + 1) * sizeof(wchar_t));
    if (!out) return NULL;
    int to_he = tikun_direction_to_hebrew(src);
    for (size_t i = 0; i < n; i++) {
        unsigned c = (unsigned)src[i], r;
        if (to_he) {
            unsigned lc = (c >= 'A' && c <= 'Z') ? c + 32 : c; /* Hebrew has no case */
            r = tikun_en_to_he(lc);
        } else {
            r = tikun_he_to_en(c);
        }
        out[i] = r ? (wchar_t)r : src[i];
    }
    out[n] = 0;
    return out;
}

#endif
