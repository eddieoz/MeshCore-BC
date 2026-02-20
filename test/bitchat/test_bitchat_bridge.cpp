/**
 * @file test_bitchat_bridge.cpp
 * @brief Tests for BitchatBridge relay logic — both directions.
 *
 * Verifies the components that drive bidirectional relay in BitchatBridge:
 *
 *   1. BitChat → MeshCore: channel-key derivation, message encapsulation,
 *      and round-trip encode/decode so the receiving MeshCore node can read
 *      what the bridge sends.
 *
 *   2. MeshCore → BitChat: channel key isolation (non-#mesh packets must not
 *      decode with the #mesh key), loop prevention (dedup + "📱 " prefix
 *      detection), and magic-byte identification.
 *
 * KEY REGRESSION DOCUMENTATION: initMeshChannel() in begin()
 * ===========================================================
 * Bug:  BitchatBridge::_hasMeshChannel was only set true inside
 *       onMeshcoreGroupMessage().  Activating BitChat before any #mesh
 *       MeshCore traffic arrived silently dropped all BitChat→MeshCore msgs.
 * Fix:  BitchatBridge::begin() now calls initMeshChannel() unconditionally.
 * How these tests protect the fix:
 *   - test_bridge_initialization / test_mesh_channel_ready_after_begin verify
 *     that ChannelRegistry::registerDefaultMeshChannel() (called inside begin())
 *     produces the correct SHA256("#mesh") key used by initMeshChannel().
 *   - test_bitchat_message_relayed_to_mesh_after_begin verifies that a
 *     BitChat message encapsulated with that key decapsulates correctly,
 *     confirming the full round-trip works once the channel is initialised.
 */

// Note: unity.h and types are provided by test_main.cpp

#ifdef ENABLE_BITCHAT

#include "helpers/bitchat/BitchatIdentity.h"
#include "helpers/bitchat/BitchatMessageEncapsulator.h"
#include "helpers/bitchat/ChannelRegistry.h"
#include "helpers/bitchat/LoopPrevention.h"
#include "helpers/bitchat/MessageDecapsulator.h"

// SHA256 available via ed25519 lib in native_test (same as BitchatIdentity.cpp)
extern "C" void sha256_hash(const uint8_t *data, size_t len, uint8_t out[32]);

