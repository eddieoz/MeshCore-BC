#pragma once

#include "helpers/BaseChatMesh.h"

class BitchatBridge; // Forward declaration

namespace mesh {
namespace bitchat {

class BitchatMesh : public BaseChatMesh {
    bool _bitchatMode;
    BitchatBridge* _bridge;

protected:
    BitchatMesh(mesh::Radio& radio, mesh::MillisecondClock& ms, mesh::RNG& rng, mesh::RTCClock& rtc, mesh::PacketManager& mgr, mesh::MeshTables& tables)
        : BaseChatMesh(radio, ms, rng, rtc, mgr, tables), _bitchatMode(true), _bridge(nullptr) {}

public:
    void setBitchatMode(bool enabled) { _bitchatMode = enabled; }
    bool isBitchatMode() const { return _bitchatMode; }

    void setBitchatBridge(BitchatBridge* bridge) { _bridge = bridge; }
    BitchatBridge* getBitchatBridge() const { return _bridge; }

    ChannelDetails* addHashtagChannel(const char* name, const uint8_t* secret, uint8_t secret_len);

    void onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt, uint32_t timestamp, const char* text) override;
};

} // namespace bitchat
} // namespace mesh
