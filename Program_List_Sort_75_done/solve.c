// solve.c
// Linked List Sorting + XOR flag (portable C, no mmap)
// Build: gcc -std=c11 -Wall -Wextra -O2 -o solve solve.c

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint32_t file_offset;  // for debugging
  uint16_t flink;        // next node offset (from file start), 0xFFFF == end
  uint16_t value;        // sort key
  uint16_t length;       // number of bytes in piece
  uint8_t *piece;        // length bytes; heap-owned
} Node;

static int cmp_value_asc(const void *a, const void *b) {
  const Node *na = (const Node *)a, *nb = (const Node *)b;
  if (na->value < nb->value) return -1;
  if (na->value > nb->value) return 1;
  return 0;
}

// read little-endian u16 from a FILE at current position
static bool fread_u16_le(FILE *f, uint16_t *out) {
  unsigned char b[2];
  if (fread(b, 1, 2, f) != 2) return false;
  *out = (uint16_t)(b[0] | ((uint16_t)b[1] << 8));
  return true;
}

// safe seek
static bool fseek_checked(FILE *f, long off) {
  if (off < 0) return false;
  return fseek(f, off, SEEK_SET) == 0;
}

int main(void) {
  const char *path = "input_stream.bin";
  FILE *fp = fopen(path, "rb");
  if (!fp) {
    fprintf(stderr, "open failed: %s\n", strerror(errno));
    return 1;
  }

  // get file size
  if (fseek(fp, 0, SEEK_END) != 0) {
    fprintf(stderr, "fseek end failed: %s\n", strerror(errno));
    fclose(fp);
    return 1;
  }
  long fsize = ftell(fp);
  if (fsize < 0) {
    fprintf(stderr, "ftell failed\n");
    fclose(fp);
    return 1;
  }

  // follow the singly-linked list starting at offset 0
  // we’ll cap nodes to a reasonable upper bound to avoid infinite loops
  const size_t MAX_NODES = 10000;
  Node *nodes = NULL;
  size_t count = 0, cap = 0;

  // simple visited-set by offset (file is small)
  bool *visited = (bool *)calloc((size_t)fsize + 1, 1);
  if (!visited) {
    fprintf(stderr, "oom\n");
    fclose(fp);
    return 1;
  }

  uint16_t off = 0;  // start of first node
  while (1) {
    if (off == 0xFFFF) break;              // end-of-list sentinel
    if (off >= fsize) {                    // bounds check
      fprintf(stderr, "bad flink offset: %u\n", off);
      break;
    }
    if (visited[off]) {                    // loop protection
      fprintf(stderr, "cycle detected at offset %u\n", off);
      break;
    }
    visited[off] = true;

    if (!fseek_checked(fp, (long)off)) {
      fprintf(stderr, "seek %u failed\n", off);
      break;
    }

    uint16_t flink, value, length;
    if (!fread_u16_le(fp, &flink) ||
        !fread_u16_le(fp, &value) ||
        !fread_u16_le(fp, &length)) {
      fprintf(stderr, "short read at offset %u\n", off);
      break;
    }

    // bounds check for piece bytes
    long piece_start = (long)off + 6;
    long piece_end = piece_start + (long)length;
    if (length > 0 && (piece_start < 0 || piece_end > fsize)) {
      fprintf(stderr, "invalid length at %u (len=%u)\n", off, length);
      break;
    }

    // grow array
    if (count == cap) {
      size_t ncap = cap ? cap * 2 : 64;
      if (ncap > MAX_NODES) ncap = MAX_NODES;
      Node *tmp = (Node *)realloc(nodes, ncap * sizeof(Node));
      if (!tmp) {
        fprintf(stderr, "oom\n");
        break;
      }
      nodes = tmp;
      cap = ncap;
    }

    Node *n = &nodes[count++];
    n->file_offset = off;
    n->flink = flink;
    n->value = value;
    n->length = length;
    n->piece = (uint8_t *)calloc((size_t)length, 1);
    if (!n->piece && length) {
      fprintf(stderr, "oom piece\n");
      break;
    }
    if (length) {
      if (!fseek_checked(fp, piece_start) ||
          fread(n->piece, 1, length, fp) != (size_t)length) {
        fprintf(stderr, "failed reading piece at %u\n", off);
        break;
      }
    }

    off = flink;  // follow the list
  }

  fclose(fp);
  free(visited);

  if (count == 0) {
    fprintf(stderr, "no nodes parsed\n");
    free(nodes);
    return 1;
  }

  // sort by value ascending
  qsort(nodes, count, sizeof(Node), cmp_value_asc);

  // XOR even indices (0,2,4,...) with zero-extension
  size_t max_len = 0;
  for (size_t i = 0; i < count; i += 2) {
    if (nodes[i].length > max_len) max_len = nodes[i].length;
  }

  uint8_t *flag = (uint8_t *)calloc(max_len + 1, 1);  // +1 for NUL
  if (!flag && max_len) {
    fprintf(stderr, "oom flag\n");
    for (size_t i = 0; i < count; ++i) free(nodes[i].piece);
    free(nodes);
    return 1;
  }

  for (size_t i = 0; i < count; i += 2) {
    for (size_t j = 0; j < nodes[i].length; ++j) {
      flag[j] ^= nodes[i].piece[j];
    }
  }

  // print as ASCII
  printf("%s\n", (char *)flag);

  // cleanup
  for (size_t i = 0; i < count; ++i) free(nodes[i].piece);
  free(nodes);
  free(flag);
  return 0;
}
