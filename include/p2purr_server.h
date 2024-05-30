#ifndef SERVER_H
#define SERVER_H


#include <iostream> 
#include <stdlib.h>
#include <stdio.h> 
#include <vector>
#include <string>
#include <tuple>
#include <unordered_map>
#include <thread>

#include <memory>
//#include <select>
#ifdef linux
#include <poll.h>
#endif  

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

//#include <cstdlib> // for exit() and EXIT_FAILURE
//#include <sys/types.h>
//#include <thread>
//#include <algorithm>
//#include <cstring>



using std::tuple;
using std::vector;
using std::string;
using std::shared_ptr;
using std::begin;
using std::end;
using std::to_string;
using std::unordered_map;
using std::thread;



#include "global_config.h"

#include "exceptionhandler.h"
#include "packet.h"
#include "sockethelper.h"
#include "peerconnhandler.h"


//expose this when we build our library, but not yet else the linker will throw an error
//extern bool on_client_connection_request(string addr, uint16_t port);


class P2PurrServer{

public:
  P2PurrServer();
  P2PurrServer(string host, uint16_t port);
  ~P2PurrServer();
  void connect_to_peer(string host, uint16_t port);
  void send_data_to_peer(string host, Packet pkt);
  void check_for_msgs();
  void handle_new_client();
  void handle_client_communications(sock_t fd);
  void add_fd_to_watch_list(sock_t sockfd, short events);
  void remove_disconnected_peers();
  void close_all_connections();
  void init_windows();
  void deinit_windows();
  void start_async_select();
  void start_async_poll();
  bool stop();
  void start();

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
    unordered_map<sock_t, shared_ptr<PeerConnHandler>> sock_to_peer_map;
    thread server_thread;
#ifdef linux
    vector<struct pollfd> pollfd_list;
#endif
};

#endif // SERVER_H