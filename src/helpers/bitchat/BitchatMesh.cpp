#include "BitchatMesh.h"
#include "helpers/bitchat/BitchatBridge.h"
#include <string.h>

namespace mesh {
namespace bitchat {

ChannelDetails* BitchatMesh::addHashtagChannel(const char* name, const uint8_t* secret, uint8_t secret_len) {
#ifdef MAX_GROUP_CHANNELS
  if (num_channels < MAX_GROUP_CHANNELS && secret_len <= 32) {
    auto dest = &channels[num_channels];

    memset(dest->channel.secret, 0, sizeof(dest->channel.secret));
    memcpy(dest->channel.secret, secret, secret_len);
    mesh::Utils::sha256(dest->channel.hash, sizeof(dest->channel.hash), dest->channel.secret, secret_len);
    
    // Copy the channel name safely
    size_t i = 0;
    while (i < sizeof(dest->name) - 1 && name[i] != '\0') {
      dest->name[i] = name[i];
      i++;
    }
    dest->name[i] = '\0';
    
    num_channels++;
    return dest;
  }
#endif
  return nullptr;
}

void BitchatMesh::onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt, uint32_t timestamp, const char* text) {
    if (_bitchatMode && _bridge) {
        const char *colonPos = strchr(text, ':');
        if (colonPos != nullptr && colonPos > text) {
            char senderName[32];
            size_t senderLen = colonPos - text;
            if (senderLen >= sizeof(senderName)) senderLen = sizeof(senderName) - 1;
            memcpy(senderName, text, senderLen);
            senderName[senderLen] = '\0';

            const char *msgText = colonPos + 1;
            while (*msgText == ' ') msgText++;

            _bridge->onMeshcoreGroupMessage(channel, timestamp, senderName, msgText);
        } else {
            _bridge->onMeshcoreGroupMessage(channel, timestamp, "Unknown", text);
        }
    }
}

} // namespace bitchat
} // namespace mesh
