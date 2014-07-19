#define COMMAND_H
typedef struct irc_argument{
  char param[MAX_BUFFER];
  char trailing[MAX_BUFFER];
}irc_argument;
void send_message(int error_num, user_info* user_inf, char* cmd, irc_argument* irc_args);
