#include  <stdlib.h>
#include <signal.h>
#include "include/p2purr_server.h"


#define SERVER_ADDRESS "0.0.0.0"
#define SERVER_PORT 10011

P2PurrServer server(SERVER_ADDRESS, SERVER_PORT);

void sig_handler(int s){
  printf("\nCaught signal(%d) SIGINT. Exiting gracefully\n", s);
  server.stop();
  exit(1); 

}


 void init_signal_handler(){
  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = sig_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);
}


int main(int argc, char *argv[]) {

  
  printf("Press CTRL-C to exit\n");
  

  init_signal_handler();
  server.start();
  

  
  pause();
  return 0;
}
