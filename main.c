#include <time.h>
#include "bokchat_getopt.h"
#include <unistd.h>
#include "main.h"
#include "config.h"
#include "connect.h"
#include <errno.h>



int main(int argc, char *argv[]){

  if(geteuid() == 0){
    fprintf(stderr, "You are running this program as root, halting \n");
    return -1;
  }

  cmd_arg_opt cmd_opt;
  get_server_opt(&cmd_opt, &argc, &argv);
  if((argc < 2) || (cmd_opt.help == true)){
    printf(
	   "Usage: %s -i irc_port [-v] [-d]\n"
	   "-h     :  This option show this program usage\n"
	   "-v     :  This option enables verbose mode [Default: false]\n"
	   "-d     :  This option tells the server to daemonize [Default: %s]\n"
	   "-i port:  Specify the port for the irc server to listen\n"
	   , argv[0], DAEMONIZE?"true":"false");
    cmd_opt.help?exit(0):exit(1);
  }
  /*options: verbose daemonize host port*/
  if(cmd_opt.irc_port == 0){
    fprintf(stderr, "No port specified. Use -i to specify a port for the irc server to listen\n");
    exit(1);
  }
  int sockfd;
  if((sockfd = irc_listen_bind_on_port(cmd_opt.irc_port)) == -1){
    fprintf(stderr, "Either binding or listening has failed\n%s\nQuitting...\n", strerror(errno));
    exit(1);
  }

  printf("Starting server...\n");
  start_server(sockfd);
  return 0;
}
