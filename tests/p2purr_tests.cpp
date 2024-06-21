#include <signal.h>

#include "test_globals.h"

void sig_handler(int s){
  printf("\nCaught signal(%d) SIGINT. Exiting gracefully\n", s);
  exit(1); 

}


 void init_signal_handler(){
  struct sigaction sigIntHandler;

  sigIntHandler.sa_handler = sig_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;

  sigaction(SIGINT, &sigIntHandler, NULL);
}

int main(int argc, char *argv[]) {

    bool success = true;

    try{
        init_signal_handler();


        // success = success && serializer_test_all();

        // if(success){
        //     printf("Serializer tests pass\n");
        // } else {
        //     printf("Serializer tests fail\n");
        //     exit(EXIT_FAILURE);
        // }
        // success = success && packet_test_all();
        // if(success){
        //     printf("Packet tests pass\n");
        // } else {
        //     printf("Packet tests fail\n");
        //     exit(EXIT_FAILURE);
        // }

        success = success && server_test_all();
        if(success){
            printf("Server tests pass\n");
        } else {
            printf("Server tests fail\n");
            exit(EXIT_FAILURE);
        }

    } catch(std::exception& e) {
        //Other errors
        std::cout << "System error occured errno: " << errno << " -> " << strerror(errno) << std::endl;
        std::cout << e.what() << std::endl;
        printf("Tests failed\n");
        exit(EXIT_FAILURE);
    } catch (...) {
        printf("An error occured but, dont know why. You are on your own");
        printf("Tests failed\n");
        exit(EXIT_FAILURE);
    }

   
    if(success){
        printf("All tests pass!\n");
    } else{
        printf("Tests failed\n");
    }
    
    return 0;
}
