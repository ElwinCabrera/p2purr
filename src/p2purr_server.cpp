#include "../include/p2purr_server.h"



P2PurrServer::P2PurrServer() {}

P2PurrServer::P2PurrServer(string host, uint16_t port): host(host), port(port){
  //this->host = host;
  //this->port = port;
  this->server_sock_helper = shared_ptr<SocketHelper>(new SocketHelper(host, port)); // or just make_shared<SocketHelper>(host, port);
  //this->server_sock_helper = make_shared<SocketHelper>(host, port);
  this->is_local_server_running = false;
  this->max_inbound_connections = 1000;
  this->max_outbound_connections = 1000;
  //this->backlog = 2;
  //this->timeout = 10.0;
  this->server_sock_helper->set_blocking(false);
  this->server_sock_helper->set_sock_options();

  this->add_fd_to_watch_list(this->server_sock_helper->get_sockfd(), POLLIN | POLLERR | POLLHUP | POLLNVAL);

}

P2PurrServer::~P2PurrServer(){
  //printf("~P2PurrServer() %d\n", this->server_sock_helper.use_count());
  if(this->is_local_server_running) this->stop();
}



void P2PurrServer::add_fd_to_watch_list(sock_t sockfd, short events){
  struct pollfd pfd;
  pfd.fd = (int) sockfd;
  pfd.events = events;
  this->pollfd_list.push_back(pfd);
}




void P2PurrServer::handle_client_communications(sock_t fd){
  
  if(this->sock_to_peer_map.find(fd) != this->sock_to_peer_map.end()){
    //Packet *pkt = sock_to_peer_map[fd]->recv_pkt();
    shared_ptr<PeerConnHandler> peer = sock_to_peer_map[fd];
    shared_ptr<Packet> pkt = peer->recv_pkt();

#if defined(_TEST) || defined(_LIB)
    if(pkt.get()){
      //implement a semaphore/lock
      on_packet_received(peer->get_host(), peer->get_port(), pkt);
      //on_data_received(peer->get_host(), peer->get_port(), payload);
    }
#endif
  } /*else { // so this socket exists already. TODO: Handle this
      
  }*/

}


void P2PurrServer::handle_new_client(){
  string ipaddr;
  int port;
  sock_t client_sock;
  tie(ipaddr, port, client_sock) = this->server_sock_helper->server_accept_conns();
  //TODO: Inform the user that a client is trying to connect to us and ask them
  // if they want to accept the connection or not, for now just
  shared_ptr<PeerConnHandler> peer = shared_ptr<PeerConnHandler>(new PeerConnHandler(ipaddr, port, client_sock));
  
  bool accept_connection = true;
  //accept_connection = on_client_connection_request(ipaddr, port);
  
  if(this->sock_to_peer_map.size() > this->max_inbound_connections){
    printf("Not accepting new clients. Max connections reached\n");
    accept_connection = false;
  }

  if(accept_connection){
    this->add_fd_to_watch_list(client_sock, POLLIN | POLLERR | POLLHUP | POLLNVAL);

    if(this->sock_to_peer_map.find(client_sock) == this->sock_to_peer_map.end()){ // great we dont have that socket open thus its a new client
      this->sock_to_peer_map[client_sock] = peer;
    } /*else { // so this socket exists already. TODO: Handle this
      
    }*/
    
  } else {
    printf("server refusing client connection\n");
    peer->close();
  }
}


void P2PurrServer::connect_to_peer(string host, uint16_t port){

  printf("Attempting to connect to %s:%d ... ", host.c_str(), port);
  
  if(this->sock_to_peer_map.size() >= this->max_outbound_connections){
    printf("Connection failed. Max outbound connections reached\n");
    return;
  }
  
  try{
    shared_ptr<PeerConnHandler> peer = shared_ptr<PeerConnHandler>(new PeerConnHandler(host, port));
    peer->connect();
    sock_t peer_server_sock = peer->get_sock_helper()->get_sockfd();
    this->add_fd_to_watch_list(peer_server_sock, POLLIN | POLLERR | POLLHUP | POLLNVAL);

    if(this->sock_to_peer_map.find(peer_server_sock) == this->sock_to_peer_map.end()){ // great we dont have that socket open thus we are connecting to a new server
      this->sock_to_peer_map[peer_server_sock] = peer;
    } /*else { // so this socket exists already. TODO: Handle this
      
    }*/

    printf("Connection sucessful!\n");

  } catch (GenericException &e){
    cout << "P2PurrServer::connect_to_peer() ";
    cout << "Could not connect to host" << endl; 
    cout << e.what();
  }
}


void P2PurrServer::send_pkt_to_peer(Packet pkt, string host, uint16_t port){
  try {
    bool peer_found = false;
    for(auto& [fd, peer] : this->sock_to_peer_map){
      if(peer->get_host() == host && peer->get_port() == port){
        peer->send_pkt(pkt);
        peer_found = true;
        break;
      }
    }

    if(!peer_found){
      string msg = "Could not send data to '" + host + ":" + to_string(port) + "' because host does not have an established connecton to us\n";
      throw SendException(msg.c_str());
    }

  } catch (GenericException &e) {
    cout << e.what();
    
  }
}

