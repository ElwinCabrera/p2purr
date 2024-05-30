#ifndef SOCKET_HELPER_HPP
#define SOCKET_HELPER_HPP

#include <iostream> 
//#include <stdlib.h>
//#include <stdio.h> 
//#include <sys/types.h>
//#include <cstring>

#include <arpa/inet.h>  
#include <sys/socket.h> // for socket functions
#include <netinet/in.h> // for sockaddr_in 
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>


#ifdef _WIN32  //_WIN32_WINNT
//windows only headers
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
//in Windows, need to link with the ws2_32 Winsock library
//Some compliers let us do this with the pragma below, if not we will need to link via command line argument
#pragma comment(lib, "ws2_32.lib") //
#endif

#include <vector>
#include <string>
#include <tuple>

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::tuple;
using std::min;
using std::to_string;



#include "global_config.h"
#include "exceptionhandler.h"

//template <typename T>
class SocketHelper {

public:
  SocketHelper();
  SocketHelper(string host, uint16_t port, sock_t sock = -1);
  ~SocketHelper();
  void init_server(int backlog);
  tuple<string, int, sock_t> server_accept_conns();
  tuple<string, int> handle_addr_fam_and_get_ip_port(sa_family_t sa_family, struct sockaddr *sa);
  void connect_to_host();
  bool send_data(string msg);
  bool send_data2(const uint8_t *data, int data_len);
  string receive_fixed_len(int msg_len);
  char* receive_all();
  int receive(uint8_t *buffer, int len);
  //probably not going to use this often here just in case
  //struct sockaddr* build_sockaddr_struct(string host, uint16_t port, int sa_family);
  struct addrinfo* build_addrinfo_struct(string host, uint16_t port, int addr_family, int socktype, int flags);
  void get_all_ip_from_addrinfo();
  void set_sock_options();
  void set_blocking(bool blocking);
  bool close_sock();
  int get_sockfd();
  bool get_sock_open();

protected:
  sock_t sock;
  //T sock;
  //SOCKET winsock = INVALID_SOCKET;
  string host;
  uint16_t port;
  bool is_sock_open = false;
  int max_recv_bytes_at_once;
  struct addrinfo *ai = NULL;
  vector<string> ipv4_addrs;
  vector<string> ipv6_addrs;
};



#endif // SOCKET_HELPER_HPP