#include "../include/packet.h"


Packet::Packet(const Packet &other){
  

  this->type = other.type;
  this->charset = other.charset;
  this->encoding = other.encoding;
  this->encryption = other.encryption; 
  this->compression = other.compression;

  this->attachment_file_path = other.attachment_file_path; 
  this->has_file_attachment = other.has_file_attachment;

  
  
  
  
  this->built_pkt = other.built_pkt;
  this->header = other.header;
  this->payload = other.payload;
  
  this->pkt_len = other.pkt_len;
  this->hdr_len = other.hdr_len;
  this->full_hdr_len = other.full_hdr_len;
  this->payload_len = other.payload_len;
  

  this->buffer = other.buffer;
  this->curr_buff_idx = other.curr_buff_idx;
  this->buff_total_len = other.buff_total_len;
  this->buff_local_len = other.buff_local_len;
  this->buff_pos_start = other.buff_pos_start;
  //char *buff_start;
  this->buff_next = other.buff_next;
  //int buff_pos_end;
  this->num_bytes_read = other.num_bytes_read;

  this->pkt_built = other.pkt_built;
  this->pkt_rebuilding = other.pkt_rebuilding;

}


Packet::Packet(uint8_t *payload, int payload_len, PacketType ct, PacketCharSet cs, PacketEncoding enc, PacketCompression compr, PacketEncryption encr, string attachment_file_path){
  if(payload == nullptr) {
    throw PacketException("cant create packet. payload is null\n");\
  }

  this->type        = ct;
  this->charset     = cs;
  this->encoding    = enc;
  this->encryption  = encr; 
  this->compression = compr;


  
  this->payload_len = payload_len; //TODO: will break
  this->payload = shared_ptr<uint8_t>((uint8_t*) malloc(this->payload_len + 1));
  this->payload.get()[this->payload_len] = '\0';
  memcpy(this->payload.get(), payload, this->payload_len);

  this->built_pkt = shared_ptr<uint8_t>(nullptr);
  this->header = shared_ptr<uint8_t>(nullptr);

  this->has_file_attachment = false;
  if(attachment_file_path.compare("")) { // and path is valid i.e file exists
    this->has_file_attachment = true;
  }
  this->keepalive = true;
}


Packet::Packet(shared_ptr<uint8_t> buffer, shared_ptr<int> curr_buff_idx, int buff_total_len){
  this->buffer = buffer;
  //this->rebuilding_in_progress = false;
  this->curr_buff_idx = curr_buff_idx;
  this->buff_total_len = buff_total_len;
  

  this->update_local_buff_info();

  this->pkt_built = false;
  this->pkt_rebuilding = false;
  this->has_file_attachment = false;


  this->payload   = shared_ptr<uint8_t>(nullptr);
  this->built_pkt = shared_ptr<uint8_t>(nullptr);
  this->header    = shared_ptr<uint8_t>(nullptr);

  this->pkt_len = 0;
  this->hdr_len = 0;
  this->full_hdr_len = 0;
  this->payload_len = 0;
  this->num_bytes_read = 0;
  //this->keepalive = true;
  this->set_build_pending(false);

}

Packet::~Packet(){

}

uint8_t* Packet::build(){
  
  if(this->built_pkt.get() != nullptr) return this->built_pkt.get();

  this->set_build_pending(true);
  
  this->build_header();

  this->pkt_len = sizeof(char) + sizeof(uint16_t) + this->full_hdr_len + this->payload_len;
  
  this->make_pkt();

  this->set_build_pending(false);
  //printf("pkt: %s\n", this->built_pkt.get());
   

  return this->built_pkt.get();

}

void Packet::build_header(){
  //header len will be num header properties(5) + payload_len(4)
  
  this->full_hdr_len = 12;
  this->hdr_len = 9;
  this->header = shared_ptr<uint8_t>((uint8_t*) malloc( this->full_hdr_len));

  //int header_data_len = htons(this->full_hdr_len - 3); 

  uint8_t *h = (uint8_t*) this->header.get();
  *h++ = 0xFF;
  Serializer::serialize_int16(h, this->hdr_len);
  h += sizeof(uint16_t);
  *h++  = (uint8_t) this->type;
  *h++  = (uint8_t) this->charset;
  *h++  = (uint8_t) this->encoding;
  *h++  = (uint8_t) this->encryption;
  *h++  = (uint8_t) this->compression;
  Serializer::serialize_int32(h, this->payload_len);


  //printf("header: %s\n",(char*) this->header.get());

}

