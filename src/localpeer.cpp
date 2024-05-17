#include "../include/localpeer.h"
// #include "../include/exceptionhandler.h"
// #include "../include/packet.h"
// #include "../include/sockethelper.h"
// #include "../include/peerconnhandler.h"
// #include "localpeer.h"


LocalPeer::LocalPeer() {}

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
  //printf("~LocalPeer() %d\n", this->server_sock_helper.use_count());
}


void LocalPeer::handle_client(string ipaddr, int port, int client_sock){
  shared_ptr<PeerConnHandler> peer_conn = shared_ptr<PeerConnHandler>(new PeerConnHandler(ipaddr, port, client_sock));
  
  bool accept_connection = true;
  //accept_connection = on_client_connection_request(ipaddr, port);
  
  if(this->inbound_connections.size() > this->max_inbound_connections){
    printf("Not accepting new clients. Max connections reached\n");
    accept_connection = false;
  }

  if(accept_connection){
    this->add_pollfd_to_poll_list(client_sock, POLLIN | POLLERR | POLLHUP | POLLNVAL);
    this->inbound_connections.push_back(peer_conn);
  } else {
    printf("server refusing client connection\n");
    peer_conn->close();
  }
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
    for(size_t i = 0; i < this->inbound_connections.size(); i++){
      this->inbound_connections.at(i)->close();
    }

    for(size_t i = 0; i < this->outbound_connections.size(); i++){
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
            sock_t client_sock;
            tie(ipaddr, port, client_sock) = this->server_sock_helper->server_accept_conns();
            //TODO: Inform the user that a client is trying to connect to us and ask them
            // if they want to accept the connection or not, for now just
            this->handle_client(ipaddr, port, client_sock);
            poll_it = this->pollfd_list.begin();  // at this point since we modified our list, reset the iterator since it was probably invalidated when we modified the list
            ++poll_it;
            continue;
          } else {
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

