#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
                        timeout = atoi(optarg);
                        if (timeout <= 0) {
                            printf("timeout must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 4:
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
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
               argv[0]);
        return 1;
    }

    // Set up signal handler for timeout
    if (timeout > 0) {
        signal(SIGALRM, timeout_handler);
        alarm(timeout);
    }

    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);
    int active_child_processes = 0;

    // Arrays to store child PIDs and pipes/file descriptors
    pid_t *child_pids = malloc(sizeof(pid_t) * pnum);
    int *pipe_fds = NULL;
    
    if (!with_files) {
        pipe_fds = malloc(sizeof(int) * pnum * 2);
    }

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Create pipes if not using files
    if (!with_files) {
        for (int i = 0; i < pnum; i++) {
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                return 1;
            }
            pipe_fds[i * 2] = pipefd[0]; // read end
            pipe_fds[i * 2 + 1] = pipefd[1]; // write end
        }
    }

    // Fork child processes
    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            // successful fork
            active_child_processes += 1;
            child_pids[i] = child_pid;
            
            if (child_pid == 0) {
                // child process
                if (timeout > 0) {
                    // Reset alarm in child process
                    alarm(0);
                }

                // Calculate the portion of array for this child
                int chunk_size = array_size / pnum;
                int start = i * chunk_size;
                int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;
                
                struct MinMax local_min_max = GetMinMax(array, start, end);

                if (with_files) {
                    // use files here
                    char filename[20];
                    sprintf(filename, "min_max_%d.txt", i);
                    FILE *file = fopen(filename, "w");
                    if (file) {
                        fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
                        fclose(file);
                    }
                } else {
                    // use pipe here
                    close(pipe_fds[i * 2]); // close read end in child
                    write(pipe_fds[i * 2 + 1], &local_min_max, sizeof(struct MinMax));
                    close(pipe_fds[i * 2 + 1]);
                }
                free(array);
                if (!with_files) free(pipe_fds);
                free(child_pids);
                exit(0);
            }

        } else {
            printf("Fork failed!\n");
            return 1;
        }
    }

    // Parent process - wait for children with timeout
    int completed_children = 0;
    
    while (active_child_processes > 0) {
        if (timeout_occurred) {
            printf("Timeout occurred! Killing child processes...\n");
            for (int i = 0; i < pnum; i++) {
                if (child_pids[i] > 0) {
                    kill(child_pids[i], SIGKILL);
                }
            }
            break;
        }

        int status;
        pid_t finished_pid = waitpid(-1, &status, WNOHANG);
        
        if (finished_pid > 0) {
            active_child_processes -= 1;
            completed_children++;
            
            // Mark this PID as completed
            for (int i = 0; i < pnum; i++) {
                if (child_pids[i] == finished_pid) {
                    child_pids[i] = 0;
                    break;
                }
            }
        } else if (finished_pid == 0) {
            // No child finished yet, sleep a bit
            usleep(10000); // 10ms
        } else {
            // Error
            perror("waitpid");
            break;
        }
    }

    // Cancel alarm if all children finished before timeout
    if (timeout > 0) {
        alarm(0);
    }

    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    // Collect results from completed children only
    for (int i = 0; i < completed_children; i++) {
        int min = INT_MAX;
        int max = INT_MIN;

        if (with_files) {
            // read from files
            char filename[20];
            sprintf(filename, "min_max_%d.txt", i);
            FILE *file = fopen(filename, "r");
            if (file) {
                fscanf(file, "%d %d", &min, &max);
                fclose(file);
                remove(filename); // clean up
            }
        } else {
            // read from pipes
            if (i < pnum) {
                struct MinMax local_min_max;
                close(pipe_fds[i * 2 + 1]); // close write end in parent
                read(pipe_fds[i * 2], &local_min_max, sizeof(struct MinMax));
                close(pipe_fds[i * 2]);
                min = local_min_max.min;
                max = local_min_max.max;
            }
        }

        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);
    free(child_pids);
    if (!with_files) free(pipe_fds);

    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    printf("Completed children: %d/%d\n", completed_children, pnum);
    fflush(NULL);
    return 0;
}
