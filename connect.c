#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
char* get_cmd(int client_socket);
void client_connect_loop(int* sockfd_p);
void client_connect(user_info* user_inf);
void liveness_check_loop();
int init_user(user_info* user_inf, char* buf);
/*
191. be sure to perform liveness(ping) check after each loop

*/
void server_mutex_init();
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
  init_user(user_inf, buf);/* user have to send in NICK and USER command to the server to this function, this function shall initialize everything in the user_info struct for the user */
  
  
  
  
  return;
}

void liveness_check_loop(){
  while(1){
    /* NOT finished*/
    
    
    
  }
  
}
char* get_cmd(int client_socket){
  
  
  
  
  
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
  char* cmd;
  struct timeval wait_time;
  wait_time.tv_sec = 15;/*wait for 15 secs */
  wait_time.tv_usec = 0;
  fd_set set;
  FD_ZERO(&set);
  FD_SET(user_inf->socket, &set);
  while((nick ==0) || (user == 0)){
    ready = select(user_inf->socket+1,  &set, NULL, NULL, &wait_time);
    if(ready < 0){
      return -1;
    }
    if(FD_ISSET(user_inf->socket, &set)){
      cmd = get_cmd(user_inf->socket, buf);
      
    }
    if((time(NULL) - start_time) >= 60){
      return -1;
    }
  }
  return 0;/* user and nick commands are both set to 1: ready to go*/
  
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
