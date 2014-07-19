/* todo: perform liveness check*/
#include "connect.h"
#ifndef MAIN_H
#include "main.h"
#endif
#ifndef LIST_H
#include "list.h"
#endif
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
  user_inf->user_nick = user_nick;
  return 0;
}
int quit_server(user_info* user_inf){
  channel_list* joined_channels;
  for(joined_channels = user_inf->joined_channels;joined_channels != NULL;joined_channels = joined_channels->next){
    quit_user_from_channel(user_inf, joined_channels->channel_info);
  }
  free(user_inf->user_nick);
  free(user_inf);
  user_inf = NULL;
  return 0;

}
int join_user_to_global_list(user_info* user_inf){
  user_list* head = global_user_list;
  if(head != NULL){
    while(head->next != NULL){
      head = head->next;
    }
    head->next = (user_list *)malloc(sizeof(user_list));
    if(head->next == NULL){
      return -1;
    }
    head->next->priv = head;
    head->next->next = NULL;
    head->next->user_info = user_inf;
  }
  else{
    head = (user_list *)malloc(sizeof(user_list));
    if(head == NULL){
      return -1;
    }
    head->priv = NULL;
    head->next = NULL;
    head->user_info = user_inf;
  }
  return 0;
}


int user_already_exist_in_global_user_list(user_info* user_inf, int* error_num){
  user_list* head = global_user_list;
  while(head != NULL){
    if(head->user_info == user_inf){
      return 1;
    }
  }
  return 0;
}
