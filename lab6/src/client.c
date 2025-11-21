#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <arpa/inet.h>

struct Server {
  char ip[255];
  int port;
};

struct ThreadData {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
};

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
  uint64_t result = 0;
  a = a % mod;
  while (b > 0) {
    if (b % 2 == 1)
      result = (result + a) % mod;
    a = (a * 2) % mod;
    b /= 2;
  }

  return result % mod;
}

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

void* ServerThread(void* arg) {
  struct ThreadData* data = (struct ThreadData*)arg;
  
  struct hostent *hostname = gethostbyname(data->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", data->server.ip);
    data->result = 0;
    return NULL;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(data->server.port);
  
  if (hostname->h_addr_list[0] != NULL) {
    memcpy(&server_addr.sin_addr.s_addr, hostname->h_addr_list[0], hostname->h_length);
  } else {
    fprintf(stderr, "No address found for %s\n", data->server.ip);
    data->result = 0;
    return NULL;
  }

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    data->result = 0;
    return NULL;
  }

  if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "Connection to %s:%d failed\n", data->server.ip, data->server.port);
    close(sck);
    data->result = 0;
    return NULL;
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &data->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &data->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &data->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed to %s:%d\n", data->server.ip, data->server.port);
    close(sck);
    data->result = 0;
    return NULL;
  }

  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Receive failed from %s:%d\n", data->server.ip, data->server.port);
    close(sck);
    data->result = 0;
    return NULL;
  }

  uint64_t answer = 0;
  memcpy(&answer, response, sizeof(uint64_t));
  data->result = answer;

  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = 0;
  uint64_t mod = 0;
  char servers_file[255] = {'\0'};
  int servers_file_empty = 1;

  while (true) {
    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        if (!ConvertStringToUI64(optarg, &k)) {
          fprintf(stderr, "Invalid k value\n");
          return 1;
        }
        break;
      case 1:
        if (!ConvertStringToUI64(optarg, &mod)) {
          fprintf(stderr, "Invalid mod value\n");
          return 1;
        }
        break;
      case 2:
        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
        servers_file_empty = 0;
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == 0 || mod == 0 || servers_file_empty) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  FILE* file = fopen(servers_file, "r");
  if (file == NULL) {
    fprintf(stderr, "Cannot open servers file: %s\n", servers_file);
    return 1;
  }

  struct Server servers[100];
  int servers_num = 0;
  char line[255];
  
  while (fgets(line, sizeof(line), file) != NULL && servers_num < 100) {
    line[strcspn(line, "\n")] = 0;
    
    char* colon = strchr(line, ':');
    if (colon != NULL) {
      *colon = '\0';
      strncpy(servers[servers_num].ip, line, sizeof(servers[servers_num].ip) - 1);
      servers[servers_num].port = atoi(colon + 1);
      if (servers[servers_num].port > 0) {
        servers_num++;
      }
    }
  }
  fclose(file);

  if (servers_num == 0) {
    fprintf(stderr, "No valid servers found in file\n");
    return 1;
  }

  printf("Found %d servers\n", servers_num);

  pthread_t threads[servers_num];
  struct ThreadData thread_data[servers_num];

  uint64_t numbers_per_server = k / servers_num;
  uint64_t remainder = k % servers_num;
  uint64_t current = 1;

  for (int i = 0; i < servers_num; i++) {
    thread_data[i].server = servers[i];
    thread_data[i].begin = current;
    
    // ИСПРАВЛЕНИЕ: приведение типов
    uint64_t extra = ((uint64_t)i < remainder) ? 1 : 0;
    thread_data[i].end = current + numbers_per_server - 1 + extra;
    thread_data[i].mod = mod;
    thread_data[i].result = 0;

    current = thread_data[i].end + 1;

    printf("Server %d: %s:%d - range %lu to %lu\n", 
           i, servers[i].ip, servers[i].port, thread_data[i].begin, thread_data[i].end);

    if (pthread_create(&threads[i], NULL, ServerThread, &thread_data[i])) {
      fprintf(stderr, "Error creating thread for server %d\n", i);
    }
  }

  uint64_t total_result = 1;
  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    if (thread_data[i].result != 0) {
      total_result = MultModulo(total_result, thread_data[i].result, mod);
      printf("Server %s:%d returned: %lu\n", 
             servers[i].ip, servers[i].port, thread_data[i].result);
    }
  }

  printf("\nFinal result: %lu! mod %lu = %lu\n", k, mod, total_result);

  return 0;
}
