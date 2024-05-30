#include "../include/serializer.h"


uint64_t Serializer::serialize_float(long double frac_num, Serializer::Precision p ) {
    
    struct precision_info p_info = get_precision_info(p);
    long double float_norm;
    long long sign, exponent, biased_exponent, significand;

    
    if(frac_num == 0.0){
      return 0;
    }
    if(frac_num < 0){
      sign = 1;
      frac_num *= -1;
    }

    exponent = ilogb(frac_num);
    float_norm = scalbn(frac_num, exponent * -1);
    float_norm -= 1;  // the leading 1 bit is implicit so we leave it out and gain an extra bit for our fraction
    
    biased_exponent = exponent + p_info.exponent_bias;
    significand = static_cast<long long>(scalbn(float_norm, p_info.num_significand_bits)) ;


    return (sign << p_info.total_num_bits) | (biased_exponent << p_info.num_significand_bits) | significand;

}






long double Serializer::deserialize_float(uint64_t num, Serializer::Precision p){

    struct precision_info p_info = get_precision_info(p);
    long long sign, exponent, biased_exponent, significand;
    
    long double original_num, significand_norm;

    sign =  (num >> p_info.total_num_bits);
    biased_exponent = 0;
    significand = 0 + 1; //TODO: need to figure out how to convert this to long double without bit manipulation

    exponent = biased_exponent - p_info.exponent_bias;
    significand_norm = scalbn(static_cast<long double>(significand), p_info.num_significand_bits * -1);

    original_num = ldexp(significand_norm, exponent);

    if(sign & 0x1){
      original_num *= -1;
    }

    return original_num;

}



struct Serializer::precision_info Serializer::get_precision_info(Serializer::Precision p){
    precision_info pi;

    pi.num_sign_bits = 1;

    switch(p){
      case SINGLE_PRECISION:
        pi.num_exponent_bits = 8;
        pi.num_significand_bits = 23;
        pi.total_num_bits = 32;
        pi.exponent_bias = 127;
        
        break;
      case DOUBLE_PRECISION:
        pi.num_exponent_bits = 11;
        pi.num_significand_bits = 52;
        pi.total_num_bits = 64;
        pi.exponent_bias = 1023;
        break;
      case x86_EXTENDED_PRECISION:
        pi.num_exponent_bits = 15;
        pi.num_significand_bits = 64;
        pi.total_num_bits = 80;
        pi.exponent_bias = 16383;
        break;
      case QUAD:
        pi.num_exponent_bits = 15;
        pi.num_significand_bits = 112;
        pi.total_num_bits = 128;
        pi.exponent_bias = 16383;
        break;

      default:
        break;

    }
    return pi;
}



 void Serializer::serialize_int16(uint8_t *buff, int64_t num) {
    //int header_data_len = htons(this->full_hdr_len - 3); 
    *buff++ = (num >> 8) & 0xFF;
    *buff++ = num & 0xFF; 

    //or least significant byte first little endian
    // *buff++ = num & 0xFF; 
    // *buff++ = (num >> 8) & 0xFF;

 }

void Serializer::serialize_int32(uint8_t *buff, int64_t num){
    int byte_idx = 3;
    while(byte_idx >= 0){
        *buff++  = (num >> (8 * byte_idx--)) & 0xFF;
    }
}

void Serializer::serialize_int64(uint8_t *buff, int64_t num){
    int byte_idx = 7;
    while(byte_idx >= 0){
        *buff++  = (num >> (8 * byte_idx--)) & 0xFF;
    }
}


int16_t Serializer::deserialize_int16(uint8_t *buff) {
    int16_t result = 0;
    result = *buff++;
    result = (result << 8) | *buff++;
    //result = ntohs(result); //+3;
    return result;
}

int32_t Serializer::deserialize_int32(uint8_t *buff) {
    int32_t result = 0;
    int bytes = 4;
    int i = 0;
    while(i++ < bytes){
        result = (result << 8) | *buff++;
    }
    return result;
}

int64_t Serializer::deserialize_int64(uint8_t *buff){
    int64_t result = 0;
    int bytes = 8;
    int i = 0;
    while(i++ < bytes){
        result = (result << 8) | *buff++;
    }
    return result;

}
