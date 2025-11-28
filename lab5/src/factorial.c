#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

// Глобальные переменные
int k = 0;
int pnum = 1;
int mod = 1;
unsigned long long result = 1;

// Семафоры
sem_t semaphore;

// Функция для обработки аргументов командной строки
void parse_args(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            mod = atoi(argv[i] + 6);
        }
    }
}

// Структура для передачи данных в поток
typedef struct {
    int thread_id;
    int start;
    int end;
} thread_data_t;

// Функция, выполняемая в каждом потоке
void* calculate_partial_factorial(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    unsigned long long partial_result = 1;
    
    printf("Thread %d: calculating from %d to %d\n", 
           data->thread_id, data->start, data->end);
    
    // Вычисление частичного факториала
    for (int i = data->start; i <= data->end; i++) {
        partial_result = (partial_result * i) % mod;
    }
    
    printf("Thread %d: partial result = %llu\n", 
           data->thread_id, partial_result);
    
    // Захватываем семафор для обновления общего результата
    sem_wait(&semaphore);
    result = (result * partial_result) % mod;
    sem_post(&semaphore);
    
    return NULL;
}

int main(int argc, char *argv[]) {
    // Парсинг аргументов командной строки
    parse_args(argc, argv);
    
    // Проверка корректности входных данных
    if (k <= 0 || pnum <= 0 || mod <= 1) {
        printf("Usage: %s -k <number> --pnum=<threads> --mod=<modulus>\n", argv[0]);
        printf("k must be > 0, pnum must be > 0, mod must be > 1\n");
        return 1;
    }
    
    printf("Calculating %d! mod %d using %d threads\n", k, mod, pnum);
    
    // Инициализация семафора (1 - бинарный семафор)
    if (sem_init(&semaphore, 0, 1) != 0) {
        perror("sem_init");
        return 1;
    }
    
    // Создание потоков
    pthread_t threads[pnum];
    thread_data_t thread_data[pnum];
    
    // Распределение работы между потоками
    int numbers_per_thread = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;
    
    for (int i = 0; i < pnum; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].start = current_start;
        
        // Распределяем остаток по первым потокам
        int numbers_for_this_thread = numbers_per_thread;
        if (i < remainder) {
            numbers_for_this_thread++;
        }
        
        thread_data[i].end = current_start + numbers_for_this_thread - 1;
        current_start = thread_data[i].end + 1;
        
        // Создаем поток
        if (pthread_create(&threads[i], NULL, calculate_partial_factorial, &thread_data[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }
    
    // Вывод результата
    printf("\nFinal result: %d! mod %d = %llu\n", k, mod, result);
    
    // Уничтожение семафора
    sem_destroy(&semaphore);
    
    return 0;
}
