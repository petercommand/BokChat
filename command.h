#define COMMAND_H
#ifndef LIST_H
#include "list.h"
#endif
typedef struct channel_list channel_list;
typedef struct user_list user_list;
typedef struct irc_argument{
  char param[MAX_BUFFER];
  char trailing[MAX_BUFFER];
}irc_argument;
typedef struct irc_channel_msg{
  char message_type[10];
  channel_info* channel_info;
  user_info* user_inf;
  char msg_body[MAX_BUFFER];
}irc_channel_msg;
typedef struct list_msg{
  user_list* user_lst;
  char msg_body[MAX_BUFFER];
}list_msg;

void send_message(int error_num, user_info* user_inf, char* cmd, irc_argument* irc_args);
void motd(user_info* user_inf);
