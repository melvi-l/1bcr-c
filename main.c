#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <bits/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define FILE_PATH "measurements.txt"

#define HASH_MULT 257
#define HASH_SEED 146527
#define HASH_MAP_CAPACITY 16384 // >= 8 * ~400 stations

typedef struct {
  uint64_t hash;   // 8B
  const char *ptr; // 8B
  int len;         // 4B
} Key;

typedef struct {
  int min;   // 4B
  int sum;   // 4B
  int max;   // 4B
  int count; // 4B
} Value;

typedef struct {
  Key key;     // 24B
  Value value; // 16B
  int used;    // 4B
} Entry;

// open addressing map
static Entry map[HASH_MAP_CAPACITY];

static inline int key_equals(Key *k, const char *ptr, int len) {
  return (k->len == len) && (memcmp(k->ptr, ptr, len) == 0);
}

static uint64_t total_inserts = 0;
static uint64_t total_probes = 0;
static uint64_t total_insert_probes = 0;
static uint64_t total_collisions = 0;
static inline void insert_map(const char *ptr, int len, uint64_t hash,
                              int temperature) {

  total_inserts++;

  uint64_t index =
      hash & (HASH_MAP_CAPACITY - 1); // hash % capacity si non puissance 2
  uint64_t probes = 0;

  while (1) {
    Entry *e = &map[index];

    // insert
    if (!e->used) {
      e->used = 1;
      e->key.ptr = ptr;
      e->key.len = len;
      e->key.hash = hash;
      e->value.min = temperature;
      e->value.max = temperature;
      e->value.sum = temperature;
      e->value.count = 1;

      total_probes += probes;
      total_insert_probes += probes;
      return;
    }

    // if hit -> collision check
    if (e->key.hash == hash && key_equals(&e->key, ptr, len)) {
      // ifn collision -> update
      if (temperature < e->value.min)
        e->value.min = temperature;
      if (temperature > e->value.max)
        e->value.max = temperature;
      e->value.sum += temperature;
      e->value.count++;

      total_probes += probes;
      return;
    }

    total_collisions++;
    // else -> linear probing
    probes++;
    index = (index + 1) & (HASH_MAP_CAPACITY - 1);
  }
}

static int cmp_entry(const void *a, const void *b) {
  const Entry *ea = (const Entry *)a;
  const Entry *eb = (const Entry *)b;

  const Key *ka = &ea->key;
  const Key *kb = &eb->key;

  int min_len = (ka->len < kb->len ? ka->len : kb->len);

  // compare common bytes
  int r = memcmp(ka->ptr, kb->ptr, min_len);
  if (r != 0)
    return r;

  // sort by len
  return ka->len - kb->len;
}

void serialized_map(FILE *f) {
  Entry result[HASH_MAP_CAPACITY];
  // printf("start serialized_map\n");
  int j = 0;
  for (int i = 0; i < HASH_MAP_CAPACITY; i++) {
    // printf("%i === %lu -> %i\n", j, map[i].key.hash, map[i].value.sum);
    if (map[i].used) {
      result[j++] = map[i];
    }
  }
  qsort(result, j, sizeof(Entry), cmp_entry);

  fprintf(f, "{\n");
  for (int i = 0; i < j; i++) {
    Entry e = result[i];
    int mean = e.value.sum / e.value.count;
    fprintf(f, "\t%.*s=%d.%d/%d.%d/%d.%d, \n", e.key.len, e.key.ptr,
            e.value.min / 10, abs(e.value.min % 10), mean / 10, abs(mean % 10),
            e.value.max / 10, abs(e.value.max % 10));
  }
  fprintf(f, "}");
}

static inline void process_line(const char *line, int size) {
  const char *p = line;

  uint64_t hash = HASH_SEED;
  while (*p != ';') {
    hash = (hash << 5) - hash + (unsigned char)*p;
    p++;
  }
  int station_len = p - line;
  p++;

  int neg = (*p == '-');
  p += neg;

  int val = (p[0] - '0') * 10 + (p[1] - '0');
  p += 2;

  if (*p == '.') {
    p++;
    val = val * 10 + (*p - '0');
  }

  if (neg)
    val = -val;

  insert_map(line, station_len, hash, val);
}

char *data;
size_t size;
void process_file(const char *file_path) {
  int fd = open(FILE_PATH, O_RDONLY);
  if (fd < 0) {
    perror("open measurement file");
    exit(1);
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    perror("fstat error");
    close(fd);
    exit(1);
  }
  size = st.st_size;

  data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);

  if (data == MAP_FAILED) {
    perror("mmap error");
    close(fd);
    exit(1);
  }

  close(fd);

  size_t start = 0;
  char *p = data;
  char *end = data + size;
  while (p < end) {
    char *nl = memchr(p, '\n', end - p);
    if (!nl)
      break;
    process_line(p, nl - p);
    p = nl + 1;
  }
}

int main() {
  struct timespec start, end;

  FILE *f_result = fopen("result.txt", "w");
  if (!f_result) {
    fprintf(stderr, "result opening error\n");
    return 1;
  }

  clock_gettime(CLOCK_MONOTONIC, &start);

  process_file(FILE_PATH);

  fprintf(f_result, "Result:\n");
  serialized_map(f_result);

  clock_gettime(CLOCK_MONOTONIC, &end);

  double elapsed =
      (end.tv_sec - start.tv_sec) * 1e3 + (end.tv_nsec - start.tv_nsec) * 1e-6;
  fprintf(f_result, "\n\n====\n\nTime:\n%.9fms\n", elapsed);

  printf("Hashmap inserts: %lu\n", total_inserts);
  printf("Hashmap collisions: %lu\n", total_collisions);
  printf("Hashmap total probes: %lu\n", total_probes);
  printf("Hashmap total insert probes: %lu\n", total_insert_probes);
  printf("Average probes per insert: %.4f\n",
         (double)total_probes / total_inserts);

  fclose(f_result);
  return 0;
}
