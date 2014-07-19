#include <time.h>
#ifndef IRC_GETOPT_H
#include "irc_getopt.h"
#endif
#include <unistd.h>
#ifndef MAIN_H
#include "main.h"
#endif 
#ifndef CONFIG_H
#include "config.h"
#endif
#ifndef CONNECT_H
#include "connect.h"
#endif
#include <errno.h>










int main(int argc, char *argv[]){

  if(geteuid() == 0){
    fprintf(stderr, "You are running this program as root, halting \n");
    return -1;
  }
  if(argc < 2){
    printf(
	   "Usage: %s [-h host] -p port [-v] [-d]\n"
	   "-v     :  This option enables verbose mode [Default: false]\n"
	   "-d     :  This option tells the server to daemonize [Default: %s]\n"
	   "-h host:  Tells the server which host to listen\n"
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
  if((sockfd = listen_bind_on_port(cmd_opt.port)) == -1){
    fprintf(stderr, "Either binding or listening has failed\n%s\nQuitting...\n", strerror(errno));
    exit(1);
  }
  printf("Starting server...\n");
  start_server(sockfd);
  return 0;
}
