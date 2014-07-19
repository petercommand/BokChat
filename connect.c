#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#ifndef CONNECT_H
#include "connect.h"
#endif
#ifndef MAIN_H
#include "main.h"
#endif
#ifndef CONFIG_H
#include "config.h"
#endif
#ifndef LIST_H
#include "list.h"
#endif
#ifndef COMMAND_H
#include "command.h"
#endif
#include <errno.h>
#define SERVER_NAME "rlhsu.cukcake"
int get_cmd(int socket, char* buf, char* cmd);
void client_connect_loop(int* sockfd_p);
void client_connect(user_info* user_inf);
void liveness_check_loop();
int init_user(user_info* user_inf, char* buf);
int line_terminated(char* input, int size);
int null_terminated(char* input, int size);
user_cmd parse_cmd(char* cmd);
void server_mutex_init();
ssize_t irc_recv(int sockfd, void* buf, size_t len, int flags);
ssize_t irc_send(int sockfd, const void *buf, size_t len, int flags);
void trim_msg(char* buf, size_t len);
int join_user_to_global_list(user_info* user_info);
int process_cmd(user_cmd cmd_info, user_info* user_info);



void start_server(int sockfd){
  server_mutex_init();
  pthread_t client_connect_thread;
  pthread_t liveness_check_thread;
  int* sockfd_p = (int *)malloc(sizeof(int));
  if(sockfd_p == NULL){
    fprintf(stderr, "Failed to malloc: %s\n", strerror(errno));
    exit(1);
  }
  *sockfd_p = sockfd;
  pthread_create(&client_connect_thread, NULL, (void *(*)(void *))client_connect_loop, (void *)sockfd_p);
  pthread_create(&liveness_check_thread, NULL, (void *(*)(void *))liveness_check_loop, NULL);


  while(1){
    pause();
  }

  free(sockfd_p);
  sockfd_p = NULL;

}

/* mutex lock sequence: user then channel*/

int listen_bind_on_port(int port){

  int sockfd;

  struct sockaddr_in irc_socket;


  sockfd = socket(PF_INET, SOCK_STREAM, 0);

  memset(&irc_socket, 0, sizeof(irc_socket));
  irc_socket.sin_family = AF_INET;
  irc_socket.sin_port = htons(port);
  irc_socket.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(sockfd, (struct sockaddr*)&irc_socket, sizeof(irc_socket)) < 0){
    fprintf(stderr, "Failed to bind socket: %s\n", strerror(errno));
    return -1;
  }
  if(listen(sockfd, MAXCONNECTION) < 0){
    fprintf(stderr, "Failed to listen on port %d: %s\n", port, strerror(errno));
    return -1;
  }
  return sockfd;
  
}
void client_connect_loop(int* sockfd_p){
  while(1){
    int sockfd = *sockfd_p;
    int client_socket = -1;
    struct sockaddr client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr));
    socklen_t client_len = sizeof(struct sockaddr);
    client_socket = accept(sockfd, &client_addr, &client_len);
    if(client_socket < 0){/* failed to accept socket*/
      fprintf(stderr, "Error occurred while accepting client connection: %s\n", strerror(errno));
      continue;
    }
    user_info* user_inf = (user_info *)malloc(sizeof(user_info));
    memset(user_inf, 0, sizeof(user_info));
    user_inf->socket = client_socket;
    user_inf->liveness = time(NULL);
    user_inf->client_addr = client_addr;
    pthread_create(&(user_inf->user_thread), NULL, (void *(*)(void *))client_connect, (void *)user_inf);
  }
}
void client_connect(user_info* user_inf){
  char* buf = (char *)malloc(MAX_BUFFER);
  memset(buf, 0, MAX_BUFFER);
  if(buf == NULL){
    close(user_inf->socket);
    free(user_inf);
    pthread_exit(NULL);
  }
  if(init_user(user_inf, buf) != 0){
/* user have to send in NICK and USER command to the server to init_user function, this function shall initialize everything in the user_info struct for the user. If init_user return non-zero value, disconnect the user immediately */
    /* free everything*/
    close(user_inf->socket);
    free(buf);
    free(user_inf);
    pthread_exit(NULL);
  }
  




  /*NOT finished*/
  
  
  
  return;
}

