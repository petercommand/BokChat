#ifndef USER_H
#define USER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "config.h"
#include <netdb.h>
typedef struct user_list user_list;
typedef struct channel_list channel_list;
typedef struct user_info{
  pthread_t user_thread;
  char user_nick[MAX_NICK_LENGTH+1];
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
int quit_server(user_info* user_info, char* reason);
int join_user_to_global_list(user_info* user_info);
int is_user_in_global_user_list(user_info* user_inf);
int remove_user_from_global_list(user_info* user_inf);
int is_node_in_user_list(user_list* user_lst, user_info* user_inf);
void update_user_liveness(user_info* user_inf);


#endif
