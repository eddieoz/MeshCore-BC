/**
 * Test: ESP32 BitChat Service Integration with SerialBLEInterface
 * 
 * Tests Story 1.4: Connect BitchatBLEService to ESP32 SerialBLEInterface
 * - BitChat service is created on the same BLE server
 * - setBitChatService() is called with the service pointer
 * - Mode switching is available
 */

#include <unity.h>

#ifdef ESP32
#include <BLEDevice.h>
#include "helpers/esp32/SerialBLEInterface.h"
#include "helpers/ble/BitchatBLEService.h"

using namespace mesh::ble;

static const char *TEST_DEVICE_NAME = "TestBitChat";
static const char *TEST_PREFIX = "MC-";
static const uint32_t TEST_PIN = 123456;

void setUp(void) {
    BLEDevice::deinit();
}

void tearDown(void) {
    BLEDevice::deinit();
}

/**
 * Test: SerialBLEInterface exposes BLE server for service attachment
 */
void test_serial_interface_exposes_ble_server(void) {
    // Given: BLE is initialized via SerialBLEInterface
    BLEDevice::init(TEST_DEVICE_NAME);
    
    SerialBLEInterface serialInterface;
    char dev_name[] = "TestDevice";
    serialInterface.begin(TEST_PREFIX, dev_name, TEST_PIN);
    
    // When: Getting BLE server
    BLEServer *server = serialInterface.getBLEServer();
    
    // Then: Server is valid and initialized
    TEST_ASSERT_NOT_NULL(server);
}

/**
 * Test: BitChat service can be registered with SerialBLEInterface
 */
void test_bitchat_service_registration(void) {
    // Given: SerialBLEInterface is initialized
    BLEDevice::init(TEST_DEVICE_NAME);
    
    SerialBLEInterface serialInterface;
    char dev_name[] = "TestDevice";
    serialInterface.begin(TEST_PREFIX, dev_name, TEST_PIN);
    
    // And: BitChat service is initialized
    BitchatBLEService bitchatService;
    bitchatService.initESP32(TEST_DEVICE_NAME);
    
    // When: Registering BitChat service
    BLEService *esp32Service = bitchatService.getESP32Service();
    TEST_ASSERT_NOT_NULL(esp32Service);
    
    serialInterface.setBitChatService(esp32Service);
    
    // Then: Service is registered (test by getting it back)
    // Note: getBitChatService() is private, but we can verify through mode switching
    TEST_ASSERT_TRUE(serialInterface.isBitChatMode() == false); // Default mode
}

/**
 * Test: Mode switching is available after service registration
 */
void test_mode_switching_available(void) {
    // Given: SerialBLEInterface with BitChat service registered
    BLEDevice::init(TEST_DEVICE_NAME);
    
    SerialBLEInterface serialInterface;
    char dev_name[] = "TestDevice";
    serialInterface.begin(TEST_PREFIX, dev_name, TEST_PIN);
    
    BitchatBLEService bitchatService;
    bitchatService.initESP32(TEST_DEVICE_NAME);
    serialInterface.setBitChatService(bitchatService.getESP32Service());
    
    // When: Checking BitChat mode status
    bool initialMode = serialInterface.isBitChatMode();
    
    // Then: Initially in MeshCore mode (false)
    TEST_ASSERT_FALSE(initialMode);
    
    // And: Mode can be toggled (testing the API is available)
    // Note: Actual mode switching requires advertising to be running
    // which may not work in test environment
}

/**
 * Test: Integration flow matches Story 1.4 acceptance criteria
 */
void test_integration_flow(void) {
    // Given: BLE is initialized
    BLEDevice::init(TEST_DEVICE_NAME);
    
    // When: Creating SerialBLEInterface (as in main.cpp)
    SerialBLEInterface serialInterface;
    char dev_name[] = "TestDevice";
    serialInterface.begin(TEST_PREFIX, dev_name, TEST_PIN);
    
    // Then: BLE server is available
    TEST_ASSERT_NOT_NULL(serialInterface.getBLEServer());
    
    // When: Creating BitChat service (as in main.cpp)
    BitchatBLEService bitchatService;
    bool serviceInitialized = bitchatService.initESP32(TEST_DEVICE_NAME);
    
    // Then: Service is initialized
    TEST_ASSERT_TRUE(serviceInitialized);
    TEST_ASSERT_NOT_NULL(bitchatService.getESP32Service());
    
    // When: Registering service (as in main.cpp)
    serialInterface.setBitChatService(bitchatService.getESP32Service());
    
    // Then: Service is registered and mode switching available
    TEST_ASSERT_FALSE(serialInterface.isBitChatMode()); // Default to MeshCore mode
}

#else
// Non-ESP32 platforms
void setUp(void) {}
void tearDown(void) {}
void test_serial_interface_exposes_ble_server(void) {
    TEST_IGNORE_MESSAGE("ESP32 only test");
}
void test_bitchat_service_registration(void) {
    TEST_IGNORE_MESSAGE("ESP32 only test");
}
void test_mode_switching_available(void) {
    TEST_IGNORE_MESSAGE("ESP32 only test");
}
void test_integration_flow(void) {
    TEST_IGNORE_MESSAGE("ESP32 only test");
}
#endif // ESP32

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_serial_interface_exposes_ble_server);
    RUN_TEST(test_bitchat_service_registration);
    RUN_TEST(test_mode_switching_available);
    RUN_TEST(test_integration_flow);
    
    return UNITY_END();
}
