#include <stdint.h>
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <arpa/inet.h>
#endif
#include <unistd.h>

#include "varnum.h"
#include "globals.h"
#include "tools.h"

int32_t readVarInt (int client_fd) {
  int32_t value = 0;
  int position = 0;
  uint8_t byte;

  while (true) {
    byte = readByte(client_fd);
    if (recv_count != 1) return VARNUM_ERROR;

    value |= (byte & SEGMENT_BITS) << position;

    if ((byte & CONTINUE_BIT) == 0) break;

    position += 7;
    if (position >= 32) return VARNUM_ERROR;
  }

  return value;
}

int sizeVarInt (uint32_t value) {
  int size = 1;
  while ((value & ~SEGMENT_BITS) != 0) {
    value >>= 7;
    size ++;
  }
  return size;
}

void writeVarInt (int client_fd, uint32_t value) {
  while (true) {
    if ((value & ~SEGMENT_BITS) == 0) {
      writeByte(client_fd, value);
      return;
    }

    writeByte(client_fd, (value & SEGMENT_BITS) | CONTINUE_BIT);

    value >>= 7;
  }
}

int sizeVarLong (uint64_t value) {
  int size = 1;
  while ((value & ~(uint64_t)SEGMENT_BITS) != 0) {
    value >>= 7;
    size ++;
  }
  return size;
}

void writeVarLong (int client_fd, uint64_t value) {
  while (true) {
    if ((value & ~(uint64_t)SEGMENT_BITS) == 0) {
      writeByte(client_fd, value);
      return;
    }

    writeByte(client_fd, (value & SEGMENT_BITS) | CONTINUE_BIT);

    value >>= 7;
  }
}

// Reads and discards a low-precision Vec3 ("LpVec3")
// A zero vector is a single 0 byte, otherwise it takes 6 bytes,
// followed by a VarInt if the scale doesn't fit in 2 bits
int skipLpVec3 (int client_fd) {
  uint8_t lowest = readByte(client_fd);
  if (recv_count != 1) return 1;
  if (lowest == 0) return 0;
  readByte(client_fd);
  readUint32(client_fd);
  if (recv_count == -1) return 1;
  if (lowest & 4) {
    if (readVarInt(client_fd) == VARNUM_ERROR) return 1;
  }
  return 0;
}
