#include <bits/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define FILE_PATH "measurements.txt"
#define BLOCK_SIZE (64 * 1024 * 1024) // 64MB
char *block_buf;
#define MAX_ENTRY 1000000
#define MAX_STATION_LEN 100
#define MAX_TEMP_LEN 100

typedef struct {
  int min;
  int sum;
  int max;
  int count;
  char *key;
} Entry;

typedef struct {
  int size;
  int capacity;
  Entry buf[MAX_ENTRY];
} Map;

Map map = {0, MAX_ENTRY};

void add_map(Map *map, const char *key, int key_len, int min, int sum, int max,
             int count) {
  for (int j = 0; j < map->size; j++) {
    if (!strncmp(map->buf[j].key, key, key_len)) {
      // update
      if (map->buf[j].min > min)
        map->buf[j].min = min;
      if (map->buf[j].max < max)
        map->buf[j].max = max;
      map->buf[j].count += count;
      map->buf[j].sum += sum;
      return;
    }
  }
  if (map->size >= map->capacity) {
    fprintf(stderr, "Map overflow\n");
    exit(1);
  }
  // copy key
  char *_key = malloc(key_len + 1);
  memcpy(_key, key, key_len);
  _key[key_len] = '\0';

  // add
  map->buf[map->size] =
      (Entry){.min = min, .sum = sum, .max = max, .count = count, .key = _key};
  map->size++;
}

void serialized_map(Map *map, FILE *f) {

  fprintf(f, "{\n");
  for (int j = 0; j < map->size; j++) {
    Entry curr = map->buf[j];
    int mean = curr.sum / curr.count;
    fprintf(f, "\t%s=%d.%d/%d.%d/%d.%d, \n", curr.key, curr.min / 10,
            abs(curr.min % 10), mean / 10, abs(mean % 10), curr.max / 10,
            abs(curr.max % 10));
  }
  fprintf(f, "}");
}

static int cmp_entry(const void *a, const void *b) {
  const Entry *ea = (const Entry *)a;
  const Entry *eb = (const Entry *)b;
  return strcmp(ea->key, eb->key);
}
void sort_map(Map *map) {
  qsort(&map->buf, map->size, sizeof(Entry), cmp_entry);
}

void destroy_map(Map *map) {
  for (int j = 0; j < map->size; j++) {
    free(map->buf[j].key);
  }
}

void process_line(const char *line, int size) {
  char temp[MAX_TEMP_LEN];
  int is_station = 1;
  int i = 0;
  while (i < size && line[i] != ';') {
    i++;
  }

  int station_len = i;
  i++;

  int neg = 0;
  if (line[i] == '-') {
    // printf("neg");
    neg = 1;
    i++;
  }

  int val = 0;
  while (i < size && line[i] != '.') {
    val = val * 10 + (line[i] - '0');
    i++;
  }

  if (line[i] == '.') {
    i++;
    val = val * 10 + (line[i] - '0');
  }

  if (neg)
    val = -val;

  // printf("%.*s -- %i\n", station_len, line, val);
  add_map(&map, line, station_len, val, val, val, 1);
}

void process_file(const char *file_path) {
  int fd = open(FILE_PATH, O_RDONLY);
  if (fd < 0) {
    perror("open measurement file");
    exit(1);
  }

  block_buf = malloc(BLOCK_SIZE + 1024);
  if (!block_buf) {
    perror("malloc block buffer");
    exit(1);
  }

  int rem = 0;
  while (1) {
    int n = read(fd, block_buf + rem, BLOCK_SIZE);
    if (n < 0) {
      perror("read error");
      exit(1);
    }
    if (n == 0 && rem == 0)
      break;

    int total = rem + n;

    int last_nl = -1;

    for (int i = total - 1; i >= 0; i--) { // check bound
      if (block_buf[i] == '\n') {
        last_nl = i;
        break;
      }
    }

    if (last_nl < 0) {
      perror("line longer than buffer");
      exit(1);
    }

    int start = 0;
    for (int i = 0; i <= last_nl; i++) {
      if (block_buf[i] == '\n') {
        process_line(&block_buf[start], i - start); // exclude \n
        start = i + 1;
      }
    }

    rem = total - last_nl - 1;
    memcpy(block_buf, block_buf + last_nl + 1,
           rem); // quite a stretch, should be a memmove; supposed no overlap
                 // (should be good on >16MB)

    if (n == 0) {
      if (rem > 0) {
        process_line(block_buf, rem);
      }
      break;
    }
  }

  close(fd);
  free(block_buf);
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

  sort_map(&map);

  fprintf(f_result, "Result:\n");
  serialized_map(&map, f_result);

  clock_gettime(CLOCK_MONOTONIC, &end);

  double elapsed =
      (end.tv_sec - start.tv_sec) * 1e3 + (end.tv_nsec - start.tv_nsec) * 1e-6;
  fprintf(f_result, "\n\n====\n\nTime:\n%.9fms\n", elapsed);

  fclose(f_result);
  destroy_map(&map);
  return 0;
}