void P2PurrServer::remove_disconnected_peers(){
  if(!this->is_local_server_running) return;
  unordered_map<sock_t, shared_ptr<PeerConnHandler>>::iterator it = this->sock_to_peer_map.begin();
  
  vector<sock_t> fds_to_rm;

  
  while(it != this->sock_to_peer_map.end() && !this->sock_to_peer_map.empty()){
    shared_ptr<PeerConnHandler> peer = it->second;
    //shared_ptr<PeerConnHandler> = it->second.get(); // ???
    if(!peer->conn_active()){
      fds_to_rm.push_back(it->first);
      it = this->sock_to_peer_map.erase(it);
      printf("Removed client %s from peer list\n", peer->get_host().c_str());
    } else {
      ++it;
    }
    
  }

  // it = this->outbound_connections.begin();
  // while(it != this->outbound_connections.end()){
  //   if(!(it->get()->conn_active())){
  //     printf("Removed server %s from peer list", it->get()->get_host().c_str());
  //     fds_to_rm.push_back(it->get()->get_sock_helper()->get_sockfd());
  //     it = this->outbound_connections.erase(it);
      
  //   } else {
  //     ++it;
  //   }
    
  // }

#ifdef linux
  vector<struct pollfd>::iterator poll_it = this->pollfd_list.begin();
  for(sock_t fd : fds_to_rm) {
    while(poll_it != this->pollfd_list.end()){
      if((*poll_it).fd == (int) fd || (*poll_it).fd == -1){ 
        cout << "Removing sockfd " << fd << " from fd poll list\n";
        poll_it = this->pollfd_list.erase(poll_it);
      } else {
        ++poll_it;
      }
      
    }
  }
#endif  
  
}

void P2PurrServer::close_all_connections(){

  try{
// #ifdef linux
//     for(int i = 1; i < this->pollfd_list.size(); ++i){
//       this->pollfd_list.at(i).fd = -1;
//     }
// #endif
      for(auto& [fd, peer] : this->sock_to_peer_map){
        /*if(peer != nullptr)*/ peer->close();
      }
    
    this->sock_to_peer_map.clear();
    this->pollfd_list.clear();
  } catch(GenericException &e) {
    cout << e.what();
  } catch(std::exception& e){
    cout << e.what() << endl;
    printf("system error\n");
  }
}



void P2PurrServer::start_async_poll(){
#ifdef linux
  int ready = 0;
  int poll_timeout = -1;
  vector<struct pollfd>::iterator poll_it;

  while(this->is_local_server_running){
    
    
    ready = poll(this->pollfd_list.data(),(unsigned long) this->pollfd_list.size(), poll_timeout);

    if(ready < 0) throw PollException("", ready);
    //if(ready == 0) throw PollException("Poll timeout", ready);
    //ready is >0 do stuff ...
    //if(this->follfd_list.at(0).fd & POLLIN) printf("");
    poll_it = this->pollfd_list.begin();
    while(poll_it != this->pollfd_list.end() && this->is_local_server_running){
      struct pollfd pfd = *poll_it;
        
        
      if((pfd.revents & POLLIN) == POLLIN){

        if(pfd.fd == this->server_sock_helper->get_sockfd()){
          this->handle_new_client();
            
          poll_it = this->pollfd_list.begin();  // reset the iterator since the original iterator was probably invalidated because we modified our list by adding a new element.
          ++poll_it; //skip ourself
          continue;
        } else {

          this->handle_client_communications((sock_t) pfd.fd);

        }
          
      }

      //if((pfd.revents & POLLHUP ) == POLLHUP) prntf("some error");
      //if((pfd.revents & POLLERR) == POLLERR) prntf("some error");
      //if((pfd.revents & POLLNVAL) == POLLNVAL) prntf("some error");
      ++poll_it;
      
    }
    this->remove_disconnected_peers();
    sleep(0.1);

  }

#endif
}

void P2PurrServer::start_async_select(){

}



void P2PurrServer::init_windows(){
#ifdef _WIN32
  WSADATA wsaData;
  // Initialize Winsock
  int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed: %d\n", iResult);
    return 1;
  }
#endif
}

void P2PurrServer::deinit_windows(){
#ifdef _WIN32
  WSACleanup();
#endif
}



void P2PurrServer::start(){
  
  try{

    this->init_windows();
    this->server_sock_helper->init_server(2);
    this->is_local_server_running = true;

    
#ifdef _WIN32
    this->server_thread = thread(&P2PurrServer::start_async_select, this);
#else
    this->server_thread = thread(&P2PurrServer::start_async_poll, this);
#endif
    sleep(0.5);
    //this->server_thread.join();
    this->server_thread.detach();
    

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
  
}


bool P2PurrServer::stop(){
  printf("Stopping server at %s:%d\n", this->host.c_str(), this->port);
  
  this->is_local_server_running = false;  

  this->close_all_connections();

  
  if(this->server_sock_helper->close_sock()){
    if(this->server_thread.joinable()){
      this->server_thread.join();
    }
    
    return true;
  }
  this->is_local_server_running = true; 
  
#ifdef _WIN32
  this->deinit_windows();
#endif
  return false;
  
}

