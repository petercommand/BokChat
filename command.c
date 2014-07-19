/* make change to the necessary structs and return the necessary data*/
/*return int to reflect the result of command processing */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef CONNECT_H
#include "connect.h"
#endif
#ifndef LIST_H
#include "list.h"
#endif
#ifndef COMMAND_H
#include "command.h"
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


irc_argument parse_argument(char* args);
ssize_t irc_recv(int sockfd, void* buf, size_t len, int flags);
ssize_t irc_send(int sockfd, const void *buf, size_t len, int flags);
void to_upper_case(char* input);









int process_cmd_nick_init(user_cmd cmd_info, user_info* user_inf){
  char* cmd = cmd_info.cmd;
  char* args = cmd_info.args;
  int error_num;
  irc_argument irc_args = parse_argument(args);
  pthread_mutex_lock(&global_user_mutex);
  if(nick_change(user_inf, irc_args.param, &error_num) == 0){ 
    pthread_mutex_unlock(&global_user_mutex);
    return 0;
  }
  pthread_mutex_unlock(&global_user_mutex);
  send_message(error_num, user_inf, cmd, &irc_args);
  return -1;
}



int process_cmd(user_cmd cmd_info, user_info* user_inf){
  char* cmd = cmd_info.cmd;
  char* args = cmd_info.args;
  int error_num;
  irc_argument irc_args = parse_argument(args);
  int i;
  /*
  if((irc_args.param[0] == '\0') && (irc_args.trailing[0] == '\0')){
    return -1;
  }
  */
  if(strcmp(cmd, "NICK") == 0){
    char old_nick[MAX_NICK_LENGTH+1];
    for(i=0;user_inf->user_nick[i] != '\0';i++){
      old_nick[i] = user_inf->user_nick[i];
    }
    old_nick[i] = '\0';
    pthread_mutex_lock(&global_user_mutex);
    if(nick_change(user_inf, irc_args.param, &error_num) == 0){ 
      pthread_mutex_unlock(&global_user_mutex);
      pthread_mutex_lock(&global_channel_mutex);

      pthread_t nickmsg;
      channel_list* head;
      for(head = user_inf->joined_channels;head!=NULL;head = head->next){
	irc_channel_msg* channel_msg = (irc_channel_msg *)malloc(sizeof(irc_channel_msg));
	if(channel_msg == NULL){
	  goto error;
	}
	memset(channel_msg, 0, sizeof(*channel_msg));
	const char* msg_type = "NICK";
	for(i=0;msg_type[i]!='\0';i++){
	  channel_msg->message_type[i] = msg_type[i];
	}
	channel_msg->channel_info = head->channel_info;
	channel_msg->user_inf = user_inf;
	
	for(i=0; old_nick[i] != '\0'; i++){
	  channel_msg->msg_body[i] = old_nick[i];
	}
	channel_msg->msg_body[i] = '\0';
	pthread_create(&nickmsg, NULL, (void *(*)(void *))send_message_to_all_users_in_channel, (void *)channel_msg);
      }
      pthread_mutex_unlock(&global_channel_mutex);
      goto exit;
    }
    pthread_mutex_unlock(&global_user_mutex);
    send_message(error_num, user_inf, cmd, &irc_args);
    goto error;
  }
  else if(strcmp(cmd, "PRIVMSG") == 0){
    if(irc_args.param[0] == '#'){/*channel*/
      pthread_mutex_lock(&global_channel_mutex);
      channel_info* channel_inf = channel_exist_by_name(irc_args.param);
      if(channel_inf == NULL){
	send_message(403, user_inf, cmd, &irc_args);
	pthread_mutex_unlock(&global_channel_mutex);
	goto error;
      }
      if(is_user_in_channel(user_inf, channel_inf) == 0){
	send_message(404, user_inf, cmd, &irc_args);
	pthread_mutex_unlock(&global_channel_mutex);
	goto error;
      }
      pthread_mutex_unlock(&global_channel_mutex);
      /* send to all users in channel */
      irc_channel_msg* channel_msg = (irc_channel_msg *)malloc(sizeof(irc_channel_msg));
      if(channel_msg == NULL){
	goto error;
      }
      memset(channel_msg, 0, sizeof(*channel_msg));
      const char* msg_type = "PRIVMSG";
      for(i=0;msg_type[i]!='\0';i++){
	channel_msg->message_type[i] = msg_type[i];
      }
      channel_msg->channel_info = channel_inf;
      channel_msg->user_inf = user_inf;
      for(i=0; irc_args.trailing[i] != '\0'; i++){
	channel_msg->msg_body[i] = irc_args.trailing[i];
      }
      channel_msg->msg_body[i] = '\0';
      pthread_t privmsg;
      pthread_create(&privmsg, NULL, (void *(*)(void *))send_message_to_all_users_in_channel, (void *)channel_msg);
      goto exit;
    }
    else{/*person*/
      

      goto exit;
    }
  }
  else if(strcmp(cmd, "USER") == 0){
    pthread_mutex_lock(&global_user_mutex);
    if(is_user_in_global_user_list(user_inf)){
      /*might have some problem here, fix it */
      pthread_mutex_unlock(&global_user_mutex);
      goto error;
    }
    if((irc_args.param[0] == '\0') || (irc_args.trailing[0] == '\0')){
      pthread_mutex_unlock(&global_user_mutex); 
      send_message(461, user_inf, cmd, &irc_args);
      goto error;
    }
    join_user_to_global_list(user_inf);
    pthread_mutex_unlock(&global_user_mutex);
    goto exit;
  }
  else if(strcmp(cmd, "JOIN") == 0){
    printf("Received join msg\n");
    pthread_t msg_thread;
    irc_channel_msg* channel_msg = (irc_channel_msg *)malloc(sizeof(irc_channel_msg));
    if(channel_msg == NULL){
      goto error;
    }
    memset(channel_msg, 0, sizeof(*channel_msg));
    const char* msg_type = "JOIN";
    for(i=0;msg_type[i]!='\0';i++){
      channel_msg->message_type[i] = msg_type[i];
    }
    channel_msg->user_inf = user_inf;
    for(i=0; irc_args.trailing[i] != '\0'; i++){
      channel_msg->msg_body[i] = irc_args.param[i];
    }
    channel_msg->msg_body[i] = '\0';
    channel_info* channel_inf = channel_exist_by_name(irc_args.param);
    if(channel_inf == NULL){
      if(strlen(irc_args.param) > MAX_CHAN_NAME_LENGTH){
	goto error;
      }
      if(valid_channel(irc_args.param) != 1){
	goto error;
      }
      /*create channel*/
      channel_inf = (channel_info *)malloc(sizeof(channel_info));
      if(channel_inf == NULL){
	printf("malloc error\n");
	goto error;
      }
      memset(channel_inf, 0, sizeof(channel_info));
      int i;
      for(i=0;irc_args.param[i] != '\0';i++){
	channel_inf->channel_name[i] = irc_args.param[i];
      }
      pthread_mutex_lock(&global_channel_mutex);
      create_channel(channel_inf);
      if(join_user_to_channel(user_inf, channel_inf) != 0){
	pthread_mutex_unlock(&global_channel_mutex);
	goto error;
      }
      pthread_mutex_unlock(&global_channel_mutex);
      channel_msg->channel_info = channel_inf;
      pthread_create(&msg_thread, NULL, (void *(*)(void *))send_message_to_all_users_in_channel, (void *)channel_msg);
      goto exit;
    }
    if(is_user_in_channel(user_inf, channel_inf) == 1){
      send_message(443, user_inf, cmd, &irc_args);
      goto error;
    }
    if(join_user_to_channel(user_inf, channel_inf) != 0){      
      goto error;
    }


    channel_msg->channel_info = channel_inf;
    pthread_create(&msg_thread, NULL, (void *(*)(void *))send_message_to_all_users_in_channel, (void *)channel_msg);
    goto exit;
  }
  else if(strcmp(cmd, "PART") == 0){
    
    goto exit;
  }
  else if(strcmp(cmd, "KICK") == 0){
    
    goto exit;
  }
  else if(strcmp(cmd, "QUIT") == 0){
    
    goto exit;
  }
  else if(strcmp(cmd, "MOTD") == 0){
    motd(user_inf);
    goto exit;
  }


  /*No command matches */
  send_message(421, user_inf, cmd, NULL);
  goto error;


 exit:
  return 0;
 error: 
  return -1;

}
irc_argument parse_argument(char* args){
  int i,j,k;
  int colon = 0;
  irc_argument irc_args;
  memset(&irc_args, 0, sizeof(irc_args));
  for(i=0;args[i]!='\0';i++){
    if(args[i] == ':'){
      colon = 1;
      for(j=0;j<i;j++){
	irc_args.param[j] = args[j];
      }
      irc_args.param[j] = '\0';
      j = i + 1;
      k = 0;
      while(args[j] != '\0'){
	irc_args.trailing[k] = args[j];
	j++;
	k++;
      }
      irc_args.trailing[k] = '\0';
      break;
    }
  }
  if(colon == 0){
    for(i=0;args[i]!='\0';i++){
      irc_args.param[i] = args[i];
    }
    irc_args.param[i] = '\0';
  }
  /*trim space*/
  if(irc_args.param[0] == '\0'){
    return irc_args;
  }
  char* start = irc_args.param;
  char* end = irc_args.param + strlen(irc_args.param) - 1;/*pos of null term - 1*/
  while(*start == ' '){
    start = start + 1;
  }
  while((end > start) && (*end == ' ')){
    end = end - 1;
  }
  end++;
  *end = '\0';
  memmove(irc_args.param, start, end - start + 1);
  return irc_args;
}


