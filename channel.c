#ifndef LIST_H
#include "list.h"
#endif
#ifndef MAIN_H
#include "main.h"
#endif

int create_channel(channel_info* channel_info){
  /*append channel to global channel list*/
  channel_list* head = global_channel_list;
  if(head != NULL){
    while(head->next != NULL){
      head = head->next;
    }
    head->next = (channel_list *)malloc(sizeof(channel_list));
    if(head->next == NULL){
      return -1;
    }
    head->next->priv = head;
    head->next->next = NULL;
    head->next->channel_info = channel_info;
  }
  else{
    head = (channel_list *)malloc(sizeof(channel_list));
    if(head == NULL){
      return -1;
    }
    head->priv = NULL;
    head->next = NULL;
    head->channel_info = channel_info;
  }


  return 0;


}

int join_user_to_channel(user_info* user_info, channel_info* channel_info){
  /*update user_info->joined_channels & update channel user list*/
  user_list* channel_user_list;
  channel_user_list = channel_info->joined_users;
  while(channel_user_list->next != NULL){
    channel_user_list = channel_user_list->next;
  }
  channel_user_list->next = (user_list *)malloc(sizeof(user_list));
  if(channel_user_list->next == NULL){
    return -1;
  }
  channel_user_list->next->priv = channel_user_list;
  channel_user_list->next->next = NULL;
  channel_user_list = channel_user_list->next;
  channel_user_list->user_info = user_info;
  channel_list* user_channel_list;
  user_channel_list = user_info->joined_channels;
  while(user_channel_list->next != NULL){
    user_channel_list = user_channel_list->next;
  }
  user_channel_list->next = (channel_list *)malloc(sizeof(channel_list));
  if(user_channel_list->next == NULL){
    return -1;
  }
  user_channel_list->next->priv = user_channel_list;
  user_channel_list->next->next = NULL;
  user_channel_list = user_channel_list->next;
  user_channel_list->channel_info = channel_info;
  return 0;
}

int quit_user_from_channel(user_info* user_info, channel_info* channel_info){
  /*update user_info->joined_channels & update channel user list*/
  user_list* channel_user_list;
  channel_user_list = channel_info->joined_users;
  while(channel_user_list != NULL){
    if(channel_user_list->user_info == user_info){
      if(channel_user_list->priv != NULL){
	if(channel_user_list->next != NULL){
	  channel_user_list = channel_user_list->priv;
	  user_list* temp = channel_user_list->next;
	  channel_user_list->next->next->priv = channel_user_list;
	  channel_user_list->next = channel_user_list->next->next;
	  free(temp);
	  temp = NULL;
	}
	else{
	  channel_user_list = channel_user_list->priv;
	  free(channel_user_list->next);
	  channel_user_list->next = NULL;
	}
      }
      else{
	if(channel_user_list->next != NULL){
	  channel_user_list->next->priv = NULL;
	  free(channel_user_list);
	  channel_user_list = NULL;
	}
	else{
	  free(channel_user_list);
	  channel_user_list = NULL;
	}
      }
      break;
    }
    channel_user_list = channel_user_list->next;
  }
  channel_list* user_channel_list;
  user_channel_list = user_info->joined_channels;
  while(user_channel_list != NULL){
    if(user_channel_list->channel_info == channel_info){
      user_channel_list = user_channel_list->priv;
      channel_list* temp = user_channel_list->next;
      user_channel_list->next->next->priv = user_channel_list;
      user_channel_list->next = user_channel_list->next->next;
      free(temp);
      temp = NULL;
      break;
    }
    user_channel_list = user_channel_list->next;
  }
  return 0;


}


int is_user_op_in_channel(user_info* user_info, channel_info* channel_info){
  user_list* channel_op;
  for(channel_op = channel_info->channel_op;channel_op != NULL;channel_op = channel_op->next){
    if(channel_op->user_info == user_info){
      return 1;
    }
  }
  return 0;

}
int if_channel_exist_by_name(char* channel_name){
  channel_list* channel_list;
  for(channel_list = global_channel_list;channel_list != NULL;channel_list = channel_list->next){
    if(strcmp(channel_list->channel_info->channel_name, channel_name) == 0){
      return 1;
    }
  }
  return 0;
}

char* get_channel_topic(channel_info* channel_info){
  if(channel_info != NULL){
    return channel_info->topic;
  }
  else{
    return NULL;
  }
}
int set_channel_topic(channel_info* channel_info, char* topic){
  if((channel_info != NULL) && (topic != NULL)){
    channel_info->topic = topic;
    return 0;
  }
  else{
    return -1;
  }
}
