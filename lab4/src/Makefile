# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Все цели
all: parallel_min_max process_memory

# Программа из lab3 - просто компилируем без зависимостей
parallel_min_max:
	$(CC) $(CFLAGS) -o $@ ../lab3/src/parallel_min_max.c ../lab3/src/find_min_max.c ../lab3/src/utils.c

# Программа из lab4
process_memory:
	$(CC) $(CFLAGS) -o $@ process_memory.c

# Очистка
clean:
	rm -f parallel_min_max process_memory

# Тесты
test: all
	@echo "=== Testing process_memory ==="
	./process_memory
	@echo "=== Testing parallel_min_max ==="
	./parallel_min_max --seed 123 --array_size 100 --pnum 2

.PHONY: all clean test
