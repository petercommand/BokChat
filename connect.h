#define CONNECT_H
#ifndef CONFIG_H
#include "config.h"
#endif
#ifndef LIST_H
#include "list.h"
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
int valid_username(char* input);
void send_message_to_user(user_info* user_inf, char* msg_body);
void send_message_to_user_in_list(list_msg* user_list_msg);
