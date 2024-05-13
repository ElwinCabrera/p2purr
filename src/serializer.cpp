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

void test() {

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
