#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>

#include <wchar.h>
#include <locale.h>

#if defined(__GLIBC__)
  #define SUPPORT_GLIBC 1
#else
  #define SUPPORT_GLIBC 0
#endif

#define ERROR "\033[48;5;9m\033[38;5;16m ERROR \033[49;39m"
#define WARNING "\033[48;5;11m\033[38;5;16m WARNING \033[49;39m"
#define SUCCESS "\033[48;5;10m\033[38;5;16m SUCCESS \033[49;39m"

// helpers
static void pushRange(
  unsigned int (**arr)[2], size_t *count, size_t *capacity, unsigned int start, unsigned int end
);
static void pushCP(unsigned int **arr, size_t *count, size_t *capacity, unsigned int cp);
static void writeCP(FILE *out, unsigned int *codepoints, size_t cp_count);
static void writeRange(FILE *out, unsigned int (*ranges)[2], size_t range_count);

int main(void) {
  if (!SUPPORT_GLIBC) {
    fprintf(
      stderr,
      WARNING" Please compile on a glibc-based Linux distro (e.g. Debian).\n"
    );
    return 1;
  }

  // initialization
  if (!setlocale(LC_ALL, "C.UTF-8")) {
    fprintf(stderr, ERROR" C.UTF-8 not found.\n");
    return 1;
  }

  FILE *out = fopen("table.ts", "w");

  if (!out) {
    fprintf(stderr, ERROR" fopen() failed.\n");
    return 1;
  }

  fprintf(
    out,
    "/* Auto-generated lookup table from glibc's wcwidth results */\n"
    "\n// ---------- Build widths array ----------"
    "\nconst MAX_CP = 0x10FFFF;"
    "\nconst widths = new Uint8Array(MAX_CP + 1);\n"
    "\n// Start with everything as non-printable"
    "\nwidths.fill(0xFF);\n"
  );

  // individual code points
  unsigned int *codepoints0 = NULL;
  size_t cp_count0 = 0;
  size_t cp_capacity0 = 0;

  unsigned int *codepoints1 = NULL;
  size_t cp_count1 = 0;
  size_t cp_capacity1 = 0;

  unsigned int *codepoints2 = NULL;
  size_t cp_count2 = 0;
  size_t cp_capacity2 = 0;

  // code point ranges
  unsigned int (*ranges0)[2] = NULL;
  size_t range_count0 = 0;
  size_t range_capacity0 = 0;

  unsigned int (*ranges1)[2] = NULL;
  size_t range_count1 = 0;
  size_t range_capacity1 = 0;

  unsigned int (*ranges2)[2] = NULL;
  size_t range_count2 = 0;
  size_t range_capacity2 = 0;

  // collect code points
  unsigned int curr_range_start = 0;
  int curr_range_width = wcwidth(0);

  for (unsigned int cp = 1; cp <= 0x10FFFF; cp++) {
    int width = wcwidth(cp);

    if (width != curr_range_width || cp == 0x10FFFF) {
      if (curr_range_width >= 0) {  // skip -1
        unsigned int **cp_arr;
        size_t *cp_count, *cp_capacity;
        unsigned int (**range_arr)[2];
        size_t *range_count, *range_capacity;

        if (curr_range_width == 0) {
          cp_arr = &codepoints0;
          cp_count = &cp_count0;
          cp_capacity = &cp_capacity0;
          range_arr = &ranges0;
          range_count = &range_count0;
          range_capacity = &range_capacity0;
        } else if (curr_range_width == 1) {
          cp_arr = &codepoints1;
          cp_count = &cp_count1;
          cp_capacity = &cp_capacity1;
          range_arr = &ranges1;
          range_count = &range_count1;
          range_capacity = &range_capacity1;
        } else if (curr_range_width == 2) {
          cp_arr = &codepoints2;
          cp_count = &cp_count2;
          cp_capacity = &cp_capacity2;
          range_arr = &ranges2;
          range_count = &range_count2;
          range_capacity = &range_capacity2;
        } else {
          fprintf(
            stderr, ERROR "Unexpected width %d at U+%06X.\n", curr_range_width,
            curr_range_start
          );
          return 1;
        }

        unsigned int end_cp = (cp == 0x10FFFF ? cp : cp - 1);
        if (curr_range_start == end_cp) {
          pushCP(cp_arr, cp_count, cp_capacity, curr_range_start);
        } else {
          pushRange(
            range_arr, range_count, range_capacity, curr_range_start, end_cp
          );
        }
      }

      curr_range_start = cp;
      curr_range_width = width;
    }
  }

  fprintf(out, "\nconst cp0 = ");
  writeCP(out, codepoints0, cp_count0);
  free(codepoints0);

  fprintf(out, "\nconst cp1 = ");
  writeCP(out, codepoints1, cp_count1);
  free(codepoints1);

  fprintf(out, "\nconst cp2 = ");
  writeCP(out, codepoints2, cp_count2);
  free(codepoints2);

  fprintf(out, "\nconst range0 = ");
  writeRange(out, ranges0, range_count0);
  free(ranges0);

  fprintf(out, "\nconst range1 = ");
  writeRange(out, ranges1, range_count1);
  free(ranges1);

  fprintf(out, "\nconst range2 = ");
  writeRange(out, ranges2, range_count2);
  free(ranges2);

  fprintf(
    out,
    "\n\nconst cps = [cp0, cp1, cp2];"
    "\nfor (let i = 0; i <= 2; i++) {"
    "\n  let curr = 0;"
    "\n  for (const cp of cps[i]) {"
    "\n    curr += cp;"
    "\n    widths[curr] = i;"
    "\n  }"
    "\n}\n"
    "\nconst ranges = [range0, range1, range2];"
    "\nfor (let i = 0; i <= 2; i++) {"
    "\n  let curr = 0;"
    "\n  for (let j = 0; j < ranges[i].length; j += 2) {"
    "\n    const start = curr + ranges[i][j];"
    "\n    const end = start + ranges[i][j + 1];"
    "\n    for (let cp = start; cp <= end; cp++) {"
    "\n      widths[cp] = i;"
    "\n    }"
    "\n    curr = end;"
    "\n  }"
    "\n}\n"
    "\n// ---------- Build lookup table ----------"
    "\nconst SHIFT2 = 5;"
    "\nconst SHIFT1 = 11;"
    "\nconst MASK3 = (1 << SHIFT2) - 1;"
    "\nconst MASK2 = (1 << (SHIFT1 - SHIFT2)) - 1;"
    "\nconst TOP_BOUND = (0x10FFFF >> SHIFT1) + 1;\n"
    "\nconst L2Blocks: Uint32Array[] = [];"
    "\nconst leafBlocks: Uint8Array[] = [];\n"
    "\nfor (let i1 = 0; i1 < TOP_BOUND; i1++) {"
    "\n  const L2tab = new Uint32Array(MASK2 + 1);\n"
    "\n  for (let i2 = 0; i2 < L2tab.length; i2++) {"
    "\n    const baseCp = (i1 << SHIFT1) | (i2 << SHIFT2);"
    "\n    const leaf = new Uint8Array(MASK3 + 1);\n"
    "\n    for (let i3 = 0; i3 <= MASK3; i3++) {"
    "\n      const cp = baseCp + i3;"
    "\n      leaf[i3] = cp <= 0x10FFFF ? widths[cp] : 0xFF;"
    "\n    }\n"
    "\n    if (baseCp <= 0x10FFFF) {"
    "\n      leafBlocks.push(leaf);"
    "\n      L2tab[i2] = leafBlocks.length; // temporary index"
    "\n    }"
    "\n  }\n"
    "\n  L2Blocks.push(L2tab);"
    "\n}\n"
    "\nconst HEADER_BYTES = 5 * 4;"
    "\nconst L1_BYTES = TOP_BOUND * 4;"
    "\nconst L2_TOTAL = L2Blocks.reduce((sum, tab) => sum + tab.length * 4, 0);"
    "\nconst LEAF_TOTAL = leafBlocks.length * (MASK3 + 1);"
    "\nconst TOTAL_BYTES = HEADER_BYTES + L1_BYTES + L2_TOTAL + LEAF_TOTAL;\n"
    "\nconst LOOKUP_TABLE = new Uint8Array(TOTAL_BYTES);"
    "\nconst view = new DataView(LOOKUP_TABLE.buffer);\n"
    "\n// Header"
    "\nview.setUint32(0, SHIFT1, true);"
    "\nview.setUint32(4, TOP_BOUND, true);"
    "\nview.setUint32(8, SHIFT2, true);"
    "\nview.setUint32(12, MASK2, true);"
    "\nview.setUint32(16, MASK3, true);\n"
    "\n// Offsets"
    "\nlet L2ptr = HEADER_BYTES + L1_BYTES;"
    "\nlet LEAFptr = L2ptr + L2_TOTAL;\n"
    "\nfor (let i1 = 0; i1 < TOP_BOUND; i1++) {"
    "\n  const L2tab = L2Blocks[i1];"
    "\n  view.setUint32(20 + i1 * 4, L2ptr, true);\n"
    "\n  for (let i2 = 0; i2 < L2tab.length; i2++) {"
    "\n    const leafIndex = L2tab[i2];"
    "\n    if (leafIndex) {"
    "\n      view.setUint32(L2ptr + i2 * 4, LEAFptr, true);"
    "\n      LOOKUP_TABLE.set(leafBlocks[leafIndex - 1], LEAFptr);"
    "\n      LEAFptr += MASK3 + 1;"
    "\n    } else {"
    "\n      view.setUint32(L2ptr + i2 * 4, 0, true);"
    "\n    }"
    "\n  }\n"
    "\n  L2ptr += L2tab.length * 4;"
    "\n}\n"
    "\n/**"
    "\n * Wcwidth lookup table."
    "\n */"
    "\nexport const TABLE: Uint8Array = LOOKUP_TABLE;\n"
  );

  printf("\n"SUCCESS" table.ts generated successfully.\n");

  fclose(out);
  return 0;
}

