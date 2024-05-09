#include "p2pconn.hpp"
#include "exceptionhandler.hpp"


using namespace std;

//TODO: change NULL to nullptr




Packet::Packet(uint8_t *payload, PacketType ct, PacketCharSet cs, PacketEncoding enc, PacketCompression compr, PacketEncryption encr, string attachment_file_path){
  if(payload == nullptr) throw PacketException("cant create packet. payload is null\n");

  this->type        = ct;
  this->charset     = cs;
  this->encoding    = enc;
  this->encryption  = encr; 
  this->compression = compr;


  
  this->payload_len = strlen((char*) payload); //TODO: will break
  this->payload = shared_ptr<uint8_t>((uint8_t*) malloc(this->payload_len + 1));
  this->payload.get()[this->payload_len] = '\0';
  memcpy(this->payload.get(), payload, this->payload_len);

  this->built_pkt = shared_ptr<uint8_t>(nullptr);
  this->header = shared_ptr<uint8_t>(nullptr);

  this->has_file_attachment = false;
  if(attachment_file_path.compare("")) { // and path is valid i.e file exists
    this->has_file_attachment = true;
  }
  this->keepalive = true;
}


Packet::Packet(shared_ptr<uint8_t> buffer, shared_ptr<int> curr_buff_idx, int buff_total_len){
  this->buffer = buffer;
  //this->rebuilding_in_progress = false;
  this->curr_buff_idx = curr_buff_idx;
  this->buff_total_len = buff_total_len;
  

  this->update_local_buff_info();

  this->pkt_built = false;
  this->pkt_rebuilding = false;
  this->has_file_attachment = false;


  this->payload   = shared_ptr<uint8_t>(nullptr);
  this->built_pkt = shared_ptr<uint8_t>(nullptr);
  this->header    = shared_ptr<uint8_t>(nullptr);

  this->pkt_len = 0;
  this->hdr_len = 0;
  this->payload_len = 0;
  this->num_bytes_read = 0;
  //this->keepalive = true;
  this->set_build_pending(false);

}

Packet::~Packet(){

}

uint8_t* Packet::build(){
  
  if(this->built_pkt.get() != nullptr) return this->built_pkt.get();

  this->set_build_pending(true);
  
  this->build_header();

  this->pkt_len = sizeof(char) + sizeof(uint16_t) + this->hdr_len + this->payload_len;
  this->built_pkt = shared_ptr<uint8_t> ((uint8_t*) malloc(this->pkt_len + 1));
  this->built_pkt.get()[this->pkt_len] = '\0';

  int header_len = htons(this->hdr_len); 

  uint8_t *pkt = this->built_pkt.get();
  *pkt++ = 0xFF;
  *pkt++ = (header_len >> 8) & 0xFF; // or *pkt++ = header_len & 0xFF00;
  *pkt++ = header_len & 0xFF; // or *pkt++ = header_len & 0x00FF;
  memcpy(pkt, this->header.get(), this->hdr_len);
  pkt += this->hdr_len;
  memcpy(pkt, this->payload.get(), this->payload_len);

  this->set_build_pending(false);
  //printf("pkt: %s\n", this->built_pkt.get());
   

  return this->built_pkt.get();

}

void Packet::build_header(){
  //header len will be num header properties(5) + payload_len(4)
  //uint payload_len_hton = hton(this->payload_len);
  uint payload_len_hton = this->payload_len;

  this->hdr_len = 9;
  this->header = shared_ptr<uint8_t>((uint8_t*) malloc( this->hdr_len + 1));
  this->header.get()[this->hdr_len] = '\0';

  uint8_t *h = (uint8_t*) this->header.get();
  *h++  = (uint8_t) this->type;
  *h++  = (uint8_t) this->charset;
  *h++  = (uint8_t) this->encoding;
  *h++  = (uint8_t) this->encryption;
  *h++  = (uint8_t) this->compression;

  int byte_idx = 3;
  while(byte_idx >= 0){
    *h++  = (payload_len_hton >> (8 * byte_idx--)) & 0xFF;
  }

  //printf("header: %s\n",(char*) this->header.get());

}



void Packet::rebuild_header(){
  

  uint8_t *hdr = this->header.get();

  char start_symbol = *hdr++;
  
  this->hdr_len = *hdr++;
  this->hdr_len = (this->hdr_len << 8) | *hdr++;
  this->hdr_len = ntohs(this->hdr_len) +3;

  if(this->hdr_len <= 0) throw PacketException("Got a header length of zero or less\n");
  //if(this->hdr_len > remaining_buff_len) throw PacketException("Header length is bigger than the remaining buffer\n");


  //printf("Rebuilding header of length %d\n", this->hdr_len);

  this->type = (PacketType) *hdr++;
  this->charset = (PacketCharSet) *hdr++;
  this->encoding = (PacketEncoding) *hdr++;
  this->encryption = (PacketEncryption) *hdr++;
  this->compression = (PacketCompression) *hdr++;
  
  int bytes = 4;
  int i = 0;
  while(i++ < bytes){
    this->payload_len = (this->payload_len << 8) | *hdr++;
  }

  //this->payload_len = ntoh(this->payload_len);
  if(this->payload_len <= 0) throw PacketException("Got a payload length of zero or less\n");

  if(this->payload.get() == nullptr){
    this->payload = shared_ptr<uint8_t>((uint8_t*) malloc(this->payload_len));
    memset(this->payload.get(), '\0', this->payload_len);
  }

  
  
  this->update_buff_idx();
  
}

