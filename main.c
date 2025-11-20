#include <bits/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FILE_PATH "measurements.txt"
#define MAX_ENTRY 1000000
#define MAX_STATION_LEN 100
#define MAX_TEMP_LEN 100

typedef struct {
  float min;
  float sum;
  float max;
  float count;
  char *key;
} Entry;

typedef struct {
  int size;
  int capacity;
  Entry buf[MAX_ENTRY];
} Map;

Map map = {0, MAX_ENTRY};

void add_map(Map *map, Entry entry) {
  for (int j = 0; j < map->size; j++) {
    if (!strcmp(map->buf[j].key, entry.key)) {
      // update
      if (map->buf[j].min > entry.min)
        map->buf[j].min = entry.min;
      if (map->buf[j].max < entry.max)
        map->buf[j].max = entry.max;
      map->buf[j].count += entry.count;
      map->buf[j].sum += entry.sum;
      return;
    }
  }
  if (map->size >= map->capacity) {
    fprintf(stderr, "Map overflow\n");
    exit(1);
  }
  // copy key
  size_t len = strlen(entry.key);
  char *key = malloc(len + 1);
  memcpy(key, entry.key, len + 1);

  // add
  map->buf[map->size] = (Entry){.min = entry.min,
                                .sum = entry.sum,
                                .max = entry.max,
                                .count = entry.count,
                                .key = key};
  map->size++;
}

void serialized_map(Map *map, FILE *f) {

  fprintf(f, "{\n");
  for (int j = 0; j < map->size; j++) {
    Entry curr = map->buf[j];
    float mean = curr.sum / curr.count;
    fprintf(f, "\t%s=%f/%f/%f, \n", curr.key, curr.min, mean, curr.max);
  }
  fprintf(f, "}");
}

void destroy_map(Map *map) {
  for (int j = 0; j < map->size; j++) {
    free(map->buf[j].key);
  }
}

int bcr() {
  FILE *f = fopen(FILE_PATH, "r");
  if (!f) {
    fprintf(stderr, "Unable to open file\n");
    exit(1);
  }

  char c;
  int i;
  int len;
  char station[MAX_STATION_LEN];
  char temp[MAX_TEMP_LEN];
  int is_station = 1;
  while ((c = fgetc(f)) != EOF) {
    if (c == '\n') {
      temp[i] = '\0';

      char *end;
      float value = strtof(temp, &end);

      add_map(&map, (Entry){.min = value,
                            .sum = value,
                            .max = value,
                            .count = 1,
                            .key = station});

      // reset
      i = 0;
      is_station = 1;
      station[0] = '\0';
      temp[0] = '\0';
      continue;
    }
    if (c == ';') {
      station[i++] = '\0';
      is_station = 0;
      i = 0;
      continue;
    }
    //
    if (is_station) {
      station[i++] = c;
    } else {
      temp[i++] = c;
    }
  }

  fclose(f);
  return 0;
}

int main() {
  struct timespec start, end;

  clock_gettime(CLOCK_MONOTONIC, &start);
  bcr();
  clock_gettime(CLOCK_MONOTONIC, &end);

  FILE *f_result = fopen("result.txt", "w");
  if (!f_result) {
    fprintf(stderr, "result opening error\n");
    return 1;
  }

  double elapsed =
      (end.tv_sec - start.tv_sec) * 1e3 + (end.tv_nsec - start.tv_nsec) * 1e-6;
  fprintf(f_result, "Time:\n%.9fms\n", elapsed);

  fprintf(f_result, "Result:\n");
  serialized_map(&map, f_result);

  fclose(f_result);
  destroy_map(&map);
  return 0;
}
