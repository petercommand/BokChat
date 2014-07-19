#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define USER_H
#include <pthread.h>
#ifndef CONFIG_H
#include "config.h"
#endif
#include <netdb.h>
typedef struct channel_list channel_list;
typedef struct user_info{
  pthread_t user_thread;
  char* user_nick;
  int socket;
  channel_list* joined_channels;
  pthread_mutex_t sock_mutex;
  struct sockaddr client_addr;
  char hostname[NI_MAXHOST];
  int priv;
  time_t liveness;
}user_info;
int nick_exist(char* user_nick, int* error_num);
int nick_change(user_info* user_info, char* user_nick, int* error_num);
int quit_server(user_info* user_info);
int join_user_to_global_list(user_info* user_info);
int user_already_exist_in_global_user_list(user_info* user_inf, int* error_num);
