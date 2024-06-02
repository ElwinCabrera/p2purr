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

    try{
        init_signal_handler();
        if(serializer_test_all()){
            printf("Serializer tests pass\n");
        } else {
            printf("Serializer tests fail\n");
            exit(EXIT_FAILURE);
        }
    
        if(packet_test_all()){
            printf("Packet tests pass\n");
        } else {
            printf("Packet tests fail\n");
            exit(EXIT_FAILURE);
        }

        if(server_test_all()){
            printf("Server tests pass\n");
        } else {
            printf("Server tests fail\n");
            exit(EXIT_FAILURE);
        }

    } catch(std::exception& e) {
        //Other errors
        std::cout << "System error occured errno: " << errno << " -> " << strerror(errno) << std::endl;
        std::cout << e.what() << std::endl;
        printf("One or more test failed\n");
        exit(EXIT_FAILURE);
    } catch (...) {
        printf("An error occured but, dont know why. You are on your own");
        printf("One or more test failed\n");
        exit(EXIT_FAILURE);
    }

   

    printf("All tests pass!\n");
    return 0;
}
