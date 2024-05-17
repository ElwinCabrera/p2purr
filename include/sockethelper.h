#ifndef SOCKET_HELPER_HPP
#define SOCKET_HELPER_HPP

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
using namespace std;


#include "global_config.h"

//template <typename T>
class SocketHelper {

public:
  SocketHelper();
  SocketHelper(string host, uint16_t port, sock_t sock= -1);
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