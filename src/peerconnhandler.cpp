#include "../include/peerconnhandler.h"




PeerConnHandler::PeerConnHandler() {}

PeerConnHandler::PeerConnHandler(string host, uint16_t port, sock_t sock): host(host), port(port){
  
  //this->host = host
  //this->port= port
#ifdef _WIN32
  if(sock == INVALID_SOCKET) this->set_server();
  else this->set_client();
#else
  if(sock == -1) this->set_server();
  else this->set_client();
#endif

  this->sock_helper = shared_ptr<SocketHelper>(new SocketHelper(host, port, sock));
  //this->sock_helper = unique_ptr<SocketHelper>(new SocketHelper(host, port, sock));
  this->sock_helper->set_sock_options();
  this->sock_helper->set_blocking(false);

  //create circular buffer
  //this->payload_size = 1500;
  //this->recv_buffer; //this->max_payload_size * 2;
  //this->recv_buffer.reserve(this->max_payload_size * 2);
  //this->processing_data = false;
  this->buff_size = 1500;
  this->recv_buffer = shared_ptr<uint8_t>((uint8_t*) malloc(this->buff_size));
  this->curr_buff_idx = shared_ptr<int>((int*) malloc(sizeof(int)));
  this->clear_buff();

}

void PeerConnHandler::clear_buff(){
  memset(this->recv_buffer.get(), '\0', this->buff_size);
  int *pos = this->curr_buff_idx.get();
  *pos = 0;
}

void PeerConnHandler::connect(){
  try{
    this->sock_helper->connect_to_host();
  } catch (GenericException &e){
    cout << "PeerConnHandler::connect_to_host(..)" << endl;
    throw;
  }
  
  this->client_conn = true;
  
}

PeerConnHandler::~PeerConnHandler(){
  //printf("~PeerConnHandler()\n");
}


void PeerConnHandler::send_pkt(Packet pkt){
  try{
    this->sock_helper->send_data(pkt.get_packet(), pkt.get_pkt_len());
    
  } catch(GenericException &e){
    cout << e.what();
  }
}

shared_ptr<Packet> PeerConnHandler::recv_pkt(){
  
  try{
    //msg = this->sock_helper->receive_all();
    uint8_t *buf = this->recv_buffer.get();
    int *pos = this->curr_buff_idx.get();
    int recv_len = this->sock_helper->receive(buf + *pos, this->buff_size - *pos);
    //do processing here
    if(recv_len > 0 && *pos < recv_len){
      while(*pos < recv_len){
        if(this->current_pkt == nullptr){
          this->current_pkt = shared_ptr<Packet>(new Packet(this->recv_buffer, this->curr_buff_idx, this->buff_size));
        }
        this->current_pkt->rebuild();
        if(this->current_pkt->is_build_done()){
          this->completed_pkts.push_back(current_pkt); //store all packets in an array until i figure out what to do with them
          //on_packet_received(current_pkt);
          current_pkt = nullptr;
        }

      }
        if(*pos >= recv_len ) this->clear_buff();
    }

    if(!this->completed_pkts.empty()){
      shared_ptr<Packet> pkt = this->completed_pkts.at(this->completed_pkts.size()-1);
      return pkt;
      //printf("Latest packet received %d(+header) bytes from %s:%d -> '%s'\n", pkt->get_pkt_len(), this->host.c_str(), this->port, (char*) pkt->get_payload());
    }
   
  } catch (GenericException &e){
    cout << e.what();
  }

  return nullptr;
}


bool PeerConnHandler::close() {
  return this->sock_helper->close_sock();
}



 void PeerConnHandler::set_client(){ 
  this->client_conn = true; 
  this->server_conn = false; 
}
void PeerConnHandler::set_server(){ 
  this->server_conn = true; 
  this->client_conn = false; 
}

bool PeerConnHandler::is_client(){ 
  return this->client_conn; 
}

bool PeerConnHandler::is_server(){ 
  return this->server_conn; 
}

void PeerConnHandler::set_keepalive(bool ka) {
  this->keepalive = ka; 
}

  // stop receiving data, sending data, or both, but dont disconnect
void PeerConnHandler::set_mute_flags(bool mute_send, bool mute_recv) {
  this->pause_send = mute_send; 
  this->pause_recv = mute_recv; 
}

string PeerConnHandler::get_host() { 
  return this->host; 
}

shared_ptr<SocketHelper> PeerConnHandler::get_sock_helper(){ 
  return this->sock_helper; 
}

bool PeerConnHandler::conn_active() { 
  return this->sock_helper->get_sock_open(); 
}
