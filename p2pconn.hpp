#ifndef P2P_CONN_H
#define P2P_CONN_H


#include <iostream> 
#include <stdlib.h>
#include <stdio.h> 
#include <cstdlib> // for exit() and EXIT_FAILURE
#include <sys/types.h>
#include <memory>  // for std::unique_ptr<T> var_name(new T)
#include <thread>
#include <algorithm>
#include<cstring>
#include<signal.h>

#include<math.h>
//#include<cmath>
#include<limits>

#include <arpa/inet.h>  
#include <sys/socket.h> // for socket functions
#include <netinet/in.h> // for sockaddr_in 
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>
 
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



  // For more info visit https://www.iana.org/assignments/media-types/media-types.xhtml
enum PacketType: uint8_t{
  text_plain = 1,
  text_csv = 2,
  text_xml = 3,
  application_json = 4,
  application_xml = 5,
  application_yamal = 6,
  application_pdf = 7,
  image_jpeg = 8,
  image_png = 9,
  image_gif = 10,
  image_tiff = 11,
  
  audio_mpeg = 13,
  audio_mp4 = 14,
  video_mp4 = 15,
  video_mpeg = 16,
 
};

enum PacketEncoding: uint8_t{
  base64 = 1,
  elwin_enc = 2,
};

enum PacketCompression: uint8_t{
  gzip = 1,
  lzma = 2,
  _7zip = 3,
};

enum PacketEncryption: uint8_t{
  
  LAST
};

enum PacketCharSet{
  iso8859_1 = 1,
  utf8= 2,
  ascii = 3,

  
};




class Packet{
private:
  
  PacketType type;
  PacketCharSet charset;
  PacketEncoding encoding;
  PacketEncryption encryption; 
  PacketCompression compression;

  string attachment_file_path;
  bool has_file_attachment;

  
  
  
  
  shared_ptr<uint8_t> built_pkt;
  shared_ptr<uint8_t> header;
  shared_ptr<uint8_t> payload;
  
  int pkt_len;
  uint16_t hdr_len;
  uint payload_len;
  

  shared_ptr<uint8_t> buffer;
  shared_ptr<int> curr_buff_idx;
  int buff_total_len;
  int buff_local_len;
  int buff_pos_start;
  //char *buff_start;
  uint8_t *buff_next;
  //int buff_pos_end;
  int num_bytes_read;

  bool pkt_built;
  bool pkt_rebuilding;

  

  bool keepalive;

public:
  Packet(uint8_t *payload, PacketType ct, PacketCharSet cs, PacketEncoding enc, PacketCompression compr, PacketEncryption encr, string attachment_file_path = "");
  Packet(shared_ptr<uint8_t> buffer, shared_ptr<int> curr_buff_idx, int buff_total_len);
  Packet(void *buffer);
  //Packet() {}
  ~Packet();
  uint8_t* build();
  void build_header();
  void rebuild_header();
  void rebuild();

  bool is_build_done() {return this->pkt_built; }
  bool is_build_pending() { return this->pkt_rebuilding; }

  void set_build_done(bool is_build_done) { this->pkt_built = is_build_done; this->pkt_rebuilding = !is_build_done; }
  void set_build_pending(bool is_pending) { this->pkt_rebuilding = is_pending; this->pkt_built = !is_pending; }

  void update_buff_idx();
  void update_local_buff_info();

  //void* get_buff_start(){ return (uint8_t*) this->buffer.get() + (uint8_t*) this->buff_pos_start; }

  uint8_t* get_payload() {  return this->payload.get();   }
  uint8_t* get_packet()  {  return this->built_pkt.get(); }

  int get_pkt_size(){ return this->pkt_len; }

  

};


class SocketHelper {

public:
  SocketHelper() {}
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
  struct sockaddr* build_sockaddr_struct(string host, uint16_t port, int sa_family);
  struct addrinfo* build_addrinfo_struct(string host, uint16_t port, int addr_family, int socktype, int flags);
  void get_all_ip_from_addrinfo();
  void set_sock_options();
  void set_blocking(bool blocking);
  bool close_sock();
  int get_sockfd() {return this->sock;}
  bool get_sock_open() {return this->is_sock_open;}

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
  void send_data(Packet pkt);
  void recv_data();
  void clear_buff();
  bool close();

  void set_client(){ this->client_conn = true; this->server_conn = false; }
  void set_server(){ this->server_conn = true; this->client_conn = false; }
  bool is_client(){ return this->client_conn; }
  bool is_server(){ return this->server_conn; }

  void set_keepalive(bool ka) {this->keepalive = ka; }

  // stop reciving data, sending data, or both, but dont disconnect
  void set_mute_flags(bool mute_send=true, bool mute_recv=true) {this->pause_send = mute_send; this->pause_recv = mute_recv; }

  string get_host() { return this->host; }

  shared_ptr<SocketHelper> get_sock_helper(){ return this->sock_helper; }

  bool conn_active() { return this->sock_helper->get_sock_open(); }

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
  vector<Packet*> completed_pkts;
  Packet *current_pkt;
  int payload_size;

  bool keepalive = true;
  bool bloking = false;
  bool pause_send = false;
  bool pause_recv = false;
  bool client_conn = false;
  bool server_conn = false;
};










class LocalPeer{

public:
  LocalPeer() {}
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
    int max_inbound_connections;
    int max_outbound_connections;
    vector<shared_ptr<PeerConnHandler>> inbound_connections;  //we are the server to those connections, serve them!
    vector<shared_ptr<PeerConnHandler>> outbound_connections; // We are the client to those connections, ask them to serve us stuff
    vector<struct pollfd> pollfd_list;
};



#endif








