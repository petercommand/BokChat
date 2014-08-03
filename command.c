/* make change to the necessary structs and return the necessary data*/
/*return int to reflect the result of command processing */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "connect.h"
#include "list.h"
#include "command.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


irc_argument parse_argument(char* args);
ssize_t irc_recv(int sockfd, void* buf, size_t len, int flags);
ssize_t irc_send(int sockfd, const void *buf, size_t len, int flags);
void to_upper_case(char* input);
int null_terminated(char* input, int size);
void names_command(user_info* user_inf, channel_info* channel_inf, irc_argument* irc_args);





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
  send_message(error_num, user_inf, NULL, cmd, &irc_args);
  return -1;
}



int process_cmd(user_cmd cmd_info, user_info* user_inf){
  char* cmd = cmd_info.cmd;
  char* args = cmd_info.args;
  int error_num;
  irc_argument irc_args = parse_argument(args);
  int i;
  if(strcmp(cmd, "NICK") == 0){
    char old_nick[MAX_NICK_LENGTH+1];
    for(i=0;user_inf->user_nick[i] != '\0';i++){
      old_nick[i] = user_inf->user_nick[i];
    }
    old_nick[i] = '\0';
    if(nick_change(user_inf, irc_args.param, &error_num) == 0){ 
      pthread_mutex_lock(&global_channel_mutex);
      

      channel_list* head;
      user_list* head2;
      user_list* user_lst = NULL;
      add_node_to_user_list(&user_lst, user_inf);
      for(head = user_inf->joined_channels;head!=NULL;head = head->next){
	for(head2 = head->channel_info->joined_users;head2 != NULL;head2 = head2->next){
	  if(is_node_in_user_list(user_lst, head2->user_info) == 0){
	    add_node_to_user_list(&user_lst, head2->user_info);
	  }
	}
      }
      pthread_mutex_unlock(&global_channel_mutex);
      list_msg* nick_list_msg = (list_msg *)malloc(sizeof(list_msg));
      if(nick_list_msg == NULL){
	goto error;
      }
      char msg[MAX_BUFFER] = {0};
      memset(nick_list_msg, 0, sizeof(list_msg));
      snprintf(msg, MAX_BUFFER -3, ":%s!%s NICK :%s", old_nick, old_nick, user_inf->user_nick);
      snprintf(nick_list_msg->msg_body, MAX_BUFFER, "%s\r\n", msg);
      nick_list_msg->user_lst = user_lst;
      pthread_t nick_thread;
      pthread_attr_t nick_thread_attr;
      pthread_attr_init(&nick_thread_attr);
      pthread_attr_setdetachstate(&nick_thread_attr, PTHREAD_CREATE_DETACHED);
      pthread_create(&nick_thread, &nick_thread_attr, (void *(*)(void *))send_message_to_user_in_list, (void *)nick_list_msg);

      goto exit;
    }
    send_message(error_num, user_inf, NULL, cmd, &irc_args);
    goto error;
  }
  else if(strcmp(cmd, "PRIVMSG") == 0){
    if(irc_args.param[0] == '#'){/*channel*/
      pthread_mutex_lock(&global_channel_mutex);
      channel_info* channel_inf = channel_exist_by_name(irc_args.param);
      if(channel_inf == NULL){
	send_message(403, user_inf, NULL, cmd, &irc_args);
	pthread_mutex_unlock(&global_channel_mutex);
	goto error;
      }
      if(is_user_in_channel(user_inf, channel_inf) == 0){
	send_message(404, user_inf, NULL, cmd, &irc_args);
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
      pthread_attr_t privmsg_attr;
      pthread_attr_init(&privmsg_attr);
      pthread_attr_setdetachstate(&privmsg_attr, PTHREAD_CREATE_DETACHED);
      pthread_create(&privmsg, &privmsg_attr, (void *(*)(void *))send_message_to_all_users_in_channel, (void *)channel_msg);
      goto exit;
    }
    else{/*person*/
      user_info* target_user = user_exist_by_name(irc_args.param);
      if(target_user == NULL){
	send_message(401, user_inf, NULL, cmd, &irc_args);
	goto error;
      }
      char privmsg_buf[MAX_BUFFER];
      char privmsg_buf2[MAX_BUFFER];
      snprintf(privmsg_buf, MAX_BUFFER, ":%s!%s@%s PRIVMSG %s :%s", user_inf->user_nick, user_inf->user_nick, user_inf->hostname, target_user->user_nick, irc_args.trailing);
      snprintf(privmsg_buf2, MAX_BUFFER, "%s\r\n", privmsg_buf);
      send_message_to_user(target_user, privmsg_buf2);

      goto exit;
    }
  }
  else if(strcmp(cmd, "USER") == 0){
    if(is_user_in_global_user_list(user_inf)){
      /*might have some problem here, fix it */
      goto error;
    }
    if((irc_args.param[0] == '\0') || (irc_args.trailing[0] == '\0')){
      send_message(461, user_inf, NULL, cmd, &irc_args);
      goto error;
    }
    join_user_to_global_list(user_inf);
    goto exit;
  }
  else if(strcmp(cmd, "JOIN") == 0){
    if(irc_args.param[0] == '\0'){
      goto error;
    } 
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
    for(i=0; irc_args.param[i] != '\0'; i++){
      channel_msg->msg_body[i] = irc_args.param[i];
    }
    channel_msg->msg_body[i] = '\0';
    pthread_mutex_lock(&global_channel_mutex);
    channel_info* channel_inf = channel_exist_by_name(irc_args.param);
    pthread_mutex_unlock(&global_channel_mutex);
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
      pthread_mutex_unlock(&global_channel_mutex);
      goto join_send_msg;
    }
    /*channel_inf != NULL */
    if(is_user_in_channel(user_inf, channel_inf) == 1){
      send_message(443, user_inf, NULL, cmd, &irc_args);
      goto error;
    }
  join_send_msg:/*at this point, the channel creation process must be completed, both originally channel_inf == NULL and channel_inf != NULL conditions should met this reqirement atm*/
    pthread_mutex_lock(&global_channel_mutex);
    if(join_user_to_channel(user_inf, channel_inf) != 0){      
      pthread_mutex_unlock(&global_channel_mutex);
      goto error;
    }
    pthread_mutex_unlock(&global_channel_mutex);
    channel_msg->channel_info = channel_inf;
    send_message_to_all_users_in_channel(channel_msg);
    /*show topic to user*/
    pthread_mutex_lock(&global_channel_mutex);
    char* topic = get_channel_topic(channel_inf);
    if(topic[0] == '\0'){
      send_message(331, user_inf, NULL, cmd, &irc_args);
    }
    else{
      send_message(332, user_inf, channel_inf, cmd, &irc_args);
    }
    pthread_mutex_unlock(&global_channel_mutex);
    /*show channel names to user */ 
    names_command(user_inf, channel_inf, &irc_args);



    
    
    goto exit;
  }
  else if(strcmp(cmd, "NAMES") == 0){
    channel_info* channel_inf = channel_exist_by_name(irc_args.param);
    if(channel_inf == NULL){
      send_message(403, user_inf, NULL, cmd, &irc_args);
      goto error;
    }
    names_command(user_inf, channel_inf, &irc_args);
    goto exit;
  }
  else if(strcmp(cmd, "PART") == 0){
    if(irc_args.param[0] == '\0'){
      goto error;
    } 
    irc_channel_msg* channel_msg = (irc_channel_msg *)malloc(sizeof(irc_channel_msg));
    if(channel_msg == NULL){
      goto error;
    }
    memset(channel_msg, 0, sizeof(*channel_msg));
    const char* msg_type = "PART";
    for(i=0;msg_type[i]!='\0';i++){
      channel_msg->message_type[i] = msg_type[i];
    }
    channel_msg->user_inf = user_inf;
    for(i=0; irc_args.param[i] != '\0'; i++){
      channel_msg->msg_body[i] = irc_args.param[i];
    }
    channel_msg->msg_body[i] = '\0';
    pthread_mutex_lock(&global_channel_mutex);
    channel_info* channel_inf = channel_exist_by_name(irc_args.param);
    if(channel_inf == NULL){
      send_message(403, user_inf, NULL, cmd, &irc_args);
      pthread_mutex_unlock(&global_channel_mutex);
      goto error;
    }
    if(is_user_in_channel(user_inf, channel_inf) != 1){
      /*user not in channel*/
      pthread_mutex_unlock(&global_channel_mutex);
      goto error;
    }
    channel_msg->channel_info = channel_inf;
    pthread_mutex_unlock(&global_channel_mutex);
    send_message_to_all_users_in_channel(channel_msg);
    pthread_mutex_lock(&global_channel_mutex);
    quit_user_from_channel(user_inf, channel_inf);
    pthread_mutex_unlock(&global_channel_mutex);



    
    goto exit;
  }
  else if(strcmp(cmd, "TOPIC") == 0){
    channel_info* channel_inf;
    if(irc_args.trailing[0] == '\0'){/*get topic */
      pthread_mutex_lock(&global_channel_mutex);
      channel_inf = channel_exist_by_name(irc_args.param);
      if(channel_inf == NULL){
	send_message(403, user_inf, NULL, cmd, &irc_args);/*no such channel*/
	pthread_mutex_unlock(&global_channel_mutex);
	goto error;
      }
      else{
	char* topic = get_channel_topic(channel_inf);
	if(topic[0] == '\0'){
	  send_message(331, user_inf, NULL, cmd, &irc_args);
	  pthread_mutex_unlock(&global_channel_mutex);
	  goto exit;
	}
	send_message(332, user_inf, channel_inf, cmd, &irc_args);
	pthread_mutex_unlock(&global_channel_mutex);
	goto exit;
      }

    }
    else{/*change topic*/
      pthread_mutex_lock(&global_channel_mutex);
      channel_inf = channel_exist_by_name(irc_args.param);
      if(channel_inf == NULL){
	send_message(403, user_inf, NULL, cmd, &irc_args);/*no such channel*/
	pthread_mutex_unlock(&global_channel_mutex);
	goto error;
      }
      set_channel_topic(channel_inf, irc_args.trailing);
      irc_channel_msg* channel_msg = (irc_channel_msg *)malloc(sizeof(irc_channel_msg));
      if(channel_msg == NULL){
	pthread_mutex_unlock(&global_channel_mutex);
	goto error;
      }
      memset(channel_msg, 0, sizeof(*channel_msg));
      char* message_type = "TOPIC";
      for(i=0;message_type[i] != '\0';i++){
	channel_msg->message_type[i] = message_type[i];
      }
      channel_msg->message_type[i] = '\0';
      channel_msg->user_inf = user_inf;
      channel_msg->channel_info = channel_inf;
      send_message_to_all_users_in_channel(channel_msg);
      pthread_mutex_unlock(&global_channel_mutex);
      goto exit;
    }
      
    goto exit;
  }
  else if(strcmp(cmd, "KICK") == 0){
    
    goto exit;
  }
  else if(strcmp(cmd, "WHOIS") == 0){


    goto exit;
  }
  else if(strcmp(cmd, "QUIT") == 0){
    if(irc_args.trailing[0] == '\0'){
      quit_server(user_inf, NULL);
    }
    else{
      quit_server(user_inf, irc_args.trailing);
    }
    return -2;
  }
  else if(strcmp(cmd, "MOTD") == 0){
    motd(user_inf);
    goto exit;
  }
  else if(strcmp(cmd, "PING") == 0){
    if(strcmp(irc_args.param, SERVER_NAME) == 0){/*client is pinging the server*/
      char buf[MAX_BUFFER];
      snprintf(buf, MAX_BUFFER, "PONG :%s\r\n", SERVER_NAME);
      pthread_mutex_lock(&user_inf->sock_mutex);
      irc_send(user_inf->socket, buf, strlen(buf), MSG_DONTWAIT);
      pthread_mutex_unlock(&user_inf->sock_mutex);
      goto exit;
    }
    goto error;
  }

  /*No command matches */
  send_message(421, user_inf, NULL, cmd, NULL);
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


void names_command(user_info* user_inf, channel_info* channel_inf, irc_argument* irc_args){
  char buf[MAX_BUFFER];
  snprintf(buf, MAX_BUFFER, ":%s 353 %s = %s :", SERVER_NAME, user_inf->user_nick, channel_inf->channel_name);
  char buf2[MAX_BUFFER];
  char buf3[MAX_BUFFER];
  char temp[MAX_BUFFER];
  user_list* head = channel_inf->joined_users;

  while(head != NULL){
    snprintf(buf2, MAX_BUFFER, "%s", buf);
    while(strlen(buf2) < 450){
      if(head != NULL){
	snprintf(temp, MAX_BUFFER, "%s ", head->user_info->user_nick);
	strncat(buf2, temp, 20);
	head = head->next;
      }
      else{
	break;
      }
    }
    snprintf(buf3, MAX_BUFFER, "%s\r\n", buf2);
    pthread_mutex_lock(&user_inf->sock_mutex);
    irc_send(user_inf->socket, buf3, strlen(buf3), MSG_DONTWAIT);
    pthread_mutex_unlock(&user_inf->sock_mutex);
  }
  snprintf(buf, MAX_BUFFER -3, ":%s 366 %s %s :End of /NAMES list.", SERVER_NAME, user_inf->user_nick, channel_inf->channel_name);
  snprintf(buf2, MAX_BUFFER, "%s\r\n", buf);
  pthread_mutex_lock(&user_inf->sock_mutex);
  irc_send(user_inf->socket, buf2, strlen(buf2), MSG_DONTWAIT);
  pthread_mutex_unlock(&user_inf->sock_mutex);

}




int null_terminated(char* input, int size){
  int i;
  int result = -1;
  for(i=0;i < size;i++){
    if(input[i] == '\0'){
      result = i;
      break;
    }
  }
  return result;
}

