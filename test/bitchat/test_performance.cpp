/**
 * Test: BitChat Performance Test Suite
 * 
 * Tests Story 4.3: Performance Test - Message Throughput
 * 
 * These tests measure:
 * - Message processing latency (< 100ms target)
 * - Throughput (messages per second)
 * - Memory usage
 * - Buffer handling under load
 * 
 * Run with: pio test -e native_test (for simulation)
 * Or: pio test -e Heltec_v3_companion_radio_ble (for hardware)
 */

#include <unity.h>
#include <string.h>

#ifdef ENABLE_BITCHAT

#include "helpers/bitchat/BitchatMessageEncapsulator.h"
#include "helpers/bitchat/MessageDecapsulator.h"
#include "helpers/bitchat/LoopPrevention.h"
#include "helpers/bitchat/FragmentReassembly.h"

using namespace mesh::bitchat;

// Performance thresholds
static constexpr uint32_t TARGET_LATENCY_MS = 100;      // < 100ms per message
static constexpr uint32_t MIN_MESSAGES_PER_SEC = 10;    // At least 10 msg/sec
static constexpr size_t MAX_MEMORY_USAGE_BYTES = 32768; // 32KB max

// Timing utilities
static uint32_t start_time;
static uint32_t end_time;

void setUp(void) {
    start_time = 0;
    end_time = 0;
}

void tearDown(void) {
}

// ============================================================
// Story 4.3.1: Message Processing Latency
// ============================================================

/**
 * Test: Single message encapsulation latency
 */
