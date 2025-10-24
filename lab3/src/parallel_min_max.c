#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            break;
          case 1:
            array_size = atoi(optarg);
            break;
          case 2:
            pnum = atoi(optarg);
            break;
          case 3:
            with_files = true;
            break;
        }
        break;
      case 'f':
        with_files = true;
        break;
    }
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n", argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  int pipes[2 * pnum];
  // УБИРАЕМ filenames из здесь - создаем в дочерних процессах
  
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes + i * 2) < 0) {
        printf("Pipe failed\n");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      if (child_pid == 0) {
        // child process
        int begin = i * (array_size / pnum);
        int end = (i == pnum - 1) ? array_size : (i + 1) * (array_size / pnum);
        struct MinMax local_min_max = GetMinMax(array, begin, end);
        
        if (with_files) {
          // СОЗДАЕМ имя файла в дочернем процессе
          char filename[20];
          sprintf(filename, "mm_%d.txt", i);
          FILE *f = fopen(filename, "w");
          fprintf(f, "%d %d", local_min_max.min, local_min_max.max);
          fclose(f);
        } else {
          close(pipes[i * 2]);
          write(pipes[i * 2 + 1], &local_min_max.min, sizeof(int));
          write(pipes[i * 2 + 1], &local_min_max.max, sizeof(int));
          close(pipes[i * 2 + 1]);
        }
        free(array);
        exit(0);
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  while (wait(NULL) > 0);

  struct MinMax min_max = {INT_MAX, INT_MIN};

  for (int i = 0; i < pnum; i++) {
    int min, max;
    
    if (with_files) {
      // СОЗДАЕМ такое же имя файла для чтения
      char filename[20];
      sprintf(filename, "mm_%d.txt", i);
      FILE *f = fopen(filename, "r");
      fscanf(f, "%d %d", &min, &max);
      fclose(f);
      remove(filename);
    } else {
      close(pipes[i * 2 + 1]);
      read(pipes[i * 2], &min, sizeof(int));
      read(pipes[i * 2], &max, sizeof(int));
      close(pipes[i * 2]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  return 0;
}
