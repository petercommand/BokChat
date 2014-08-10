#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "connect.h"
#include "main.h"
#include "config.h"
#include "list.h"
#include "command.h"
#include <signal.h>
#include <errno.h>
#include <ctype.h>
int get_cmd(int socket, char* buf, char* cmd, int* timeout);
static void irc_client_connect_loop(int* sockfd_p);
void client_connect(user_info* user_inf);
static void irc_user_liveness_check_loop();
int irc_init_user(user_info* user_inf, char* buf);
int line_terminated(char* input, int size);
user_cmd parse_cmd(char* cmd);
void server_mutex_init();
ssize_t irc_recv(int sockfd, void* buf, size_t len, int flags);
ssize_t irc_send(int sockfd, const void *buf, size_t len, int flags);
void trim_msg(char* buf, size_t len);
int process_cmd(user_cmd cmd_info, user_info* user_info);
int process_cmd_nick_init(user_cmd cmd_info, user_info* user_inf);
static void reverse_dns(user_info* user_inf);
void send_message_by_type(user_info* user_inf, const char* msg_type, char* msg_body);
void send_message_by_number(int num, user_info* user_inf, char* msg_body);
void send_message(int error_num, user_info* user_inf, channel_info* channel_inf, char* cmd, irc_argument* irc_args);
void print_hex(char *input);
void motd(user_info* user_inf);

void start_irc_server(int irc_sockfd, int ssl_irc_sockfd){
  server_mutex_init();
  pthread_t client_connect_thread;
  pthread_t liveness_check_thread;
  pthread_attr_t client_connect_thread_attr;
  pthread_attr_setdetachstate(&client_connect_thread_attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_init(&client_connect_thread_attr);
  pthread_attr_t liveness_check_thread_attr;
  pthread_attr_init(&liveness_check_thread_attr);
  pthread_attr_setdetachstate(&liveness_check_thread_attr, PTHREAD_CREATE_DETACHED);
  int* sockfd_p = (int *)malloc(sizeof(int));
  if(sockfd_p == NULL){
    fprintf(stderr, "Failed to malloc: %s\n", strerror(errno));
    exit(1);
  }
  *sockfd_p = irc_sockfd;
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sa, 0);
  pthread_create(&client_connect_thread, &client_connect_thread_attr, (void *(*)(void *))irc_client_connect_loop, (void *)sockfd_p);
  pthread_create(&liveness_check_thread, &liveness_check_thread_attr, (void *(*)(void *))irc_user_liveness_check_loop, NULL);


  while(1){
    pause();
  }

  free(sockfd_p);
}

/* mutex lock sequence: user then channel*/




int irc_listen_bind_on_port(int port){

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
void irc_client_connect_loop(int* sockfd_p){
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
    pthread_mutex_init(&user_inf->sock_mutex, NULL);
    pthread_attr_t user_thread_attr;
    pthread_attr_init(&user_thread_attr);
    pthread_attr_setdetachstate(&user_thread_attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&user_inf->user_thread, &user_thread_attr, (void *(*)(void *))client_connect, (void *)user_inf);
  }
}
void client_connect(user_info* user_inf){
  char* buf = (char *)malloc(MAX_BUFFER);
  char* cmd = (char *)malloc(MAX_BUFFER);
  char* msg = (char *)malloc(MAX_BUFFER);
  char* msg2 = (char *)malloc(MAX_BUFFER);
  int get_cmd_num;
  if((buf == NULL) || (cmd == NULL) || (msg == NULL) || (msg2 == NULL)){
    goto error;
  }
  memset(buf, 0, MAX_BUFFER);
  memset(cmd, 0, MAX_BUFFER);
  memset(msg, 0, MAX_BUFFER);
  memset(msg2, 0, MAX_BUFFER);
  if(irc_init_user(user_inf, buf) != 0){
/* user have to send in NICK and USER command to the server to init_user function, this function shall initialize everything in the user_info struct for the user. If init_user return non-zero value, disconnect the user immediately */
    goto error;
  }
  snprintf(msg, MAX_BUFFER -3, ":Welcome to the bokchat Internet Relay Chat Network %s", user_inf->user_nick);
  snprintf(msg2, MAX_BUFFER, "%s\r\n", msg);
  send_message_by_number(001, user_inf, msg2);
  snprintf(msg, MAX_BUFFER, ":Your host is %s, running version bokchat-1.0.0\r\n", SERVER_NAME);
  send_message_by_number(002, user_inf, msg);
  motd(user_inf);



  while(1){
    get_cmd_num = get_cmd(user_inf->socket, buf, cmd, NULL);/*blocks until it receives command */
    if(get_cmd_num == -1){
      goto error;
    }
    pthread_mutex_lock(&global_user_mutex);
    update_user_liveness(user_inf);
    if(get_cmd_num == -2){
      pthread_mutex_unlock(&global_user_mutex);
      continue;
    }

    user_cmd cmd_info = parse_cmd(cmd);   
    if(0){/*debug*/
      printf("cmd_info->cmd:%s\ncmd_info->args:%s\n", cmd_info.cmd, cmd_info.args);
    }
    if(process_cmd(cmd_info, user_inf) == -2){
      pthread_mutex_unlock(&global_user_mutex);
      goto quit;
    }
    pthread_mutex_unlock(&global_user_mutex);
   }
 error:
  quit_server(user_inf, "Client Quit");
  pthread_mutex_unlock(&global_user_mutex);
 quit:
  free(buf);
  free(cmd);
  free(msg);
  free(msg2);
  pthread_exit(NULL);
}

