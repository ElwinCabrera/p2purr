#include <iostream> 
#include <stdlib.h>
#include <stdio.h> 
#include <cstdlib> // for exit() and EXIT_FAILURE
#include <sys/types.h>
#include <memory>  // for std::unique_ptr<T> var_name(new T)
//#include <thread>
#include <algorithm>
#include<cstring>

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

using namespace std;



class SocketHelper {
public:

  SocketHelper() {}

  SocketHelper(string host, uint16_t port, int sock=-1): host(host), port(port), sock(sock){
    //TODO: check if host is a domain name or ip address
    //if this->host is a domain name convert it to an ip address got from getaddrinfo
    if(sock == -1){
      this->sock = socket(PF_INET, SOCK_STREAM, 0);
      if( sock == -1){
        cout << "Failed to create socket. errno: " << errno << endl;
        exit(EXIT_FAILURE);
      }
    }


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

  ~SocketHelper(){
    freeaddrinfo(this->ai); // free linked list
  }

  void init_server(int backlog, float timeout){
    int bind_retcode = bind(this->sock, this->ai->ai_addr, this->ai->ai_addrlen);
    if(bind_retcode < 0 ) {

    }
    int yes = 1;
    int sockopt_retcode = setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if(sockopt_retcode == -1){
      printf("setsockopt error");
    }
    int listen_retcode = listen(this->sock, backlog);
    if(listen_retcode == -1){
      printf("listen error");
    }

  }


  tuple<int, string, int> server_accept_conns(){
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);

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

    }
    return {client_sock, ipstr, port};
    //get data by auto [var1, var2, var3] = fun(...) or tie(var1,var2,var3) = fun(...)
  }

  void connect_to_host(){
    int conn_retcode = connect(this->sock, this->ai->ai_addr, this->ai->ai_addrlen);
    if(conn_retcode == -1){
      printf("Error connecting to host");
    }
  }
  
  bool send_data(string msg){  //doesnt have to be a string can be anything sine send(...) accepts a 'void*'
    int msg_len = msg.size(); // or strlen(msg)
    int total_bytes_sent =0;
    while(total_bytes_sent < msg_len){
      string msg_substr = msg.substr(total_bytes_sent);
      int num_bytes_sent = send(this->sock, msg_substr.c_str(), msg_substr.size(), 0);
      if (num_bytes_sent == -1){
        printf("send error");
      }
      total_bytes_sent += num_bytes_sent;
    }
    return total_bytes_sent == msg_len;
  }




  string receive_data(int msg_len){
    vector<string> chunks;
    int total_bytes_recvd = 0;
    while(total_bytes_recvd < msg_len){
      int recv_len = min(msg_len - total_bytes_recvd, 4096);
      char chunk[recv_len];
      int bytes_recvd = recv(this->sock, chunk, recv_len, 0);
      if(bytes_recvd == 0){
        printf("Client closed the connection");
        //lets close the connection and move on
      } else if (bytes_recvd == -1){
        printf("recv error ");
      }

      chunks.push_back(chunk);
      total_bytes_recvd += bytes_recvd;

    }
    string msg_recvd = "";
    for(int i = 0; i < chunks.size(); i++){
      msg_recvd += chunks[i];
    }
    return msg_recvd;
  }

  //probably not going to use this often here just in case
  struct sockaddr* build_sockaddr_struct(string host, uint16_t port, int sa_family){
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

  struct addrinfo* build_addrinfo_struct(string host, uint16_t port, int addr_family, int socktype, int flags){
    
    struct addrinfo hints;
    struct addrinfo *ai_result;
    memset(&hints, 0, sizeof(hints)); //make sure the struct is empty
    hints.ai_family = addr_family;
    hints.ai_socktype = socktype;
    //hints.ai_flags = flags;
    
    string port_str = to_string(port);
    
    int status = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &ai_result);
    if(status != 0){
      //error
      
    }

    return ai_result;
  }

  void get_all_ip_from_addrinfo(){

    printf("IP addresses for %s:\n\n", host);
    struct addrinfo *p;
    //char ipstr[INET6_ADDRSTRLEN];
    for(p = this->ai; p != NULL; p=p->ai_next){
      void *addr;
      
      if(p->ai_family == AF_INET){ //IPv4
        struct sockaddr_in *ipv4 = (struct sockaddr_in*) p->ai_addr;
        addr = &(ipv4->sin_addr);
        
        
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(p->ai_family, addr, ipstr, sizeof(INET_ADDRSTRLEN));
        this->ipv4_addrs.push_back(ipstr);
        printf("  IPv4: %s\n", ipstr);
      } else { //IPv6
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*) p->ai_addr;
        addr = &(ipv6->sin6_addr);

        
        char ipstr[INET6_ADDRSTRLEN];
        inet_ntop(p->ai_family, addr, ipstr, sizeof(INET6_ADDRSTRLEN));
        this->ipv6_addrs.push_back(ipstr);
        printf("  IPv6: %s\n", ipstr);
      }

      //inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
      //printf("  %s: %s\n", ipver, ipstr);

    }

  }

  

  bool close_sock();

protected:
  int sock;
  string host;
  uint16_t port;
  bool is_sock_open;
  int max_recv_bytes_at_once;
  
  struct addrinfo *ai;
  vector<string> ipv4_addrs;
  vector<string> ipv6_addrs;



};



class PeerConnHandler {

public:
  PeerConnHandler(string host, uint16_t port, int sock=-1): host(host), port(port), sock(sock){
    //this->host = host
    //this->port= port
    this->sock_helper = SocketHelper(host, port, sock);
    this->conn_is_active = true;
    this->keepalive = true;

  }
  void connect();
  void send_data(string data);
  string recv_data();
  void stop_thread();
  void run();

private:
  string host;
  uint16_t port;
  int sock;
  SocketHelper sock_helper;
  bool conn_is_active;
  bool keepalive;
};


class LocalPeer{
public:
  LocalPeer(string host, uint16_t port): host(host), port(port){
    //this->host = host;
    //this->port = port;
    this->server_sock_helper = SocketHelper(host, port);

    this->is_local_server_running = false;
    this->max_inbound_connections = 1;
    this->max_outbound_connections = 1;
  }

  void connect_to_peer(string host, uint16_t port){

  }

  void send_data_to_peer(string host, string data){

  }

  void handle_client(int client_sock){

  }

  void close_all_connections(){

  }

  void stop_task(){

  }

protected:
    string host;
    uint16_t port;
    SocketHelper server_sock_helper;

    bool is_local_server_running;
    int max_inbound_connections;
    int max_outbound_connections;

    vector<PeerConnHandler> inbound_connections;  //we are the server to those connections, serve them!
    vector<PeerConnHandler> outbound_connections; // We are the client to those connections, ask them to serve us stuff
    

};







int main() {


  
  

}
