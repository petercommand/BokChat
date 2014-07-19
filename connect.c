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
#include <errno.h>
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
void trim_msg(char* buf, size_t len);
int process_cmd(user_cmd cmd_info);
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


  while(1);

  free(sockfd_p);
  sockfd_p = NULL;

}



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
    
    
    
  }
  
}
int get_cmd(int socket, char* buf, char* cmd){
  memset(cmd, 0, sizeof(&cmd));
  int recv_byte = -1;
  int term_buf = -1;
  int null_buf = -1;
  char recv_cmd[MAX_BUFFER+1] = {'\0'};
  recv_byte = irc_recv(socket, recv_cmd, MAX_BUFFER, MSG_DONTWAIT);/* already used the select call to block, thus recv should be set to nonblocking */
  printf("recv_byte: %d\n", recv_byte);
  if(recv_byte <= 0){
    return -1;
  }
  trim_msg(recv_cmd, recv_byte);
  strncat(buf, recv_cmd, MAX_BUFFER-strlen(buf)-1);
  term_buf = line_terminated(buf, MAX_BUFFER);
  null_buf = null_terminated(buf, MAX_BUFFER);
  printf("term_buf: %d\nnull_buf: %d\n", term_buf, null_buf);
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
  memmove(buf, &buf[term_buf + 2], MAX_BUFFER - term_buf -1);
  return 0;
}
    


int valid_string(char* input){
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

int init_user(user_info* user_inf, char* buf){
  time_t start_time = time(NULL);
  int ready;
  int nick = 0;
  int user = 0;
  int nick_already_set = 0;
  char* cmd = (char *)malloc(MAX_BUFFER);
  user_cmd cmd_info;
  memset(&cmd_info, 0, sizeof(cmd_info));
  if(cmd == NULL){
    return -1;
  }
  fd_set set;
  struct timeval wait_time;
  while((nick == 0) || (user == 0)){
    FD_ZERO(&set);
    FD_SET(user_inf->socket, &set);
    wait_time.tv_sec = 15;/*wait for 15 secs. Remember to reset timer after every select call!*/
    wait_time.tv_usec = 0;
    ready = select(user_inf->socket+1,  &set, NULL, NULL, &wait_time);
    printf("select socket: %d\n", user_inf->socket);
    printf("select: %d\n", ready);
    if(ready < 0){
      return -1;
    }
    if(FD_ISSET(user_inf->socket, &set)){
      if(get_cmd(user_inf->socket, buf, cmd) == -1){
	return -1;
      }
      if(cmd != NULL){
	cmd_info = parse_cmd(cmd);
	printf("cmd:%s\ncmd_info->cmd:%s\ncmd_info->args:%s\n",cmd, cmd_info.cmd, cmd_info.args);
	/*put user into global user list after first nick command as we find user by nick */
	if(strcmp(cmd_info.cmd, "NICK") == 0){
	  pthread_mutex_lock(&global_user_mutex);
	  if(process_cmd(cmd_info) == 0){
	    nick = 1;
	    if(nick_already_set == 0){
	      /*join user to global list*/
	      nick_already_set = 1;
	      if(join_user_to_global_list(user_inf) != 0){
		/*join to global list failed*/
		/*not finished*/
	      }

	    }
	    
	  }
	  pthread_mutex_unlock(&global_user_mutex);
	}
	else if(strcmp(cmd_info.cmd, "USER") == 0){
	  if(process_cmd(cmd_info) == 0){
	    user = 1;
	  }
	}
	  
      }
    }

    if((time(NULL) - start_time) >= 60){/*timeout value*/
      return -1;
    }
  }
  return 0;/* user and nick commands are both set to 1: ready to go*/
  
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


user_cmd parse_cmd(char* cmd){/* note that we don't need to process the string when no space is present as it is surely a invald command */
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
