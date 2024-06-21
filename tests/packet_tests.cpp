#include "test_globals.h"
#include "../include/packet.h"
#include "../include/serializer.h"


#include <cstdint>
#include <vector>
#include <string.h>
#include <stdlib.h>



using std::vector;
using std::string;
using std::to_string;
using std::min;






bool verify_header_integrity(Packet *pkt, uint8_t *expected_hdr_data, int len){
    uint8_t *pkt_data = pkt->get_packet();
    uint8_t *header_data = pkt->get_header();
    
    //if(pkt_data == nullptr && expected_hdr_data != nullptr) return false;
    if(header_data == nullptr && expected_hdr_data != nullptr) return false;
    if(header_data == nullptr || expected_hdr_data == nullptr) return false;

    int hdr_len = pkt->get_full_header_len();
    if(hdr_len != len) return false;
    

    int hdr_idx = 0;
    while(hdr_idx < hdr_len){
        if(header_data[hdr_idx] != expected_hdr_data[hdr_idx]) return false;
        ++hdr_idx;
    }


    hdr_idx = 0;
    int pkt_idx = 0;
    while(hdr_idx < len){
        if(pkt_data[pkt_idx++] != expected_hdr_data[hdr_idx++]) return false;
    } 
    return true;
}

bool verify_payload_integrity(Packet *pkt, uint8_t *expected_payload_data, int len){
    uint8_t *pkt_data = pkt->get_packet();
    uint8_t *payload_data = pkt->get_payload();
    
    if(pkt_data == nullptr && expected_payload_data != nullptr) return false;
    if(pkt_data == nullptr || expected_payload_data == nullptr) return false;

    int payload_len = pkt->get_payload_len();
    if(payload_len != len) return false;

    int idx = 0;
    while(idx < payload_len){
        if(payload_data[idx] != expected_payload_data[idx]) return false;
        ++idx;
    }


    idx = 0;
    int pkt_idx = pkt->get_full_header_len();
    int pkt_end = pkt->get_pkt_len();
    while(pkt_idx < pkt_end){
        if(pkt_data[pkt_idx] != expected_payload_data[idx]) return false;
        ++idx;
        ++pkt_idx;
    }

    return true;
}


Packet rebuild_pkt_from_pkt(Packet orig_pkt_recvd, int buff_size){
  
    //setup
    shared_ptr<uint8_t> recv_buffer = shared_ptr<uint8_t>((uint8_t*) malloc(buff_size));
    shared_ptr<int> curr_buff_idx = shared_ptr<int>((int*) malloc(sizeof(int)));
  
    memset(recv_buffer.get(), '\0',buff_size);
    *(curr_buff_idx.get()) = 0;
    memcpy(recv_buffer.get(),(char*) orig_pkt_recvd.get_packet(), min(buff_size, orig_pkt_recvd.get_pkt_len()));
    


    //Simulate a reveived packet
    //uint8_t *p =  orig_pkt_recvd.get_packet();
    Packet rebuild_pkt(recv_buffer, curr_buff_idx, buff_size);
    rebuild_pkt.rebuild();
  
    int orig_pkt_offset = min(buff_size, orig_pkt_recvd.get_pkt_len());
    int bytes_read = orig_pkt_offset;
    uint8_t *orig_pkt_ptr = orig_pkt_recvd.get_packet() + orig_pkt_offset;
    
    while(!rebuild_pkt.is_build_done()){
        //clear buff and set buff idx to zero
        memset(recv_buffer.get(), '\0', buff_size); //clear
        *(curr_buff_idx.get()) = 0; // buff pos zero

        //populate buffer with next chunck of data
        orig_pkt_offset = min(buff_size, orig_pkt_recvd.get_pkt_len() - bytes_read);
        memcpy(recv_buffer.get(), (char*) orig_pkt_ptr, orig_pkt_offset);

        rebuild_pkt.rebuild(); // call rebuild

        //update our pointer of our testing data and num bytes read 
        orig_pkt_ptr += orig_pkt_offset; 
        bytes_read += orig_pkt_offset;
    }
    return rebuild_pkt;

}






bool test_packet_data_integrity(){

    bool success = true;

    string test_str_payload = get_all_ascii_chars();

    Packet pkt((uint8_t*) test_str_payload.data(), test_str_payload.size(), PacketType::text_plain, PacketCharSet::utf8, PacketEncoding::elwin_enc, PacketCompression::_7zip, PacketEncryption::LAST);
     
    vector<uint8_t> expected_hdr;
    expected_hdr.push_back(0xFF);
    uint16_t hdr_len = 9;
    expected_hdr.push_back((hdr_len >> 8) & 0xFF);
    expected_hdr.push_back(hdr_len & 0xFF);
    expected_hdr.push_back((uint8_t) PacketType::text_plain);
    expected_hdr.push_back((uint8_t) PacketCharSet::utf8);
    expected_hdr.push_back((uint8_t) PacketEncoding::elwin_enc);
    expected_hdr.push_back((uint8_t) PacketEncryption::LAST);
    expected_hdr.push_back((uint8_t) PacketCompression::_7zip);
    uint32_t payload_len = test_str_payload.size();
    int byte_idx = 3;
    while(byte_idx >= 0){
        expected_hdr.push_back((payload_len >> (8 * byte_idx--)) & 0xFF);
    }
        
    success = success && verify_header_integrity(&pkt, (uint8_t*) expected_hdr.data(), expected_hdr.size());
    
    success = success && verify_payload_integrity(&pkt, (uint8_t*) test_str_payload.data(), test_str_payload.size());

    return success;
    
}


