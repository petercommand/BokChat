#include <time.h>
#include "irc_getopt.h"
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
  if(argc < 2){
    printf(
	   "Usage: %s -p port [-v] [-d]\n"
	   "-v     :  This option enables verbose mode [Default: false]\n"
	   "-d     :  This option tells the server to daemonize [Default: %s]\n"
	   "-p port:  Specify the port for the server to listen\n"
	   , argv[0], DAEMONIZE?"true":"false");
    exit(1);
  }
  cmd_arg_opt cmd_opt;
  get_server_opt(&cmd_opt, &argc, &argv);
  
  /*options: verbose daemonize host port*/
  if(cmd_opt.port == 0){
    fprintf(stderr, "No port specified. Use -p to specify a port for the server to listen\n");
    exit(1);
  }
  int sockfd;
  if((sockfd = irc_listen_bind_on_port(cmd_opt.port)) == -1){
    fprintf(stderr, "Either binding or listening has failed\n%s\nQuitting...\n", strerror(errno));
    exit(1);
  }

  printf("Starting server...\n");
  start_server(sockfd);
  return 0;
}
