#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_zombies>\n", argv[0]);
        return 1;
    }

    int num_zombies = atoi(argv[1]);
    
    printf("Parent PID: %d\n", getpid());
    printf("Creating %d zombie processes...\n", num_zombies);
    printf("Run 'ps aux | grep defunct' in another terminal to see zombies\n");
    printf("Press Enter to clean up zombies and exit...\n");

    // Создаем зомби-процессы
    for (int i = 0; i < num_zombies; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Дочерний процесс
            printf("Child %d (PID: %d) created and exiting immediately\n", i, getpid());
            exit(0); // Завершаемся сразу, становимся зомби
        } else if (pid > 0) {
            // Родительский процесс
            printf("Parent: created child %d with PID: %d\n", i, pid);
        } else {
            perror("fork failed");
            return 1;
        }
    }

    // Родитель НЕ вызывает wait() - процессы становятся зомби
    printf("\n%d zombie processes created!\n", num_zombies);
    printf("They will appear as '<defunct>' in process list\n");
    printf("Check with: ps aux | grep defunct\n\n");

    // Ждем нажатия Enter
    getchar();

    // Теперь собираем зомби
    printf("Cleaning up zombies...\n");
    while (wait(NULL) > 0) {
        // Ждем завершения всех дочерних процессов
    }
    
    printf("All zombies cleaned up!\n");
    return 0;
}
