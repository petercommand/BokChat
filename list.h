#define LIST_H
#ifndef USER_H
#include "user.h"
#endif
#ifndef CHANNEL_H
#include "channel.h"
#endif



typedef struct channel_list{
  struct channel_info* channel_info;
  struct channel_list* priv;
  struct channel_list* next;
}channel_list;


typedef struct user_list{
  struct user_info* user_info;
  struct user_list* priv;
  struct user_list* next;
}user_list;

/*
typedef struct event_list{
  struct event_info* event_info;
  struct event_list* priv;
  struct event_list* next;
}event_list;

*/
extern channel_list* global_channel_list;
extern user_list* global_user_list;
extern pthread_mutex_t global_channel_mutex;
extern pthread_mutex_t global_user_mutex;
int add_node_to_user_list(user_list** user_list_ptr, user_info* user_inf);
int add_node_to_channel_list(channel_list** channel_list_ptr, channel_info* channel_inf);
int remove_node_from_channel_list(channel_list** channel_list_ptr, channel_info* channel_inf);
int remove_node_from_user_list(user_list** user_list_ptr, user_info* user_inf);
