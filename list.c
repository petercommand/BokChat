#ifndef LIST_H
#include "list.h"
#endif
#include <stdlib.h>
#include <string.h>
channel_list* global_channel_list = NULL;
user_list* global_user_list = NULL;
pthread_mutex_t global_channel_mutex;
pthread_mutex_t global_user_mutex;


void server_mutex_init(){
  pthread_mutex_init(&global_channel_mutex, NULL);
  pthread_mutex_init(&global_user_mutex, NULL);
}

int add_node_to_user_list(user_list** user_list_ptr, user_info* user_inf){
  user_list* user_lst = *user_list_ptr;
  if(user_lst == NULL){
    user_lst = (user_list *)malloc(sizeof(user_list));
    if(user_lst == NULL){
      return -1;
    }
    *user_list_ptr = user_lst;
    user_lst->priv = NULL;
    user_lst->next = NULL;
    user_lst->user_info = user_inf;
  }
  else{
    while(user_lst->next != NULL){
      user_lst = user_lst->next;
    }
    user_lst->next = (user_list *)malloc(sizeof(user_list));
    if(user_lst->next == NULL){
      return -1;
    }
    user_lst->next->priv = user_lst;
    user_lst->next->next = NULL;
    user_lst = user_lst->next;
    user_lst->user_info = user_inf;
  }
  return 0;
}


int add_node_to_channel_list(channel_list** channel_list_ptr, channel_info* channel_inf){
  channel_list* channel_lst = *channel_list_ptr;
  if(channel_lst == NULL){
    channel_lst = (channel_list *)malloc(sizeof(channel_list));
    if(channel_lst == NULL){
      return -1;
    }
    *channel_list_ptr = channel_lst;
    channel_lst->priv = NULL;
    channel_lst->next = NULL;
    channel_lst->channel_info = channel_inf;
  }
  else{
    while(channel_lst->next != NULL){
      channel_lst = channel_lst->next;
    }
    channel_lst->next = (channel_list *)malloc(sizeof(channel_list));
    if(channel_lst->next == NULL){
      return -1;
    }
    channel_lst->next->priv = channel_lst;
    channel_lst->next->next = NULL;
    channel_lst = channel_lst->next;
    channel_lst->channel_info = channel_inf;
  }
  return 0;
}
int remove_node_from_channel_list(channel_list** channel_lst, channel_info* channel_inf){
  while(*channel_lst != NULL){
    if((*channel_lst)->channel_info == channel_inf){
      if((*channel_lst)->priv != NULL){
	if((*channel_lst)->next != NULL){
	  *channel_lst = (*channel_lst)->priv;
	  channel_list* temp = (*channel_lst)->next;
	  (*channel_lst)->next->next->priv = *channel_lst;
	  (*channel_lst)->next = (*channel_lst)->next->next;
	  free(temp);
	  temp = NULL;
	}
	else{
	  *channel_lst = (*channel_lst)->priv;
	  free((*channel_lst)->next);
	  (*channel_lst)->next = NULL;
	}
      }
      else{
	if((*channel_lst)->next != NULL){
	  (*channel_lst)->next->priv = NULL;
	  free(*channel_lst);
	  (*channel_lst) = NULL;
	}
	else{
	  free(*channel_lst);
	  *channel_lst = NULL;
	}
      }
      break;
    }
    *channel_lst = (*channel_lst)->next;
  }
  return 0;
}

int remove_node_from_user_list(user_list** user_lst, user_info* user_inf){
  while(*user_lst != NULL){
    if((*user_lst)->user_info == user_inf){
      if((*user_lst)->priv != NULL){
	if((*user_lst)->next != NULL){
	  *user_lst = (*user_lst)->priv;
	  user_list* temp = (*user_lst)->next;
	  (*user_lst)->next->next->priv = *user_lst;
	  (*user_lst)->next = (*user_lst)->next->next;
	  free(temp);
	  temp = NULL;
	}
	else{
	  *user_lst = (*user_lst)->priv;
	  free((*user_lst)->next);
	  (*user_lst)->next = NULL;
	}
      }
      else{
	if((*user_lst)->next != NULL){
	  (*user_lst)->next->priv = NULL;
	  free(*user_lst);
	  *user_lst = NULL;
	}
	else{
	  free(*user_lst);
	  *user_lst = NULL;
	}
      }
      break;
    }
    *user_lst = (*user_lst)->next;
  }
  return 0;
}