void liveness_check_loop(){
  while(1){
    /* NOT finished*/
    
    pause();
    
  }
  
}
int get_cmd(int socket, char* buf, char* cmd){
  memset(cmd, 0, sizeof(&cmd));
  int recv_byte = -1;
  int term_buf = -1;
  int null_buf = -1;
  char recv_cmd[MAX_BUFFER+1] = {'\0'};
  recv_byte = irc_recv(socket, recv_cmd, MAX_BUFFER, MSG_DONTWAIT);/* already used the select call to block, thus recv should be set to nonblocking */
  if(recv_byte <= 0){
    return -1;
  }
  trim_msg(recv_cmd, recv_byte);
  strncat(buf, recv_cmd, MAX_BUFFER-strlen(buf)-1);
  term_buf = line_terminated(buf, MAX_BUFFER);
  null_buf = null_terminated(buf, MAX_BUFFER);
  if((term_buf == -1) && (null_buf == (MAX_BUFFER-1))){
    /* truncate and clear buf here */
    strncpy(cmd, buf, MAX_BUFFER);
    memset(buf, 0, sizeof(&buf));
    return 0;
  }
  if((term_buf == -1) && (term_buf < (MAX_BUFFER-1))){
    /* no command received */
    return 0;
  }
  buf[term_buf] = '\0';
  strncpy(cmd, buf, MAX_BUFFER);
  memmove(buf, &buf[term_buf + 2], MAX_BUFFER - term_buf -2);
  return 0;
}
    


int valid_channel(char* input){
  if(input == NULL){
    return 0;
  }
  int len = strlen(input);
  int i;
  for(i=0;i<len;i++){
    if((input[i] >= 'a') && (input[i] <= 'z')){
      continue;
    }
    if((input[i] >= 'a') && (input[i] <= 'Z')){
      continue;
    }
    if((input[i] == '_') || (input[i] == '-') || (input[i] == '#') || (input[i] == '.')){
      continue;
    }
    return 0;
  }
  return 1;
}


int valid_nick(char* input){
  if(input == NULL){
    return 0;
  }
  int len = strlen(input);
  int i;
  for(i=0;i<len;i++){
    if((input[i] >= 'a') && (input[i] <= 'z')){
      continue;
    }
    if((input[i] >= 'a') && (input[i] <= 'Z')){
      continue;
    }
    if((input[i] == '_') || (input[i] == '-') || (input[i] == '.')){
      continue;
    }
    return 0;
  }
  return 1;
}

int init_user(user_info* user_inf, char* buf){
  time_t start_time = time(NULL);
  int ready;
  int nick = 0;
  int user = 0;
  char* cmd = (char *)malloc(MAX_BUFFER);
  user_cmd cmd_info;
  memset(&cmd_info, 0, sizeof(cmd_info));
  if(cmd == NULL){
    goto error;
  }
  fd_set set;
  struct timeval wait_time;
  while((nick == 0) || (user == 0)){
    FD_ZERO(&set);
    FD_SET(user_inf->socket, &set);
    wait_time.tv_sec = 15;/*wait for 15 secs. Remember to reset timer after every select call!*/
    wait_time.tv_usec = 0;
    ready = select(user_inf->socket+1,  &set, NULL, NULL, &wait_time);
    if(ready < 0){
      goto error;
    }
    if(FD_ISSET(user_inf->socket, &set)){
      if(get_cmd(user_inf->socket, buf, cmd) == -1){
	goto error;
      }
      if(cmd != NULL){
	cmd_info = parse_cmd(cmd);
	printf("cmd_info->cmd:%s\ncmd_info->args:%s\n", cmd_info.cmd, cmd_info.args);
	/*put user into global user list after first nick command as we find user by nick */
	if(strcmp(cmd_info.cmd, "NICK") == 0){
	  pthread_mutex_lock(&global_user_mutex);
	  if(process_cmd(cmd_info, user_inf) == 0){
	    nick = 1;
	  }
	  pthread_mutex_unlock(&global_user_mutex);
	}
	else if(strcmp(cmd_info.cmd, "USER") == 0){
	  pthread_mutex_lock(&global_user_mutex);
	  if(process_cmd(cmd_info, user_inf) == 0){
	    user = 1;
	  }
	  pthread_mutex_unlock(&global_user_mutex);
	}
	else{
	  send_message(451, user_inf->socket, NULL, NULL);
	}
	  
      }
    }

    if((time(NULL) - start_time) >= 60){/*timeout value*/
      goto error;
    }
  }
  goto exit;
 exit:
  free(cmd);
  cmd = NULL;
  return 0;/* user and nick commands are both set to 1: ready to go*/
 error:
  free(cmd);
  cmd = NULL;
  return -1;
  
}  
int line_terminated(char* input, int size){
  int i;
  int result = -1;
  for(i=0;i<size;i++){
    if(input[i] == '\n'){
      result = i;
      break;
    }
  }
  return result;
}


