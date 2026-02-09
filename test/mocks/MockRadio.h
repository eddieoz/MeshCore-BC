#pragma once

#include <Dispatcher.h>
#include <vector>
#include <queue>
#include <cstring>

namespace mesh {
namespace test {

class MockRadio : public Radio {
public:
    struct PacketData {
        uint8_t data[256];
        int length;
        uint32_t timestamp;
    };

    std::queue<PacketData> rxQueue;
    std::vector<PacketData> txHistory;
    bool _isReceiving = false;
    uint32_t _airtime = 100; // ms

    void begin() override {}

    int recvRaw(uint8_t* bytes, int sz) override {
        if (rxQueue.empty()) {
            return 0;
        }

        PacketData& pkt = rxQueue.front();
        int len = pkt.length;
        if (len > sz) len = sz;
        
        memcpy(bytes, pkt.data, len);
        rxQueue.pop();
        return len;
    }

    uint32_t getEstAirtimeFor(int len_bytes) override {
        return _airtime;
    }

    float packetScore(float snr, int packet_len) override {
        return 1.0f; // Perfect score
    }

    bool startSendRaw(const uint8_t* bytes, int len) override {
        PacketData pkt;
        if (len > sizeof(pkt.data)) len = sizeof(pkt.data);
        memcpy(pkt.data, bytes, len);
        pkt.length = len;
        pkt.timestamp = 0; // Set by caller if needed
        txHistory.push_back(pkt);
        return true;
    }

    bool isSendComplete() override {
        return true; // Always complete immediately
    }

    void onSendFinished() override {}

    bool isInRecvMode() const override {
        return true;
    }

    bool isReceiving() override {
        return _isReceiving;
    }

    // Helper methods for testing
    void queuePacket(const uint8_t* data, int len) {
        PacketData pkt;
        if (len > sizeof(pkt.data)) len = sizeof(pkt.data);
        memcpy(pkt.data, data, len);
        pkt.length = len;
        rxQueue.push(pkt);
    }

    bool hasSentPacket() const {
        return !txHistory.empty();
    }

    PacketData getLastSentPacket() {
        if (txHistory.empty()) {
            PacketData empty;
            empty.length = 0;
            return empty;
        }
        return txHistory.back();
    }

    void clear() {
        while(!rxQueue.empty()) rxQueue.pop();
        txHistory.clear();
    }
};

} // namespace test
} // namespace mesh
