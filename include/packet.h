#ifndef PACKET_HPP
#define PACKET_HPP

#include <iostream>
#include <memory>
#include <cstdint>
#include <string.h> //#include <cstring>

using std::string;
using std::shared_ptr;


#include "exceptionhandler.h"
#include "global_config.h"
#include "serializer.h"



  // For more info visit https://www.iana.org/assignments/media-types/media-types.xhtml
enum PacketType: uint8_t{
  text_plain = 1,
  text_csv = 2,
  text_xml = 3,
  application_json = 4,
  application_xml = 5,
  application_yamal = 6,
  application_pdf = 7,
  image_jpeg = 8,
  image_png = 9,
  image_gif = 10,
  image_tiff = 11,
  
  audio_mpeg = 13,
  audio_mp4 = 14,
  video_mp4 = 15,
  video_mpeg = 16,
 
};

enum PacketEncoding: uint8_t{
  base64 = 1,
  elwin_enc = 2,
};

enum PacketCompression: uint8_t{
  gzip = 1,
  lzma = 2,
  _7zip = 3,
};

enum PacketEncryption: uint8_t{
  
  LAST
};

enum PacketCharSet{
  iso8859_1 = 1,
  utf8= 2,
  ascii = 3,

  
};




class Packet{
private:
  
  PacketType type;
  PacketCharSet charset;
  PacketEncoding encoding;
  PacketEncryption encryption; 
  PacketCompression compression;

  string attachment_file_path;
  bool has_file_attachment;

  
  
  
  
  shared_ptr<uint8_t> built_pkt;
  shared_ptr<uint8_t> header;
  shared_ptr<uint8_t> payload;
  uint8_t tmp_hdr[3];
  
  int pkt_len;
  uint16_t full_hdr_len;
  uint16_t hdr_len;
  uint payload_len;
  

  shared_ptr<uint8_t> buffer;
  shared_ptr<int> curr_buff_idx;
  int buff_total_len;
  int buff_local_len;
  int buff_pos_start;
  //char *buff_start;
  uint8_t *buff_next;
  //int buff_pos_end;
  int num_bytes_read;

  bool pkt_built;
  bool pkt_rebuilding;

  

  bool keepalive;

  void make_pkt();

public:
  
  Packet(uint8_t *payload, int payload_len, PacketType ct, PacketCharSet cs, PacketEncoding enc, PacketCompression compr, PacketEncryption encr, string attachment_file_path = "");
  Packet(shared_ptr<uint8_t> buffer, shared_ptr<int> curr_buff_idx, int buff_total_len);
  //Packet(void *buffer);
  //Packet(const Packet &other); //copy constructor
  //Packet(Packet &&other); //move constructor
  //Packet() {}
  ~Packet();
  uint8_t* build();
  void build_header();
  bool header_ready();
  void rebuild_header();
  void rebuild();


  bool is_build_done();
  bool is_build_pending();
  void set_build_done(bool is_build_done);
  void set_build_pending(bool is_pending);
  void update_buff_idx();
  void update_local_buff_info();
  uint8_t* get_header() { return this->header.get(); }
  uint8_t* get_payload();
  uint8_t* get_packet();
  int get_header_len(){ return this->hdr_len; }
  int get_full_header_len(){ return this->full_hdr_len; }
  int get_payload_len() { return this->payload_len; }
  int get_pkt_len();
};


#endif // PACKET_HPP