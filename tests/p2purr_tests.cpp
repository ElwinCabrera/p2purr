#include <catch2/catch_test_macros.hpp>
//#include <catch2/benchmark/catch_benchmark.hpp>
#include <cstdint>
#include <vector>

#include "../include/packet.h"




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


TEST_CASE( "Creating and building a packet", "[packet]"){
    string payload = "tooooooooooooooooooooooo!!!!!!!!!!!!!!!!!!";
    Packet pkt((uint8_t*) payload.data(), PacketType::text_plain, PacketCharSet::utf8, PacketEncoding::elwin_enc, PacketCompression::_7zip, PacketEncryption::LAST);
    // SECTION(""){

    // }
}


SCENARIO( "Creating a packet object and building the data", "[paket]" ) {
    GIVEN("Some string data we want to paketize of a small length"){
        String str = "hello yo how are you doing?";
        // WHEN(""){

        // }
    }

}