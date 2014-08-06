#include "bokchat_getopt.h"
#include <string.h>
#include "main.h"
#include "config.h"

void get_server_opt(cmd_arg_opt* cmd_opt,int* argc, char** argv[]){

  /*initialize cmd_arg_opt struct*/
  cmd_opt->verbose = false;
  cmd_opt->daemonize = DAEMONIZE;
  cmd_opt->host = NULL;
  cmd_opt->irc_port = 0;
  cmd_opt->ssl_irc_port = 0;
  cmd_opt->help = 0;

  int c;
  while((c = getopt(*argc, *argv, "dvhi:j:")) != -1){
    switch(c)
      {
      case 'd': /*daemon*/
	cmd_opt->daemonize = true;
	break;
      case 'v': /*verbose*/
	cmd_opt->verbose = true;
	break;
      case 'i': /*irc port*/
	cmd_opt->irc_port = atoi(optarg);
	break;
      case 'j': /*ssl irc port*/
	cmd_opt->ssl_irc_port = atoi(optarg);
	break;
      case 'h': /*help*/
	cmd_opt->help = true;
	break;
      case '?':
	if (optopt == 'i'){
	  fprintf(stderr, "Option -%c requires an argument.\n", optopt);
	}
	else if(isprint (optopt)){
	  fprintf(stderr, "Unknown option '-%c'.\n", optopt);
	}
	else{
	  fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
	}
	/* no break */
      default:
	fprintf(stderr, "Error occurred while parsing cmd args. Quitting...\n");
	exit(1);
      }
    }


}


