#ifndef P2PURR_CHAT_H 
#define P2PURR_CHAT_H 


#include <iostream> 
#include <stdlib.h>
#include <stdio.h> 
#include <cstdlib> // for exit() and EXIT_FAILURE
#include <sys/types.h>
#include <memory>  // for std::unique_ptr<T> var_name(new T)
#include <thread>
#include <algorithm>
#include <cstring>
#include <signal.h>

#include <math.h>
//#include<cmath>
#include <limits>

#include <arpa/inet.h>  
#include <sys/socket.h> // for socket functions
#include <netinet/in.h> // for sockaddr_in 
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

// Library effective with Windows 
//#include <windows.h>

#include <exception>
#include <cerrno>

#include <vector>
#include <string>
#include <tuple>
// using std::cout;
// using std::endl;
// using std::string;
// using std::thread;


#ifdef linux
//linux only headers
#endif

#ifdef _WIN32  //_WIN32_WINNT
//windows only headers
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
//in Windows, need to link with the ws2_32 Winsock library
//Some compliers let us do this with the pragma below, if not we will need to link via command line argument
#pragma comment(lib, "ws2_32.lib") //
#endif


using namespace std;

// #include "exceptionhandler.hpp"
// #include "packet.hpp"
// #include "sockethelper.hpp"
// #include "peerconnhandler.hpp"
// #include "localpeer.hpp"



//void sig_handler(int s);
//void init_signal_handler();
void init_main_server();

//expose this when we build our library, but not yet else the linker will throw an error
//extern bool on_client_connection_request(string addr, uint16_t port);

#endif  //P2PURR_CHAT_H 








