CC=gcc
CFLAGS=-I.
TARGETS=sequential_min_max parallel_min_max exec_sequential
OBJECTS=utils.o find_min_max.o

# Основная цель - сборка всех программ
all: $(TARGETS)

# Сборка последовательной версии
sequential_min_max: $(OBJECTS) sequential_min_max.o
	$(CC) -o $@ $(OBJECTS) sequential_min_max.o $(CFLAGS)

sequential_min_max.o: sequential_min_max.c
	$(CC) -o $@ -c sequential_min_max.c $(CFLAGS)

# Сборка параллельной версии
parallel_min_max: $(OBJECTS) parallel_min_max.o
	$(CC) -o $@ $(OBJECTS) parallel_min_max.o $(CFLAGS)

parallel_min_max.o: parallel_min_max.c
	$(CC) -o $@ -c parallel_min_max.c $(CFLAGS)

# Сборка программы с exec
exec_sequential: exec_sequential.o
	$(CC) -o $@ exec_sequential.o $(CFLAGS)

exec_sequential.o: exec_sequential.c
	$(CC) -o $@ -c exec_sequential.c $(CFLAGS)

# Компиляция объектных файлов
utils.o: utils.c utils.h
	$(CC) -o $@ -c utils.c $(CFLAGS)

find_min_max.o: find_min_max.c find_min_max.h
	$(CC) -o $@ -c find_min_max.c $(CFLAGS)

# Очистка
clean:
	rm -f $(OBJECTS) $(TARGETS) *.o

# Псевдоцель
.PHONY: all clean
