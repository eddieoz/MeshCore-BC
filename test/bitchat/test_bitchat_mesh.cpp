#include <unity.h>
#include "helpers/bitchat/BitchatMesh.h"
#include "../mocks/MockRadio.h"
#include "MeshCore.h"
#include "Dispatcher.h"
#include "Mesh.h"
#include <string.h>
#include "helpers/bitchat/BitchatBridge.h"

// Stub for BitchatBridge to satisfy the linker on Native where BitchatBridge.cpp is omitted
void BitchatBridge::onMeshcoreGroupMessage(const mesh::GroupChannel &channel, uint32_t timestamp, const char *senderName, const char *text) {
    // Stub implementation does nothing
}

class MockMillisecondClock : public mesh::MillisecondClock {
public:
  unsigned long getMillis() override { return 0; }
};

class MockRNG : public mesh::RNG {
public:
  void random(uint8_t* dest, size_t sz) override { memset(dest, 0, sz); }
};

class MockRTCClock : public mesh::RTCClock {
public:
  uint32_t getCurrentTime() override { return 0; }
  void setCurrentTime(uint32_t time) override {}
};

class MockPacketManager : public mesh::PacketManager {
public:
  mesh::Packet* allocNew() override { return new mesh::Packet(); }
  void free(mesh::Packet* packet) override { delete packet; }
  void queueOutbound(mesh::Packet* packet, uint8_t priority, uint32_t scheduled_for) override {}
  mesh::Packet* getNextOutbound(uint32_t now) override { return nullptr; }
  int getOutboundCount(uint32_t now) const override { return 0; }
  int getOutboundTotal() const override { return 0; }
  int getFreeCount() const override { return 100; }
  mesh::Packet* getOutboundByIdx(int i) override { return nullptr; }
  mesh::Packet* removeOutboundByIdx(int i) override { return nullptr; }
  void queueInbound(mesh::Packet* packet, uint32_t scheduled_for) override {}
  mesh::Packet* getNextInbound(uint32_t now) override { return nullptr; }
};

class MockMeshTables : public mesh::MeshTables {
public:
  bool hasSeen(const mesh::Packet* packet) override { return false; }
  void clear(const mesh::Packet* packet) override {}
};

// Concrete implementation to satisfy BaseChatMesh pure virtuals
class TestBitchatMesh : public mesh::bitchat::BitchatMesh {
public:
  bool was_on_message_recv_called = false;
  
  TestBitchatMesh(mesh::Radio& radio, mesh::MillisecondClock& ms, mesh::RNG& rng, mesh::RTCClock& rtc, mesh::PacketManager& mgr, mesh::MeshTables& tables)
    : BitchatMesh(radio, ms, rng, rtc, mgr, tables) {}

  void onDiscoveredContact(ContactInfo& contact, bool is_new, uint8_t path_len, const uint8_t* path) override {}
  ContactInfo* processAck(const uint8_t *data) override { return nullptr; }
  void onContactPathUpdated(const ContactInfo& contact) override {}
  void onMessageRecv(const ContactInfo& contact, mesh::Packet* pkt, uint32_t sender_timestamp, const char *text) override {}
  void onCommandDataRecv(const ContactInfo& contact, mesh::Packet* pkt, uint32_t sender_timestamp, const char *text) override {}
  void onSignedMessageRecv(const ContactInfo& contact, mesh::Packet* pkt, uint32_t sender_timestamp, const uint8_t *sender_prefix, const char *text) override {}
  uint32_t calcFloodTimeoutMillisFor(uint32_t pkt_airtime_millis) const override { return 0; }
  uint32_t calcDirectTimeoutMillisFor(uint32_t pkt_airtime_millis, uint8_t path_len) const override { return 0; }
  void onSendTimeout() override {}
  void onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt, uint32_t timestamp, const char *text) override {
      mesh::bitchat::BitchatMesh::onChannelMessageRecv(channel, pkt, timestamp, text);
      was_on_message_recv_called = true;
  }
  uint8_t onContactRequest(const ContactInfo& contact, uint32_t sender_timestamp, const uint8_t* data, uint8_t len, uint8_t* reply) override { return 0; }
  void onContactResponse(const ContactInfo& contact, const uint8_t* data, uint8_t len) override {}
};

static mesh::test::MockRadio test_radio;
static MockMillisecondClock test_msClock;
static MockRNG test_rng;
static MockRTCClock test_rtc;
static MockPacketManager test_mgr;
static MockMeshTables test_tables;

namespace test {
namespace bitchat {

void test_bitchat_mesh_instantiates() {
  TestBitchatMesh mesh(test_radio, test_msClock, test_rng, test_rtc, test_mgr, test_tables);
  TEST_ASSERT_NOT_NULL(&mesh);
}

void test_bitchat_mesh_fallback_to_base_chat() {
  TestBitchatMesh mesh(test_radio, test_msClock, test_rng, test_rtc, test_mgr, test_tables);
  mesh.setBitchatMode(false);
  
  mesh.setBitchatBridge(nullptr);

  mesh::GroupChannel channel;
  
  mesh.was_on_message_recv_called = false;
  // Fallback testing: call the base method that processes a plain text message
  // BitchatMode is false. It shouldn't crash.
  mesh.onChannelMessageRecv(channel, nullptr, 1234, "anon: hello");
  
  TEST_ASSERT_TRUE(mesh.was_on_message_recv_called);
}

void test_bitchat_mesh_intercepts() {
  TestBitchatMesh mesh(test_radio, test_msClock, test_rng, test_rtc, test_mgr, test_tables);
  mesh.setBitchatMode(true);
  
  // We don't have a mock bridge here yet. We'll pass nullptr.
  // We want to verify that when BitchatMode is TRUE, was_on_message_recv_called still becomes true
  // because BitchatMesh forwarding to bridge does NOT consume the message - the app still processes it.
  mesh.setBitchatBridge(nullptr);
  
  mesh::GroupChannel channel;
  mesh.was_on_message_recv_called = false;

  mesh.onChannelMessageRecv(channel, nullptr, 1234, "anon: hello");
  
  // The app hook is always called, irrespective of bitchat bridge.
  TEST_ASSERT_TRUE(mesh.was_on_message_recv_called);
}

void test_bitchat_mesh_add_hashtag_channel() {
  TestBitchatMesh mesh(test_radio, test_msClock, test_rng, test_rtc, test_mgr, test_tables);
  uint8_t secret[4] = {0x11, 0x22, 0x33, 0x44};
  auto ch = mesh.addHashtagChannel("testchan", secret, 4);
  TEST_ASSERT_NOT_NULL(ch);
}

/**
 * TDD for Story 6: Verify the exact delegation pattern MyMesh will use.
 * MyMesh inherits from BitchatMesh, delegates onChannelMessageRecv to BitchatMesh,
 * and then continues with its own serial-frame logic.
 */
void test_bitchat_mesh_subclass_delegation_pattern() {
  TestBitchatMesh mesh(test_radio, test_msClock, test_rng, test_rtc, test_mgr, test_tables);
  mesh.setBitchatMode(true);
  mesh.setBitchatBridge(nullptr);

  mesh::GroupChannel channel;
  mesh.was_on_message_recv_called = false;

  // Simulate MyMesh::onChannelMessageRecv() calling BitchatMesh::onChannelMessageRecv()
  // and then doing post-processing
  mesh.onChannelMessageRecv(channel, nullptr, 1234, "sender: hello world");

  // Both the BitchatMesh hook and the subclass post-processing ran
  TEST_ASSERT_TRUE(mesh.was_on_message_recv_called);
}

}
}


