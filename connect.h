#define CONNECT_H
#ifndef CONFIG_H
#include "config.h"
#endif
int listen_bind_on_port(int port);
void start_server(int sockfd);
int valid_string(char* input);
typedef struct struct_user_cmd{
  char cmd[MAX_BUFFER];
  char args[MAX_BUFFER];
}user_cmd;

int valid_channel(char* input);
int valid_nick(char* input);
