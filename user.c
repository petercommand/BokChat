/* todo: perform liveness check*/
#ifndef MAIN_H
#include "main.h"
#endif
#ifndef LIST_H
#include "list.h"
#endif
extern user_list* global_user_list;
extern channel_list* global_channel_list;
extern pthread_mutex_t global_user_mutex;
/* 
features:

change nickname


user quit server

*/

int nick_exist(char* user_nick){
  user_list* head;
  for(head = global_user_list;head != NULL;head = head->next){
    if(strcmp(head->user_info->user_nick, user_nick) == 0){
      return 1;
    }
  }
  return 0;
}

int nick_change(user_info* user_info, char* user_nick){
  if(strlen(user_nick)> MAX_NICK_LENGTH){
    return -1;
  }
  if(nick_exist(user_nick)){
    return -1;
  }
  user_info->user_nick = user_nick;
  return 0;
}
int quit_server(user_info* user_info){
  channel_list* joined_channels;
  for(joined_channels = user_info->joined_channels;joined_channels != NULL;joined_channels = joined_channels->next){
    quit_user_from_channel(user_info, joined_channels->channel_info);
  }
  free(user_info->user_nick);
  free(user_info);
  user_info = NULL;
  return 0;

}


