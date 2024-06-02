#include "test_globals.h"

//#include <stdlib.h>
#include <cstdint>
#include <iostream>


string get_all_ascii_chars(){
    string all_ascii = "";
    uint8_t c = 1;
    while(c < 255){
        all_ascii += (char) c;
        ++c;
    }
    return all_ascii;
}


uint generate_random_num(int min, int max){
    srand(time(NULL));
    return (rand() % max) + min;
}