int null_terminated(char* input, int size){
  int i;
  int result = -1;
  for(i=0;i<size;i++){
    if(input[i] == '\0'){
      result = i;
      break;
    }
  }
  return result;
}


user_cmd parse_cmd(char* cmd){
  int i,j,k;
  int space = 0;
  user_cmd cmd_info;
  memset(&cmd_info, 0, sizeof(cmd_info));
  for(i=0;cmd[i]!='\0';i++){
    if(cmd[i] == ' '){
      space = 1;
      for(j=0;j<i;j++){
	cmd_info.cmd[j] = cmd[j];
      }
      cmd_info.cmd[j] = '\0';
      j = i+1;
      k = 0;
      while(cmd[j] != '\0'){
	cmd_info.args[k] = cmd[j];
	j++;
	k++;
      }
      cmd_info.args[k] = '\0';
      break;
    }
  }
  if(space == 0){
    for(i=0;cmd[i]!='\0';i++){
      cmd_info.cmd[i] = cmd[i];
    }
    cmd_info.cmd[i] = '\0';
  }
  return cmd_info;
}

  
void trim_msg(char* buf, size_t len){
  /* This function trims character '\r' out the input buf */
  int i = 0;
  int j = 0;
  for(i=0, j=0;(i<len) && (j<len);i++,j++){
    if(buf[i] == '\r'){
      j = j + 1;
    }
    buf[i] = buf[j];
  }
}

