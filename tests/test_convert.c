#include <stdio.h>
#include "../src/convert.h"

static int fails = 0, runs = 0;

static void check(const char *name, const wchar_t *in, const wchar_t *expected) {
    runs++;
    wchar_t *out = tikun_convert(in);
    if (!out || wcscmp(out, expected) != 0) {
        fails++;
        printf("FAIL: %s\n  got:      ", name);
        for (const wchar_t *p = out; p && *p; p++) printf("U+%04X ", (unsigned)*p);
        printf("\n  expected: ");
        for (const wchar_t *p = expected; *p; p++) printf("U+%04X ", (unsigned)*p);
        printf("\n");
    }
    free(out);
}

int main(void) {
    check("en->he basic",        L"akuo",            L"שלום");
    check("he->en basic",        L"שלום",            L"akuo");
    check("en->he two words",    L"akuo guko",       L"שלום עולם");
    check("he->en two words",    L"שלום עולם",       L"akuo guko");
    check("en->he uppercase",    L"AKUO",            L"שלום");
    check("en->he hello",        L"hello",           L"יקךךם");
    check("punct en->he",        L"akuo/",           L"שלום.");
    check("punct he->en",        L"שלום.",           L"akuo/");
    check("digits/space kept",   L"123 akuo",        L"123 שלום");
    check("digits kept he->en",  L"שלום 123",        L"akuo 123");
    check("empty",               L"",                L"");
    check("unmapped kept",       L"akuo @#",         L"שלום @#");

    /* every key round-trips in both directions */
    for (int i = 0; i < TIKUN_MAP_LEN; i++) {
        wchar_t a[3] = {(wchar_t)TIKUN_MAP[i].en, L'a', 0};   /* 'a' anchors direction: Latin  */
        wchar_t b[3] = {(wchar_t)TIKUN_MAP[i].he, 0x05E9, 0}; /* shin anchors direction: Hebrew */
        wchar_t *x = tikun_convert(a), *y = tikun_convert(b);
        runs += 2;
        if (!x || (unsigned)x[0] != TIKUN_MAP[i].he) { fails++; printf("FAIL: en key U+%04X\n", TIKUN_MAP[i].en); }
        if (!y || (unsigned)y[0] != TIKUN_MAP[i].en) { fails++; printf("FAIL: he key U+%04X\n", TIKUN_MAP[i].he); }
        free(x); free(y);
    }

    /* both columns must be unique, otherwise one direction would be ambiguous */
    for (int i = 0; i < TIKUN_MAP_LEN; i++)
        for (int j = i + 1; j < TIKUN_MAP_LEN; j++) {
            runs++;
            if (TIKUN_MAP[i].he == TIKUN_MAP[j].he || TIKUN_MAP[i].en == TIKUN_MAP[j].en) {
                fails++; printf("FAIL: duplicate mapping %d/%d\n", i, j);
            }
        }

    printf("%d checks, %d failures\n", runs, fails);
    return fails ? 1 : 0;
}
