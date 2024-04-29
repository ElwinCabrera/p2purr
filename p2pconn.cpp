#include "classheader.hpp"
#include <signal.h>

using namespace std;




SocketHelper::SocketHelper(string host, uint16_t port, int sock): host(host), port(port), sock(sock){
  //TODO: check if host is a domain name or ip address
    //if this->host is a domain name convert it to an ip address got from getaddrinfo
  if(sock == -1){
    this->sock = socket(PF_INET, SOCK_STREAM, 0);
    if( this->sock == -1){
      cout << "Failed to create socket. errno: " << errno << endl;
      exit(EXIT_FAILURE);
    }
  }
  printf("Socket %d created\n", this->sock);


  //TODO: set a timeout on the socket
  //this->sock = sock;
  //this->host = host;
  //this->port = port;
  //this->conn_metrics = new ConnectionStats();
  this->is_sock_open = true;
  this->max_recv_bytes_at_once = 2048;
  this->ai = this->build_addrinfo_struct(host, port, AF_UNSPEC, SOCK_STREAM, 0);

  //create the socket if none was passed 
    
  // lastly iterate over addrinfo linked list and get all ip addresses found and store them in an array
  this->get_all_ip_from_addrinfo();
    
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

void SocketHelper::init_server(int backlog, float timeout){
  printf("initializing server with socket %d\n", this->sock);
  int bind_retcode = bind(this->sock, this->ai->ai_addr, this->ai->ai_addrlen);
  if(bind_retcode < 0 ) {
    printf("bind error\n");
    exit(EXIT_FAILURE);
  }
  printf("Binded this process to %s:%d\n",this->host.c_str(),this->port);
  int yes = 1;
  int sockopt_retcode = setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if(sockopt_retcode == -1){
    printf("setsockopt error\n");
  }
  printf("Socket options set\n");
  int listen_retcode = listen(this->sock, backlog);
  if(listen_retcode == -1){
    printf("listen error");
    exit(EXIT_FAILURE);
  }
  printf("Server listening on %s:%d\n",this->host.c_str(),this->port);

}


tuple<string, int, int> SocketHelper::server_accept_conns(){
  struct sockaddr_storage their_addr;
  socklen_t addr_size = sizeof(their_addr);
  printf("Server acceping new incoming connections\n");

  int client_sock = accept(this->sock, (struct sockaddr*) &their_addr, &addr_size);
  if (client_sock == -1){
    cout << "Failed to accept and create client socket. errno: " << errno << endl;
    //exit(EXIT_FAILURE);
  }

  struct sockaddr *sa = (struct sockaddr*) &their_addr;

  char ipstr[INET6_ADDRSTRLEN];
  int port;
  if(their_addr.ss_family == AF_INET){
      
    struct sockaddr_in *ipv4 = (struct sockaddr_in*) sa;  
    inet_ntop(AF_INET, &(ipv4->sin_addr), ipstr, INET_ADDRSTRLEN);
    port = ntohs(ipv4->sin_port);
    
  } else if (their_addr.ss_family == AF_INET6){

    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*) sa;
    inet_ntop(AF_INET6, &(ipv6->sin6_addr), ipstr, INET6_ADDRSTRLEN);
    port = ntohs(ipv6->sin6_port);
    
  } else{
    printf("unhandled address family\n");
  }

  printf("Got a new connection from %s:%d", ipstr, port);

  return {ipstr, port, client_sock};
  //get data by auto [var1, var2, var3] = fun(...) or tie(var1,var2,var3) = fun(...)
}

void SocketHelper::connect_to_host(){
  int conn_retcode = connect(this->sock, this->ai->ai_addr, this->ai->ai_addrlen);
  if(conn_retcode == -1){
    printf("Error connecting to host");
  }
  printf("Connected to host %s:%d\n",this->host.c_str(),this->port);
}
  
bool SocketHelper::send_data(string msg){  //doesnt have to be a string can be anything sine send(...) accepts a 'void*'
  
  int msg_len = msg.size(); // or strlen(msg)
  int total_bytes_sent =0;
  printf("Sending %d bytes of data to %s:%d\n", msg_len, this->host.c_str(), this->port);
  while(total_bytes_sent < msg_len){
    string msg_substr = msg.substr(total_bytes_sent);
    int num_bytes_sent = send(this->sock, msg_substr.c_str(), msg_substr.size(), 0);
    if (num_bytes_sent == -1){
      printf("send error");
    }
    total_bytes_sent += num_bytes_sent;
  }
  printf("Sent %d bytes succesfully\n", total_bytes_sent);
  return total_bytes_sent == msg_len;
}




