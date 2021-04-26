#ifndef __PACKET_HEADER__
#define __PACKET_HEADER__

#include <string.h>

namespace relay {
namespace communication {

constexpr size_t header_size = 18;
constexpr size_t packet_size = 512;

struct PacketHeader {
  uint32_t id;
  uint8_t packet_num;
  uint8_t packet_index;
  uint32_t packet_size;
  uint64_t timestamp;

  void makePacketHeader(char* buffer) {
    memcpy(buffer, &id, 4);
    memcpy(buffer + 4, &packet_num, 1);
    memcpy(buffer + 5, &packet_index, 1);
    memcpy(buffer + 6, &packet_size, 4);
    memcpy(buffer + 10, &timestamp, 8);
  }

  static PacketHeader parsePacketHeader(const char* buffer) {
    PacketHeader res;
    memcpy(&res.id, buffer, 4);
    memcpy(&res.packet_num, buffer + 4, 1);
    memcpy(&res.packet_index, buffer + 5, 1);
    memcpy(&res.packet_size, buffer + 6, 4);
    memcpy(&res.timestamp, buffer + 10, 8);
    return res;
  }
};

}
}

#endif