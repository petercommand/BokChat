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
	   "-j port:  Specify the port for the irc server with ssl to listen\n"
	   , argv[0], DAEMONIZE?"true":"false");
    cmd_opt.help?exit(0):exit(1);
  }
  if((cmd_opt.irc_port == 0) && (cmd_opt.ssl_irc_port == 0)){
    fprintf(stderr, "No server option specified.\nQuitting...\n");
    exit(1);
  }
  int irc_sockfd = -1;
  /*options: verbose daemonize host port*/
  if(cmd_opt.irc_port != 0){

    irc_sockfd = irc_listen_bind_on_port(cmd_opt.irc_port);
    if(irc_sockfd == -1){
      fprintf(stderr, "Either binding or listening for irc has failed\n%s\nQuitting...\n", strerror(errno));
      exit(1);
     }
  }
  int ssl_irc_sockfd = -1;
  if(cmd_opt.ssl_irc_port != 0){/*using ssl here*/
    /*use ssl*/
    ssl_irc_sockfd = irc_listen_bind_on_port(cmd_opt.ssl_irc_port);
    if(ssl_irc_sockfd == -1){
      fprintf(stderr, "Either binding or listening for ssl irc has failed\n%s\nQuitting...\n", strerror(errno));
      exit(1);
    }
  }
  printf("Starting server...\n");
  start_irc_server(irc_sockfd, ssl_irc_sockfd);
  return 0;
}