void Packet::rebuild(){
  //if(this->buff_local_len < 2) throw PacketException("Received data is too small to process. can't even get header size\n");
  this->update_local_buff_info();
  int remaining_buff_len = this->buff_total_len - *(this->curr_buff_idx);
  
  bool build_pending = true;


  int idx = *(this->curr_buff_idx);

  if(*(this->buff_next) == 0xFF || this->num_bytes_read < this->hdr_len ){
    if(this->hdr_len == 0) this->hdr_len = 3;
    uint8_t *hdr_ptr = nullptr;
    uint8_t tmp_header[3];
    if(this->header.get() == nullptr){
      hdr_ptr = tmp_header;
    } else {
      hdr_ptr = this->header.get() + this->num_bytes_read;
    }


    while(idx < this->buff_total_len && (this->num_bytes_read < this->hdr_len)){
      *hdr_ptr++ = *(this->buff_next)++;
      
      if(this->num_bytes_read == 2){
        this->hdr_len = tmp_header[1];
        this->hdr_len = (this->hdr_len << 8) | tmp_header[2];
        this->hdr_len = ntohs(this->hdr_len) + 3;
        if(this->hdr_len <= 0) throw PacketException("Got a header length of zero or less\n");
        this->header = shared_ptr<uint8_t>((uint8_t*) malloc(this->hdr_len + 3));
        memcpy(this->header.get(), tmp_header, sizeof(tmp_header));
        hdr_ptr = this->header.get() + this->num_bytes_read + 1;
        
      }
      (this->num_bytes_read)++;
      
      idx++;
    }
  }

  

  if(idx >= this->buff_total_len || this->num_bytes_read < this->hdr_len){
    if(this->hdr_len <= 3) this->hdr_len = 0;
    this->update_buff_idx();
    this->set_build_pending(true);
    return;
  }

  //if((this->num_bytes_read < this->hdr_len) && (remaining_buff_len >= this->hdr_len - this->num_bytes_read)){
    this->rebuild_header();
  //}

  
  

  this->pkt_len = this->payload_len + this->hdr_len;

  int num_payload_bytes_read = this->num_bytes_read - this->hdr_len;
  uint8_t *pld =  this->payload.get() + num_payload_bytes_read;
  
  while((idx < this->buff_total_len) && (this->num_bytes_read < this->pkt_len)){
    *pld++ = *(this->buff_next)++;
    (this->num_bytes_read)++;
    idx++;
  }
  *(curr_buff_idx) = idx;

  int num_bytes_remaining = this->pkt_len - this->num_bytes_read;
  if (num_bytes_remaining > 0){
    build_pending = true;
  } else if(num_bytes_remaining == 0){

    build_pending = false;
  } else if(num_bytes_remaining < 0){
    throw PacketException("Something is wrong number of bytes remaining is negative\n");
  }


  //do rest of paket processing and rebuilding work here

  this->update_buff_idx();
  this->set_build_pending(build_pending);

}


void Packet::update_buff_idx(){
    int *curr_idx = this->curr_buff_idx.get();
    uint8_t *buff_head = this->buffer.get();
    *curr_idx = (this->buff_next - buff_head)  / sizeof(char);

    if(*curr_idx > this->buff_total_len){
      throw PacketException("Buffer index  is over the maximum buffer size of \n");
    }

}

void Packet::update_local_buff_info(){

  this->buff_pos_start = *curr_buff_idx;
  this->buff_next =  this->buffer.get() + *curr_buff_idx;
  //this->buff_local_len = strlen((char*) this->buff_next); 

}









SocketHelper::SocketHelper(string host, uint16_t port, int sock): host(host), port(port), sock(sock){
  //TODO: check if host is a domain name or ip address
  //if this->host is a domain name convert it to an ip address got from getaddrinfo
  try {

    if(sock == -1){
      this->sock = socket(PF_INET, SOCK_STREAM, 0);
    }
    if( this->sock == -1) throw SocketException("Failed to create socket\n");
    
    printf("Socket created\n");

    this->ai = this->build_addrinfo_struct(host, port, AF_UNSPEC, SOCK_STREAM, 0);
    // lastly iterate over addrinfo linked list and get all ip addresses found and store them in an array
    this->get_all_ip_from_addrinfo();

  } catch (GenericException &e){
    std::cout << e.what();
    exit(EXIT_FAILURE);
  } 
  
  //TODO: set a timeout on the socket
  //this->sock = sock;
  //this->host = host;
  //this->port = port;
  //this->conn_metrics = new ConnectionStats();
  this->is_sock_open = true;
  this->max_recv_bytes_at_once = 250;
  
    
}

SocketHelper::~SocketHelper(){
  printf("~SocketHelper\n");
  if(this->ai != NULL){
    printf("~SocketHelper: freeaddrinfo\n");
    freeaddrinfo(this->ai); // free linked list
    this->ai = NULL;
  }
  
  this->close_sock();
  
}