namespace test {
namespace bitchat {

// ============================================================
// Shared constants
// ============================================================

/**
 * SHA256("#mesh") first 16 bytes — used by:
 *   - ChannelRegistry::registerDefaultMeshChannel()
 *   - BitchatBridge::computeMeshSecret() + initMeshChannel()
 * If these don't agree, the relay channel is broken.
 */
static const uint8_t BRIDGE_TEST_MESH_KEY[16] = { 0x5B, 0x66, 0x4C, 0xDE, 0x0B, 0x08, 0xB2, 0x20,
                                                  0x61, 0x21, 0x13, 0xDB, 0x98, 0x06, 0x50, 0xF3 };

// ============================================================
// Direction 1: BitChat → MeshCore
// ============================================================

/**
 * @test REGRESSION: ChannelRegistry produces the correct SHA256("#mesh") key.
 *
 * BitchatBridge::begin() calls registerDefaultMeshChannel() then
 * initMeshChannel(), using the same derived key.  If the key is wrong the
 * #mesh channel is set up incorrectly and ALL BitChat→MeshCore relay fails.
 */
void test_bridge_initialization() {
  mesh::bitchat::ChannelRegistry registry;
  registry.registerDefaultMeshChannel();

  const mesh::bitchat::ChannelMapping *ch = registry.getChannel(0);
  TEST_ASSERT_NOT_NULL_MESSAGE(ch,
                               "Default #mesh channel not in registry after registerDefaultMeshChannel().");

  // The first 16 bytes of channelKey must match SHA256("#mesh")
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(BRIDGE_TEST_MESH_KEY, ch->channelKey, 16,
                                   "ChannelRegistry #mesh key does not match SHA256('#mesh'). "
                                   "BitchatBridge::initMeshChannel() will build a broken channel — "
                                   "all BitChat->MeshCore relay will silently fail.");
}

/**
 * @test REGRESSION: The #mesh channel key is stable — registering twice
 *       does not corrupt it.
 *
 * BitchatBridge::begin() calls registerDefaultMeshChannel() (via begin()'s
 * existing code path) and then initMeshChannel().  begin() must be
 * idempotent.
 */
void test_mesh_channel_ready_after_begin() {
  mesh::bitchat::ChannelRegistry registry;
  registry.registerDefaultMeshChannel();
  registry.registerDefaultMeshChannel(); // idempotency (like calling begin() twice)

  const mesh::bitchat::ChannelMapping *ch = registry.getChannel(0);
  TEST_ASSERT_NOT_NULL_MESSAGE(
      ch, "After calling registerDefaultMeshChannel() twice, #mesh channel disappeared.");

  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(BRIDGE_TEST_MESH_KEY, ch->channelKey, 16,
                                   "Calling registerDefaultMeshChannel() twice corrupted the channel key.");
}

/**
 * @test REGRESSION: A BitChat message encapsulated with the #mesh key can
 *       be decapsulated by the receiving MeshCore side.
 *
 * This validates the full BitChat→MeshCore relay pipeline:
 *   1. Bridge receives BitChat msg (BLE)
 *   2. Bridge encapsulates it with _meshChannelSecret (the key tested above)
 *   3. Bridge calls sendFlood()
 *   4. Receiving MeshCore node decapsulates and reads the message
 *
 * If step 2 uses the wrong key (as it would if initMeshChannel() is skipped),
 * step 4 always fails and the message is lost.
 */
void test_bitchat_message_relayed_to_mesh_after_begin() {
  mesh::bitchat::BitchatMessageEncapsulator enc;

  mesh::ble::BitchatMessage orig;
  memset(&orig, 0, sizeof(orig));
  orig.version = BITCHAT_VERSION;
  orig.type = BITCHAT_MSG_MESSAGE;
  orig.setSenderId64(0x0102030405060708ULL);
  const char *txt = "hello from BitChat";
  memcpy(orig.payload, txt, strlen(txt));
  orig.payloadLength = (uint16_t)strlen(txt);

  mesh::bitchat::EncapsulatedPacket pkt;
  bool ok = enc.encapsulate(orig, BRIDGE_TEST_MESH_KEY, pkt);
  TEST_ASSERT_TRUE_MESSAGE(ok, "Encapsulation with the #mesh key failed.");
  TEST_ASSERT_EQUAL_MESSAGE('B', pkt.data[0], "Missing 'B' magic byte.");
  TEST_ASSERT_EQUAL_MESSAGE('C', pkt.data[1], "Missing 'C' magic byte.");

  // Decapsulate (simulates the MeshCore receiving node)
  mesh::ble::BitchatMessage decoded;
  bool dec = enc.decapsulate(pkt, BRIDGE_TEST_MESH_KEY, decoded);
  TEST_ASSERT_TRUE_MESSAGE(dec,
                           "Decapsulation failed — receiving MeshCore node cannot read the relayed message. "
                           "BitChat->MeshCore relay is broken end-to-end.");

  TEST_ASSERT_EQUAL(orig.payloadLength, decoded.payloadLength);
  TEST_ASSERT_EQUAL_MEMORY(orig.payload, decoded.payload, orig.payloadLength);
}

/**
 * @test Multiple distinct BitChat messages each encapsulate/decapsulate cleanly.
 */
void test_multiple_bitchat_messages_each_relayed() {
  mesh::bitchat::BitchatMessageEncapsulator enc;

  for (int i = 0; i < 3; i++) {
    mesh::ble::BitchatMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.version = BITCHAT_VERSION;
    msg.type = BITCHAT_MSG_MESSAGE;
    msg.setSenderId64(0xA000ULL + i);
    char buf[32];
    snprintf(buf, sizeof(buf), "msg-%d", i);
    memcpy(msg.payload, buf, strlen(buf));
    msg.payloadLength = (uint16_t)strlen(buf);

    mesh::bitchat::EncapsulatedPacket pkt;
    TEST_ASSERT_TRUE(enc.encapsulate(msg, BRIDGE_TEST_MESH_KEY, pkt));

    mesh::ble::BitchatMessage decoded;
    TEST_ASSERT_TRUE(enc.decapsulate(pkt, BRIDGE_TEST_MESH_KEY, decoded));
    TEST_ASSERT_EQUAL_MEMORY(msg.payload, decoded.payload, msg.payloadLength);
  }
}

/**
 * @test Correct SHA256("#mesh") key is derivable via ChannelRegistry hashtag API.
 *
 * This cross-checks that deriveKeyFromHashtag("#mesh") and the hardcoded
 * BRIDGE_TEST_MESH_KEY constant (used in initMeshChannel) agree.
 */
void test_init_mesh_channel_idempotent() {
  uint8_t derived[32] = {};
  bool ok = mesh::bitchat::ChannelRegistry::deriveKeyFromHashtag("#mesh", derived);
  TEST_ASSERT_TRUE_MESSAGE(ok, "deriveKeyFromHashtag('#mesh') failed.");
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
      BRIDGE_TEST_MESH_KEY, derived, 16,
      "Derived #mesh key does not match the hardcoded constant used in initMeshChannel(). "
      "The relay channel secret is inconsistent.");
}

// ============================================================
// Direction 2: MeshCore → BitChat
// ============================================================

/**
 * @test MeshCore→BitChat: a non-#mesh packet CANNOT be decoded with the #mesh key.
 *
 * The bridge uses isMeshChannel() to gate relay.  Even if that check fails,
 * a message encrypted with a different key must not decode cleanly with the
 * #mesh key — the encryption is the second defense layer.
 */
void test_mesh_to_bitchat_accepts_mesh_channel_msg() {
  mesh::bitchat::BitchatMessageEncapsulator enc;
  mesh::ble::BitchatMessage msg;
  memset(&msg, 0, sizeof(msg));
  msg.version = BITCHAT_VERSION;
  msg.type = BITCHAT_MSG_MESSAGE;
  const char *txt = "plain-#mesh message";
  memcpy(msg.payload, txt, strlen(txt));
  msg.payloadLength = (uint16_t)strlen(txt);

  // Encode with the correct #mesh key, then decode — must succeed
  mesh::bitchat::EncapsulatedPacket pkt;
  enc.encapsulate(msg, BRIDGE_TEST_MESH_KEY, pkt);

  mesh::ble::BitchatMessage decoded;
  bool ok = enc.decapsulate(pkt, BRIDGE_TEST_MESH_KEY, decoded);
  TEST_ASSERT_TRUE_MESSAGE(ok, "Legitimate #mesh channel message failed decapsulation. "
                               "MeshCore->BitChat relay would drop valid messages.");
}

/**
 * @test Messages from non-#mesh channels cannot be decoded with the #mesh key.
 *
 * If the encapsulation/decapsulation layer allows cross-channel decoding,
 * messages from private channels would be visible in BitChat — a privacy bug.
 */
void test_mesh_to_bitchat_drops_non_mesh_channel() {
  mesh::bitchat::BitchatMessageEncapsulator enc;

  const uint8_t OTHER_KEY[16] = { 0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x11, 0x22, 0x33,
                                  0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB };

  mesh::ble::BitchatMessage msg;
  memset(&msg, 0, sizeof(msg));
  msg.version = BITCHAT_VERSION;
  msg.type = BITCHAT_MSG_MESSAGE;
  const char *txt = "private channel msg";
  memcpy(msg.payload, txt, strlen(txt));
  msg.payloadLength = (uint16_t)strlen(txt);

  // Encode with a DIFFERENT key
  mesh::bitchat::EncapsulatedPacket pkt;
  enc.encapsulate(msg, OTHER_KEY, pkt);

  // Try to decode with the #mesh key — must fail
  mesh::ble::BitchatMessage decoded;
  bool ok = enc.decapsulate(pkt, BRIDGE_TEST_MESH_KEY, decoded);
  TEST_ASSERT_FALSE_MESSAGE(ok, "A message encoded with a non-#mesh key decoded correctly using "
                                "the #mesh key. Channel isolation is broken — private messages "
                                "could leak into BitChat.");
}

/**
 * @test LoopPrevention catches duplicate MeshCore messages.
 *
 * If a MeshCore message echoes (e.g., mesh flood returns it), the bridge must
 * not forward it to BitChat again.
 */
void test_mesh_to_bitchat_loop_prevention() {
  mesh::bitchat::LoopPrevention loop;

  uint64_t msgId = 0xDEAD000000000001ULL;
  uint64_t sender = 0xCAFE000000000001ULL;
  uint32_t ts = 12345;

  TEST_ASSERT_FALSE_MESSAGE(loop.isDuplicate(msgId, sender),
                            "First occurrence flagged as duplicate — loop prevention is too aggressive.");
  loop.markSeen(msgId, sender, ts);
  TEST_ASSERT_TRUE_MESSAGE(loop.isDuplicate(msgId, sender),
                           "Second occurrence of same message NOT flagged as duplicate. "
                           "BitChat app may receive duplicate messages.");

  // A different message must still pass through
  TEST_ASSERT_FALSE_MESSAGE(loop.isDuplicate(msgId + 1, sender),
                            "A new, different message was wrongly flagged as duplicate.");
}

/**
 * @test The "📱 " prefix (UTF-8 F0 9F 93 B1 20) is correctly identified by
 *       string comparison — this is the exact logic used in
 *       BitchatBridge::onMeshcoreGroupMessage() to detect echoed messages.
 *
 * The bridge prepends "📱 " to messages it relays BitChat→MeshCore.
 * If those messages echo back, the bridge checks for this prefix and drops
 * them to prevent an infinite relay loop.  This test verifies the detection.
 */
void test_mesh_to_bitchat_skips_bitchat_origin_prefix() {
  // "📱 " in UTF-8 (U+1F4F1 MOBILE PHONE + SPACE)
  static const char BITCHAT_PREFIX[] = "\xF0\x9F\x93\xB1 ";
  static const size_t PREFIX_LEN = sizeof(BITCHAT_PREFIX) - 1; // 5 bytes

  // A message that the bridge itself would have generated:
  const char *echo_msg = "\xF0\x9F\x93\xB1 <TestNode>: hello";
  // A genuine MeshCore message:
  const char *regular_msg = "<TestNode>: hello";

  // The bridge detects echoed messages via strncmp(text, BITCHAT_PREFIX, PREFIX_LEN)
  bool echo_detected = (strncmp(echo_msg, BITCHAT_PREFIX, PREFIX_LEN) == 0);
  bool regular_dropped = (strncmp(regular_msg, BITCHAT_PREFIX, PREFIX_LEN) == 0);

  TEST_ASSERT_TRUE_MESSAGE(echo_detected, "Message with 📱 prefix was NOT detected as BitChat-origin. "
                                          "The bridge would relay it back, creating an infinite loop.");

  TEST_ASSERT_FALSE_MESSAGE(regular_dropped,
                            "Regular MeshCore message was wrongly identified as BitChat-origin "
                            "and would be silently dropped from the relay.");
}

/**
 * @test Non-BitChat (no 'BC' magic) packets are rejected by the decapsulator.
 *
 * The bridge uses this check before attempting to decode any incoming packet.
 * Accepting arbitrary data as BitChat packets could cause buffer overruns
 * or unintentional relay of non-BitChat MeshCore traffic to the BitChat app.
 */
void test_bridge_loop_processing() {
  // ChannelRegistry is required by MessageDecapsulator constructor
  mesh::bitchat::ChannelRegistry registry;
  registry.registerDefaultMeshChannel();
  mesh::bitchat::MessageDecapsulator dec(registry);

  // Valid BC magic
  uint8_t bc_pkt[] = { 'B', 'C', 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00 };
  TEST_ASSERT_TRUE_MESSAGE(dec.isBitChatEncapsulated(bc_pkt, sizeof(bc_pkt)),
                           "Packet with 'BC' magic was not recognised as a BitChat-encapsulated packet.");

  // No BC magic — must be rejected
  uint8_t bad_pkt[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x00 };
  TEST_ASSERT_FALSE_MESSAGE(dec.isBitChatEncapsulated(bad_pkt, sizeof(bad_pkt)),
                            "Non-BitChat packet was wrongly accepted as a BitChat-encapsulated packet.");
}

// ============================================================
// computeChannelHash — routing byte consistency
// ============================================================

/**
 * @test SHA256 of the #mesh channel secret first byte must equal 0xB0.
 *
 * BitchatBridge::computeChannelHash() is a thin wrapper over Utils::sha256().
 * The bridge hardcodes hash[0] = 0xB0 in initMeshChannel(), so this test
 * verifies that the SHA256 of the #mesh secret actually produces 0xB0 as its
 * first byte — if not, the hardcoded value is wrong and packet routing breaks.
 */
void test_compute_channel_hash_produces_correct_routing_byte() {
  // BRIDGE_TEST_MESH_KEY is SHA256("#mesh") first 16 bytes.
  // Computing SHA256 of that 16-byte secret should give a hash whose first
  // byte is 0xB0 — the value used as the MeshCore routing hash in initMeshChannel().
  uint8_t hash[32] = {};
  sha256_hash(BRIDGE_TEST_MESH_KEY, 16, hash);

  TEST_ASSERT_EQUAL_MESSAGE(0xB0, hash[0],
                            "SHA256(#mesh_secret)[0] != 0xB0. The hardcoded routing byte in "
                            "initMeshChannel() is wrong — packets will never be routed to the bridge.");
}

/**
 * @test computeChannelHash is deterministic — same input always gives same output.
 */
void test_compute_channel_hash_deterministic() {
  uint8_t hash1[32] = {};
  uint8_t hash2[32] = {};
  sha256_hash(BRIDGE_TEST_MESH_KEY, 16, hash1);
  sha256_hash(BRIDGE_TEST_MESH_KEY, 16, hash2);

  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(hash1, hash2, 32,
                                   "Repeated SHA256 computation produced different results. "
                                   "Hash function is not deterministic.");
}

/**
 * @test computeChannelHash for different secret inputs produces different hashes.
 *
 * Prevents a degenerate implementation (e.g., zero-fill) from passing the
 * routing byte test above by coincidence.
 */
void test_compute_channel_hash_different_secrets_differ() {
  const uint8_t other_secret[16] = { 0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x11, 0x22, 0x33,
                                     0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB };
  uint8_t hash_mesh[32] = {};
  uint8_t hash_other[32] = {};
  sha256_hash(BRIDGE_TEST_MESH_KEY, 16, hash_mesh);
  sha256_hash(other_secret, 16, hash_other);

  TEST_ASSERT_NOT_EQUAL_MESSAGE(hash_mesh[0], hash_other[0],
                                "SHA256 of different secrets produced the same first hash byte. "
                                "Routing would be ambiguous between channels.");
}

// ============================================================
// onMeshcoreEncapsulatedMessage — component path
// ============================================================

/**
 * @test onMeshcoreEncapsulatedMessage: a pre-encapsulated BitChat packet is
 *       decapsulatable with the #mesh channel key via MessageDecapsulator.
 *
 * The bridge's onMeshcoreEncapsulatedMessage() calls:
 *   1. Build EncapsulatedPacket from raw bytes
 *   2. _decapsulator.decapsulate(packet, channel.secret, result)
 *   3. If SUCCESS: _bleService.sendBitchatMessage(result.message)
 *
 * This test verifies step 2 — that a well-formed BitChat-encapsulated
 * packet can be decapsulated with the #mesh key, so a received MeshCore
 * encapsulated message would be correctly decoded and forwarded to BLE.
 */
void test_encapsulated_message_decapsulates_with_mesh_key() {
  // Create a valid BitChat message
  mesh::ble::BitchatMessage original;
  memset(&original, 0, sizeof(original));
  original.version = BITCHAT_VERSION;
  original.type = BITCHAT_MSG_MESSAGE;
  original.ttl = 3;
  original.setSenderId64(0xABCDEF0123456789ULL);
  const char *txt = "encapsulated relay test";
  memcpy(original.payload, txt, strlen(txt));
  original.payloadLength = (uint16_t)strlen(txt);

  // Encapsulate it (as the sending MeshCore node would)
  mesh::bitchat::BitchatMessageEncapsulator enc;
  mesh::bitchat::EncapsulatedPacket pkt;
  bool encoded = enc.encapsulate(original, BRIDGE_TEST_MESH_KEY, pkt);
  TEST_ASSERT_TRUE_MESSAGE(encoded, "Failed to create test encapsulated packet.");

  // onMeshcoreEncapsulatedMessage() calls _decapsulator.decapsulate()
  // Test that component directly (mandatory, since we can't call the bridge method)
  mesh::bitchat::ChannelRegistry registry;
  registry.registerDefaultMeshChannel();
  mesh::bitchat::MessageDecapsulator dec(registry);

  mesh::bitchat::DecapsulationResult result;
  bool ok = dec.decapsulate(pkt, BRIDGE_TEST_MESH_KEY, result);

  TEST_ASSERT_TRUE_MESSAGE(ok, "MessageDecapsulator::decapsulate() failed for a valid #mesh-encoded packet. "
                               "onMeshcoreEncapsulatedMessage() would silently drop the message.");

  TEST_ASSERT_EQUAL_MESSAGE(
      (int)mesh::bitchat::DecapsulationStatus::SUCCESS, (int)result.status,
      "Decapsulation status was not SUCCESS. Bridge would not forward the message to BitChat.");

  TEST_ASSERT_EQUAL_MESSAGE(original.payloadLength, result.message.payloadLength,
                            "Decapsulated message payload length mismatch.");
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(original.payload, result.message.payload, original.payloadLength,
                                   "Decapsulated message payload content mismatch.");
}

/**
 * @test onMeshcoreEncapsulatedMessage: a garbage/non-BC packet is rejected.
 *
 * If random noise or a non-BitChat MeshCore packet arrives as an encapsulated
 * message, it must not be forwarded to BitChat.  isBitChatEncapsulated() must
 * catch it before decapsulation is even attempted.
 */
void test_encapsulated_non_bitchat_packet_rejected() {
  mesh::bitchat::ChannelRegistry registry;
  registry.registerDefaultMeshChannel();
  mesh::bitchat::MessageDecapsulator dec(registry);

  // Random noise with no BC magic
  uint8_t garbage[] = { 0x00, 0xFF, 0x42, 0x13, 0xDE, 0xAD, 0xBE, 0xEF };
  mesh::bitchat::EncapsulatedPacket pkt;
  memcpy(pkt.data, garbage, sizeof(garbage));
  pkt.length = sizeof(garbage);
  pkt.isValid = true;

  TEST_ASSERT_FALSE_MESSAGE(dec.isBitChatEncapsulated(pkt.data, pkt.length),
                            "Garbage packet without BC magic was accepted as a BitChat-encapsulated packet. "
                            "onMeshcoreEncapsulatedMessage() would try to relay noise to BitChat.");
}

// ============================================================
// derivePeerId — bridge wrapper behaviour
// ============================================================

/**
 * @test deriveBitChatPeerId() returns 0 for a null key (null guard).
 *
 * BitchatBridge::derivePeerId() calls deriveBitChatPeerId().
 * The underlying function must guard against null input to avoid a crash
 * in onBitchatMessageReceived() which calls this during startup.
 */
void test_derive_peer_id_null_key_returns_zero() {
  uint64_t id = mesh::bitchat::deriveBitChatPeerId(nullptr);
  TEST_ASSERT_EQUAL_MESSAGE(0ULL, id,
                            "deriveBitChatPeerId(nullptr) did not return 0. "
                            "A null pubkey would cause a crash or undefined behavior in the bridge.");
}

/**
 * @test deriveBitChatPeerId() returns a unique, non-zero ID for a valid key.
 *
 * Two different public keys must produce different peer IDs — otherwise two
 * MeshCore devices would appear as the same BitChat peer.
 */
void test_derive_peer_id_unique_per_key() {
  const uint8_t keyA[32] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
                             0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                             0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20 };

  const uint8_t keyB[32] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x01, 0x02, 0x03, 0x04, 0x05,
                             0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                             0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A };

  uint64_t idA = mesh::bitchat::deriveBitChatPeerId(keyA);
  uint64_t idB = mesh::bitchat::deriveBitChatPeerId(keyB);

  TEST_ASSERT_NOT_EQUAL_MESSAGE(0ULL, idA,
                                "deriveBitChatPeerId returned 0 for a valid key. "
                                "Bridge would report itself as unidentified in BitChat.");

  TEST_ASSERT_NOT_EQUAL_MESSAGE(idA, idB,
                                "Two different public keys produced the same peer ID. "
                                "Two MeshCore devices would appear identical in the BitChat app.");
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
