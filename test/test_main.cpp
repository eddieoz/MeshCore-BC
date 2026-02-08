#include <unity.h>

// Include all test suites
#include "bitchat/test_shared_ble_server.cpp"
#include "bitchat/test_meshcore_uart_service.cpp"

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // Run all tests from Story 1.1
    RUN_TEST(test::bitchat::test_shared_ble_server_can_host_multiple_services);
    RUN_TEST(test::bitchat::test_services_maintain_independent_connection_state);
    RUN_TEST(test::bitchat::test_different_security_levels_per_service);
    RUN_TEST(test::bitchat::test_service_data_isolation);
    RUN_TEST(test::bitchat::test_dynamic_service_registration);
    
    // Run all tests from Story 1.2
    RUN_TEST(test::bitchat::test_meshcore_service_preserves_nordic_uart_uuid);
    RUN_TEST(test::bitchat::test_meshcore_service_requires_pin_authentication);
    RUN_TEST(test::bitchat::test_meshcore_service_works_with_shared_server);
    RUN_TEST(test::bitchat::test_meshcore_service_protocol_frames);
    RUN_TEST(test::bitchat::test_meshcore_service_backward_compatibility);
    RUN_TEST(test::bitchat::test_meshcore_service_frame_queue);
    
    UNITY_END();
    
    return 0;
}
