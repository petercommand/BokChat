/* make change to the necessary structs and return the necessary data*/
/*return int to reflect the result of the command procession */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef CONNECT_H
#include "connect.h"
#endif
#ifndef LIST_H
#include "list.h"
#endif
#include <string.h>
typedef struct irc_argument{
  char param[MAX_BUFFER];
  char trailing[MAX_BUFFER];
}irc_argument;

irc_argument parse_argument(char* args);
ssize_t irc_recv(int sockfd, void* buf, size_t len, int flags);
ssize_t irc_send(int sockfd, const void *buf, size_t len, int flags);
void send_message(int error_num, int sockfd);

int process_cmd(user_cmd cmd_info, user_info* user_info){
  char* cmd = cmd_info.cmd;
  char* args = cmd_info.args;
  int error_num;
  int sock_fd = user_info->socket;
  irc_argument irc_args = parse_argument(args);
/*  if((irc_args.param[0] == '\0') && (irc_args.trailing[0] == '\0')){
    return -1;
    }*/
  if(strcmp(cmd, "NICK") == 0){
    if(nick_change(user_info, irc_args.param, &error_num) == 0){      
      goto exit;
    }
    send_message(error_num, sock_fd);
    goto error;
  }
  else if(strcmp(cmd, "USER") == 0){
    
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
  else if(strcmp(cmd, "PRIVMSG") == 0){

    goto exit;
  }


  /*No command matches */
  send_message(421, sock_fd);
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
