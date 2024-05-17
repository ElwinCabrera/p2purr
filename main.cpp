#include<stdlib.h>
#include "include/p2purr_chat.h"


P2PurrHost host;

void sig_handler(int s){
  printf("\nCaught signal(%d) SIGINT. Exiting gracefully\n", s);
  host.stop();
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
  host.start();
  

  
  pause();
  return 0;
}
