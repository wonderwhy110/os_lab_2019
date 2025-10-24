#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s seed array_size\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    
    if (pid == 0) {
        // Дочерний процесс - запускаем sequential_min_max
        printf("Child process: Starting sequential_min_max...\n");
        
        // Формируем аргументы для exec
        char *args[] = {"./sequential_min_max", argv[1], argv[2], NULL};
        
        // Заменяем образ процесса на sequential_min_max
        execvp(args[0], args);
        
        // Если exec вернул управление - произошла ошибка
        perror("execvp failed");
        exit(1);
    } else {
        // Родительский процесс - ждем завершения дочернего
        printf("Parent process: Waiting for child (PID: %d)...\n", pid);
        
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Parent: Child process exited with status: %d\n", WEXITSTATUS(status));
        } else {
            printf("Parent: Child process terminated abnormally\n");
        }
    }
    
    return 0;
}
