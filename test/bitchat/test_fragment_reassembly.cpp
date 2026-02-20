/**
 * Story 7.3: Fragment Reassembly - BDD Tests
 * 
 * Feature: BitChat Fragment Handling
 * 
 * TDD Approach:
 * 1. Write failing tests (RED)
 * 2. Implement to pass (GREEN)
 * 3. Refactor (REFACTOR)
 */

#include <unity.h>
#include "helpers/bitchat/FragmentReassembly.h"

#ifdef ENABLE_BITCHAT

using namespace mesh::bitchat;

namespace test {
namespace bitchat {

/**
 * Scenario: Reassemble fragmented message
 * 
 * Given BitChat FRAGMENT messages received
 * And fragments form complete message
 * When all fragments received
 * Then reassemble in order
 * And process complete message
 */
void test_reassemble_fragments(void) {
    // Given: Fragment reassembler
    FragmentReassembly reassembly;
    
    // And: Message fragmented into 3 parts
    uint32_t msgId = 0x12345678;
    uint8_t totalFrags = 3;
    
    // Fragment 0
    uint8_t frag0[] = "Hello ";
    FragmentInfo info0;
    info0.messageId = msgId;
    info0.fragmentNum = 0;
    info0.totalFragments = totalFrags;
    info0.data = frag0;
    info0.length = sizeof(frag0) - 1;
    info0.timestamp = 1000;
    
    // Fragment 1
    uint8_t frag1[] = "World ";
    FragmentInfo info1;
    info1.messageId = msgId;
    info1.fragmentNum = 1;
    info1.totalFragments = totalFrags;
    info1.data = frag1;
    info1.length = sizeof(frag1) - 1;
    info1.timestamp = 1000;
    
    // Fragment 2
    uint8_t frag2[] = "!!!";
    FragmentInfo info2;
    info2.messageId = msgId;
    info2.fragmentNum = 2;
    info2.totalFragments = totalFrags;
    info2.data = frag2;
    info2.length = sizeof(frag2) - 1;
    info2.timestamp = 1000;
    
    // When: Add fragments
    reassembly.addFragment(info0);
    reassembly.addFragment(info1);
    reassembly.addFragment(info2);
    
    // Then: Should be complete
    TEST_ASSERT_TRUE(reassembly.isComplete(msgId));
    
    // And: Should be able to reassemble
    uint8_t reassembled[256];
    size_t reassembledLen = 0;
    bool success = reassembly.reassemble(msgId, reassembled, sizeof(reassembled), &reassembledLen);
    
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(15, reassembledLen);  // "Hello World !!!" = 6 + 6 + 3 = 15
    TEST_ASSERT_EQUAL_STRING("Hello World !!!", (char*)reassembled);
}

/**
 * Scenario: Incomplete fragment set
 * 
 * Given only some fragments received
 * When checking if complete
 * Then return false
 */
void test_incomplete_fragments(void) {
    // Given: Fragment reassembler
    FragmentReassembly reassembly;
    
    uint32_t msgId = 0xABCDEF01;
    
    // Add only 1 of 3 fragments
    uint8_t frag0[] = "Part1";
    FragmentInfo info0;
    info0.messageId = msgId;
    info0.fragmentNum = 0;
    info0.totalFragments = 3;
    info0.data = frag0;
    info0.length = sizeof(frag0) - 1;
    info0.timestamp = 1000;
    
    reassembly.addFragment(info0);
    
    // Then: Should NOT be complete
    TEST_ASSERT_FALSE(reassembly.isComplete(msgId));
}

/**
 * Scenario: Timeout partial fragments
 * 
 * Given partial fragments
 * When timeout expires
 * Then clear old fragments
 */
void test_fragment_timeout(void) {
    // Given: Fragment reassembler
    FragmentReassembly reassembly;
    
    uint32_t msgId = 0xDEADBEEF;
    uint32_t oldTime = 1000;
    uint32_t currentTime = 11000;  // 10 seconds later
    
    // Add fragment with old timestamp
    uint8_t frag0[] = "Old";
    FragmentInfo info0;
    info0.messageId = msgId;
    info0.fragmentNum = 0;
    info0.totalFragments = 3;
    info0.data = frag0;
    info0.length = sizeof(frag0) - 1;
    info0.timestamp = oldTime;
    
    reassembly.addFragment(info0);
    
    // When: Clear old fragments (timeout 5 seconds)
    reassembly.clearOldFragments(currentTime, 5000);
    
    // Then: Fragment should be cleared
    TEST_ASSERT_FALSE(reassembly.hasFragment(msgId, 0));
}

/**
 * Scenario: Duplicate fragment handling
 * 
 * Given fragment already received
 * When same fragment arrives again
 * Then ignore duplicate
 */
void test_duplicate_fragment(void) {
    // Given: Fragment reassembler
    FragmentReassembly reassembly;
    
    uint32_t msgId = 0x11223344;
    
    uint8_t frag0[] = "Original";
    FragmentInfo info0;
    info0.messageId = msgId;
    info0.fragmentNum = 0;
    info0.totalFragments = 2;
    info0.data = frag0;
    info0.length = sizeof(frag0) - 1;
    info0.timestamp = 1000;
    
    reassembly.addFragment(info0);
    
    // When: Add same fragment again (with different data)
    uint8_t fragDup[] = "Duplicate";
    FragmentInfo infoDup;
    infoDup.messageId = msgId;
    infoDup.fragmentNum = 0;
    infoDup.totalFragments = 2;
    infoDup.data = fragDup;
    infoDup.length = sizeof(fragDup) - 1;
    infoDup.timestamp = 1001;
    
    reassembly.addFragment(infoDup);
    
    // Then: Should still have original data (not overwritten)
    // Add fragment 1 to complete and check
    uint8_t frag1[] = "End";
    FragmentInfo info1;
    info1.messageId = msgId;
    info1.fragmentNum = 1;
    info1.totalFragments = 2;
    info1.data = frag1;
    info1.length = sizeof(frag1) - 1;
    info1.timestamp = 1000;
    
    reassembly.addFragment(info1);
    
    uint8_t reassembled[256];
    size_t reassembledLen = 0;
    reassembly.reassemble(msgId, reassembled, sizeof(reassembled), &reassembledLen);
    
    // Should have "Original" not "Duplicate"
    TEST_ASSERT_TRUE(reassembledLen >= 8);
    TEST_ASSERT_EQUAL_MEMORY("Original", reassembled, 8);
}

/**
 * Scenario: Clear all fragments for message
 * 
 * Given fragments for message
 * When clearing that message
 * Then remove all its fragments
 */
void test_clear_message_fragments(void) {
    // Given: Fragment reassembler
    FragmentReassembly reassembly;
    
    uint32_t msgId = 0x55667788;
    
    uint8_t frag0[] = "Data";
    FragmentInfo info0;
    info0.messageId = msgId;
    info0.fragmentNum = 0;
    info0.totalFragments = 2;
    info0.data = frag0;
    info0.length = sizeof(frag0) - 1;
    info0.timestamp = 1000;
    
    reassembly.addFragment(info0);
    
    // When: Clear message
    reassembly.clearMessage(msgId);
    
    // Then: Fragment should be gone
    TEST_ASSERT_FALSE(reassembly.hasFragment(msgId, 0));
}

/**
 * Scenario: Multiple message fragments
 * 
 * Given fragments for different messages
 * When adding fragments
 * Then keep them separate
 */
void test_multiple_messages(void) {
    // Given: Fragment reassembler
    FragmentReassembly reassembly;
    
    uint32_t msgId1 = 0xAABBCCDD;
    uint32_t msgId2 = 0x11223344;
    
    // Fragments for message 1
    uint8_t frag1_0[] = "Msg1";
    FragmentInfo info1_0;
    info1_0.messageId = msgId1;
    info1_0.fragmentNum = 0;
    info1_0.totalFragments = 1;
    info1_0.data = frag1_0;
    info1_0.length = sizeof(frag1_0) - 1;
    info1_0.timestamp = 1000;
    
    reassembly.addFragment(info1_0);
    
    // Fragments for message 2
    uint8_t frag2_0[] = "Msg2";
    FragmentInfo info2_0;
    info2_0.messageId = msgId2;
    info2_0.fragmentNum = 0;
    info2_0.totalFragments = 1;
    info2_0.data = frag2_0;
    info2_0.length = sizeof(frag2_0) - 1;
    info2_0.timestamp = 1000;
    
    reassembly.addFragment(info2_0);
    
    // Then: Both should be complete
    TEST_ASSERT_TRUE(reassembly.isComplete(msgId1));
    TEST_ASSERT_TRUE(reassembly.isComplete(msgId2));
    
    // And: Should reassemble correctly
    uint8_t buf[256];
    size_t len = 0;
    
    reassembly.reassemble(msgId1, buf, sizeof(buf), &len);
    TEST_ASSERT_EQUAL_STRING("Msg1", (char*)buf);
    
    reassembly.reassemble(msgId2, buf, sizeof(buf), &len);
    TEST_ASSERT_EQUAL_STRING("Msg2", (char*)buf);
}

/**
 * Scenario: Reassembly buffer too small
 * 
 * Given complete fragment set
 * And too-small output buffer
 * When reassembling
 * Then return error
 */
void test_reassembly_buffer_too_small(void) {
    // Given: Fragment reassembler
    FragmentReassembly reassembly;
    
    uint32_t msgId = 0x99AABBCC;
    
    // Large fragment
    uint8_t frag0[200];
    memset(frag0, 'X', sizeof(frag0));
    
    FragmentInfo info0;
    info0.messageId = msgId;
    info0.fragmentNum = 0;
    info0.totalFragments = 1;
    info0.data = frag0;
    info0.length = sizeof(frag0);
    info0.timestamp = 1000;
    
    reassembly.addFragment(info0);
    
    // When: Reassemble with small buffer
    uint8_t buf[10];
    size_t len = 0;
    bool success = reassembly.reassemble(msgId, buf, sizeof(buf), &len);
    
    // Then: Should fail (buffer too small)
    TEST_ASSERT_FALSE(success);
}

/**
 * Scenario: Clear all fragments
 * 
 * Given multiple fragment sets
 * When clearing all
 * Then remove everything
 */
void test_clear_all_fragments(void) {
    // Given: Fragment reassembler with fragments
    FragmentReassembly reassembly;
    
    uint32_t msgId1 = 0x11111111;
    uint32_t msgId2 = 0x22222222;
    
    FragmentInfo info1;
    info1.messageId = msgId1;
    info1.fragmentNum = 0;
    info1.totalFragments = 1;
    info1.data = (uint8_t*)"A";
    info1.length = 1;
    info1.timestamp = 1000;
    reassembly.addFragment(info1);
    
    FragmentInfo info2;
    info2.messageId = msgId2;
    info2.fragmentNum = 0;
    info2.totalFragments = 1;
    info2.data = (uint8_t*)"B";
    info2.length = 1;
    info2.timestamp = 1000;
    reassembly.addFragment(info2);
    
    // When: Clear all
    reassembly.clearAll();
    
    // Then: All fragments gone
    TEST_ASSERT_FALSE(reassembly.hasFragment(msgId1, 0));
    TEST_ASSERT_FALSE(reassembly.hasFragment(msgId2, 0));
}

} // namespace bitchat
} // namespace test

#endif // ENABLE_BITCHAT
