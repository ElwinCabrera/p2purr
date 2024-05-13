#include "../include/sockethelper.h"
#include "../include/exceptionhandler.h"


SocketHelper::SocketHelper() {}

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
  memset(ipstr, '\0', INET6_ADDRSTRLEN);
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


  if(ipstr[0] == '\0'){
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
//TODO: this functions is probably buggy as it returns a dangling pointer
// struct sockaddr* SocketHelper::build_sockaddr_struct(string host, uint16_t port, int sa_family){
//   struct sockaddr *sa;
    
//   if (sa_family == AF_INET){
//     struct sockaddr_in sa_ipv4;
      
//     sa_ipv4.sin_family = AF_INET;
//     inet_pton(AF_INET, host.c_str(), &(sa_ipv4.sin_addr)); // populates sa_ipv4.sin_addr from a ip string
//     sa_ipv4.sin_port = htons(port);
//     memset(sa_ipv4.sin_zero, '\0', sizeof(sa_ipv4.sin_zero));

//     sa = (struct sockaddr*) &sa_ipv4;

//   } else if(sa_family == AF_INET6) {
//     struct sockaddr_in6 sa_ipv6;
//     sa_ipv6.sin6_family = AF_INET6;
//     inet_pton(AF_INET6, host.c_str(), &(sa_ipv6.sin6_addr)); // populates sa_ipv4.sin_addr from a ip string
//     sa_ipv6.sin6_port = htons(port);
//     //sa_ipv6.sin6_flowinfo = 0; // idk what this does TODO: figure it out
//     //sa_ipv6.sin6_scope_id = 0; // idk what this does TODO: figure it out

//     sa = (struct sockaddr*) &sa_ipv6;

//   } else {
//     printf("Addtress family not impemented yet");
//   }

//   return sa;
// }

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
      memset(ipstr, '\0', INET_ADDRSTRLEN);
      inet_ntop(p->ai_family, addr, ipstr, INET_ADDRSTRLEN);
      if(ipstr[0] == '\0') throw GenericException("'inet_ntop()' was not succesful\n");
      this->ipv4_addrs.push_back(ipstr);
      //printf("  IPv4: %s\n", ipstr);
      
    } else { //IPv6
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*) p->ai_addr;
      addr = &(ipv6->sin6_addr);
      char ipstr[INET6_ADDRSTRLEN];
      memset(ipstr, '\0', INET6_ADDRSTRLEN);
      inet_ntop(p->ai_family, addr, ipstr, INET6_ADDRSTRLEN);
      if(ipstr[0] == '\0') throw GenericException("'inet_ntop()' was not succesful\n");
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
  //int no = 0;
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


int SocketHelper::get_sockfd() {
  return this->sock;
}

bool SocketHelper::get_sock_open() {
  return this->is_sock_open;
}