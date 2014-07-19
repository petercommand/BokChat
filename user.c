/* todo: perform liveness check*/
#include "connect.h"
#ifndef MAIN_H
#include "main.h"
#endif
#ifndef LIST_H
#include "list.h"
#endif
#include <unistd.h>
extern user_list* global_user_list;
extern channel_list* global_channel_list;
extern pthread_mutex_t global_user_mutex;

int nick_exist(char* user_nick, int* error_num){
  user_list* head;
  for(head = global_user_list;head != NULL;head = head->next){
    if(strcmp(head->user_info->user_nick, user_nick) == 0){
      *error_num = 433;
      return 1;
    }
  }
  return 0;
}

int nick_change(user_info* user_inf, char* user_nick, int* error_num){
  if(user_nick[0] == '\0'){
    *error_num = 431;
    return -1;
  }
  if(valid_nick(user_nick) != 1){
    *error_num = 432;
    return -1;
  }
  if(strlen(user_nick) > MAX_NICK_LENGTH){
    *error_num = 432;
    return -1;
  }
  if(nick_exist(user_nick, error_num)){
    *error_num = 433;
    return -1;
  }
  int i;
  for(i=0;user_nick[i]!='\0';i++){
    user_inf->user_nick[i] = user_nick[i];
  }
  user_inf->user_nick[i] = '\0';
  return 0;
}

int quit_server(user_info* user_inf){
  channel_list* joined_channels;
  pthread_mutex_lock(&global_user_mutex);
  pthread_mutex_lock(&global_channel_mutex);
  channel_list* temp;
  for(joined_channels = user_inf->joined_channels;joined_channels != NULL;joined_channels = temp){
    if(joined_channels->next != NULL){
      temp = joined_channels->next;
    }
    else{
      temp = NULL;
    }
    quit_user_from_channel(user_inf, joined_channels->channel_info);
  }

  remove_user_from_global_list(user_inf);
  pthread_mutex_lock(&user_inf->sock_mutex);
  close(user_inf->socket);
  pthread_mutex_unlock(&user_inf->sock_mutex);
  free(user_inf);
  user_inf = NULL;
  pthread_mutex_unlock(&global_channel_mutex);
  pthread_mutex_unlock(&global_user_mutex);
  return 0;
}

int remove_user_from_global_list(user_info* user_inf){
  if(remove_node_from_user_list(&global_user_list, user_inf) != 0){
    return -1;
  }
  return 0;
}

int join_user_to_global_list(user_info* user_inf){
  if(add_node_to_user_list(&global_user_list, user_inf) != 0){
    return -1;
  }
  return 0;
}

int is_node_in_user_list(user_list* user_lst, user_info* user_inf){  
 user_list* head = user_lst;
  while(head != NULL){
    if(head->user_info == user_inf){
      return 1;
    }
    head = head->next;
  }
  return 0;
}
int is_user_in_global_user_list(user_info* user_inf){
  if(is_node_in_user_list(global_user_list, user_inf) == 1){
    return 1;
  }
  return 0;
}
