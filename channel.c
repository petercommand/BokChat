#include "list.h"
#include "main.h"
#include "command.h"
#include "connect.h"


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
    global_channel_list = head;
    head->priv = NULL;
    head->next = NULL;
    head->channel_info = channel_info;
  }


  return 0;


}



int join_user_to_channel(user_info* user_info, channel_info* channel_info){
  /*update user_info->joined_channels & update channel user list*/
  if(add_node_to_user_list(&channel_info->joined_users, user_info) != 0){
    return -1;
  }
  if(add_node_to_channel_list(&user_info->joined_channels, channel_info) != 0){
    return -1;
  }
  return 0;
}

int quit_user_from_channel(user_info* user_info, channel_info* channel_info){
  /*update user_info->joined_channels & update channel user list*/
  remove_node_from_user_list(&channel_info->joined_users, user_info);
  remove_node_from_channel_list(&user_info->joined_channels, channel_info);
  return 0;
}
int is_user_in_channel(user_info* user_info, channel_info* channel_info){
  user_list* channel_user;
  for(channel_user = channel_info->joined_users;channel_user != NULL;channel_user = channel_user->next){
    if(channel_user->user_info == user_info){
      return 1;
    }
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
channel_info* channel_exist_by_name(char* channel_name){
  if(global_channel_list == NULL){
    return NULL;
  }
  channel_list* channel_list;
  for(channel_list = global_channel_list;channel_list != NULL;channel_list = channel_list->next){
    if(strcmp(channel_list->channel_info->channel_name, channel_name) == 0){
      return channel_list->channel_info;
    }
  }
  return NULL;
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
    int i;
    for(i=0;topic[i] != '\0';i++){
      channel_info->topic[i] = topic[i];
    }
    channel_info->topic[i] = '\0';
    return 0;
  }
  else{
    return -1;
  }
}
void send_message_to_all_users_in_channel(irc_channel_msg* channel_msg){/*remember to free channel_msg after using it */
  char buf[MAX_BUFFER];
  char buf2[MAX_BUFFER];
  user_info* user_inf = channel_msg->user_inf;
  channel_info* channel_inf = channel_msg->channel_info;
  char* message_type = channel_msg->message_type;
  if(strcmp(message_type, "PRIVMSG") == 0){
    snprintf(buf, MAX_BUFFER-3, ":%s!%s@%s PRIVMSG %s :%s", user_inf->user_nick, user_inf->user_nick, user_inf->hostname, channel_inf->channel_name, channel_msg->msg_body);
    snprintf(buf2, MAX_BUFFER, "%s\r\n", buf);
    pthread_mutex_lock(&global_channel_mutex);
    user_list* head;
    for(head = channel_msg->channel_info->joined_users;head != NULL;head = head->next){
      if(head->user_info == user_inf){
	continue;
      }
      send_message_to_user(head->user_info, buf2);
    }
    pthread_mutex_unlock(&global_channel_mutex);
    goto exit;
    


  }
  else if(strcmp(message_type, "JOIN") == 0){
    snprintf(buf, MAX_BUFFER-3, ":%s!%s@%s JOIN %s", user_inf->user_nick, user_inf->user_nick, user_inf->hostname, channel_inf->channel_name);
    snprintf(buf2, MAX_BUFFER, "%s\r\n", buf);
    pthread_mutex_lock(&global_channel_mutex);
    user_list* head;
    for(head = channel_msg->channel_info->joined_users;head != NULL;head = head->next){
      send_message_to_user(head->user_info, buf2);
    }
    pthread_mutex_unlock(&global_channel_mutex);
    goto exit;
    
  }
  else if(strcmp(message_type, "PART") == 0){
    snprintf(buf, MAX_BUFFER-3, ":%s!%s@%s PART %s", user_inf->user_nick, user_inf->user_nick, user_inf->hostname, channel_inf->channel_name);
    snprintf(buf2, MAX_BUFFER, "%s\r\n", buf);
    pthread_mutex_lock(&global_channel_mutex);
    user_list* head;
    for(head = channel_msg->channel_info->joined_users;head != NULL;head = head->next){
      send_message_to_user(head->user_info, buf2);
    }
    pthread_mutex_unlock(&global_channel_mutex);
    goto exit;
    
  }


  else if(strcmp(message_type, "TOPIC") == 0){
    user_list* head;
    pthread_mutex_lock(&global_channel_mutex);
    for(head = channel_msg->channel_info->joined_users;head != NULL;head = head->next){
      snprintf(buf, MAX_BUFFER-3, ":%s 332 %s %s :%s", SERVER_NAME, head->user_info->user_nick, channel_msg->channel_info->channel_name, channel_msg->channel_info->topic);
      snprintf(buf2, MAX_BUFFER, "%s\r\n", buf);
      send_message_to_user(head->user_info, buf2);
    }
    pthread_mutex_unlock(&global_channel_mutex);

    goto exit;
  }




 exit:
  free(channel_msg);
  channel_msg = NULL;




}
