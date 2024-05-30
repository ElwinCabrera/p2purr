#include "test_globals.h"


int main(int argc, char *argv[]) {

    if(serializer_test_all()){
        printf("Serializer tests pass\n");
    } else {
        printf("Serializer tests fail\n");
    }
    
    if(packet_test_all()){
        printf("Packet tests pass\n");
    } else {
        printf("Packet tests fail\n");
    }

    printf("Tests done\n");
    return 0;
}
