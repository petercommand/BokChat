#define COMMAND_H
typedef struct irc_argument{
  char param[MAX_BUFFER];
  char trailing[MAX_BUFFER];
}irc_argument;
void send_message(int error_num, int sockfd, char* cmd, irc_argument* irc_args);
