#ifndef CONNECT_H
#define CONNECT_H
#include "config.h"
#include "list.h"
int irc_listen_bind_on_port(int port);
void start_irc_server(int irc_sockfd, int ssl_irc_sockfd);
int valid_string(char* input);
typedef struct struct_user_cmd{
  char cmd[MAX_BUFFER];
  char args[MAX_BUFFER];
}user_cmd;

int valid_channel(char* input);
int valid_nick(char* input);
int valid_username(char* input);
void send_message_by_type(user_info* user_inf, const char* msg_type, char* msg_body);
void send_message_to_user(user_info* user_inf, char* msg_body);
void send_message_to_user_in_list(list_msg* user_list_msg);
#endif