static void irc_user_liveness_check_loop(){
  user_list* head;
  user_list* temp;
  char msg[MAX_BUFFER];
  time_t diff;
  while(1){
    sleep(90);
    pthread_mutex_lock(&global_user_mutex);
    for(head = global_user_list;head != NULL; head = temp){
      if(head->next != NULL){
	temp = head->next;
      }
      else{
	temp = NULL;
      }
      diff = time(NULL) - head->user_info->liveness;
      if(0){/*debug*/
	 printf("liveness thread: time difference: %ld\n", diff);
      }
      if(diff > 270){/* user timeout */
	snprintf(msg, MAX_BUFFER, "Ping timeout: %ld seconds\r\n", diff);
	quit_server(head->user_info, msg);
      }
      else if(diff > 90){/*send ping cmd */
	snprintf(msg, MAX_BUFFER, "PING :%s\r\n", SERVER_NAME);
	send_message_to_user(head->user_info, msg);
      }
    }
    pthread_mutex_unlock(&global_user_mutex);
  }
  
}
int get_cmd(int socket, char* buf, char* cmd, int* timeout){
  memset(cmd, 0, sizeof(&cmd));
  int recv_byte = -1;
  int term_buf = -1;
  int term_buf2 = -1;
  int ready;
  char recv_cmd[MAX_BUFFER+1] = {'\0'};
  term_buf = line_terminated(buf, MAX_BUFFER);
  if(term_buf == -1){
    fd_set set;
    struct timeval wait_time;
    FD_ZERO(&set);
    FD_SET(socket, &set);
    if(timeout != NULL){
      wait_time.tv_sec = *timeout;
      wait_time.tv_usec = 0;
      ready = select(socket+1,  &set, NULL, NULL, &wait_time);
    }
    else{
      ready = select(socket+1, &set, NULL, NULL, NULL);
    }
    if(ready < 0){
      return -1;
    }
    if(FD_ISSET(socket, &set)){
      recv_byte = irc_recv(socket, recv_cmd, MAX_BUFFER -1, MSG_DONTWAIT);/* already used the select call to block, thus recv should be set to nonblocking */
      if(recv_byte <= 0){
	return -1;
      }
    }
    else{
      return -2;
    }
  }
  else if(term_buf == 0){
    memmove(buf, &buf[1], MAX_BUFFER -1);
    return -2;
  }
  else{
    goto cont;/* already has a command in buffer, no need to receive from socket */
  }
  trim_msg(recv_cmd, recv_byte);
  strncat(buf, recv_cmd, MAX_BUFFER-strlen(buf)-1);
 cont:
  term_buf2 = line_terminated(buf, MAX_BUFFER);
  if(0){/*debug*/
    printf("buf: %s\n", buf);
    printf("term_buf: %d\n", term_buf2);
  }
  if(term_buf2 == -1){
    /* truncate and clear buf here */
    buf[MAX_BUFFER-1] = '\0';
    strncpy(cmd, buf, MAX_BUFFER);
    memset(buf, 0, sizeof(&buf));
    return 0;
  }
  buf[term_buf2] = '\0';
  strncpy(cmd, buf, MAX_BUFFER);
  memset(buf, 0, term_buf2 + 1);
  if(buf[term_buf2+1] != '\0'){
    memmove(buf, &buf[term_buf2 + 1], MAX_BUFFER - term_buf2 - 1);
  }
  else{
    memmove(buf, &buf[term_buf2 + 2], MAX_BUFFER - term_buf2 - 2);
  }
  return 0;
}
    


