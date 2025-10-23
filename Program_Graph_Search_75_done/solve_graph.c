// solve_graph.c
// Build: gcc -std=c11 -Wall -Wextra -O2 -o solve_graph solve_graph.c

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint8_t  val;        // ASCII value stored by the node
  uint16_t out_count;  // number of outgoing links
  uint16_t *outs;      // indices of destination nodes (size = out_count)
} Node;

typedef struct {
  Node *items;
  size_t count;
  size_t cap;
} NodeVec;

static void die(const char *msg) {
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(1);
}

static void *xmalloc(size_t n) {
  void *p = malloc(n);
  if (!p && n) { perror("malloc"); exit(1); }
  return p;
}

static void *xrealloc(void *p, size_t n) {
  void *q = realloc(p, n);
  if (!q && n) { perror("realloc"); exit(1); }
  return q;
}

static uint16_t read_u16_le(const unsigned char *p) {
  return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

int main(void) {
  const char *path = "input_stream.bin";

  // Read entire file into memory.
  FILE *fp = fopen(path, "rb");
  if (!fp) die("open input_stream.bin");
  if (fseek(fp, 0, SEEK_END) != 0) die("fseek");
  long fsize = ftell(fp);
  if (fsize < 0) { fprintf(stderr, "ftell failed\n"); exit(1); }
  if (fseek(fp, 0, SEEK_SET) != 0) die("fseek");

  unsigned char *buf = (unsigned char *)xmalloc((size_t)fsize);
  if (fread(buf, 1, (size_t)fsize, fp) != (size_t)fsize) die("fread");
  fclose(fp);

  // Parse nodes: [ val : 1 byte ][ length : uint16 LE ][ outs : length * uint16 LE ]
  NodeVec nv = {0};
  size_t pos = 0;
  while (pos < (size_t)fsize) {
    if (pos + 3 > (size_t)fsize) { fprintf(stderr, "Truncated node header\n"); exit(1); }
    uint8_t val = buf[pos + 0];
    uint16_t out_count = read_u16_le(&buf[pos + 1]);
    size_t need = 1 + 2 + (size_t)out_count * 2;
    if (pos + need > (size_t)fsize) { fprintf(stderr, "Truncated outs list\n"); exit(1); }

    if (nv.count == nv.cap) {
      nv.cap = nv.cap ? nv.cap * 2 : 64;
      nv.items = (Node *)xrealloc(nv.items, nv.cap * sizeof(Node));
    }
    Node *n = &nv.items[nv.count++];
    n->val = val;
    n->out_count = out_count;
    n->outs = (uint16_t *)xmalloc(out_count * sizeof(uint16_t));
    for (uint16_t i = 0; i < out_count; ++i) {
      n->outs[i] = read_u16_le(&buf[pos + 3 + i * 2]);
    }

    pos += need;
  }
  free(buf);

  if (nv.count == 0) {
    fprintf(stderr, "No nodes parsed\n");
    return 1;
  }

  // BFS from node 0 to first node with val == '}' (125)
  const uint8_t TARGET = (uint8_t)'}';
  size_t n = nv.count;
  bool *visited = (bool *)calloc(n, sizeof(bool));
  int  *prev    = (int  *)malloc(n * sizeof(int));
  size_t *queue = (size_t *)malloc(n * sizeof(size_t));
  if (!visited || !prev || !queue) { perror("alloc bfs"); return 1; }

  for (size_t i = 0; i < n; ++i) prev[i] = -1;

  size_t qh = 0, qt = 0;
  visited[0] = true;
  queue[qt++] = 0;

  ssize_t target_idx = -1;
  while (qh != qt) {
    size_t u = queue[qh++];

    if (nv.items[u].val == TARGET) {
      target_idx = (ssize_t)u;
      break;
    }

    for (uint16_t i = 0; i < nv.items[u].out_count; ++i) {
      uint16_t v = nv.items[u].outs[i];
      if (v >= n) continue;             // ignore bad edges
      if (!visited[v]) {
        visited[v] = true;
        prev[v] = (int)u;
        queue[qt++] = v;
      }
    }
  }

  if (target_idx == -1) {
    fprintf(stderr, "No path to target '}' found.\n");
    free(visited); free(prev); free(queue);
    for (size_t i = 0; i < nv.count; ++i) free(nv.items[i].outs);
    free(nv.items);
    return 1;
  }

  // Reconstruct path
  size_t path_len = 0;
  for (ssize_t cur = target_idx; cur != -1; cur = prev[cur]) ++path_len;

  char *flag = (char *)xmalloc(path_len + 1);
  flag[path_len] = '\0';

  ssize_t cur = target_idx;
  for (size_t i = path_len; i-- > 0; ) {
    flag[i] = (char)nv.items[cur].val;
    cur = prev[cur];
  }

  printf("%s\n", flag);

  // Cleanup
  free(flag);
  free(visited); free(prev); free(queue);
  for (size_t i = 0; i < nv.count; ++i) free(nv.items[i].outs);
  free(nv.items);
  return 0;
}
