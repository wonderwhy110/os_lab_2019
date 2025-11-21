# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -pedantic
LDFLAGS = -lpthread

# Имена исполняемых файлов
CLIENT = client
SERVER = server
COMMON_LIB = common

# Исходные файлы
CLIENT_SRC = client.c
SERVER_SRC = server.c
COMMON_SRC = common.c

# Объектные файлы
CLIENT_OBJ = client.o
SERVER_OBJ = server.o
COMMON_OBJ = common.o

# Цель по умолчанию
all: $(CLIENT) $(SERVER)

# Сборка клиента
$(CLIENT): $(CLIENT_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_OBJ) $(COMMON_OBJ) $(LDFLAGS)

# Сборка сервера
$(SERVER): $(SERVER_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_OBJ) $(COMMON_OBJ) $(LDFLAGS)

# Компиляция клиента
$(CLIENT_OBJ): $(CLIENT_SRC) common.h
	$(CC) $(CFLAGS) -c $(CLIENT_SRC) -o $(CLIENT_OBJ)

# Компиляция сервера
$(SERVER_OBJ): $(SERVER_SRC) common.h
	$(CC) $(CFLAGS) -c $(SERVER_SRC) -o $(SERVER_OBJ)

# Компиляция общей библиотеки
$(COMMON_OBJ): $(COMMON_SRC) common.h
	$(CC) $(CFLAGS) -c $(COMMON_SRC) -o $(COMMON_OBJ)

# Очистка
clean:
	rm -f $(CLIENT) $(SERVER) $(CLIENT_OBJ) $(SERVER_OBJ) $(COMMON_OBJ)

# Пересборка
rebuild: clean all

# Запуск сервера (пример)
run-server:
	./$(SERVER) --port 20001 --tnum 4

# Запуск клиента (пример)
run-client:
	./$(CLIENT) --k 10 --mod 100 --servers servers.txt

# Отладочная сборка
debug: CFLAGS += -g -DDEBUG
debug: rebuild

.PHONY: all clean rebuild debug run-server run-client