string SocketHelper::receive_data(int msg_len){
  vector<string> chunks;
  int total_bytes_recvd = 0;
  printf("Polling recv(%d bytes) from %s:%d\n", msg_len, this->host.c_str(), this->port);
  while(total_bytes_recvd < msg_len){
    int recv_len = min(msg_len - total_bytes_recvd, 4096);
    char chunk[recv_len];
    int bytes_recvd = recv(this->sock, chunk, recv_len, 0);
    if(bytes_recvd == 0){
      printf("Client closed the connection\n");
      //lets close the connection and move on
      this->close_sock();
    } else if (bytes_recvd == -1){
      printf("recv error\n");
    }

    chunks.push_back(chunk);
    total_bytes_recvd += bytes_recvd;

  }
  string msg_recvd = "";
  for(int i = 0; i < chunks.size(); i++){
    msg_recvd += chunks[i];
  }
  printf("Received '%s' from %s:%d\n", msg_recvd.c_str(), this->host.c_str(), this->port);
  return msg_recvd;
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
    
  struct addrinfo hints;
  struct addrinfo *ai_result;
  memset(&hints, 0, sizeof(hints)); //make sure the struct is empty
  hints.ai_family = addr_family;
  hints.ai_socktype = socktype;
  hints.ai_flags = flags; // AI_PASSIVE
    
  string port_str = to_string(port);
    
  int status = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &ai_result);
  if(status != 0){
    //error
    printf("Error getting and building addrinfo struct\n");
    exit(EXIT_FAILURE);
  }

  return ai_result;
}

void SocketHelper::get_all_ip_from_addrinfo(){

  printf("Other IP addresses found for %s:%d\n", host.c_str(), this->port);
  struct addrinfo *p;
  //char ipstr[INET6_ADDRSTRLEN];
  for(p = this->ai; p != NULL; p=p->ai_next){
    void *addr;
      
    if(p->ai_family == AF_INET){ //IPv4
      
      struct sockaddr_in *ipv4 = (struct sockaddr_in*) p->ai_addr;
      addr = &(ipv4->sin_addr);
      char ipstr[INET_ADDRSTRLEN];
      inet_ntop(p->ai_family, addr, ipstr, INET_ADDRSTRLEN);
      this->ipv4_addrs.push_back(ipstr);
      printf("  IPv4: %s\n", ipstr);
      
    } else { //IPv6

      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*) p->ai_addr;
      addr = &(ipv6->sin6_addr);
      char ipstr[INET6_ADDRSTRLEN];
      inet_ntop(p->ai_family, addr, ipstr, INET6_ADDRSTRLEN);
      this->ipv6_addrs.push_back(ipstr);
      printf("  IPv6: %s\n", ipstr);
    }

    //inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
    //printf("  %s: %s\n", ipver, ipstr);
  }

}

  

bool SocketHelper::close_sock(){
  printf("Attempting to close socket for %s:%d ... ", this->host.c_str(), this->port);
  if(this->is_sock_open){
    shutdown(this->sock, 2);
    close(this->sock);
    this->sock = -1;
    this->is_sock_open = false;
  }
  if(!(this->is_sock_open)){
    printf("Socket closed successfully\n");
  } else {
    printf("Could not close socket\n");
  }
  return this->is_sock_open;
}






















/********************************************************************************************************************/


PeerConnHandler::PeerConnHandler(string host, uint16_t port, int sock=-1): host(host), port(port), sock(sock){
  
  //this->host = host
  //this->port= port
  this->pause_send = false;
  this->pause_recv = false;
  this->client_conn = false;
  this->server_conn = false;
  this->conn_is_active = false;
  this->keepalive = true;

  if(sock == -1){
    this->server_conn = true;
  } else {
    this->client_conn = true;
    this->conn_is_active = true;
  }

  this->sock_helper = SocketHelper(host, port, sock);

}

void PeerConnHandler::connect(){
  this->sock_helper.connect_to_host();
  this->client_conn = true;
  this->conn_is_active = true;
}

PeerConnHandler::~PeerConnHandler(){
  printf("~PeerConnHandler()\n");
}

void PeerConnHandler::send_data(string data){
  this->sock_helper.send_data(data);

}
string PeerConnHandler::recv_data(){
  string msg = this->sock_helper.receive_data(17);
  return msg;

}
void PeerConnHandler::communicate(){

  while(this->conn_is_active){
    string data = this->recv_data();
    sleep(1);
  }
  
}
bool PeerConnHandler::close() {
  this->conn_is_active = this->sock_helper.close_sock();
  return this->conn_is_active;
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
}

LocalPeer::~LocalPeer(){
  //this->server_sock_helper.reset();
  //delete this->server_sock_helper;
  printf("~LocalPeer() %d\n", this->server_sock_helper.use_count());
}


void LocalPeer::handle_client(string ipaddr, int port, int client_sock){
  PeerConnHandler peer_conn = PeerConnHandler(ipaddr, port, client_sock);
  this->inbound_connections.push_back(peer_conn);
  peer_conn.communicate(); //thread function
}


void LocalPeer::connect_to_peer(string host, uint16_t port){
  printf("Attepting to connect to server at %s:%d ", host.c_str(), port);
  if(this->outbound_connections.size() < this->max_outbound_connections){
      PeerConnHandler peer_conn = PeerConnHandler(host, port);
      this->outbound_connections.push_back(peer_conn);
      peer_conn.connect();
      peer_conn.communicate();  //thread function
      printf("\n");
  } else {
    printf("Max outbound connections reached");
  }
}