bool test_packet_circular_buffer(){

    bool success = true;

    string test_str_payload = "";
    size_t len_limit = 3000;
    while(test_str_payload.size() <= len_limit){
        test_str_payload += get_all_ascii_chars();
    }
    Packet orig_pkt_recvd((uint8_t*) test_str_payload.data(), test_str_payload.size(), PacketType::text_plain, PacketCharSet::utf8, PacketEncoding::elwin_enc, PacketCompression::_7zip, PacketEncryption::LAST);

    //verify_header_integrity(&orig_pkt_recvd,  (uint8_t*)  orig_pkt_recvd.get_header(), orig_pkt_recvd.get_full_header_len());
    success = success && verify_payload_integrity(&orig_pkt_recvd, (uint8_t*) test_str_payload.data(), test_str_payload.size());
    
    int buff_max_size = len_limit * 2;
    int curr_buff_size = 3;
    
    while(curr_buff_size <= buff_max_size){

        Packet rebuilt_pkt = rebuild_pkt_from_pkt(orig_pkt_recvd, curr_buff_size);
            
        success = success && verify_header_integrity(&rebuilt_pkt,  (uint8_t*)  orig_pkt_recvd.get_header(), orig_pkt_recvd.get_full_header_len());
        success = success && verify_payload_integrity(&rebuilt_pkt, (uint8_t*) orig_pkt_recvd.get_payload(), orig_pkt_recvd.get_payload_len());

        ++curr_buff_size;
    }
    if(success){
        printf("packet construction-reconstruction tests pass\n");
    }

    return success;
}



bool test_packet_file_attachment(){
    bool success = true;

    string file_name = "test_file.txt";
    string test_file_contents = get_all_ascii_chars();
    output_to_file(file_name, test_file_contents.data(), test_file_contents.size());


    Packet orig_pkt(file_name, PacketType::generic_file, PacketCharSet::utf8, PacketEncoding::elwin_enc, PacketCompression::_7zip, PacketEncryption::LAST);

    success = success && verify_payload_integrity(&orig_pkt, (uint8_t*) test_file_contents.data(), test_file_contents.size());

    Packet rebuilt_pkt = rebuild_pkt_from_pkt(orig_pkt, 1024);

    success = success && verify_header_integrity(&rebuilt_pkt,  (uint8_t*)  orig_pkt.get_header(), orig_pkt.get_full_header_len());
    success = success && verify_payload_integrity(&rebuilt_pkt, (uint8_t*) orig_pkt.get_payload(), orig_pkt.get_payload_len());

    

    delete_file(file_name);

    if(success){
        printf("packet file attachment test pass\n");
    }

    return success;

}

bool test_packet_large_file_attachment(){
    bool success = true;

    string file_name = "test_file.txt";
    string test_file_contents = "";
    size_t len_limit = 1073741824; // 1GB
    while(test_file_contents.size() <= len_limit){
        test_file_contents += get_all_ascii_chars();
    }

    output_to_file(file_name, test_file_contents.data(), test_file_contents.size());


    Packet orig_pkt(file_name, PacketType::generic_file, PacketCharSet::utf8, PacketEncoding::elwin_enc, PacketCompression::_7zip, PacketEncryption::LAST);

    success = success && verify_payload_integrity(&orig_pkt, (uint8_t*) test_file_contents.data(), test_file_contents.size());

    Packet rebuilt_pkt = rebuild_pkt_from_pkt(orig_pkt, 1024);

    success = success && verify_header_integrity(&rebuilt_pkt,  (uint8_t*)  orig_pkt.get_header(), orig_pkt.get_full_header_len());
    success = success && verify_payload_integrity(&rebuilt_pkt, (uint8_t*) orig_pkt.get_payload(), orig_pkt.get_payload_len());

    

  
    delete_file(file_name);

    if(success){
        printf("packet large file attachment test pass\n");
    }

    return success;

}


bool test_packet_large_file_attachment_circular_buffer(){
    bool success = true;

    string file_name = "test_file.txt";
    string test_file_contents = "";
    size_t len_limit = 1073741824; // 1GB
    while(test_file_contents.size() <= len_limit){
        test_file_contents += get_all_ascii_chars();
    }

    output_to_file(file_name, test_file_contents.data(), test_file_contents.size());


    Packet orig_pkt(file_name, PacketType::generic_file, PacketCharSet::utf8, PacketEncoding::elwin_enc, PacketCompression::_7zip, PacketEncryption::LAST);

    success = success && verify_payload_integrity(&orig_pkt, (uint8_t*) test_file_contents.data(), test_file_contents.size());


    int buff_max_size = len_limit * 2;
    int curr_buff_size = 3;
    
    while(curr_buff_size <= buff_max_size){
        Packet rebuilt_pkt = rebuild_pkt_from_pkt(orig_pkt, curr_buff_size);

        success = success && verify_header_integrity(&rebuilt_pkt,  (uint8_t*)  orig_pkt.get_header(), orig_pkt.get_full_header_len());
        success = success && verify_payload_integrity(&rebuilt_pkt, (uint8_t*) orig_pkt.get_payload(), orig_pkt.get_payload_len());

        ++curr_buff_size;
    }

    delete_file(file_name);

    if(success){
        printf("packet large file attachment circular buffer test pass\n");
    }

    return success;

}



bool packet_test_all(){

    if(!test_packet_data_integrity()){
        printf("packet data integrity test failed\n");
        return false;
    }
    if(!test_packet_file_attachment()){
        printf("packet file attachment test failed\n");
        return false;
    }
    if(!test_packet_large_file_attachment()){
        printf("packet large file attachment test failed\n");
        return false;
    }

    if(!test_packet_circular_buffer()){
        printf("Packet circular buffer test failed\n");
        return false;
    }

    if(!test_packet_large_file_attachment_circular_buffer()){
        printf("Packet large file attachment circular buffer test failed\n");
        return false;
    }

    
    

    return true;


}