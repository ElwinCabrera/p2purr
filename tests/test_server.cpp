#include "../include/sockethelper.h"
#include "../include/p2purr_server.h"
#include "../include/global_config.h"
#include "test_globals.h"

#define TEST_ADDRESS "127.0.0.1"
#define TEST_PORT1 8001
#define TEST_PORT2 8002

vector<shared_ptr<Packet>> pkts_received;

bool on_packet_received(string host, uint16_t port, shared_ptr<Packet> pkt){
    pkts_received.push_back(pkt);
    //printf("Got testing data: %s\n", (char*) payload);
    return pkt.get();
}


bool test_create_and_close_socket(){
    
    try{
        SocketHelper sock_helper(TEST_ADDRESS, TEST_PORT1);
        if(sock_helper.get_sockfd() < 0) return false;
        sock_helper.close_sock();
        if(sock_helper.get_sock_open()) return false;
        sleep(1);
        return true;
    } catch (GenericException &e) {
        std::cout << e.what();
        return false;
    } 
    return false;

}


bool test_server_init(){

    try{
        SocketHelper sock_helper(TEST_ADDRESS, TEST_PORT1);
        sock_helper.init_server(1);
        sock_helper.close_sock();
        sleep(1);
        return true;
    } catch (GenericException &e) {
        std::cout << e.what();
        return false;
    } 
    return false;

}

bool test_server_start_stop(){

    try{
        P2PurrServer server(TEST_ADDRESS, TEST_PORT1);
        server.start();
        sleep(2);
        server.stop();
        sleep(2);
        return true;
    } catch (GenericException &e) {
        return false;
    } catch(...){
        return false;
    }
    return false;

}


bool test_server_client_connection(){
    try{
        P2PurrServer server(TEST_ADDRESS, TEST_PORT1);
        server.start();
        sleep(1);

        PeerConnHandler client(TEST_ADDRESS, TEST_PORT1);
        client.connect();
        sleep(1);

        server.stop();
        sleep(2);
        return true;
    } catch (GenericException &e) {
        std::cout << e.what();
        return false;
    } 
    return false;
}


bool test_server_client_connection2(){
    try{
        P2PurrServer server(TEST_ADDRESS, TEST_PORT1);
        server.start();
        sleep(1);

        PeerConnHandler client(TEST_ADDRESS, TEST_PORT1);
        client.connect();
        sleep(1);

        client.close();
        sleep(1);

        server.stop();
        sleep(2);
        return true;
    } catch (GenericException &e) {
        std::cout << e.what();
        return false;
    } 
    return false;
}





bool test_multiple_client_connected_no_sending_data(){
    try{
        
        P2PurrServer server(TEST_ADDRESS, TEST_PORT1);
        server.start();
        sleep(1);

        vector<PeerConnHandler> clients;
        int i = 0;
        
        while(i <= 100){
            clients.push_back(PeerConnHandler(TEST_ADDRESS, TEST_PORT1));
            ++i;
        }
        for(PeerConnHandler client: clients){
            client.connect();
        }
        sleep(1);



        server.stop();
        //peer2.stop();
        sleep(2);
        return true;
    } catch (GenericException &e) {
        std::cout << e.what();
        return false;
    } 
    return false;
}

bool test_multiple_client_connected_no_sending_data2(){
    try{
        
        P2PurrServer server(TEST_ADDRESS, TEST_PORT1);
        server.start();
        sleep(1);

        vector<PeerConnHandler> clients;
        int i = 0;
        
        while(i <= 100){
            clients.push_back(PeerConnHandler(TEST_ADDRESS, TEST_PORT1));
            ++i;
        }
        for(PeerConnHandler client: clients){
            client.connect();
        }
        sleep(1);

        for(PeerConnHandler client: clients){
            client.close();
        }
        sleep(2);

        server.stop();
        //peer2.stop();
        sleep(2);
        return true;
    } catch (GenericException &e) {
        std::cout << e.what();
        return false;
    } 
    return false;
}


