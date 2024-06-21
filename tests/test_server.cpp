#include "../include/sockethelper.h"
#include "../include/p2purr_server.h"
#include "../include/global_config.h"
#include "test_globals.h"

#define TEST_ADDRESS "127.0.0.1"
#define TEST_PORT1 8001
#define TEST_PORT2 8002
#define MAX_CONNECTIONS 100

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
        
        while(i <= MAX_CONNECTIONS){
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
        
        while(i <= MAX_CONNECTIONS){
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
        
        while(i <= MAX_CONNECTIONS){
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

        for(size_t i = 0; i < pkts_sent.size(); ++i){
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


bool test_sending_large_file_attachment(){
     try{
        
        P2PurrServer server(TEST_ADDRESS, TEST_PORT1);
        server.start();
        sleep(1);

        PeerConnHandler client(TEST_ADDRESS, TEST_PORT1);
        client.connect();
        sleep(1);

        pkts_received.clear();

       

        string file_name = "test_send_file.txt";
        //string test_file_contents = get_all_ascii_chars();
        string test_file_contents = "";
        //size_t len_limit = 1073741824; // 1GB
        size_t len_limit = 200000; // 
        while(test_file_contents.size() <= len_limit){
            test_file_contents += get_all_ascii_chars();
        }
        output_to_file(file_name, test_file_contents.data(), test_file_contents.size());


        Packet pkt(file_name, PacketType::generic_file, PacketCharSet::utf8, PacketEncoding::elwin_enc, PacketCompression::_7zip, PacketEncryption::LAST);

        if(!verify_payload_integrity(&pkt, (uint8_t*) test_file_contents.data(), test_file_contents.size())) return false;

        client.send_pkt(pkt);
        sleep(10);

        

        if(pkts_received.size() > 1 || pkts_received.empty()) return false;
        
        
        shared_ptr<Packet> received_pkt = pkts_received.at(0);

        if(!verify_header_integrity(received_pkt.get(),  (uint8_t*)  pkt.get_header(),  pkt.get_full_header_len())) return false;
        if(!verify_payload_integrity(received_pkt.get(), (uint8_t*)  pkt.get_payload(), pkt.get_payload_len())) return false;
        if(!compare_files_equal(file_name, "out.txt")) return false;

        server.stop();
        delete_file(file_name);
        
        sleep(2);
        return true;
    } catch (GenericException &e) {
        std::cout << e.what();
        return false;
    }

    return false;

}

bool server_test_all(){
    
    // if(!test_create_and_close_socket()) {
    //     printf("Opening and Closing the socket failed\n");
    //     return false;
    // }
    // if(!test_server_init()){
    //     printf("Server initialization failed\n");
    //     return false;
    // }
    // if(!test_server_start_stop()) {
    //     printf("server smoke test failed\n");
    //     return false;
    // }

    
    // if(!test_server_client_connection()) {
    //     printf("server client connection failed\n");
    //     return false;
    // }

    
    // if(!test_multiple_client_connected_no_sending_data()) {
    //     printf("server multiple client connections failed (with server ending communications with clients)\n");
    //     return false;
    // };

    // if(!test_multiple_client_connected_no_sending_data2()) {
    //     printf("server multiple client connections failed (with clients ending communications with server)\n");
    //     return false;
    // };

    
    // if(!test_server_send_receive()) {
    //     printf("Server send and receive failed\n");
    //     return false;
    // }

    if(test_sending_large_file_attachment()){
        printf("Server sending large file attachment failed\n");
        return true;
    }

    
    // if(!test_multiple_client_send()) {
    //     printf("Server multiple client send failed\n");
    //     return false;
    // }

    
    // if(!test_p2p_connection()) {
    //     printf("p2p connection failed\n");
    //     return false;
    // }


    // if(!test_p2p_communication()) {
    //     printf("p2p communications failed\n")
    //     return false;
    // }

    return true;
}