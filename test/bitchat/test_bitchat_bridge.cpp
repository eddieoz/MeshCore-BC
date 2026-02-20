/**
 * @file test_bitchat_bridge.cpp
 * @brief TDD tests for BitchatBridge integration layer
 *
 * Tests the BitchatBridge class which orchestrates BitChat BLE to MeshCore mesh.
 */

// Note: unity.h and standard types are included by test_main.cpp

#include "helpers/bitchat/BitchatBridge.h"
#include "helpers/bitchat/ChannelRegistry.h"
#include "helpers/bitchat/LoopPrevention.h"

namespace test {
namespace bitchat {

// Mock context for mesh send callback
static int mock_bridge_send_count = 0;
static uint8_t mock_bridge_last_channel_hash = 0;
static uint8_t mock_bridge_last_data[256];
static size_t mock_bridge_last_len = 0;

static bool mock_bridge_send_group_data(uint8_t channelHash, const uint8_t *data, size_t len,
                                        uint32_t timestamp, const char *senderName, void *ctx) {
  mock_bridge_send_count++;
  mock_bridge_last_channel_hash = channelHash;
  if (len <= sizeof(mock_bridge_last_data)) {
    memcpy(mock_bridge_last_data, data, len);
  }
  mock_bridge_last_len = len;
  return true;
}

static void reset_bridge_test_mocks() {
  mock_bridge_send_count = 0;
  mock_bridge_last_channel_hash = 0;
  mock_bridge_last_len = 0;
  memset(mock_bridge_last_data, 0, sizeof(mock_bridge_last_data));
}

/**
 * @test Bridge Initialization
 */
void test_bridge_initialization() {
  reset_bridge_test_mocks();

  BitchatBridge bridge;

  const char *nodeName = "TestNode";
  uint8_t pubKey[32] = { 0 };

  bridge.begin(nodeName, pubKey, mock_bridge_send_group_data, nullptr);

  // Bridge should have registered "mesh" channel by default (without #)
  TEST_ASSERT_TRUE(bridge.isInitialized());
  TEST_ASSERT_TRUE(bridge.hasChannel("mesh")); // Note: no # prefix
}

/**
 * @test BitChat to Mesh Message Flow
 */
void test_bridge_bitchat_to_mesh() {
  reset_bridge_test_mocks();

  BitchatBridge bridge;
  uint8_t pubKey[32] = { 0 };
  bridge.begin("TestNode", pubKey, mock_bridge_send_group_data, nullptr);

  // Use channel without # prefix to match registerDefaultMeshChannel
  const char *channel = "mesh";
  const char *message = "Hello from BitChat";
  uint32_t timestamp = 1700000000;
  uint8_t senderId[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

  bool result = bridge.onBitchatMessage(channel, message, timestamp, senderId);

  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL(1, mock_bridge_send_count);
  TEST_ASSERT_GREATER_THAN(0, mock_bridge_last_len);

  // Verify BC magic header
  TEST_ASSERT_EQUAL('B', mock_bridge_last_data[0]);
  TEST_ASSERT_EQUAL('C', mock_bridge_last_data[1]);
}

/**
 * @test Mesh to BitChat Message Flow
 */
void test_bridge_mesh_to_bitchat() {
  reset_bridge_test_mocks();

  BitchatBridge bridge;
  uint8_t pubKey[32] = { 0 };
  bridge.begin("TestNode", pubKey, mock_bridge_send_group_data, nullptr);

  // Create encapsulated BitChat packet with BC magic
  uint8_t encapsulated[] = { 'B', 'C', 0x00, 0x00, // Magic header
                             0x01,                 // Version
                             0x00,                 // Flags
                             0x00, 0x00,           // Reserved
                             // Unique payload for test
                             'H', 'e', 'l', 'l', 'o', 0x99, 0x88, 0x77 };

  uint8_t channelHash = 0x42;
  uint32_t timestamp = 1700000000;
  const char *senderName = "RemoteNode";

  int result =
      bridge.onMeshChannelMessage(channelHash, encapsulated, sizeof(encapsulated), timestamp, senderName);

  TEST_ASSERT_EQUAL(1, result);
  TEST_ASSERT_TRUE(bridge.hasPendingBitchatMessage());
}

/**
 * @test Loop Prevention
 */
void test_bridge_loop_prevention() {
  reset_bridge_test_mocks();

  BitchatBridge bridge;
  uint8_t pubKey[32] = { 0 };
  bridge.begin("TestNode", pubKey, mock_bridge_send_group_data, nullptr);

  // Send a message from BitChat
  const char *channel = "mesh";
  const char *message = "Test loop";
  uint32_t timestamp = 1700000000;
  uint8_t senderId[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

  bool sent = bridge.onBitchatMessage(channel, message, timestamp, senderId);
  TEST_ASSERT_TRUE(sent);
  TEST_ASSERT_EQUAL(1, mock_bridge_send_count);
}

/**
 * @test Non-BitChat Packets Ignored
 */
void test_bridge_handles_non_bitchat() {
  reset_bridge_test_mocks();

  BitchatBridge bridge;
  uint8_t pubKey[32] = { 0 };
  bridge.begin("TestNode", pubKey, mock_bridge_send_group_data, nullptr);

  // Regular MeshCore packet without BC magic
  uint8_t regular_packet[] = { 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd' };

  int result =
      bridge.onMeshChannelMessage(0x42, regular_packet, sizeof(regular_packet), 1700000000, "OtherNode");

  TEST_ASSERT_EQUAL(0, result);
  TEST_ASSERT_FALSE(bridge.hasPendingBitchatMessage());
}

/**
 * @test Unknown Channel Routing
 */
void test_bridge_unknown_channel_routes_to_mesh() {
  reset_bridge_test_mocks();

  BitchatBridge bridge;
  uint8_t pubKey[32] = { 0 };
  bridge.begin("TestNode", pubKey, mock_bridge_send_group_data, nullptr);

  // Message to unknown channel - should fallback to "mesh"
  const char *channel = "unknown";
  const char *message = "Hello";
  uint8_t senderId[8] = { 0 };

  bool result = bridge.onBitchatMessage(channel, message, 1700000000, senderId);

  // Should succeed by routing to default mesh channel
  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL(1, mock_bridge_send_count);
}

/**
 * @test Bridge BLE Service Access
 */
void test_bridge_ble_service_access() {
  BitchatBridge bridge;
  uint8_t pubKey[32] = { 0 };
  bridge.begin("TestNode", pubKey, mock_bridge_send_group_data, nullptr);

  // Getting BLE service should not crash
  mesh::ble::BitchatBLEService &service = bridge.getBLEService();
  (void)service;

  TEST_PASS();
}

/**
 * @test Bridge Loop Processing
 */
void test_bridge_loop_processing() {
  reset_bridge_test_mocks();

  BitchatBridge bridge;
  uint8_t pubKey[32] = { 0 };
  bridge.begin("TestNode", pubKey, mock_bridge_send_group_data, nullptr);

  // Call loop multiple times without crash
  for (int i = 0; i < 10; i++) {
    bridge.loop();
  }

  TEST_PASS();
}

} // namespace bitchat
} // namespace test
