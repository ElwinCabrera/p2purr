#ifndef PACKET_HPP
#define PACKET_HPP

#include "p2pconn.h"
#include "exceptionhandler.h"


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
  
  int pkt_len;
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

public:
  Packet(uint8_t *payload, PacketType ct, PacketCharSet cs, PacketEncoding enc, PacketCompression compr, PacketEncryption encr, string attachment_file_path = "");
  Packet(shared_ptr<uint8_t> buffer, shared_ptr<int> curr_buff_idx, int buff_total_len);
  Packet(void *buffer);
  //Packet() {}
  ~Packet();
  uint8_t* build();
  void build_header();
  void rebuild_header();
  void rebuild();


  bool is_build_done();
  bool is_build_pending();
  void set_build_done(bool is_build_done);
  void set_build_pending(bool is_pending);
  void update_buff_idx();
  void update_local_buff_info();
  uint8_t* get_payload();
  uint8_t* get_packet();
  int get_pkt_size();
};



#endif // PACKET_HPP