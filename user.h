#define USER_H
#include <pthread.h>
enum user_channel_operation{
  NICK_OPERATION_SUCCESS,
  NICK_ALREADY_EXIST,
  NICK_TOO_LONG,
  NICK_INVALID,
  CHANNEL_OPERATION_SUCCESS,
  CHANNEL_NAME_TOO_LONG,
  CHANNEL_INVALID,
  FAILED_TO_MALLOC
};

typedef struct channel_list channel_list;
typedef struct user_info{
  pthread_t user_thread;
  char* user_nick;
  int socket;
  channel_list* joined_channels;
  pthread_mutex_t sock_mutex;
  int priv;
  time_t liveness;
}user_info;
int nick_exist(char* user_nick, int* error_num);
int nick_change(user_info* user_info, char* user_nick, int* error_num);
int quit_server(user_info* user_info);
int join_uuser_to_global_list(user_info* user_info);