void SocketHelper::init_server(int backlog){
  try {

    printf("Server initializing  ... ");
    
    if(bind(this->sock, this->ai->ai_addr, this->ai->ai_addrlen) < 0 ) 
      throw BindException();
    
    printf(" binded this process to socket ... ");

    if(listen(this->sock, backlog) == -1) throw ListenException();
    
    printf(" listening on %s:%d\n",this->host.c_str(),this->port);

  } catch (GenericException &e) {
    std::cout << e.what();
    exit(EXIT_FAILURE);
  }

}


tuple<string, int, int> SocketHelper::server_accept_conns(){

  struct sockaddr_storage their_addr;
  socklen_t addr_size = sizeof(their_addr);
  string ipstr;
  int port ;

  int client_sock = accept(this->sock, (struct sockaddr*) &their_addr, &addr_size);
  if (client_sock == -1) throw AcceptException();
  
  tie(ipstr, port) = handle_addr_fam_and_get_ip_port(their_addr.ss_family, (struct sockaddr*) &their_addr);

  printf("Got a new connection from %s:%d\n", ipstr.c_str(), port);

  return {ipstr, port, client_sock};
  //get data by auto [var1, var2, var3] = fun(...) or tie(var1,var2,var3) = fun(...)
}

tuple<string, int> SocketHelper::handle_addr_fam_and_get_ip_port(sa_family_t sa_family, struct sockaddr *sa){

  char ipstr[INET6_ADDRSTRLEN];
  int port;

  if(sa_family == AF_INET){
      
    struct sockaddr_in *ipv4 = (struct sockaddr_in*) sa;  
    inet_ntop(AF_INET, &(ipv4->sin_addr), ipstr, INET_ADDRSTRLEN);
    port = ntohs(ipv4->sin_port);
    
  } else if (sa_family == AF_INET6){

    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*) sa;
    inet_ntop(AF_INET6, &(ipv6->sin6_addr), ipstr, INET6_ADDRSTRLEN);
    port = ntohs(ipv6->sin6_port);
    
  } else{
    printf("unhandled address family\n");
  }

  if(ipstr == NULL){
      throw GenericException("'inet_ntop()' was not succesful\n");
  }

  return {ipstr, port};

}

void SocketHelper::connect_to_host(){
  if(this->sock <= -1) throw SocketException("Cannot connect to host, no valid socket");

  this->set_blocking(true);
  if(connect(this->sock, this->ai->ai_addr, this->ai->ai_addrlen) == -1) 
    throw ConnectException();

  this->set_blocking(false);

  //printf("Connected to host %s:%d\n",this->host.c_str(),this->port);
}
  
bool SocketHelper::send_data(string msg){  //doesnt have to be a string can be anything sine send(...) accepts a 'void*'
  
  int msg_len = msg.size(); // or strlen(msg)
  int total_bytes_sent =0;
  printf("Sending %d bytes of data to %s:%d ... ", msg_len, this->host.c_str(), this->port);
  while(total_bytes_sent < msg_len){
    string msg_substr = msg.substr(total_bytes_sent);
    int num_bytes_sent = send(this->sock, msg_substr.c_str(), msg_substr.size(), 0);
    if(num_bytes_sent == -1) throw SendException();
    total_bytes_sent += num_bytes_sent;
  }
  printf("%d bytes sent succesfully\n", total_bytes_sent);
  return total_bytes_sent == msg_len;
}

bool SocketHelper::send_data2(const uint8_t *data, int data_len){
  int bytes_sent_so_far = 0;
  int bytes_left_to_send = data_len;
  printf("Sending %d bytes of data to %s:%d ... ", data_len, this->host.c_str(), this->port);
  while(bytes_sent_so_far < data_len){
    int bytes_sent = send(this->sock, data+bytes_sent_so_far, bytes_left_to_send, 0);
    if(bytes_sent == -1 ) throw SendException();
    bytes_sent_so_far += bytes_sent;
    bytes_left_to_send -= bytes_sent;
  }
  printf("%d bytes sent\n", bytes_sent_so_far);
  return bytes_sent_so_far == data_len;
}


char* SocketHelper::receive_all(){
  string msg;
  int total_bytes_recvd = 0;
  int bytes_recvd = -2;
  
  //could cause a buffer overflow if user tries to send us something EXTREAMLY long in one 'send(...)'
  //then this loop will loop and loop and our 'msg' buffer can grow out of control 
  //to the point of potentially crashing our program or even the system on which it runs on
  while(1){
    char chunk[this->max_recv_bytes_at_once];
    memset(chunk, '\0', sizeof(chunk));
    bytes_recvd = recv(this->sock, chunk, this->max_recv_bytes_at_once, 0);
    if(bytes_recvd <= 0) break;
    msg += chunk;
    total_bytes_recvd += bytes_recvd;
    if(bytes_recvd <= this->max_recv_bytes_at_once) break; 
  }
   if(bytes_recvd == 0){
    //lets close the connection and move on
    printf("Peer closed the connection\n");  
    this->close_sock();
  } else if (bytes_recvd == -1){
    throw ReceiveException("Something is wrong with recv");
  }
  
  //could cause a slow down since object is destroyed and copied again
  return msg.data();

}