bool Packet::header_ready(){


  int idx = *(this->curr_buff_idx);

  bool special_case = false;
  if(this->buff_total_len < 3 && this->num_bytes_read < 3) special_case = true;
  
  
  if(*(this->buff_next) == 0xFF || this->num_bytes_read < this->full_hdr_len || special_case){
    if(this->full_hdr_len == 0) this->full_hdr_len = 3;
    uint8_t *hdr_ptr = nullptr;
    uint8_t tmp_header[3];
    if(this->header.get() == nullptr){
      hdr_ptr = tmp_header;
    } else {
      hdr_ptr = this->header.get() + this->num_bytes_read;
    }

    //gotta find a way to save hdr_ptr for next iteration for buffer size less than 3
    while((idx < this->buff_total_len && this->num_bytes_read < this->full_hdr_len) ){
      *hdr_ptr++ = *(this->buff_next)++;
      
      if(this->num_bytes_read == 2){
        this->hdr_len = tmp_header[1];
        this->hdr_len = (this->hdr_len << 8) | tmp_header[2];
        //this->hdr_len = ntohs(this->hdr_len); //+ 3;
        //this->hdr_len = Serializer::deserialize_int16(tmp_header);

        this->full_hdr_len = this->hdr_len + 3;
        if(this->full_hdr_len <= 0) throw PacketException("Got a header length of zero or less\n");
        this->header = shared_ptr<uint8_t>((uint8_t*) malloc(this->full_hdr_len));
        memcpy(this->header.get(), tmp_header, sizeof(tmp_header));
        hdr_ptr = this->header.get() + this->num_bytes_read + 1;
        
      }
      (this->num_bytes_read)++;
      
      idx++;
      //if(this->num_bytes_read >= this->buff_total_len) special_case = false;
    }
  }

  

  if(idx >= this->buff_total_len || this->num_bytes_read < this->full_hdr_len){
    if(this->full_hdr_len <= 3) this->full_hdr_len = 0;
    this->update_buff_idx();
    this->set_build_pending(true);
    return false;
  }

  *(this->curr_buff_idx) = idx;


  return this->num_bytes_read >= this->full_hdr_len;

}

void Packet::rebuild_header(){
  
  

  uint8_t *hdr = this->header.get();

  uint8_t start_symbol = *hdr++;

  this->hdr_len = Serializer::deserialize_int16(hdr);
  hdr += sizeof(uint16_t);

  if(this->hdr_len <= 0) throw PacketException("Got a header length of zero or less\n");
  //if(this->hdr_len > remaining_buff_len) throw PacketException("Header length is bigger than the remaining buffer\n");


  //printf("Rebuilding header of length %d\n", this->hdr_len);

  this->type = (PacketType) *hdr++;
  this->charset = (PacketCharSet) *hdr++;
  this->encoding = (PacketEncoding) *hdr++;
  this->encryption = (PacketEncryption) *hdr++;
  this->compression = (PacketCompression) *hdr++;

  this->payload_len = Serializer::deserialize_int32(hdr);

  if(this->payload_len <= 0) throw PacketException("Got a payload length of zero or less\n");

  if(this->payload.get() == nullptr){
    this->payload = shared_ptr<uint8_t>((uint8_t*) malloc(this->payload_len));
    memset(this->payload.get(), '\0', this->payload_len);
  }

  this->update_buff_idx();
  
}

void Packet::rebuild(){
  //if(this->buff_local_len < 2) throw PacketException("Received data is too small to process. can't even get header size\n");
  this->update_local_buff_info();
  //int remaining_buff_len = this->buff_total_len - *(this->curr_buff_idx);
  
  bool build_pending = true;


  if(!(this->header_ready())){
    return;
  }

  this->rebuild_header();
  
  int idx = *(this->curr_buff_idx);


  this->pkt_len = this->full_hdr_len + this->payload_len;

  int num_payload_bytes_read = this->num_bytes_read - this->full_hdr_len;
  uint8_t *pld =  this->payload.get() + num_payload_bytes_read;
  
  while((idx < this->buff_total_len) && (this->num_bytes_read < this->pkt_len)){
    *pld++ = *(this->buff_next)++;
    (this->num_bytes_read)++;
    idx++;
  }
  *(this->curr_buff_idx) = idx;

  int num_bytes_remaining = this->pkt_len - this->num_bytes_read;
  if (num_bytes_remaining > 0){
    build_pending = true;
  } else if(num_bytes_remaining == 0){
    build_pending = false;
    this->make_pkt();
  } else if(num_bytes_remaining < 0){
    throw PacketException("Something is wrong number of bytes remaining is negative\n");
  }

  

  this->update_buff_idx();
  this->set_build_pending(build_pending);

}


void Packet::make_pkt(){

  this->built_pkt = shared_ptr<uint8_t> ((uint8_t*) malloc(this->pkt_len));
  uint8_t *pkt = this->built_pkt.get();
  memcpy(pkt, this->header.get(), this->full_hdr_len );
  pkt += this->full_hdr_len;
  memcpy(pkt, this->payload.get(), this->payload_len);

}


void Packet::update_buff_idx(){
    int *curr_idx = this->curr_buff_idx.get();
    uint8_t *buff_head = this->buffer.get();
    *curr_idx = (this->buff_next - buff_head)  / sizeof(char);

    if(*curr_idx > this->buff_total_len){
      throw PacketException("Buffer index  is over the maximum buffer size of \n");
    }

}

void Packet::update_local_buff_info(){

  this->buff_pos_start = *curr_buff_idx;
  this->buff_next =  this->buffer.get() + *curr_buff_idx;
  //this->buff_local_len = strlen((char*) this->buff_next); 

}

bool Packet::is_build_done() {
  return this->pkt_built; 
}
bool Packet::is_build_pending() { 
  return this->pkt_rebuilding; 
}

void Packet::set_build_done(bool is_build_done) { 
  this->pkt_built = is_build_done; 
  this->pkt_rebuilding = !is_build_done; 
}
void Packet::set_build_pending(bool is_pending) { 
  this->pkt_rebuilding = is_pending; 
  this->pkt_built = !is_pending; 
}

uint8_t* Packet::get_payload() {  
  return this->payload.get();   
}
uint8_t* Packet::get_packet()  {
  return this->built_pkt.get(); 
}

int Packet::get_pkt_len(){ 
  return this->pkt_len; 
}