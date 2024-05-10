#include "include/p2pconn.h"
#include "include/exceptionhandler.h"
#include "include/packet.h"
// #include "sockethelper.h"
// #include "peerconnhandler.h"
#include "include/localpeer.h"

//TODO: change NULL to nullptr





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