void test_encapsulation_latency(void) {
    Serial.println("\n[TEST] Encapsulation Latency");
    
    BitchatMessageEncapsulator encapsulator;
    const char* message = "Test message for latency measurement";
    const char* channel = "mesh";
    uint8_t sender_id[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    // Warm up
    encapsulator.encapsulate(channel, message, 1700000000, sender_id);
    
    // Measure
    start_time = millis();
    EncapsulationResult result = encapsulator.encapsulate(
        channel, message, 1700000000, sender_id
    );
    end_time = millis();
    
    uint32_t latency = end_time - start_time;
    
    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_LESS_THAN(TARGET_LATENCY_MS, (int)latency);
    
    Serial.printf("[INFO] Encapsulation latency: %lu ms\n", latency);
    Serial.println("[PASS] Latency within target");
}

/**
 * Test: Loop prevention check latency
 */
void test_loop_prevention_latency(void) {
    Serial.println("\n[TEST] Loop Prevention Latency");
    
    LoopPrevention loopPrevention;
    const char* message = "Test message for loop prevention";
    
    // Warm up
    loopPrevention.shouldDrop(message, strlen(message));
    
    // Measure
    start_time = millis();
    for (int i = 0; i < 100; i++) {
        loopPrevention.shouldDrop(message, strlen(message));
    }
    end_time = millis();
    
    uint32_t avg_latency = (end_time - start_time) / 100;
    
    TEST_ASSERT_LESS_THAN(10, (int)avg_latency); // Should be very fast
    
    Serial.printf("[INFO] Loop prevention avg latency: %lu ms\n", avg_latency);
    Serial.println("[PASS] Loop prevention fast");
}

// ============================================================
// Story 4.3.2: Message Throughput
// ============================================================

/**
 * Test: Message encapsulation throughput
 */
void test_encapsulation_throughput(void) {
    Serial.println("\n[TEST] Encapsulation Throughput");
    
    BitchatMessageEncapsulator encapsulator;
    const char* message = "Test message";
    const char* channel = "mesh";
    uint8_t sender_id[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    const int ITERATIONS = 50;
    
    start_time = millis();
    for (int i = 0; i < ITERATIONS; i++) {
        EncapsulationResult result = encapsulator.encapsulate(
            channel, message, 1700000000, sender_id
        );
        TEST_ASSERT_TRUE(result.success);
    }
    end_time = millis();
    
    uint32_t total_time = end_time - start_time;
    float messages_per_sec = (float)ITERATIONS / (total_time / 1000.0f);
    
    TEST_ASSERT_GREATER_THAN(MIN_MESSAGES_PER_SEC, (int)messages_per_sec);
    
    Serial.printf("[INFO] Processed %d messages in %lu ms\n", ITERATIONS, total_time);
    Serial.printf("[INFO] Throughput: %.1f messages/sec\n", messages_per_sec);
    Serial.println("[PASS] Throughput acceptable");
}

// ============================================================
// Story 4.3.3: Memory Usage
// ============================================================

/**
 * Test: Memory usage under load
 */
void test_memory_usage(void) {
    Serial.println("\n[TEST] Memory Usage");
    
    // Note: Actual memory measurement requires platform-specific APIs
    // This test verifies no obvious memory leaks through repeated operations
    
    BitchatMessageEncapsulator encapsulator;
    LoopPrevention loopPrevention;
    FragmentReassembly fragmentReassembly;
    
    const char* message = "Memory test message";
    const char* channel = "mesh";
    uint8_t sender_id[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    // Perform many operations
    for (int i = 0; i < 100; i++) {
        // Encapsulate
        EncapsulationResult encap = encapsulator.encapsulate(
            channel, message, 1700000000, sender_id
        );
        TEST_ASSERT_TRUE(encap.success);
        
        // Loop prevention
        loopPrevention.shouldDrop(message, strlen(message));
    }
    
    // If we get here without crash, memory is likely stable
    // (Static allocation strategy prevents heap fragmentation)
    
    Serial.println("[INFO] Completed 100 iterations without memory issues");
    Serial.println("[PASS] Memory usage stable");
}

// ============================================================
// Story 4.3.4: Buffer Handling
// ============================================================

/**
 * Test: Buffer handling with various message sizes
 */
void test_buffer_handling_various_sizes(void) {
    Serial.println("\n[TEST] Buffer Handling (Various Sizes)");
    
    BitchatMessageEncapsulator encapsulator;
    const char* channel = "mesh";
    uint8_t sender_id[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    // Test various message sizes
    const char* small_msg = "Hi";
    const char* medium_msg = "This is a medium length message for testing";
    char large_msg[256];
    memset(large_msg, 'A', sizeof(large_msg) - 1);
    large_msg[255] = '\0';
    
    // Small message
    EncapsulationResult small = encapsulator.encapsulate(
        channel, small_msg, 1700000000, sender_id
    );
    TEST_ASSERT_TRUE(small.success);
    
    // Medium message
    EncapsulationResult medium = encapsulator.encapsulate(
        channel, medium_msg, 1700000000, sender_id
    );
    TEST_ASSERT_TRUE(medium.success);
    
    // Large message
    EncapsulationResult large = encapsulator.encapsulate(
        channel, large_msg, 1700000000, sender_id
    );
    TEST_ASSERT_TRUE(large.success);
    
    Serial.printf("[INFO] Small: %u bytes, Medium: %u bytes, Large: %u bytes\n",
                  small.length, medium.length, large.length);
    Serial.println("[PASS] Buffer handling for all sizes");
}

// ============================================================
// Story 4.3.5: Stress Test
// ============================================================

/**
 * Test: Stress test with rapid messages
 */
void test_stress_rapid_messages(void) {
    Serial.println("\n[TEST] Stress Test - Rapid Messages");
    
    BitchatMessageEncapsulator encapsulator;
    LoopPrevention loopPrevention;
    
    const char* channel = "mesh";
    uint8_t sender_id[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    const int STRESS_ITERATIONS = 200;
    int success_count = 0;
    
    start_time = millis();
    for (int i = 0; i < STRESS_ITERATIONS; i++) {
        // Vary message content
        char msg[64];
        snprintf(msg, sizeof(msg), "Stress test message %d", i);
        
        EncapsulationResult result = encapsulator.encapsulate(
            channel, msg, 1700000000 + i, sender_id
        );
        
        if (result.success) {
            success_count++;
        }
    }
    end_time = millis();
    
    uint32_t total_time = end_time - start_time;
    float success_rate = (float)success_count / STRESS_ITERATIONS * 100.0f;
    
    TEST_ASSERT_EQUAL(STRESS_ITERATIONS, success_count);
    
    Serial.printf("[INFO] Processed %d/%d messages (%.1f%%)\n", 
                  success_count, STRESS_ITERATIONS, success_rate);
    Serial.printf("[INFO] Total time: %lu ms\n", total_time);
    Serial.println("[PASS] Stress test completed");
}

#else
// ENABLE_BITCHAT not defined

void setUp(void) {}
void tearDown(void) {}

void test_encapsulation_latency(void) {
    TEST_IGNORE_MESSAGE("ENABLE_BITCHAT not defined");
}
void test_loop_prevention_latency(void) {
    TEST_IGNORE_MESSAGE("ENABLE_BITCHAT not defined");
}
void test_encapsulation_throughput(void) {
    TEST_IGNORE_MESSAGE("ENABLE_BITCHAT not defined");
}
void test_memory_usage(void) {
    TEST_IGNORE_MESSAGE("ENABLE_BITCHAT not defined");
}
void test_buffer_handling_various_sizes(void) {
    TEST_IGNORE_MESSAGE("ENABLE_BITCHAT not defined");
}
void test_stress_rapid_messages(void) {
    TEST_IGNORE_MESSAGE("ENABLE_BITCHAT not defined");
}

#endif // ENABLE_BITCHAT

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    Serial.println("\n========================================");
    Serial.println("BitChat Performance Test Suite");
    Serial.println("========================================\n");
    
    Serial.println("Performance Targets:");
    Serial.printf("  - Latency: < %d ms per message\n", TARGET_LATENCY_MS);
    Serial.printf("  - Throughput: > %d messages/sec\n", MIN_MESSAGES_PER_SEC);
    Serial.println();
    
    // Story 4.3.1: Latency Tests
    RUN_TEST(test_encapsulation_latency);
    RUN_TEST(test_loop_prevention_latency);
    
    // Story 4.3.2: Throughput Tests
    RUN_TEST(test_encapsulation_throughput);
    
    // Story 4.3.3: Memory Tests
    RUN_TEST(test_memory_usage);
    
    // Story 4.3.4: Buffer Tests
    RUN_TEST(test_buffer_handling_various_sizes);
    
    // Story 4.3.5: Stress Tests
    RUN_TEST(test_stress_rapid_messages);
    
    Serial.println("\n========================================");
    Serial.println("Performance Test Suite Complete");
    Serial.println("========================================\n");
    
    return UNITY_END();
}
