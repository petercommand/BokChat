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
  
