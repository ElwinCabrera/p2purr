#include "../include/p2purr_chat.h"
#include "../include/exceptionhandler.h"
#include "../include/packet.h"
// #include "sockethelper.h"
// #include "peerconnhandler.h"
#include "../include/localpeer.h"



//TODO: change NULL to nullptr

unique_ptr<LocalPeer> local_server;
thread server_thread;


void sig_handler(int s){
  printf("\nCaught signal(%d) SIGINT. Exiting gracefully\n", s);
  local_server->stop_server();
  server_thread.join();
  exit(1); 

}

void init_signal_handler(){
  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = sig_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);
}


void init_main_server(){
  local_server = unique_ptr<LocalPeer>(new LocalPeer("0.0.0.0", 10011));
  local_server->start_server();
}


int main(int argc, char *argv[]) {

  

  // unsigned int n = std::thread::hardware_concurrency();
  // std::cout << n << " concurrent threads are supported.\n";

  try{
   
    server_thread = thread(init_main_server);
    sleep(1);


    printf("Press CTRL-C to exit\n");

    server_thread.join();

  } catch(GenericException &e){
    cout << e.what();
  } catch(std::exception& e) {
    //Other errors
    cout << "System error occured errno: " << errno << " -> " << std::strerror(errno) << endl;
    cout << e.what() << endl;
    exit(EXIT_FAILURE);
  } catch (...) {
    cout << "An error occured but, dont know why. You are on your own" << endl;
    exit(EXIT_FAILURE);
  }

 

  pause();


  return 0;
}
