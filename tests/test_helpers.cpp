#include "test_helpers.h"

string get_all_ascii_chars(){
    string all_ascii = "";
    uint8_t c = 0;
    while(c < 255){
        all_ascii += (char) c;
        ++c;
    }
    return all_ascii;
}


void g(){
    //  srand (time(NULL));

    // //between 1 and 10
    // int random_size = rand() % 10 + 1;
    // printf("list size should be %d\n", random_size);

    // HuffmanListTree hlt;
    // for(int i = 0; i < random_size; ++i){
    //     int random_num = rand() % 32 + 1;
    //     hlt.add_to_list('a' + random_num, random_num);
    // }
}