string SocketHelper::receive_fixed_len(int msg_len){
  vector<string> chunks;
  int total_bytes_recvd = 0;
  //printf("Polling recv(%d bytes) from %s:%d\n", msg_len, this->host.c_str(), this->port);
  while(total_bytes_recvd < msg_len){
    int recv_len = min(msg_len - total_bytes_recvd, 4096);
    char chunk[recv_len];
    int bytes_recvd = recv(this->sock, chunk, recv_len, MSG_WAITALL);
    //if(bytes_recvd > 0 && bytes_recvd <= recv_len && msg_len > recv_len) msg_len = bytes_recvd; // avoid infinite loop where msg_len > total_bytes
    if(bytes_recvd == 0){
      
      printf("Peer closed the connection\n");
      this->close_sock();
    }
    if (bytes_recvd == -1) throw ReceiveException("Something is wrong with recv\n");

    chunks.push_back(chunk);
    total_bytes_recvd += bytes_recvd;
  }
  string msg_recvd = "";
  for(string s: chunks) msg_recvd += s;
  msg_recvd += '\0';
  printf("Received '%s' from %s:%d\n", msg_recvd.c_str(), this->host.c_str(), this->port);
  return msg_recvd;
}


int SocketHelper::receive(uint8_t *buffer, int max_msg_len) {

  
  int total_bytes_recvd = 0;
  //printf("Polling recv(%d bytes) from %s:%d\n", max_msg_len, this->host.c_str(), this->port);
  while(total_bytes_recvd < max_msg_len){
    int recv_len = min(max_msg_len - total_bytes_recvd, max_msg_len);
    int bytes_recvd = recv(this->sock, buffer + total_bytes_recvd, recv_len, 0);
    //if(bytes_recvd > 0 && bytes_recvd <= recv_len && max_msg_len > recv_len) max_msg_len = bytes_recvd; // avoid infinite loop where msg_len > total_bytes
    if(bytes_recvd > 0 && bytes_recvd <= max_msg_len) max_msg_len = bytes_recvd;

    if(bytes_recvd == 0){
      printf("Received 0, peer closed the connection\n");
      this->close_sock();
      break;
    }
    if (bytes_recvd == -1) throw ReceiveException("Something is wrong with recv\n");
    total_bytes_recvd += bytes_recvd;
    
  }
  
  // char *msg = (char*) malloc(total_bytes_recvd + 1);
  // msg[total_bytes_recvd] = '\0'; 
  // memcpy(msg, buffer, total_bytes_recvd);
  
  // printf("Received %d bytes from %s:%d -> '%s'\n", total_bytes_recvd, this->host.c_str(), this->port, msg);
  // free(msg);
  return total_bytes_recvd;
}

//probably not going to use this often here just in case
struct sockaddr* SocketHelper::build_sockaddr_struct(string host, uint16_t port, int sa_family){
  struct sockaddr *sa;
    
  if (sa_family == AF_INET){
    struct sockaddr_in sa_ipv4;
      
    sa_ipv4.sin_family = AF_INET;
    inet_pton(AF_INET, host.c_str(), &(sa_ipv4.sin_addr)); // populates sa_ipv4.sin_addr from a ip string
    sa_ipv4.sin_port = htons(port);
    memset(sa_ipv4.sin_zero, '\0', sizeof(sa_ipv4.sin_zero));

    sa = (struct sockaddr*) &sa_ipv4;

  } else if(sa_family == AF_INET6) {
    struct sockaddr_in6 sa_ipv6;
    sa_ipv6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, host.c_str(), &(sa_ipv6.sin6_addr)); // populates sa_ipv4.sin_addr from a ip string
    sa_ipv6.sin6_port = htons(port);
    //sa_ipv6.sin6_flowinfo = 0; // idk what this does TODO: figure it out
    //sa_ipv6.sin6_scope_id = 0; // idk what this does TODO: figure it out

    sa = (struct sockaddr*) &sa_ipv6;

  } else {
    printf("Addtress family not impemented yet");
  }

  return sa;
}

struct addrinfo* SocketHelper::build_addrinfo_struct(string host, uint16_t port, int addr_family, int socktype, int flags){
  
  
  string port_str = to_string(port);  
  struct addrinfo hints;
  struct addrinfo *ai_result;
  memset(&hints, 0, sizeof(hints)); //make sure the struct is empty

  hints.ai_family = addr_family;
  hints.ai_socktype = socktype;
  //hints.ai_flags = flags; // AI_PASSIVE
  hints.ai_flags = AI_PASSIVE;
    
    
  int get_ai_ret = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &ai_result);
  if(get_ai_ret != 0){
    throw GetAddrInfoException("", get_ai_ret);
  }

  return ai_result;
}

void SocketHelper::get_all_ip_from_addrinfo(){

  
  struct addrinfo *p;
  int count = 0;
  
  for(p = this->ai; p != NULL; p=p->ai_next){
    void *addr;
      
    if(p->ai_family == AF_INET){ //IPv4
      count += 1;
      struct sockaddr_in *ipv4 = (struct sockaddr_in*) p->ai_addr;
      addr = &(ipv4->sin_addr);
      char ipstr[INET_ADDRSTRLEN];
      inet_ntop(p->ai_family, addr, ipstr, INET_ADDRSTRLEN);
      if(ipstr == NULL) throw GenericException("'inet_ntop()' was not succesful\n");
      this->ipv4_addrs.push_back(ipstr);
      //printf("  IPv4: %s\n", ipstr);
      
    } else { //IPv6
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*) p->ai_addr;
      addr = &(ipv6->sin6_addr);
      char ipstr[INET6_ADDRSTRLEN];
      inet_ntop(p->ai_family, addr, ipstr, INET6_ADDRSTRLEN);
      if(ipstr == NULL) throw GenericException("'inet_ntop()' was not succesful\n");
      this->ipv6_addrs.push_back(ipstr);
      //printf("  IPv6: %s\n", ipstr);
    }

    //inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
    //printf("  %s: %s\n", ipver, ipstr);
  }
  if(count >1 ) printf("Other IP addresses found for %s:%d\n", host.c_str(), this->port);

}

