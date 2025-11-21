# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -pedantic
LDFLAGS = -lpthread

# Имена исполняемых файлов
CLIENT = client
SERVER = server

# Исходные файлы
CLIENT_SRC = client.c
SERVER_SRC = server.c

# Объектные файлы
CLIENT_OBJ = client.o
SERVER_OBJ = server.o

# Цель по умолчанию
all: $(CLIENT) $(SERVER)

# Сборка клиента
$(CLIENT): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_OBJ) $(LDFLAGS)

# Сборка сервера
$(SERVER): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_OBJ) $(LDFLAGS)

# Компиляция клиента
$(CLIENT_OBJ): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC) -o $(CLIENT_OBJ)

# Компиляция сервера
$(SERVER_OBJ): $(SERVER_SRC)
	$(CC) $(CFLAGS) -c $(SERVER_SRC) -o $(SERVER_OBJ)

# Очистка
clean:
	rm -f $(CLIENT) $(SERVER) $(CLIENT_OBJ) $(SERVER_OBJ)

# Пересборка
rebuild: clean all

.PHONY: all clean rebuild
