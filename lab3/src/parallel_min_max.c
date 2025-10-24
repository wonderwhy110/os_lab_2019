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
#include <sys/stat.h>

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
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--by_files]\n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  // Создаем пайпы или файлы для каждого процесса
  int pipes[2 * pnum];
  char filenames[pnum][32];
  
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes + i * 2) < 0) {
        printf("Failed to create pipe for process %d\n", i);
        return 1;
      }
    }
  } else {
    for (int i = 0; i < pnum; i++) {
      snprintf(filenames[i], sizeof(filenames[i]), "min_max_%d.txt", i);
    }
  }

  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Создаем дочерние процессы
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process
        
        // Вычисляем диапазон для текущего процесса
        int chunk_size = array_size / pnum;
        int begin = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;
        
        // Находим min/max в своем диапазоне
        struct MinMax local_min_max = GetMinMax(array, begin, end);
        
        if (with_files) {
          // use files here
          FILE *file = fopen(filenames[i], "w");
          if (file == NULL) {
            printf("Failed to open file %s\n", filenames[i]);
            exit(1);
          }
          fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
          fclose(file);
        } else {
          // use pipe here
          close(pipes[i * 2]); // закрываем чтение
          
          write(pipes[i * 2 + 1], &local_min_max.min, sizeof(int));
          write(pipes[i * 2 + 1], &local_min_max.max, sizeof(int));
          
          close(pipes[i * 2 + 1]); // закрываем запись
        }
        free(array);
        exit(0);
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Родительский процесс ждет завершения всех детей
  while (active_child_processes > 0) {
    wait(NULL); // ждем завершения любого дочернего процесса
    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  // Собираем результаты от всех процессов
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // read from files
      FILE *file = fopen(filenames[i], "r");
      if (file == NULL) {
        printf("Failed to open file %s\n", filenames[i]);
        return 1;
      }
      fscanf(file, "%d %d", &min, &max);
      fclose(file);
      
      // Удаляем временный файл
      remove(filenames[i]);
    } else {
      // read from pipes
      close(pipes[i * 2 + 1]); // закрываем запись
      
      read(pipes[i * 2], &min, sizeof(int));
      read(pipes[i * 2], &max, sizeof(int));
      
      close(pipes[i * 2]); // закрываем чтение
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
  printf("Number of processes: %d\n", pnum);
  fflush(NULL);
  return 0;
}
