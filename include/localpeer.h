#ifndef LOCAL_PEER_H
#define LOCAL_PEER_H


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

#include "exceptionhandler.h"
#include "packet.h"
#include "sockethelper.h"
#include "peerconnhandler.h"


class LocalPeer{

public:
  LocalPeer();
  LocalPeer(string host, uint16_t port);
  ~LocalPeer();
  void connect_to_peer(string host, uint16_t port);
  void send_data_to_peer(string host, Packet pkt);
  void check_for_msgs();
  string get_header();
  void handle_client(string ipaddr, int port, int client_sock);
  shared_ptr<PeerConnHandler> get_peer_from_sockfd(int sockfd);
  void add_pollfd_to_poll_list(int sockfd, short events);
  void remove_disconnected_peers();
  void close_all_connections();
  bool stop_server();
  void start_server();

  //shared_ptr<SocketHelper> get_sock_helper() { return this->server_sock_helper; }

protected:
    string host;
    uint16_t port = -1;
    shared_ptr<SocketHelper> server_sock_helper;
    bool is_local_server_running = false;
    size_t max_inbound_connections;
    size_t max_outbound_connections;
    vector<shared_ptr<PeerConnHandler>> inbound_connections;  //we are the server to those connections, serve them!
    vector<shared_ptr<PeerConnHandler>> outbound_connections; // We are the client to those connections, ask them to serve us stuff
    vector<struct pollfd> pollfd_list;
};

#endif // LOCAL_PEER_H