int valid_channel(char* input){
  if(input == NULL){
    return 0;
  }
  int len = strlen(input);
  int i;
  if(strlen(input) < 2){
    return 0;
  }
  if(input[0] != '#'){
    return 0;
  }

  for(i=1;i<len;i++){
    if((input[i] >= 'a') && (input[i] <= 'z')){
      continue;
    }
    if((input[i] >= 'A') && (input[i] <= 'Z')){
      continue;
    }
    if((input[i] == '_') || (input[i] == '-') || (input[i] == '.') || (input[i] == '(') || (input[i] == ')')){
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
    if((input[i] >= '0') && (input[i] <= '9')){
      continue;
    }
    if((input[i] == '_') || (input[i] == '-') || (input[i] == '.') || (input[i] == '^') || (input[i] == '(') || (input[i] == ')')){
      continue;
    }
    return 0;
  }
  return 1;
}

void print_hex(char *input){
  while(*input){
    printf("hex: %02x char: %c\n", (unsigned int)*input, (unsigned char)*input);
    input++;
  }
}

int irc_init_user(user_info* user_inf, char* buf){
  time_t start_time = time(NULL);
  int get_cmd_num;
  int nick = 0;
  int user = 0;
  char* cmd = (char *)malloc(MAX_BUFFER);
  if(cmd == NULL){
    goto error;
  }
  user_cmd cmd_info;
  memset(&cmd_info, 0, sizeof(cmd_info));
  if(cmd == NULL){
    goto error;
  }
  pthread_t user_dns;
  pthread_create(&user_dns, NULL, (void *(*)(void *))reverse_dns, (void *)user_inf);
  while((nick == 0) || (user == 0)){
    int timeout = 15;
    get_cmd_num = get_cmd(user_inf->socket, buf, cmd, &timeout);
    if(get_cmd_num == -1){
      goto error;
    }
    if(get_cmd_num == -2){
      continue;
    }
    if((cmd != NULL) && (cmd[0] != '\0')){
      cmd_info = parse_cmd(cmd);
      if(0){/*debug*/
	 printf("cmd_info->cmd:%s\ncmd_info->args:%s\n", cmd_info.cmd, cmd_info.args);
      }
      /*put user into global user list after first nick command as we find user by nick */
      if(strcmp(cmd_info.cmd, "NICK") == 0){
	if(process_cmd_nick_init(cmd_info, user_inf) == 0){
	  nick = 1;
	}
      }
      else if(strcmp(cmd_info.cmd, "USER") == 0){
	if(process_cmd(cmd_info, user_inf) == 0){
	  user = 1;
	}
      }
      else if(strcmp(cmd_info.cmd, "CAP") == 0){/* currently we are ignoring irc cap command, might implement it in the future */
	continue;
      }
      else{
	send_message(451, user_inf, NULL, NULL, NULL);
      }
      
    }
  
  
    if((time(NULL) - start_time) >= 60){/*timeout value*/
      goto error;
    }
  }
  goto exit;
 exit:
  free(cmd);
  pthread_join(user_dns, NULL);
  return 0;/* user and nick commands are both set to 1: ready to go*/
 error:
  free(cmd);
  pthread_join(user_dns, NULL);
  return -1;
}
int line_terminated(char* input, int size){
  int i;
  int result = -1;
  for(i=0;(input[i] != '\0') && (i < size);i++){
    if(input[i] == '\n'){
      result = i;
      break;
    }
  }
  return result;
}



static void reverse_dns(user_info* user_inf){/*this function do reverse and forward dns to check client's hostname */
  /*only this thread can access user's hostname field when this thread is active, so there is no need to lock*/
  struct sockaddr client_addr = user_inf->client_addr;
  /*reverse dns*/
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  char* ip = inet_ntoa(((struct sockaddr_in *)&user_inf->client_addr)->sin_addr);
  char hostname[NI_MAXHOST];
  int i;
  send_message_by_type(user_inf, "NOTICE", "Looking up your hostname...\r\n");
  int res = getnameinfo(&client_addr, sizeof(client_addr), hostname, sizeof(hostname), NULL, 0, NI_NAMEREQD);
  if(res != 0){
    send_message_by_type(user_inf, "NOTICE", "No hostname found\r\n");
    for(i=0;ip[i] != '\0';i++){
      hostname[i] = ip[i];
    }
    return;
  }
  /*forward dns*/
  res = getaddrinfo(hostname, NULL, &hints, &result);
  if(res != 0){
    send_message_by_type(user_inf, "NOTICE", "No hostname found\r\n");
    for(i=0;ip[i] != '\0';i++){
      hostname[i] = ip[i];
    }
    return;
  }
  for(rp = result; rp != NULL; rp = rp->ai_next){
    if(strcmp(rp->ai_addr->sa_data, client_addr.sa_data)){
      for(i=0; hostname[i] != '\0'; i++){
	user_inf->hostname[i] = hostname[i];
      }
      send_message_by_type(user_inf, "NOTICE", "Found your hostname\r\n");
      printf("hostname: %s\n", hostname);
      freeaddrinfo(result);
      return;
    }
  }
  freeaddrinfo(result);
  return;


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
  for(i=0;cmd_info.cmd[i] != '\0';i++){
    cmd_info.cmd[i] = toupper(cmd_info.cmd[i]);
  }

  
  return cmd_info;
}

  
void trim_msg(char* buf, size_t len){
  /* This function replaces character '\r' from the input buf */
  int i = 0;
  for(i=0;i<len;i++){
    if((unsigned int)buf[i] == 13){
      buf[i] = '\n';
    }
  }
}

ssize_t irc_recv(int sockfd, void* buf, size_t len, int flags){
  return recv(sockfd, buf, len, flags);/* currently, it is only a simply wrapper function*/
}
ssize_t irc_send(int sockfd, const void *buf, size_t len, int flags){
  return send(sockfd, buf, len, flags);
}
void send_message_to_user(user_info* user_inf, char* msg_body){
  pthread_mutex_lock(&user_inf->sock_mutex);
  irc_send(user_inf->socket, msg_body, strlen(msg_body), MSG_DONTWAIT);
  pthread_mutex_unlock(&user_inf->sock_mutex);


}

void send_message_to_user_in_list(list_msg* user_list_msg){/*remember to free in callee*/
  user_list* user_lst = user_list_msg->user_lst;
  char* buf = user_list_msg->msg_body;
  while(user_lst != NULL){
    send_message_to_user(user_lst->user_info, buf);
    remove_node_from_user_list(&user_lst, user_lst->user_info);
  }
  free(user_list_msg);
	     



}

void send_message_by_type(user_info* user_inf, const char* msg_type, char* msg_body){
  char buf[MAX_BUFFER];
  if(strcmp(msg_type, "NOTICE") == 0){
    snprintf(buf, sizeof(buf), ":%s NOTICE * :*** %s", SERVER_NAME, msg_body);
    send_message_to_user(user_inf, buf);
    goto exit;
  }
  if(strcmp(msg_type, "MOTD") == 0){
    snprintf(buf, sizeof(buf), ":%s 372 :- %s", SERVER_NAME, msg_body);
    send_message_to_user(user_inf, buf);
    goto exit;
  }
 exit:
  return;
  
}
void send_message_by_number(int num, user_info* user_inf, char* msg_body){
  char msg[MAX_BUFFER];
  snprintf(msg, sizeof(msg), ":%s %03d %s %s", SERVER_NAME, num, user_inf->user_nick, msg_body);
  send_message_to_user(user_inf, msg);
}


void motd(user_info* user_inf){
  char buf[MAX_BUFFER] = {0};
  int i;
  char* motd[] = {"Welcome to bokchat.cupcake.\r\n",
		  "\r\n",
		  "\r\n",
		  "\r\n",
		  "Welcome to BokChat\r\n",
		  "By connecting to this server, you indicate that you\r\n",
		  "have agreed the following terms set forth by the service owner\r\n",
		  "Disclaimer:\r\n",
		  "THERE IS NO WARRANTY FOR THE USE OF THIS SERVICE, TO THE\r\n",
		  "EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE\r\n",
		  "STATED IN WRITING, THIS SERVICE IS PROVIDED \"AS IS\"\r\n",
		  "WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,\r\n",
		  "INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES FOR\r\n",
		  "A PARTICULAR PURPOSE.\r\n",
		  "\r\n",
		  "In accordance with ROC law, rlhsu has no tolerance for any\r\n",
		  "activity which could be construed as:\r\n",
		  "    *incitement to racial hatred\r\n",
		  "    *incitement to religious hatred\r\n",
		  "    *any behaviour meant to deliberately bring\r\n",
		  "     upon a person harassment, alarm or distress.\r\n",
		  "    *any form of discrimination on the ground of\r\n",
		  "     race, religion, gender, sexual preference or\r\n",
		  "     other lifestyle choices\r\n",
		  "\r\n",
		  "This server is the first project I ever\r\n",
		  "made in the C language\r\n",
		  "If there are any problem with my server,\r\n",
		  "please open an issue on github.\r\n",
		  "May you have a good time on this server!\r\n"
  };
  snprintf(buf, sizeof(buf), ":- %s Message of the Day -\r\n", SERVER_NAME);
  send_message_by_number(375, user_inf, buf);
  for(i=0;i<(sizeof(motd)/sizeof(char*));i++){
    send_message_by_type(user_inf, "MOTD", motd[i]);
  }
  snprintf(buf, sizeof(buf), ":End of /MOTD command\r\n");
  send_message_by_number(376, user_inf, buf);
}








void send_message(int error_num, user_info* user_inf, channel_info* channel_inf, char* cmd, irc_argument* irc_args){
  char msg[MAX_BUFFER] = {0};
  char msg2[MAX_BUFFER] = {0};
  int sockfd = user_inf->socket;
  switch(error_num){
  case 401:
    break;
  case 402:
    break;
  case 403:
    snprintf(msg, sizeof(msg)-3, ":%s 403 %s :No such channel", SERVER_NAME, irc_args->param);
    snprintf(msg2, sizeof(msg), "%s\r\n", msg);
    pthread_mutex_lock(&user_inf->sock_mutex);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    pthread_mutex_unlock(&user_inf->sock_mutex);
    break;
  case 404:
    snprintf(msg, sizeof(msg)-3, ":%s 404 %s :Cannot send to channel", SERVER_NAME, irc_args->param);
    snprintf(msg2, sizeof(msg), "%s\r\n", msg);
    pthread_mutex_lock(&user_inf->sock_mutex);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    pthread_mutex_unlock(&user_inf->sock_mutex);
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
    snprintf(msg, sizeof(msg)-3, ":%s 421 %s :Unknown command", SERVER_NAME, cmd);
    snprintf(msg2, sizeof(msg2), "%s\r\n", msg);
    pthread_mutex_lock(&user_inf->sock_mutex);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    pthread_mutex_unlock(&user_inf->sock_mutex);
    break;
  case 422:
    break;
  case 423:
    break;
  case 424:
    break;
  case 431:
    snprintf(msg, sizeof(msg)-3, ":%s 431 :No nickname given", SERVER_NAME);
    snprintf(msg2, sizeof(msg2), "%s\r\n", msg);
    pthread_mutex_lock(&user_inf->sock_mutex);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    pthread_mutex_unlock(&user_inf->sock_mutex);
    break;
  case 432:
    snprintf(msg, sizeof(msg)-3, ":%s 432 %s :Erroneus nickname", SERVER_NAME, irc_args->param);
    snprintf(msg2, sizeof(msg2), "%s\r\n", msg);
    pthread_mutex_lock(&user_inf->sock_mutex);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    pthread_mutex_unlock(&user_inf->sock_mutex);
    break;
  case 433:
    snprintf(msg, sizeof(msg)-3, ":%s 433 %s :Nickname is already in use", SERVER_NAME, irc_args->param);
    snprintf(msg2, sizeof(msg2), "%s\r\n", msg);
    pthread_mutex_lock(&user_inf->sock_mutex);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    pthread_mutex_unlock(&user_inf->sock_mutex);
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
    pthread_mutex_lock(&user_inf->sock_mutex);
    irc_send(sockfd, msg, strlen(msg), 0);
    pthread_mutex_unlock(&user_inf->sock_mutex);
    break;
  case 461:
    snprintf(msg, sizeof(msg)-strlen(SERVER_NAME)-5, ":%s 461 %s :Not enough parameters", SERVER_NAME, cmd);
    snprintf(msg2, sizeof(msg2), "%s\r\n", msg);
    pthread_mutex_lock(&user_inf->sock_mutex);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    pthread_mutex_unlock(&user_inf->sock_mutex);
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
    snprintf(msg, sizeof(msg)-3, ":%s 331 %s :No topic is set", SERVER_NAME, irc_args->param);
    snprintf(msg2, sizeof(msg2), "%s\r\n", msg);
    pthread_mutex_lock(&user_inf->sock_mutex);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    pthread_mutex_unlock(&user_inf->sock_mutex);
    break;
  case 332:
    snprintf(msg, sizeof(msg)-3, ":%s 332 %s %s :%s", SERVER_NAME, user_inf->user_nick, channel_inf->channel_name, channel_inf->topic);
    snprintf(msg2, sizeof(msg2), "%s\r\n", msg);
    pthread_mutex_lock(&user_inf->sock_mutex);
    irc_send(sockfd, msg2, strlen(msg2), 0);
    pthread_mutex_unlock(&user_inf->sock_mutex);
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
