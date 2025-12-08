#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "pthread.h"
#include "common.h"

struct FactorialArgs {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};

uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1;
  for (uint64_t i = args->begin; i <= args->end; i++) {
    ans = MultModulo(ans, i, args->mod);
  }
  return ans;
}

void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  uint64_t *result = malloc(sizeof(uint64_t));
  *result = Factorial(fargs);
  return (void *)result;
}

int main(int argc, char **argv) {
  int tnum = -1;
  int port = -1;

  while (true) {
    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0: 
            port = atoi(optarg); 
            break;
          case 1: 
            tnum = atoi(optarg); 
            break;
          default: 
            printf("Index %d is out of options\n", option_index);
        }
        break;
      default: 
        fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  // 1. СОЗДАНИЕ IPv6 СЕРВЕРНОГО СОКЕТА
  int server_fd = socket(AF_INET6, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fprintf(stderr, "Can not create server socket: %s\n", strerror(errno));
    return 1;
  }

  // 2. ОПЦИЯ ДЛЯ БЫСТРОГО ПЕРЕЗАПУСКА СЕРВЕРА
  int opt_val = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val)) < 0) {
    fprintf(stderr, "setsockopt SO_REUSEADDR failed: %s\n", strerror(errno));
    close(server_fd);
    return 1;
  }

  // 3. ОБЯЗАТЕЛЬНО: ТОЛЬКО IPv6, БЕЗ DUAL-STACK
  int v6only = 1;
  if (setsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only)) < 0) {
    fprintf(stderr, "setsockopt IPV6_V6ONLY failed: %s\n", strerror(errno));
    close(server_fd);
    return 1;
  }

  // 4. НАСТРОЙКА IPv6 АДРЕСА СЕРВЕРА
  struct sockaddr_in6 server;
  memset(&server, 0, sizeof(server));
  server.sin6_family = AF_INET6;
  server.sin6_port = htons((uint16_t)port);
  server.sin6_addr = in6addr_any;  // Слушаем на всех IPv6 интерфейсах (::)

  // 5. ПРИВЯЗКА СОКЕТА К АДРЕСУ
  if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Can not bind to socket: %s\n", strerror(errno));
    close(server_fd);
    return 1;
  }

  // 6. ПЕРЕВОД В РЕЖИМ ПРОСЛУШИВАНИЯ
  if (listen(server_fd, 128) < 0) {
    fprintf(stderr, "Could not listen on socket: %s\n", strerror(errno));
    close(server_fd);
    return 1;
  }

  // Выводим информацию о том, на каких адресах слушаем
  char server_ip[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, &server.sin6_addr, server_ip, sizeof(server_ip));
  printf("Server listening on [%s]:%d (IPv6 only)\n", server_ip, port);
  printf("Threads per request: %d\n", tnum);

  // 7. ОСНОВНОЙ ЦИКЛ ПРИНЯТИЯ СОЕДИНЕНИЙ
  while (true) {
    struct sockaddr_in6 client;
    socklen_t client_len = sizeof(client);
    
    printf("\nWaiting for connections...\n");
    
    // 8. ПРИНЯТИЕ НОВОГО СОЕДИНЕНИЯ
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);
    if (client_fd < 0) {
      fprintf(stderr, "Could not accept new connection: %s\n", strerror(errno));
      continue;
    }

    // 9. ОПРЕДЕЛЕНИЕ АДРЕСА КЛИЕНТА
    char client_ip[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &client.sin6_addr, client_ip, sizeof(client_ip));
    printf("New client connected from [%s]:%d\n", 
           client_ip, ntohs(client.sin6_port));

    // 10. ОБРАБОТКА КЛИЕНТА
    while (true) {
      unsigned int buffer_size = sizeof(uint64_t) * 3;
      char from_client[buffer_size];
      
      // 11. ПОЛУЧЕНИЕ ДАННЫХ ОТ КЛИЕНТА
      int read_bytes = recv(client_fd, from_client, buffer_size, 0);

      if (read_bytes == 0) {
        printf("Client [%s]:%d disconnected\n", client_ip, ntohs(client.sin6_port));
        break;
      }
      
      if (read_bytes < 0) {
        fprintf(stderr, "Client read failed: %s\n", strerror(errno));
        break;
      }
      
      if ((unsigned int)read_bytes < buffer_size) {
        fprintf(stderr, "Client sent wrong data format\n");
        break;
      }

      // 12. ПАРСИНГ ПАРАМЕТРОВ
      uint64_t begin = 0, end = 0, mod = 0;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      printf("Received request from [%s]:%d\n", client_ip, ntohs(client.sin6_port));
      printf("  Range: %lu to %lu\n", begin, end);
      printf("  Mod: %lu\n", mod);
      printf("  Threads to use: %d\n", tnum);

      // 13. РАСПРЕДЕЛЕНИЕ РАБОТЫ ПО ПОТОКАМ
      pthread_t threads[tnum];
      struct FactorialArgs args[tnum];
      
      uint64_t range = end - begin + 1;
      uint64_t step = range / tnum;
      uint64_t remainder = range % tnum;
      uint64_t current = begin;

      printf("  Dividing work:\n");
      
      for (int i = 0; i < tnum; i++) {
        args[i].begin = current;
        args[i].end = current + step - 1 + (i < (int)remainder ? 1 : 0);
        args[i].mod = mod;
        
        printf("    Thread %d: %lu - %lu\n", i, args[i].begin, args[i].end);
        
        current = args[i].end + 1;

        if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i])) {
          fprintf(stderr, "Error: pthread_create failed!\n");
          return 1;
        }
      }

      // 14. СБОР РЕЗУЛЬТАТОВ ОТ ПОТОКОВ
      uint64_t total = 1;
      for (int i = 0; i < tnum; i++) {
        uint64_t *result = NULL;
        pthread_join(threads[i], (void **)&result);
        if (result != NULL) {
          total = MultModulo(total, *result, mod);
          free(result);
        }
      }

      printf("  Result: %lu\n", total);

      // 15. ОТПРАВКА РЕЗУЛЬТАТА КЛИЕНТУ
      char buffer[sizeof(total)];
      memcpy(buffer, &total, sizeof(total));
      
      if (send(client_fd, buffer, sizeof(total), 0) < 0) {
        fprintf(stderr, "Can't send data to client: %s\n", strerror(errno));
        break;
      }
      
      printf("  Result sent to client\n");
    }

    // 16. ЗАКРЫТИЕ СОКЕТА КЛИЕНТА
    if (shutdown(client_fd, SHUT_RDWR) < 0) {
      fprintf(stderr, "shutdown failed: %s\n", strerror(errno));
    }
    
    close(client_fd);
    printf("Client socket closed\n");
  }

  // 17. ЗАКРЫТИЕ СЕРВЕРНОГО СОКЕТА (никогда не выполнится в данном цикле)
  close(server_fd);
  return 0;
}
