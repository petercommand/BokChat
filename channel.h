#ifndef CHANNEL_H
#define CHANNEL_H
typedef struct channel_info channel_info;
#include "list.h"
#include "command.h"
typedef struct channel_info{
  struct user_list* joined_users;
  int flags;
  struct user_list* channel_op;
  char channel_name[MAX_CHAN_NAME_LENGTH+1];
  char topic[MAX_BUFFER+1];
}channel_info;

int create_channel(channel_info* channel_info);
int join_user_to_channel(user_info* user_info, channel_info* channel_info);
int quit_user_from_channel(user_info* user_info, channel_info* channel_info);
int is_user_op_in_channel(user_info* user_info, channel_info* channel_info);
channel_info* channel_exist_by_name(char* channel_name);
int is_user_in_channel(user_info* user_info, channel_info* channel_info);
void send_message_to_all_users_in_channel(irc_channel_msg* channel_msg);
char* get_channel_topic(channel_info* channel_info);
int set_channel_topic(channel_info* channel_info, char* topic);


#endif
