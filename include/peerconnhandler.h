#ifndef PEER_CONN_HANDLER_HPP
#define PEER_CONN_HANDLER_HPP

#include <iostream> 
#include <memory> 
#include <cstdint>
#include <stdlib.h>
#include <stdio.h> 
//#include <sys/types.h>


#include <cstring>
#include <string>

#include <vector>

// using std::cout;
// using std::endl;
using std::string;
using std::vector;


#include "global_config.h"

#include "exceptionhandler.h"
#include "packet.h"
#include "sockethelper.h"



class PeerConnHandler {

public:
  PeerConnHandler();
  PeerConnHandler(string host, uint16_t port, sock_t sock = -1);
  ~PeerConnHandler();
  void connect(); //only called if we are the client and they are the server
  void send_pkt(Packet pkt);
  shared_ptr<Packet> recv_pkt();
  void clear_buff();
  bool close();

  void set_client();
  void set_server();
  bool is_client();
  bool is_server();

  void set_keepalive(bool ka);

  // stop receiving data, sending data, or both, but dont disconnect
  void set_mute_flags(bool mute_send=true, bool mute_recv=true);

  string get_host() ;
  uint16_t get_port() {return this->port;}

  shared_ptr<SocketHelper> get_sock_helper();

  bool conn_active();

private:
  string host;
  uint16_t port;
  //SocketHelper sock_helper;
  shared_ptr<SocketHelper> sock_helper;
  //unique_ptr<SocketHelper> sock_helper;
  shared_ptr<uint8_t> recv_buffer;
  shared_ptr<int> curr_buff_idx;
  int buff_size;
  //vector<uint8_t> recv_buffer;
  vector<shared_ptr<Packet>> completed_pkts;
  shared_ptr<Packet> current_pkt;
  int payload_size;

  bool keepalive = true;
  bool bloking = false;
  bool pause_send = false;
  bool pause_recv = false;
  bool client_conn = false;
  bool server_conn = false;
};



#endif // PEER_CONN_HANDLER_HPP