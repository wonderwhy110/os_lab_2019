# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Все цели
all: parallel_min_max process_memory

# Программа из lab3
parallel_min_max: ../lab3/src/parallel_min_max.c ../lab3/src/find_min_max.c ../lab3/src/utils.c
	$(CC) $(CFLAGS) -o $@ $^

# Программа из lab4
process_memory: process_memory.c
	$(CC) $(CFLAGS) -o $@ $<

# Очистка
clean:
	rm -f parallel_min_max process_memory

# Тесты
test: all
	./process_memory
	./parallel_min_max --seed 123 --array_size 100 --pnum 2

.PHONY: all clean test
