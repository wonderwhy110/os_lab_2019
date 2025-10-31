#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

volatile sig_atomic_t timeout_occurred = 0;

void timeout_handler(int sig) {
    timeout_occurred = 1;
}

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    int timeout = 0; // 0 means no timeout
    bool with_files = false;

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"timeout", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {0, 0, 0, 0}
        };

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
                        timeout = atoi(optarg);
                        break;
                    case 4:
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
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n", argv[0]);
        return 1;
    }

    // Set up signal handler for timeout
    if (timeout > 0) {
        signal(SIGALRM, timeout_handler);
        alarm(timeout);
    }

    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);

    int pipes[2 * pnum];
    pid_t child_pids[pnum]; // Store child PIDs for potential killing
    
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

    // Fork child processes
    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            if (child_pid == 0) {
                // child process
                // Reset alarm in child process
                if (timeout > 0) {
                    alarm(0);
                }

                int begin = i * (array_size / pnum);
                int end = (i == pnum - 1) ? array_size : (i + 1) * (array_size / pnum);
                struct MinMax local_min_max = GetMinMax(array, begin, end);
                
                if (with_files) {
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
            } else {
                // parent process - store child PID
                child_pids[i] = child_pid;
            }
        } else {
            printf("Fork failed!\n");
            return 1;
        }
    }

    // Parent process - wait for children with timeout handling
    int active_children = pnum;
    int completed_children = 0;

    while (active_children > 0) {
        if (timeout_occurred) {
            printf("Timeout occurred! Killing child processes...\n");
            for (int i = 0; i < pnum; i++) {
                if (child_pids[i] > 0) {
                    kill(child_pids[i], SIGKILL);
                }
            }
            break;
        }

        // Non-blocking wait
        int status;
        pid_t finished_pid = waitpid(-1, &status, WNOHANG);
        
        if (finished_pid > 0) {
            // A child finished
            active_children--;
            completed_children++;
            
            // Mark this PID as finished
            for (int i = 0; i < pnum; i++) {
                if (child_pids[i] == finished_pid) {
                    child_pids[i] = 0; // Mark as finished
                    break;
                }
            }
        } else if (finished_pid == 0) {
            // No child has finished yet, sleep briefly
            usleep(10000); // 10ms
        } else {
            // Error in waitpid
            perror("waitpid");
            break;
        }
    }

    // Cancel alarm if all children finished before timeout
    if (timeout > 0) {
        alarm(0);
    }

    struct MinMax min_max = {INT_MAX, INT_MIN};

    // Collect results from completed children
    for (int i = 0; i < pnum; i++) {
        // Skip if this child was killed by timeout
        if (timeout_occurred && child_pids[i] != 0) {
            continue;
        }

        int min, max;
        
        if (with_files) {
            char filename[20];
            sprintf(filename, "mm_%d.txt", i);
            FILE *f = fopen(filename, "r");
            if (f) {
                fscanf(f, "%d %d", &min, &max);
                fclose(f);
                remove(filename);
            } else {
                // File might not exist if child was killed
                continue;
            }
        } else {
            close(pipes[i * 2 + 1]);
            if (read(pipes[i * 2], &min, sizeof(int)) > 0 && 
                read(pipes[i * 2], &max, sizeof(int)) > 0) {
                // Successfully read data
            } else {
                // Pipe might be empty if child was killed
                close(pipes[i * 2]);
                continue;
            }
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
    printf("Completed children: %d/%d\n", completed_children, pnum);
    
    if (timeout_occurred) {
        printf("Program terminated due to timeout\n");
    }
    
    return 0;
}
