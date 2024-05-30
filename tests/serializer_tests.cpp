#include "test_globals.h"
#include "test_helpers.h"
#include "../include/serializer.h"

#include <vector>
#include <limits.h>


bool test_serializer_int16(){
    uint16_t max_int = 0xFF;
    int expected = 0;
    while(expected != max_int){
        uint8_t buff[sizeof(uint16_t)];
        Serializer::serialize_int16(buff, expected);
        uint16_t res = (uint16_t) Serializer::deserialize_int16(buff);
        if(res != expected) return false;
        ++expected; 
    }
    return true;

}

bool test_serializer_int32(){
    std::vector<uint> test_cases;
    test_cases.push_back(0);
    test_cases.push_back(0xFF000000);
    test_cases.push_back(0x00FF0000);
    test_cases.push_back(0x0000FF00);
    test_cases.push_back(0x000000FF);
    test_cases.push_back(0xFF0000FF);
    test_cases.push_back(0x00FFFF00);
    test_cases.push_back(0xFFFFFFFF);

    for(uint expected: test_cases){
        uint8_t buff[sizeof(uint)];
        Serializer::serialize_int32(buff, expected);
        uint res = (uint) Serializer::deserialize_int32(buff);
        if(res != expected) return false; 
        ++expected;   
    }
    return true;
    
}

bool test_serializer_int64(){
    
    std::vector<uint> test_cases;
    test_cases.push_back(0);
    test_cases.push_back(0xFF00000000000000);
    test_cases.push_back(0x00FF000000000000);
    test_cases.push_back(0x0000FF0000000000);
    test_cases.push_back(0x000000FF00000000);
    test_cases.push_back(0x00000000FF000000);
    test_cases.push_back(0x0000000000FF0000);
    test_cases.push_back(0x000000000000FF00);
    test_cases.push_back(0x00000000000000FF);
    test_cases.push_back(0xFF000000000000FF);
    test_cases.push_back(0x00FF00000000FF00);
    test_cases.push_back(0x0000FF0000FF0000);
    test_cases.push_back(0x000000FFFF000000);
    test_cases.push_back(0xFF0000FFFF0000FF);
    test_cases.push_back(0xFFFFFFFFFFFFFFFF);

    for(uint expected: test_cases){
        uint8_t buff[sizeof(uint)];
        Serializer::serialize_int32(buff, expected);
        uint res = (uint) Serializer::deserialize_int32(buff);
        if(res != expected) return false; 
        ++expected;   
    }
    return true;
    
}





bool test_serializer_float() {

  //  U u;
    
  //   u.f = 10.0;
    
  //   printf("%g = %#x\n", u.f, u.i);

  //   double num = 3.14545304958;
  //   double num_frac = num - (int) num;
  //   int num_whole = (int) (num - num_frac); 
  //   printf("Number = %g, Integer = %g, Fraction = %d\n", num, num_frac, num_whole);

  float f = 1738.145;
  //long double f = 3.1415928;
  long long sign = (f < 0) ? 1 : 0;
  long long exponent = ilogb(f);
  long double f_norm = scalbn(f, -1*exponent);
  //normalize f without direct bit manipulation
  long double mantissa = f_norm - 1;

  long double f_whole_part;
  long double f_frac_part = modf(f, &f_whole_part);

  cout << "Given the number " << f << "\n";
  cout << "sign: " << sign <<"\n";
  cout << "exponent: " << exponent <<"\n";
  cout << "normalized: " << f_norm << "\n";
  cout << "mantissa: " << mantissa << "\n"; 
  cout << "modf() =  " << f_whole_part << " + " << f_frac_part << '\n';
  cout << "logb()/ilogb() make " << f_norm << " * " << std::numeric_limits<double>::radix << "^" << exponent << '\n';

  

  cout << "\n";
  long double original_f = ldexp(f_norm, exponent);

  f_whole_part;
  f_frac_part = modf(original_f, &f_whole_part);

  cout << "ldexp() (original unpacked) = " << original_f << "\n";
  cout << "modf(unpacked) =  " << f_whole_part << " + " << f_frac_part << '\n';


  long long biased_exponent = exponent + 127;
  long long significand = static_cast<long long>(scalbn(f_norm - 1, 23)) ;
  
  long long res = (sign << 32) | (biased_exponent << 23) | significand;


  
  printf("Significand in long long %016X\n", significand);

  printf("Final serialized representation: %016X\n", res);
  

}


bool serializer_test_all(){

    return test_serializer_int16() && test_serializer_int32() && test_serializer_int64();

}