static void pushRange(
  unsigned int (**arr)[2], size_t *count, size_t *capacity, unsigned int start, unsigned int end
) {
  if (*count >= *capacity) {
    *capacity = *capacity ? *capacity * 2 : 1024;
    *arr = realloc(*arr, *capacity * sizeof **arr);
    if (!*arr) {
      perror(ERROR" realloc failed.\n");
      exit(EXIT_FAILURE);
    }
  }

  (*arr)[*count][0] = start;
  (*arr)[*count][1] = end;
  (*count)++;
}

static void pushCP(unsigned int **arr, size_t *count, size_t *capacity, unsigned int cp) {
  if (*count >= *capacity) {
    *capacity = *capacity ? *capacity * 2 : 512;
    *arr = realloc(*arr, *capacity * sizeof **arr);
    if (!*arr) {
      perror(ERROR" realloc failed.\n");
      exit(EXIT_FAILURE);
    }
  }

  (*arr)[(*count)++] = cp;
}

static void writeCP(FILE *out, unsigned int *codepoints, size_t cp_count) {
  fprintf(out, "[");

  if (cp_count != 0) fprintf(out, "%d", codepoints[0]);
  if (cp_count > 1) fprintf(out, ",");

  for (size_t i = 1; i < cp_count; i++) {
    fprintf(out, "%d", codepoints[i] - codepoints[i - 1]);
    if (i != cp_count - 1) fprintf(out, ",");
  }

  fprintf(out, "];");
}

static void writeRange(FILE *out, unsigned int (*ranges)[2], size_t range_count) {
  fprintf(out, "[");

  if (range_count != 0) fprintf(out, "%d,%d", ranges[0][0], ranges[0][1] - ranges[0][0]);
  if (range_count > 1) fprintf(out, ",");

  for (size_t i = 1; i < range_count; i++) {
    fprintf(out, "%d,%d", ranges[i][0] - ranges[i - 1][1], ranges[i][1] - ranges[i][0]);
    if (i != range_count - 1) fprintf(out, ",");
  }

  fprintf(out, "];");
}
