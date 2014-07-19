/* make change to the necessary structs and return the necessary data*/
/*return int to reflect the result of the command procession */
#ifndef CONNECT_H
#include "connect.h"
#endif
#ifndef LIST_H
#include "list.h"
#endif
#include <string.h>

int process_cmd(user_cmd cmd_info){
  char* cmd = cmd_info.cmd;
  char* args = cmd_info.args;
  if(strcmp(cmd, "NICK") == 0){
    
    goto exit;
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
  goto error;


 exit:
  return 0;
 error: 
  return -1;





}
