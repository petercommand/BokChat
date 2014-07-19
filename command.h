#define COMMAND_H
#ifndef LIST_H
#include "list.h"
#endif
typedef struct irc_argument{
  char param[MAX_BUFFER];
  char trailing[MAX_BUFFER];
}irc_argument;
typedef struct irc_channel_privmsg{
  channel_info* channel_info;
  user_info* user_inf;
  char msg_body[MAX_BUFFER];
}irc_channel_privmsg;


void send_message(int error_num, user_info* user_inf, char* cmd, irc_argument* irc_args);
void motd(user_info* user_inf);
