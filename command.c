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
#include <string.h>


irc_argument parse_argument(char* args);
ssize_t irc_recv(int sockfd, void* buf, size_t len, int flags);
ssize_t irc_send(int sockfd, const void *buf, size_t len, int flags);

int process_cmd(user_cmd cmd_info, user_info* user_inf){
  char* cmd = cmd_info.cmd;
  char* args = cmd_info.args;
  int error_num;
  irc_argument irc_args = parse_argument(args);
  if((irc_args.param[0] == '\0') && (irc_args.trailing[0] == '\0')){
    return -1;
  }
  if(strcmp(cmd, "NICK") == 0){
    if(nick_change(user_inf, irc_args.param, &error_num) == 0){      
      goto exit;
    }
    send_message(error_num, user_inf, cmd, &irc_args);
    goto error;
  }
  else if(strcmp(cmd, "PRIVMSG") == 0){
    if(irc_args.param[0] == '#'){/*channel*/
      channel_info* channel_inf = channel_exist_by_name(irc_args.param);
      if(channel_inf == NULL){
	send_message(403, user_inf, cmd, &irc_args);
	goto error;
      }
      if(is_user_in_channel(user_inf, channel_inf) == 0){
	send_message(404, user_inf, cmd, &irc_args);
	goto error;
      }
      /* send to all users in channel */
      irc_channel_privmsg* channel_msg = (irc_channel_privmsg *)malloc(sizeof(irc_channel_privmsg));
      if(channel_msg == NULL){
	goto error;
      }
      memset(channel_msg, 0, sizeof(*channel_msg));
      channel_msg->channel_info = channel_inf;
      channel_msg->user_inf = user_inf;
      int i;
      for(i=0; irc_args.trailing[i] != '\0'; i++){
	channel_msg->msg_body[i] = irc_args.trailing[i];
      }
      channel_msg->msg_body[i] = '\0';
      pthread_t privmsg;
      pthread_create(&privmsg, NULL, (void *(*)(void *))send_message_to_all_users_in_channel, (void *)channel_msg);
      
      
    }
    else{/*person*/



    }
  }
  else if(strcmp(cmd, "USER") == 0){
    if(user_already_exist_in_global_user_list(user_inf, &error_num)){
      send_message(error_num, user_inf, cmd, &irc_args);
      goto error;
    }
    if((irc_args.param[0] == '\0') || (irc_args.trailing[0] == '\0')){
      send_message(461, user_inf, cmd, &irc_args);
      goto error;
    }
    goto exit;

  }
  else if(strcmp(cmd, "JOIN") == 0){
    
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
  return irc_args;
}
