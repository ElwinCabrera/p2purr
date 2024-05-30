#ifndef TEST_GLOBALS_H
#define TEST_GLOBALS_H

#include <cstdint>
#include <stdio.h>

#include "../include/packet.h"


//serializer tests
bool test_serializer_int16();
bool test_serializer_int32();
bool test_serializer_int64();
bool test_serializer_float();
bool serializer_test_all();



//Packet tests
bool verify_header_integrity(Packet pkt, uint8_t *expected_hdr_data, int len);
bool verify_payload_integrity(Packet pkt, uint8_t *expected_payload_data, int len);
Packet rebuild_pkt_from_pkt(Packet orig_pkt_recvd, int buff_size);
bool test_packet_data_integrity();
bool test_packet_circular_buffer();
bool packet_test_all();



//socket tests

#endif
