#ifndef LIST_H
#include "list.h"
#endif

channel_list* global_channel_list = NULL;
user_list* global_user_list = NULL;
pthread_mutex_t global_channel_mutex;
pthread_mutex_t global_user_mutex;


void server_mutex_init(){
  pthread_mutex_init(&global_channel_mutex, NULL);
  pthread_mutex_init(&global_user_mutex, NULL);
}
