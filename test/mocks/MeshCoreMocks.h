/**
 * @file MeshCoreMocks.h
 * @brief Mock implementations of MeshCore classes for native testing
 */

#pragma once

#include <cstdint>
#include <cstring>

// Mock mesh namespace
namespace mesh {

// Forward declarations
class Packet;
struct GroupChannel;

// Mock Utils
class Utils {
public:
    static void sha256(uint8_t *out, size_t outLen, const uint8_t *in, int inLen) {
        // Simple mock - just copy/truncate
        if (outLen >= 32 && inLen > 0) {
            for (size_t i = 0; i < 32; i++) {
                out[i] = (i < (size_t)inLen) ? in[i] ^ 0x5A : i;
            }
        }
    }
};

// Mock LocalIdentity
class LocalIdentity {
public:
    uint8_t pub_key[32] = {0};
    
    void sign(uint8_t *sig, const uint8_t *msg, int msgLen) const {
        // Mock signature - just fill with pattern
        if (sig && msg && msgLen > 0) {
            for (int i = 0; i < 64; i++) {
                sig[i] = (i < msgLen) ? msg[i] ^ 0xAA : i;
            }
        }
    }
};

// Mock Packet
class Packet {
public:
    uint8_t *payload = nullptr;
    size_t payloadLen = 0;
};

// Mock Mesh
class Mesh {
public:
    Packet* createGroupDatagram(uint8_t type, const GroupChannel &channel, 
                                const uint8_t *data, size_t len) {
        (void)type; (void)channel;
        // Return a mock packet
        Packet *pkt = new Packet();
        if (data && len > 0) {
            pkt->payload = new uint8_t[len];
            memcpy(pkt->payload, data, len);
            pkt->payloadLen = len;
        }
        return pkt;
    }
    
    void sendFlood(Packet *pkt, unsigned int delay) {
        (void)delay;
        // Mock send - just delete the packet
        if (pkt) {
            delete[] pkt->payload;
            delete pkt;
        }
    }
};

} // namespace mesh
