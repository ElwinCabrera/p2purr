#ifndef SOCKET_HELPER_HPP
#define SOCKET_HELPER_HPP

#include "p2purr_chat.h"



class SocketHelper {

public:
  SocketHelper();
  SocketHelper(string host, uint16_t port, int sock=-1);
  ~SocketHelper();
  void init_server(int backlog);
  tuple<string, int, int> server_accept_conns();
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
  int sock = -1;
  string host;
  uint16_t port;
  bool is_sock_open = false;
  int max_recv_bytes_at_once;
  struct addrinfo *ai = NULL;
  vector<string> ipv4_addrs;
  vector<string> ipv6_addrs;
};



#endif // SOCKET_HELPER_HPP