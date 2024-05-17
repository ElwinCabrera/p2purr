#include "../include/p2purr_chat.h"
#include "../include/exceptionhandler.h"
#include "../include/packet.h"
// #include "sockethelper.h"
// #include "peerconnhandler.h"
//#include "../include/localpeer.h"



void P2PurrHost::init_main_server(){
  this->local_server = unique_ptr<LocalPeer>(new LocalPeer("0.0.0.0", 10011));
  this->local_server->start_server();
}


void P2PurrHost::start(){
   // unsigned int n = std::thread::hardware_concurrency();
  // std::cout << n << " concurrent threads are supported.\n";

  try{
#ifdef _WIN32
  this->init_windows();
#endif
    
    this->server_thread = thread(&P2PurrHost::init_main_server, this);
    sleep(1);
    this->running = true;
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

}


void P2PurrHost::stop(){
  printf("Stopping Server\n");
  this->local_server->stop_server();
  this->server_thread.join();
  this->running = false;
#ifdef _WIN32
  this->deinit_windows();
#endif
}



P2PurrHost::~P2PurrHost(){
  if(this->running) this->stop();
}



#ifdef _WIN32
void P2PurrHost::init_windows(){
  WSADATA wsaData;
  // Initialize Winsock
  int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed: %d\n", iResult);
    return 1;
  }

}

void P2PurrHost::deinit_windows(){
  WSACleanup();
}

#endif