bool test_server_send_receive(){

    try{
        
        P2PurrServer server(TEST_ADDRESS, TEST_PORT1);
        server.start();
        sleep(1);

        

        

        PeerConnHandler client(TEST_ADDRESS, TEST_PORT1);
        client.connect();
        sleep(1);

        pkts_received.clear();
        string test_str_payload = get_all_ascii_chars();
        //string test_str_payload = "Fuck you!";
        Packet pkt((uint8_t*) test_str_payload.data(), test_str_payload.size(), PacketType::text_plain, PacketCharSet::utf8, PacketEncoding::elwin_enc, PacketCompression::_7zip, PacketEncryption::LAST);
        
        if(!verify_payload_integrity(&pkt, (uint8_t*) test_str_payload.data(), test_str_payload.size())) return false;

        client.send_pkt(pkt);
        sleep(2);
        
        
        

        if(pkts_received.size() > 1 || pkts_received.empty()) return false;
        
        
        shared_ptr<Packet> received_pkt = pkts_received.at(0);

        if(!verify_header_integrity(received_pkt.get(),  (uint8_t*)  pkt.get_header(),  pkt.get_full_header_len())) return false;
        if(!verify_payload_integrity(received_pkt.get(), (uint8_t*)  pkt.get_payload(), pkt.get_payload_len())) return false;

        server.stop();
        
        sleep(2);
        return true;
    } catch (GenericException &e) {
        std::cout << e.what();
        return false;
    }

    return false;
}



bool test_multiple_client_send(){
     try{
        
        P2PurrServer server(TEST_ADDRESS, TEST_PORT1);
        server.start();
        sleep(1);

        vector<PeerConnHandler> clients;
        int i = 0;
        
        while(i <= 100){
            clients.push_back(PeerConnHandler(TEST_ADDRESS, TEST_PORT1));
            ++i;
        }
        
        for(PeerConnHandler client: clients){
            client.connect();
        }

        pkts_received.clear();
        vector<Packet> pkts_sent;
        string test_str_payload = get_all_ascii_chars();
        for(PeerConnHandler client: clients){
            Packet pkt((uint8_t*) test_str_payload.data(), test_str_payload.size(), PacketType::text_plain, PacketCharSet::utf8, PacketEncoding::elwin_enc, PacketCompression::_7zip, PacketEncryption::LAST);
            if(!verify_payload_integrity(&pkt, (uint8_t*) test_str_payload.data(), test_str_payload.size())) return false;
            client.send_pkt(pkt);
            pkts_sent.push_back(pkt);
            test_str_payload += get_all_ascii_chars();
        }
        sleep(2);
        
        
        if(pkts_received.size() !=  pkts_sent.size()) return false;

        for(int i = 0; i < pkts_sent.size(); ++i){
            Packet orig_pkt = pkts_sent.at(i);
            shared_ptr<Packet> received_pkt = pkts_received.at(i);

            if(!verify_header_integrity(received_pkt.get(),  (uint8_t*)  orig_pkt.get_header(),  orig_pkt.get_full_header_len())) return false;
            if(!verify_payload_integrity(received_pkt.get(), (uint8_t*)  orig_pkt.get_payload(), orig_pkt.get_payload_len())) return false;

        }
        
        //peer2.send_pkt_to_peer(TEST_ADDRESS, pkt);
        sleep(1);

        server.stop();
        //peer2.stop();
        sleep(2);
        return true;
    } catch (GenericException &e) {
        std::cout << e.what();
        return false;

    }
    return false;
}


bool test_p2p_connection(){
    try{
        
        P2PurrServer peer1(TEST_ADDRESS, TEST_PORT1);
        P2PurrServer peer2(TEST_ADDRESS, TEST_PORT2);
        peer1.start();
        sleep(2);

        peer2.start();
        sleep(2);

        peer2.connect_to_peer(TEST_ADDRESS, TEST_PORT1);
        sleep(1);


        peer1.stop();
        peer2.stop();
        sleep(1);
        return true;
    } catch (GenericException &e) {
        std::cout << e.what();
        return false;
    } 
    return false;
}

bool test_p2p_communication(){


    return false;
}

bool server_test_all(){
    bool success = true;

    success = success && test_create_and_close_socket();
    success = success && test_server_init();
    success = success && test_server_start_stop();
    success = success && test_server_client_connection();
    success = success && test_multiple_client_connected_no_sending_data();
    success = success && test_multiple_client_connected_no_sending_data2();
    success = success && test_server_send_receive();
    success = success && test_multiple_client_send();
    success = success && test_p2p_connection();
    success = success && test_p2p_communication();



    return success;
}