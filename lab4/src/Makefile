# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread  # Добавил -pthread

# Все цели - ДОБАВИЛ parallel_sum
all: parallel_min_max process_memory parallel_sum

# Программа из lab3 - просто компилируем без зависимостей
parallel_min_max:
	$(CC) $(CFLAGS) -o $@ ../../lab3/src/parallel_min_max.c ../../lab3/src/find_min_max.c ../../lab3/src/utils.c

# Программа из lab4
process_memory:
	$(CC) $(CFLAGS) -o $@ process_memory.c

# Программа из задания 5 - НОВАЯ ЦЕЛЬ
parallel_sum:
	$(CC) $(CFLAGS) -o $@ parallel_sum.c sum_lib.c ../../lab3/src/utils.c

# Очистка - ДОБАВИЛ parallel_sum
clean:
	rm -f parallel_min_max process_memory parallel_sum

# Тесты - ДОБАВИЛ тест для parallel_sum
test: all
	@echo "=== Testing process_memory ==="
	./process_memory
	@echo "=== Testing parallel_min_max ==="
	./parallel_min_max --seed 123 --array_size 100 --pnum 2
	@echo "=== Testing parallel_sum ==="
	./parallel_sum --threads_num 4 --array_size 10000 --seed 123

.PHONY: all clean test
