#ifndef CLASS_HEADER_H
#define CLASS_HEADER_H


#include <iostream> 
#include <stdlib.h>
#include <stdio.h> 
#include <cstdlib> // for exit() and EXIT_FAILURE
#include <sys/types.h>
#include <memory>  // for std::unique_ptr<T> var_name(new T)
//#include <thread>
#include <algorithm>
#include<cstring>

#include<math.h>
//#include<cmath>
#include<limits>

#include <arpa/inet.h>  
#include <sys/socket.h> // for socket functions
#include <netinet/in.h> // for sockaddr_in 
#include <netdb.h>
 
#include <unistd.h> // Read

#include <vector>
#include <string>
#include <tuple>
// using std::cout;
// using std::endl;
// using std::string;
// using std::thread;

// Library effective with Windows 
//#include <windows.h>
 
// Library effective with Linux
#include <unistd.h>

using namespace std;


class SocketHelper {

public:
  SocketHelper() {}
  SocketHelper(string host, uint16_t port, int sock=-1);
  ~SocketHelper();
  void init_server(int backlog, float timeout);
  tuple<string, int, int> server_accept_conns();
  void connect_to_host();
  bool send_data(string msg);
  string receive_data(int msg_len);
  //probably not going to use this often here just in case
  struct sockaddr* build_sockaddr_struct(string host, uint16_t port, int sa_family);
  struct addrinfo* build_addrinfo_struct(string host, uint16_t port, int addr_family, int socktype, int flags);
  void get_all_ip_from_addrinfo();
  bool close_sock();

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












class PeerConnHandler {

public:
  PeerConnHandler() {}
  PeerConnHandler(string host, uint16_t port, int sock);
  ~PeerConnHandler();
  void connect(); //only called if we are the client and they are the server
  void send_data(string data);
  string recv_data();
  void communicate();
  bool close();

  void set_client(){ this->client_conn = true; this->server_conn = false; }
  void set_server(){ this->server_conn = true; this->client_conn = false; }
  bool is_client(){ return this->client_conn; }
  bool is_server(){ return this->server_conn; }

  void set_keepalive(bool ka) {this->keepalive = ka; }

  // stop reciving data, sending data, or both, but dont disconnect
  void set_mute_flags(bool mute_send=true, bool mute_recv=true) {this->pause_send = mute_send; this->pause_recv = mute_recv; }

  string get_host() { return this->host; }


private:
  string host;
  uint16_t port;
  int sock = -1;
  SocketHelper sock_helper;
  bool conn_is_active = false;
  bool keepalive;

  bool pause_send;
  bool pause_recv;
  bool client_conn;
  bool server_conn;
};










class LocalPeer{

public:
  LocalPeer() {}
  LocalPeer(string host, uint16_t port);
  ~LocalPeer();
  void connect_to_peer(string host, uint16_t port);
  void send_data_to_peer(string host, string data);
  void handle_client(string ipaddr, int port, int client_sock);
  void close_all_connections();
  void stop_server();
  void start_server();

  //shared_ptr<SocketHelper> get_sock_helper() { return this->server_sock_helper; }

protected:
    string host;
    uint16_t port = -1;
    shared_ptr<SocketHelper> server_sock_helper;
    bool is_local_server_running = false;
    int max_inbound_connections;
    int max_outbound_connections;
    vector<PeerConnHandler> inbound_connections;  //we are the server to those connections, serve them!
    vector<PeerConnHandler> outbound_connections; // We are the client to those connections, ask them to serve us stuff
};






#endif