void LocalPeer::send_data_to_peer(string host, string data){
  PeerConnHandler peer_conn;
  for(int i = 0; i < this->inbound_connections.size(); i++){
    if(this->inbound_connections.at(i).get_host() == host){
      peer_conn = this->inbound_connections.at(i);
    }
  }

  for(int i = 0; i < this->outbound_connections.size(); i++){
    if(this->outbound_connections.at(i).get_host() == host){
      peer_conn = this->outbound_connections.at(i);
    }
  }
  peer_conn.send_data(data);

}


void LocalPeer::close_all_connections(){
  for(int i = 0; i < this->inbound_connections.size(); i++){
    this->inbound_connections.at(i).close();
  }

  for(int i = 0; i < this->outbound_connections.size(); i++){
    this->outbound_connections.at(i).close();
  }

  // for(int i = 0; i < this->inbound_connections.size(); i++){
  //   this->inbound_connections.get(i).join();
  // }

  // for(int i = 0; i < this->outbound_connections.size(); i++){
  //   this->outbound_connections.get(i).join()
  // }
}


void LocalPeer::start_server(){
  
  this->server_sock_helper->init_server(2, 10.0);
  this->is_local_server_running = true;

  while(this->is_local_server_running){
    
    string ipaddr;
    int port;
    int client_sock;
    tie(ipaddr, port, client_sock) = this->server_sock_helper->server_accept_conns();
    //TODO: Inform the user that a client is trying to connect to us and ask them
    // if they want to accept the connection or not, for now just
    if(this->inbound_connections.size() < this->max_inbound_connections){
      this->handle_client(ipaddr, port, client_sock);
    }
    sleep(0.1);
  }
  this->close_all_connections();
}


void LocalPeer::stop_server(){
  printf("Stopping server at %s:%d ", this->host.c_str(), this->port);
  this->close_all_connections();
  this->is_local_server_running = this->server_sock_helper->close_sock();
  
}
























/****************************************************************************************************************************************/

vector<unique_ptr<LocalPeer>> test_peers;




void sig_handler(int s){
  printf("\nCaught signal %d\n",s);
  printf("SIGINT or CTRL-C detected. Exiting gracefully\n");

  for(int i = 0; i < test_peers.size(); i++){
    test_peers.at(i)->stop_server();
  }

  test_peers.clear();


  // vector< unique_ptr<LocalPeer> >::iterator it = test_peers.begin();

  // while(it != test_peers.end()) {
  //   printf("sig_handler() %d\n", **it->get_sock_helper().use_count());
  //   **it->stop_server();
  //   printf("sig_handler() %d\n", **it->get_sock_helper().use_count());
  //   it = test_peers.erase(it);
  //   printf("sig_handler() %d\n", **it->get_sock_helper().use_count());
  // }
  
  exit(1); 

}




void init_test_peer(string bind_addr, int bind_port, string local_peer_name="", string other_peer_addr="", int other_peer_port=-1, string data_to_send=""){
  if (local_peer_name == ""){
    local_peer_name = bind_addr + ":" + to_string(bind_port);
  }

  unique_ptr<LocalPeer> test_peer(new LocalPeer(bind_addr, bind_port));
  test_peers.push_back(move(test_peer));
  test_peers.at(test_peers.size() -1)->start_server();
  

  if(other_peer_addr != "" && other_peer_port != -1){
    test_peers.at(test_peers.size() -1)->connect_to_peer(other_peer_addr, other_peer_port);
    sleep(1);
    test_peers.at(test_peers.size() -1)->send_data_to_peer(other_peer_addr, data_to_send);
    sleep(11);
    test_peers.at(test_peers.size() -1)->send_data_to_peer(other_peer_addr, data_to_send+" x2");
  }

}

int main(int argc, char *argv[]) {

  

  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = sig_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);
  
  int port1 = 8001;
  int port2 = 8002;
  
  printf("Press CTRL-C to exit\n");
  if(argc > 1) {
    if(strcmp(argv[1], "server") == 0){
      printf("In server mode\n");
      init_test_peer("0.0.0.0", port1, "peer1");
    }

    if(strcmp(argv[1], "p2p") == 0 || strcmp(argv[1], "client") == 0){
      printf("In peer-to-Peer mode\n");
      init_test_peer("0.0.0.0", port2, "peer2", "127.0.0.1", port1, "Hello from peer2!");
    }
  } else {
    init_test_peer("0.0.0.0", port1, "peer1");
  }

  //init_test_peer("0.0.0.0", port2, "peer2", "127.0.0.1", port1, "Hello from peer2!");
    
    
    

  //test_peer1.stop_thread();
  //test_peer2.stop_thread();
  //sleep(1);
  //test_peer1.join()
  //test_peer2.join()

  pause();


  return 0;
}