void SocketHelper::set_sock_options(){
  int yes = 1;
  int no = 0;
  int sockopt_retcode = 0;
  sockopt_retcode += setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  sockopt_retcode += setsockopt(this->sock, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
  sockopt_retcode += setsockopt(this->sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
  struct linger lin;
  lin.l_onoff = 1; /* linger active */
  lin.l_linger = 10;/* how many seconds to linger for */
  sockopt_retcode += setsockopt(this->sock, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  sockopt_retcode += setsockopt(this->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  tv.tv_sec = 30;
  sockopt_retcode += setsockopt(this->sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));


  if(sockopt_retcode <= -1){
    throw SocketOptionsException();
    printf("setsockopt error\n");
  }

  printf("Socket options set\n");


  /*
  
  SO_TIMESTAMP
  SO_TIMESTAMPNS
  */

}

void SocketHelper::set_blocking(bool blocking){

  int flags = fcntl(this->sock, F_GETFL, 0);
  if (flags == -1) throw FCTRLException();
  flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);

  if(fcntl(this->sock, F_SETFL, flags) == -1) throw FCTRLException("Failed to set socket to blocking\n");

  //cout << "Socket blocking=" << blocking << endl;

} 

bool SocketHelper::close_sock(){
  
  if(!this->is_sock_open){
    //printf("Socket already closed\n");
    return true;
  }
  printf("Attempting to close socket for %s:%d ... ", this->host.c_str(), this->port);

  

  try{
    
    int shtdn_ret = shutdown(this->sock, 2);
    if(shtdn_ret == -1 && errno != ENOTCONN) throw SocketShutdownException();
    
    int close_ret = close(this->sock);
    if(close_ret == -1) throw SocketCloseException();
    

    this->sock = -1;
    this->is_sock_open = false;
    printf("Socket closed successfully\n");

    return true;

  } catch(GenericException &e){
    printf("Could not close socket\n");
    cout << e.what();
  }
    
  
  return false;
}






















/********************************************************************************************************************/


PeerConnHandler::PeerConnHandler(string host, uint16_t port, int sock=-1): host(host), port(port){
  
  //this->host = host
  //this->port= port
  

  if(sock == -1){
    this->set_server();
  } else {
    this->set_client();
  }


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
  printf("~PeerConnHandler()\n");
}


void PeerConnHandler::send_data(Packet pkt){
  try{
    this->sock_helper->send_data2(pkt.get_packet(), pkt.get_pkt_size()); // or pkt.build()
    
  } catch(GenericException &e){
    cout << e.what();
  }
  

}

void PeerConnHandler::recv_data(){
  char *msg = nullptr;
  try{
    //msg = this->sock_helper->receive_all();
    uint8_t *buf = this->recv_buffer.get();
    int *pos = this->curr_buff_idx.get();
    int recv_len = this->sock_helper->receive(buf + *pos, this->buff_size - *pos);
    //do processing here
    if(recv_len > 0 && *pos < recv_len){
      while(*pos < recv_len){
        if(this->current_pkt == nullptr){
          this->current_pkt = new Packet(this->recv_buffer, this->curr_buff_idx, this->buff_size);
        }
        this->current_pkt->rebuild();
        if(this->current_pkt->is_build_done()){
          this->completed_pkts.push_back(current_pkt);
          current_pkt = nullptr;
        }

      }
        if(*pos >= recv_len ) this->clear_buff();
    }
    
   

  
  //first read the first two bytes which tells us how long our header is going to be
  
  //read and process the header (it should tell us how long is the next string of bytes)
  //process the following bytes after the header according to what the header specifies.
  //check if size of data - header >= buffer size 
  //if yes then process data in buffer buffer and remove it after we are done

  //TODO: 
  //make sure to limit buffer size, so that it wont get out of hand
  } catch (GenericException &e){
    cout << e.what();
  }

  
  if(!this->completed_pkts.empty()){
    Packet *pkt = this->completed_pkts.at(this->completed_pkts.size()-1);
    printf("Latest packet received %d(+header) bytes from %s:%d -> '%s'\n", pkt->get_pkt_size(), this->host.c_str(), this->port, (char*) pkt->get_payload());
  }
  


}


bool PeerConnHandler::close() {
  return this->sock_helper->close_sock();
}






















/*********************************************************************************************************************/


LocalPeer::LocalPeer(string host, uint16_t port): host(host), port(port){
  //this->host = host;
  //this->port = port;
  this->server_sock_helper = shared_ptr<SocketHelper>(new SocketHelper(host, port)); // or just make_shared<SocketHelper>(host, port);
  //this->server_sock_helper = make_shared<SocketHelper>(host, port);
  this->is_local_server_running = false;
  this->max_inbound_connections = 1;
  this->max_outbound_connections = 1;
  //this->backlog = 2;
  //this->timeout = 10.0;
  this->server_sock_helper->set_blocking(false);
  this->server_sock_helper->set_sock_options();

  this->add_pollfd_to_poll_list(this->server_sock_helper->get_sockfd(), POLLIN | POLLERR | POLLHUP | POLLNVAL);

  

}



shared_ptr<PeerConnHandler> LocalPeer::get_peer_from_sockfd(int sockfd){
  shared_ptr<PeerConnHandler> peer_conn;

  for(shared_ptr<PeerConnHandler> p_conn : this->inbound_connections){
    if(p_conn->get_sock_helper()->get_sockfd() == sockfd){
      peer_conn = p_conn;
      break;
    }
  }

  for(shared_ptr<PeerConnHandler> p_conn : this->outbound_connections){
    if(p_conn->get_sock_helper()->get_sockfd() == sockfd){
      peer_conn = p_conn;
      break;
    }
  }

  // try {
  //   if(peer_conn == nullptr){
  //     string msg = "Could not send data to '" + host + ":" + to_string(port) + "' because host does not have an established connecton to us\n";
  //     throw SendException(msg.c_str());
  //   }
  //   peer_conn->send_data(data);

  // } catch (GenericException &e) {
  //   cout << e.what();
  // }
  return peer_conn;


}

void LocalPeer::add_pollfd_to_poll_list(int sockfd, short events){
  struct pollfd pfd;
  pfd.fd = sockfd;
  pfd.events = events;
  this->pollfd_list.push_back(pfd);
}

LocalPeer::~LocalPeer(){
  //this->server_sock_helper.reset();
  //delete this->server_sock_helper;
  printf("~LocalPeer() %d\n", this->server_sock_helper.use_count());
}


void LocalPeer::handle_client(string ipaddr, int port, int client_sock){
  shared_ptr<PeerConnHandler> peer_conn = shared_ptr<PeerConnHandler>(new PeerConnHandler(ipaddr, port, client_sock));
  this->add_pollfd_to_poll_list(client_sock, POLLIN | POLLERR | POLLHUP | POLLNVAL);
  this->inbound_connections.push_back(peer_conn);
  
}


void LocalPeer::connect_to_peer(string host, uint16_t port){
  printf("Attempting to connect to %s:%d ... ", host.c_str(), port);
  if(this->outbound_connections.size() >= this->max_outbound_connections){
    printf("Connection failed. Max outbound connections reached\n");
    return;
  }
  
  try{
    shared_ptr<PeerConnHandler> peer_conn = shared_ptr<PeerConnHandler>(new PeerConnHandler(host, port));
    peer_conn->connect();
    this->add_pollfd_to_poll_list(peer_conn->get_sock_helper()->get_sockfd(), POLLIN | POLLERR | POLLHUP | POLLNVAL);
    this->outbound_connections.push_back(peer_conn);
    printf("Connection sucessful!\n");

  } catch (GenericException &e){
    cout << "LocalPeer::connect_to_peer() ";
    cout << "Could not connect to host" << endl; 
    cout << e.what();
  }
}


void LocalPeer::send_data_to_peer(string host, Packet pkt){
  shared_ptr<PeerConnHandler> peer_conn;

  for(shared_ptr<PeerConnHandler> p_conn : this->inbound_connections){
    if(p_conn->get_host() == host){
      peer_conn = p_conn;
    }
  }

  for(shared_ptr<PeerConnHandler> p_conn : this->outbound_connections){
    if(p_conn->get_host() == host){
      peer_conn = p_conn;
    }
  }

  try {
    if(peer_conn == nullptr){
      string msg = "Could not send data to '" + host + ":" + to_string(port) + "' because host does not have an established connecton to us\n";
      throw SendException(msg.c_str());
    }
    peer_conn->send_data(pkt);

  } catch (GenericException &e) {
    cout << e.what();
  }
  

}

void LocalPeer::remove_disconnected_peers(){
  vector<shared_ptr<PeerConnHandler>>::iterator it = this->inbound_connections.begin();
  vector<struct pollfd>::iterator poll_it = this->pollfd_list.begin();
  vector<int> fds_to_rm;
  
  while(it != this->inbound_connections.end()){
    if(!(it->get()->conn_active())){
      printf("Removed client %s from peer list\n", it->get()->get_host().c_str());
      fds_to_rm.push_back(it->get()->get_sock_helper()->get_sockfd());
      it = this->inbound_connections.erase(it);
    } else {
      ++it;
    }
    
    
  }

  it = this->outbound_connections.begin();
  while(it != this->outbound_connections.end()){
    if(!(it->get()->conn_active())){
      printf("Removed server %s from peer list", it->get()->get_host().c_str());
      fds_to_rm.push_back(it->get()->get_sock_helper()->get_sockfd());
      it = this->outbound_connections.erase(it);
      
    } else {
      ++it;
    }
    
  }

  
  poll_it = this->pollfd_list.begin();
  for(int fd : fds_to_rm) {
    while(poll_it != this->pollfd_list.end()){
      if((*poll_it).fd == fd || (*poll_it).fd == -1){
        cout << "Removing sockfd " << fd << " from fd poll list\n";
        poll_it = this->pollfd_list.erase(poll_it);
      } else {
        ++poll_it;
      }
      
    }
  }
  
}

void LocalPeer::close_all_connections(){

  try{
    for(int i = 0; i < this->inbound_connections.size(); i++){
      this->inbound_connections.at(i)->close();
    }

    for(int i = 0; i < this->outbound_connections.size(); i++){
      this->outbound_connections.at(i)->close();
    }
    this->inbound_connections.clear();
    this->outbound_connections.clear();
  } catch(GenericException &e) {
    cout << e.what();
  }
}


void LocalPeer::start_server(){
  
  this->server_sock_helper->init_server(2);
  this->is_local_server_running = true;
  int ready = 0;
  int poll_timeout = -1;
  vector<struct pollfd>::iterator poll_it;

  while(this->is_local_server_running){
    // if(this->inbound_connections.size() >= this->max_inbound_connections){
    //   printf("Not accepting new clients. Max connections reached, sleeping for 10s\n");
    //   sleep(10);
    //   continue;
    // }
    try {
      ready = poll(this->pollfd_list.data(),(unsigned long) this->pollfd_list.size(), poll_timeout);

      if(ready < 0) throw PollException("", ready);
      //if(ready == 0) throw PollException("Poll timeout", ready);
      //ready is >0 do stuff ...
      //if(this->follfd_list.at(0).fd & POLLIN) printf("");
      poll_it = this->pollfd_list.begin();
      while(poll_it != this->pollfd_list.end()){
        struct pollfd pfd = *poll_it;
        
        
        if((pfd.revents & POLLIN) == POLLIN){
          if(pfd.fd == this->server_sock_helper->get_sockfd()){
            string ipaddr;
            int port;
            int client_sock;
            tie(ipaddr, port, client_sock) = this->server_sock_helper->server_accept_conns();
            //TODO: Inform the user that a client is trying to connect to us and ask them
            // if they want to accept the connection or not, for now just
            this->handle_client(ipaddr, port, client_sock);
            continue;
          }
          shared_ptr<PeerConnHandler> peer =  get_peer_from_sockfd(pfd.fd);  //TODO: implement a map sockfd ->peer
          
          if(peer != nullptr) {
            peer->recv_data();
            if(!(peer->conn_active())){
              printf("not active\n");
              (*poll_it).fd = -1;
              //poll_it = this->pollfd_list.erase(poll_it);
            }
          }
          

        }

        //if((pfd.revents & POLLHUP ) == POLLHUP) prntf("some error");
        //if((pfd.revents & POLLERR) == POLLERR) prntf("some error");
        //if((pfd.revents & POLLNVAL) == POLLNVAL) prntf("some error");
        
        ++poll_it;
      }
      //printf("Server acceping new connections\n");
      
      
      this->remove_disconnected_peers();
    } catch(GenericException &e){
      cout << e.what();
    }

    sleep(0.1);
  }
  this->close_all_connections();
}


bool LocalPeer::stop_server(){
  printf("Stopping server at %s:%d\n", this->host.c_str(), this->port);
  this->close_all_connections();

  if(this->server_sock_helper->close_sock()){
    this->is_local_server_running = false;
    return true;
  }
  
  return false;
  
}
























/****************************************************************************************************************************************/

vector<unique_ptr<LocalPeer>> test_peers;
vector<thread> threads;




void sig_handler(int s){
  printf("\nCaught signal %d\n",s);
  printf("SIGINT or CTRL-C detected. Exiting gracefully\n");



  for(int i = 0; i < test_peers.size(); i++){
    test_peers.at(i)->stop_server();
  }

  test_peers.clear();

  exit(1); 

}


Packet to_pkt(string payload){

  Packet pkt((uint8_t*) payload.data(), PacketType::text_plain, PacketCharSet::utf8, PacketEncoding::elwin_enc, PacketCompression::_7zip, PacketEncryption::LAST);
  pkt.build();

  // printf("packet1 size is: %d\n", pkt.get_pkt_size());
  // printf("packet1 payload: %s\n", (char*) pkt.get_payload());


  // int buff_size = 3;
  // shared_ptr<uint8_t> recv_buffer = shared_ptr<uint8_t>((uint8_t*) malloc(buff_size));
  // shared_ptr<int> curr_buff_idx = shared_ptr<int>((int*) malloc(sizeof(int)));
  
  // memset(recv_buffer.get(), '\1',buff_size);
  // int *pos = curr_buff_idx.get();
  // *pos = 0;
  // memcpy(recv_buffer.get(),(char*) pkt.get_packet(), min(buff_size, pkt.get_pkt_size()));

  
  
  // uint8_t *p =  pkt.get_packet();
  // Packet pkt2(recv_buffer, curr_buff_idx, buff_size);
  // pkt2.rebuild();
  
  // int cop_offset = min(buff_size, pkt.get_pkt_size());;
  // int bytes_sent = cop_offset;;

  // uint8_t *cop = pkt.get_packet() + cop_offset;
  // while(!pkt2.is_build_done()){
  //   memset(recv_buffer.get(), '\1', buff_size);
  //   int *pos = curr_buff_idx.get(); *pos = 0;

  //   cop_offset = min(buff_size, pkt.get_pkt_size() - bytes_sent);
  //   memcpy(recv_buffer.get(),(char*) cop, cop_offset);
  //   pkt2.rebuild();
  //   cop += cop_offset;
  //   bytes_sent += cop_offset;
  // }
  // printf("packet2 size is: %d\n", pkt2.get_pkt_size());
  // printf("packet2 payload: %s\n", (char*) pkt2.get_payload());
  return pkt;

}



void init_test_peer(string bind_addr, int bind_port, string local_peer_name="", string other_peer_addr="", int other_peer_port=-1, string data_to_send=""){
  if (local_peer_name == ""){
    local_peer_name = bind_addr + ":" + to_string(bind_port);
  }

  
  unique_ptr<LocalPeer> test_peer(new LocalPeer(bind_addr, bind_port));
  
  test_peers.push_back(move(test_peer));
  

  if(other_peer_addr != "" && other_peer_port != -1){

    test_peers.at(test_peers.size() -1)->connect_to_peer(other_peer_addr, other_peer_port);
    sleep(1);

    Packet pkt1 = to_pkt(data_to_send.c_str());
    Packet pkt2 = to_pkt(data_to_send + " x2\0");
    Packet pkt3 = to_pkt("1Small data null terminated\0");
    Packet pkt4 = to_pkt("2Small data not null terminated");
    Packet pkt5 = to_pkt("3 maybe this will continue here since previous data was not null terminated ?\0");
    Packet pkt6 = to_pkt("4Now I am just going to write a long message that is null terminated lalaalalalallalal this data could be anything the quick brown fox jummped over the fence or something like that.\0");
    Packet pkt7 = to_pkt("5Sending the same data as before. Now I am just going to with a random null terminator HERE write a long message that is null terminated lalaalalalallalal this data could be anything the quick brown fox jummped over the fence or something like that.\0");
    Packet pkt8 = to_pkt("6Sending something really long while a single recv call accepts fewer bytes than what I am sending now THE FOLLOWING IS A REPEAT OF THE LAST TRANSMISSION 'Sending the same data as before. Now I am just going to write a long message that is null terminated lalaalalalallalal this data could be anything the quick brown fox jummped over the fence or something like that.'\0");

    

    test_peers.at(test_peers.size() -1)->send_data_to_peer(other_peer_addr, pkt1);
    sleep(20);
    test_peers.at(test_peers.size() -1)->send_data_to_peer(other_peer_addr, pkt2);
    sleep(20);
    test_peers.at(test_peers.size() -1)->send_data_to_peer(other_peer_addr, pkt3);
     sleep(20);
    test_peers.at(test_peers.size() -1)->send_data_to_peer(other_peer_addr, pkt4);
    //sleep(2);
    test_peers.at(test_peers.size() -1)->send_data_to_peer(other_peer_addr, pkt5);
    sleep(20);
    test_peers.at(test_peers.size() -1)->send_data_to_peer(other_peer_addr, pkt6);
    sleep(20);
    test_peers.at(test_peers.size() -1)->send_data_to_peer(other_peer_addr, pkt7);
    sleep(20);
    test_peers.at(test_peers.size() -1)->send_data_to_peer(other_peer_addr, pkt8);
  }

  
  //thread t1(&LocalPeer::start_server, test_peers.at(test_peers.size() -1));
  //thread t2(&LocalPeer::check_for_msgs, test_peers.at(test_peers.size() -1));
  //t1.detach(); // as opposed to .join, which runs on the current thread
  //t2.detach();
  
  test_peers.at(test_peers.size() -1)->start_server();

}

int main(int argc, char *argv[]) {

  

  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = sig_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);

  unsigned int n = std::thread::hardware_concurrency();
  std::cout << n << " concurrent threads are supported.\n";
  //thread t1(&LocalPeer::start_server, this);
  //thread t2(&LocalPeer::check_for_msgs, this);
  //t1.detach(); // as opposed to .join, which runs on the current thread
  //t2.detach();

  try{
    int port1 = 8000;
    int port2 = 8001;
  
    
    if(argc > 1) {
      if(strcmp(argv[1], "server") == 0){
        printf("In server mode\n");
        init_test_peer("0.0.0.0", port1, "peer1");
      }

      if(strcmp(argv[1], "p2p") == 0 || strcmp(argv[1], "client") == 0){
        printf("In peer-to-Peer mode\n");
        init_test_peer("0.0.0.0", port2, "peer2", "127.0.0.1", port1, "Hello from peer2!\0");
      }
    } else {
      init_test_peer("0.0.0.0", port1, "peer1");
    }

  } catch(GenericException &e){
    cout << e.what();
  } catch(std::exception& e) {
    //Other errors
    cout << "System error occured errno: " << errno << " -> " << std::strerror(errno) << endl;
    cout << e.what() << endl;
    exit(EXIT_FAILURE);
  } catch (...) {
    cout << "An error occured but, dont know why. You are on your own" << endl;
    exit(EXIT_FAILURE);
  }

  printf("Press CTRL-C to exit\n");

  //test_peer1.stop_thread();
  //test_peer2.stop_thread();
  //sleep(1);
  //test_peer1.join()
  //test_peer2.join()

  pause();


  return 0;
}
