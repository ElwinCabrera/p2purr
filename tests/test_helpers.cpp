#include "test_globals.h"

//#include <stdlib.h>
#include <cstdint>
#include <iostream>

#include <cstdio>
#include <fstream>

using std::ifstream;
using std::ofstream;


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


void output_to_file(string file_name, char *data, int size){
    
  ofstream out_file(file_name, std::ios::out | std::ios::binary);
    
  if(out_file.is_open()) {
    out_file.write(data, size); // Writing bytes from 'data' array
    out_file.close();
  } else {
    printf("Unable to open file for writing!\n");
  }

}


bool compare_files_equal(string file_path1, string file_path2){
    
  
  ifstream in_file1(file_path1, std::ios::in | std::ios::binary);
  ifstream in_file2(file_path2, std::ios::in | std::ios::binary);
    
  if(in_file1.is_open() && in_file2.is_open()) {
     
    char buffer1[1024];
    char buffer2[1024];
    size_t bytes_read1 = 0;
    size_t bytes_read2 = 0;
        
    while(!in_file1.eof() && !in_file2.eof()) {
      in_file1.read(buffer1, sizeof(buffer1)); 
      in_file2.read(buffer2, sizeof(buffer2));

      if(in_file1.gcount() != in_file2.gcount()) return false;
      int idx = 0;
      while(idx < sizeof(buffer1)){
        if(buffer1[idx] != buffer2[idx]) return false;
        ++idx;
      }

      bytes_read1 += in_file1.gcount();
      bytes_read2 += in_file2.gcount();
    }
    in_file1.close();
    in_file2.close();

    if(bytes_read1 != bytes_read2) return false; //kinda redundant bc we are already checking inside for loop

  } else {
    printf("Unable to open file for writing!\n");
  }

  return true;

}

void delete_file(string file_name){
    int result = std::remove(file_name.data());
    if(result == 0) {
        //printf("File successfully deleted.\n");
    } else {
        printf("Error deleting file.\n");
    }

}
