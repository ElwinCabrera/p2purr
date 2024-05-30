#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <iostream> 
#include <stdio.h> 
#include <cstdint>
#include <algorithm>
#include <math.h>
//#include<cmath>
#include <limits>

//#include <stdlib.h>
//#include <cstdlib> // for exit() and EXIT_FAILURE
//#include <sys/types.h>
//#include <cstring>






using std::cout;
using std::endl;
using std::string;



#include "global_config.h"


class Serializer {

private:

    struct precision_info{
    int num_sign_bits = 1;
    int num_exponent_bits;
    int num_significand_bits;
    int exponent_bias;
    int total_num_bits;
    };

    uint8_t *buffer;
    size_t size;
    size_t capacity;

public:

  enum Precision{
    HALF_PRECISION = 16,
    SINGLE_PRECISION = 32,
    DOUBLE_PRECISION = 64,
    x86_EXTENDED_PRECISION = 80,
    QUAD = 128
  };





  /*
    Currently C/C++ does not provide a way to serialize floats so lets build our own float serializer.

    Because floating point numbers can be represented different across different computer architectures
    it is not guaranteed that if we send a float number it will be represented the same on the other side.
    In order to keep the integrety of the data we are sending we need to come up with a way to represent a 
    floating point number in binary, pack(or serialize) those bites in an integer, send it over, then 
    unpack(or deserialize) the integer that is representing our float number on the other side.

    
    Generally most computer architectures use the IEEE754 representation for floats and if you are trying to 
    avoid this serialization overhead then you can its probably safe to assume that the reciving machine also 
    represents floating point numbers the same way. However, we are not trying to make any assumption here so 
    to be safe we will serialize the data before sending and deserialize on the other end.

    Since we are trying to achive portability we need to avoid direct bit manipulation during serialization 
    because the way one machine represents a number like 3.14 might be different than the machine we are 
    sending our data to.

    This serializer will be following the IEEE754 almost universal standard for floating point representation.

    https://en.wikipedia.org/wiki/Floating-point_arithmetic
    https://en.wikipedia.org/wiki/IEEE_754 

    TODO: Handle +/- Zero, +/- Inf, NaN, and other special cases
    
  */
  uint64_t serialize_float(long double frac_num, Precision p );

  long double deserialize_float(uint64_t num, Precision p);

  struct precision_info get_precision_info(Precision p);

  // static uint16_t serialize_int16(int64_t num);
  // static uint32_t serialize_int32(int64_t num);
  // static uint64_t serialize_int64(int64_t num);
  
  // static int16_t deserialize_int16(int16_t num);
  // static int32_t deserialize_int32(int32_t num);
  // static int64_t deserialize_int64(int64_t num);


  static void serialize_int16(uint8_t *buff, int64_t num);
  static void serialize_int32(uint8_t *buff, int64_t num);
  static void serialize_int64(uint8_t *buff, int64_t num);

  static int16_t deserialize_int16(uint8_t *buff);
  static int32_t deserialize_int32(uint8_t *buff);
  static int64_t deserialize_int64(uint8_t *buff);

  void test();

};



#endif