ssize_t irc_recv(int sockfd, void* buf, size_t len, int flags){
  return recv(sockfd, buf, len, flags);/* currently, it is only a simply wrapper function*/
}
ssize_t irc_send(int sockfd, const void *buf, size_t len, int flags){
  return send(sockfd, buf, len, flags);
}
void send_message_by_type(int sockfd, char* msg_type, char* msg_body){
  char buf[MAX_BUFFER];
  char buf2[MAX_BUFFER];
  if(strcmp(msg_type, "NOTICE") == 0){
    snprintf(buf, sizeof(buf), ":%s NOTICE * :*** %s", SERVER_NAME, msg_body);
    irc_send(sockfd, buf, strlen(buf), 0);
    return;
  }
  else if(strcmp(msg_type, "PING") == 0){


    return;
  }
  else if(strcmp(msg_type, "PRIVMSG") == 0){


    return;
  }


}
void send_message_by_number(int num, int sockfd, char* msg_body){
  char msg[MAX_BUFFER];
  snprintf(msg, sizeof(msg), ":%s %d %s\r\n", SERVER_NAME, num, msg_body);
  irc_send(sockfd, msg, strlen(msg), 0);



}
void send_message(int error_num, int sockfd, char* cmd, irc_argument* irc_args){
  char msg[MAX_BUFFER] = {0};
  char msg2[MAX_BUFFER] = {0};
  switch(error_num){
  case 401:
    break;
  case 402:
    break;
  case 403:
    break;
  case 404:
    break;
  case 405:
    break;
  case 406:
    break;
  case 407:
    break;
  case 409:
    break;
  case 411:
    break;
  case 412:
    break;
  case 413:
    break;
  case 414:
    break;
  case 421:
    snprintf(msg, sizeof(msg)-strlen(SERVER_NAME)-5, ":%s 421 %s :Unknown command", SERVER_NAME, cmd);
    snprintf(msg2, sizeof(msg2), "%s\r\n", msg);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    break;
  case 422:
    break;
  case 423:
    break;
  case 424:
    break;
  case 431:
    snprintf(msg, sizeof(msg)-strlen(SERVER_NAME)-5, ":%s 431 :No nickname given", SERVER_NAME);
    snprintf(msg2, sizeof(msg2), "%s\r\n", msg);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    break;
  case 432:
    snprintf(msg, sizeof(msg)-strlen(SERVER_NAME)-5, ":%s 432 %s :Erroneus nickname", SERVER_NAME, irc_args->param);
    snprintf(msg2, sizeof(msg2), "%s\r\n", msg);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    break;
  case 433:
    snprintf(msg, sizeof(msg)-strlen(SERVER_NAME)-5, ":%s 433 %s :Nickname is already in use", SERVER_NAME, irc_args->param);
    snprintf(msg2, sizeof(msg2), "%s\r\n", msg);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    break;
  case 436:
    break;
  case 441:
    break;
  case 442:
    break;
  case 443:
    break;
  case 444:
    break;
  case 445:
    break;
  case 446:
    break;
  case 451:
    snprintf(msg, sizeof(msg), ":%s 451 :You have not registered\r\n", SERVER_NAME);
    irc_send(sockfd, msg, strlen(msg), 0);
    break;
  case 461:
    break;
  case 462:
    break;
  case 463:
    break;
  case 464:
    break;
  case 465:
    break;
  case 467:
    break;
  case 471:
    break;
  case 472:
    break;
  case 473:
    break;
  case 474:
    break;
  case 475:
    break;
  case 481:
    break;
  case 482:
    break;
  case 483:
    break;
  case 491:
    break;
  case 501:
    break;
  case 502:
    break;
  case 300:
    break;
  case 302:
    break;
  case 303:
    break;
  case 301:
    break;
  case 305:
    break;
  case 306:
    break;
  case 311:
    break;
  case 312:
    break;
  case 313:
    break;
  case 317:
    break;
  case 318:
    break;
  case 319:
    break;
  case 314:
    break;
  case 369:
    break;
  case 321:
    break;
  case 322:
    break;
  case 323:
    break;
  case 324:
    break;
  case 331:
    break;
  case 332:
    break;
  case 341:
    break;
  case 342:
    break;
  case 351:
    break;
  case 352:
    break;
  case 315:
    break;
  case 353:
    break;
  case 366:
    break;
  case 364:
    break;
  case 365:
    break;
  case 367:
    break;
  case 368:
    break;
  case 371:
    break;
  case 374:
    break;
  case 375:
    break;
  case 372:
    break;
  case 376:
    break;
  case 381:
    break;
  case 382:
    break;
  case 391:
    break;
  case 392:
    break;
  case 393:
    break;
  case 394:
    break;
  case 395:
    break;
  case 200:
    break;
  case 201:
    break;
  case 202:
    break;
  case 203:
    break;
  case 204:
    break;
  case 205:
    break;
  case 206:
    break;
  case 208:
    break;
  case 261:
    break;
  case 211:
    break;
  case 212:
    break;
  case 213:
    break;
  case 214:
    break;
  case 215:
    break;
  case 216:
    break;
  case 218:
    break;
  case 219:
    break;
  case 241:
    break;
  case 242:
    break;
  case 243:
    break;
  case 244:
    break;
  case 221:
    break;
  case 251:
    break;
  case 252:
    break;
  case 253:
    break;
  case 254:
    break;
  case 255:
    break;
  case 256:
    break;
  case 257:
    break;
  case 258:
    break;
  case 259:
    break;

  default:
    break;
  }
}
