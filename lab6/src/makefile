# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
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

# Запуск сервера (пример)
run-server:
	./$(SERVER) --port 20001 --tnum 4

# Запуск клиента (пример)
run-client:
	./$(CLIENT) --k 10 --mod 100 --servers servers.txt

# Отладочная сборка
debug: CFLAGS += -g -DDEBUG
debug: rebuild

# Профилировочная сборка
profile: CFLAGS += -pg
profile: LDFLAGS += -pg
profile: rebuild

# Установка (если нужно)
install: all
	cp $(CLIENT) $(SERVER) /usr/local/bin/

# Помощь
help:
	@echo "Доступные цели:"
	@echo "  all       - сборка клиента и сервера (по умолчанию)"
	@echo "  client    - сборка только клиента"
	@echo "  server    - сборка только сервера"
	@echo "  clean     - удаление скомпилированных файлов"
	@echo "  rebuild   - полная пересборка"
	@echo "  debug     - сборка с отладочной информацией"
	@echo "  profile   - сборка для профилирования"
	@echo "  run-server- запуск сервера с примерными параметрами"
	@echo "  run-client- запуск клиента с примерными параметрами"

# Псевдонимы
client: $(CLIENT)
server: $(SERVER)

# Объявление фейковых целей
.PHONY: all clean rebuild debug profile install help client server run-server run-client
