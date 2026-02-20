#include <unity.h>

// Include all test suites
#include "test_button_ble_controller.h"
#include "bitchat/test_bitchat_announce.cpp"
#include "bitchat/test_bitchat_ble_service.cpp"
#include "bitchat/test_bitchat_bridge.cpp"
#include "bitchat/test_bitchat_cli.cpp"
#include "bitchat/test_bitchat_config.cpp"
#include "bitchat/test_bitchat_identity.cpp"
#include "bitchat/test_bitchat_message_reception.cpp"
#include "bitchat/test_bitchat_peer_cache.cpp"
#include "bitchat/test_ble_connection_management.cpp"
#include "bitchat/test_channel_registry.cpp"
#include "bitchat/test_compression.cpp"
#include "bitchat/test_dm_decapsulation.cpp"
#include "bitchat/test_dm_encapsulation.cpp"
#include "bitchat/test_dynamic_channel_creation.cpp"
#include "bitchat/test_fragment_reassembly.cpp"
#include "bitchat/test_group_message_decapsulation.cpp"
#include "bitchat/test_hashtag_key_derivation.cpp"
#include "bitchat/test_integration_message_flow.cpp"
#include "bitchat/test_loop_prevention.cpp"
#include "bitchat/test_mesh_channel_auto_registration.cpp"
#include "bitchat/test_meshcore_uart_service.cpp"
#include "bitchat/test_message_encapsulation.cpp"
#include "bitchat/test_message_formatting.cpp"
#include "bitchat/test_multi_channel_routing.cpp"
#include "bitchat/test_protocol_vectors.cpp"
#include "bitchat/test_public_messages.cpp"
#include "bitchat/test_shared_ble_server.cpp"
#include "bitchat/test_signature.cpp"
#include "bitchat/test_timestamp_translation.cpp"

// Forward declaration for channel test reset
namespace test {
namespace bitchat {
void resetChannelTestState();
}
namespace button_ble {
struct ButtonBLETestState;
extern ButtonBLETestState* g_buttonBLEState;
}
} // namespace test

// Global state for ButtonBLEController tests
test::button_ble::ButtonBLETestState* test::button_ble::g_buttonBLEState = nullptr;

void setUp(void) {
  // Setup before each test
  test::bitchat::resetChannelTestState();
  
  // Initialize ButtonBLEController test state
  if (test::button_ble::g_buttonBLEState == nullptr) {
    test::button_ble::g_buttonBLEState = new test::button_ble::ButtonBLETestState();
  }
}

void tearDown(void) {
  // Cleanup after each test
  if (test::button_ble::g_buttonBLEState != nullptr) {
    test::button_ble::g_buttonBLEState->reset();
  }
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

  // Run all tests from Story 1.3
  RUN_TEST(test::bitchat::test_bitchat_service_adds_to_shared_server);
  RUN_TEST(test::bitchat::test_bitchat_service_has_open_security);
  RUN_TEST(test::bitchat::test_bitchat_service_handles_message_type);
  RUN_TEST(test::bitchat::test_bitchat_service_handles_announce_type);
  RUN_TEST(test::bitchat::test_bitchat_service_sends_announcements);
  RUN_TEST(test::bitchat::test_bitchat_message_queue_fifo);

  // Run all tests from Story 1.4
  RUN_TEST(test::bitchat::test_both_services_connected_simultaneously);
  RUN_TEST(test::bitchat::test_data_isolation_between_services);
  RUN_TEST(test::bitchat::test_disconnect_isolation);
  RUN_TEST(test::bitchat::test_multiple_clients_on_same_service);
  RUN_TEST(test::bitchat::test_connection_state_per_service);
  RUN_TEST(test::bitchat::test_rapid_connect_disconnect);

  // Run all tests from Story 2.1
  RUN_TEST(test::bitchat::test_config_defaults_to_disabled);
  RUN_TEST(test::bitchat::test_enable_and_save_config);
  RUN_TEST(test::bitchat::test_disable_and_save_config);
  RUN_TEST(test::bitchat::test_config_persists_across_reboots);
  RUN_TEST(test::bitchat::test_invalid_config_handled_gracefully);
  RUN_TEST(test::bitchat::test_reset_config_to_defaults);
  RUN_TEST(test::bitchat::test_timestamp_updated_on_save);

  // Run all tests from Story 2.2
  RUN_TEST(test::bitchat::test_cli_enable_bitchat);
  RUN_TEST(test::bitchat::test_cli_disable_bitchat);
  RUN_TEST(test::bitchat::test_cli_status_bitchat);
  RUN_TEST(test::bitchat::test_cli_help_bitchat);
  RUN_TEST(test::bitchat::test_cli_non_bitchat_command_not_handled);
  RUN_TEST(test::bitchat::test_cli_unknown_subcommand_shows_error);
  RUN_TEST(test::bitchat::test_cli_command_detection);
  RUN_TEST(test::bitchat::test_cli_enable_when_already_enabled);

  // Run all tests from Story 3.1
  RUN_TEST(test::bitchat::test_derive_peer_id_from_pubkey);
  RUN_TEST(test::bitchat::test_derive_consistent_result);
  RUN_TEST(test::bitchat::test_peer_id_to_bytes);
  RUN_TEST(test::bitchat::test_bytes_to_peer_id);
  RUN_TEST(test::bitchat::test_round_trip_conversion);
  RUN_TEST(test::bitchat::test_verify_peer_id_match);
  RUN_TEST(test::bitchat::test_verify_peer_id_mismatch);
  RUN_TEST(test::bitchat::test_null_pubkey_returns_zero);
  RUN_TEST(test::bitchat::test_zero_pubkey_returns_zero);

  // Run all tests from Story 3.2
  RUN_TEST(test::bitchat::test_add_peer_to_cache);
  RUN_TEST(test::bitchat::test_find_peer_by_id);
  RUN_TEST(test::bitchat::test_find_peer_by_nickname);
  RUN_TEST(test::bitchat::test_update_existing_peer);
  RUN_TEST(test::bitchat::test_lru_eviction);
  RUN_TEST(test::bitchat::test_remove_peer);
  RUN_TEST(test::bitchat::test_touch_peer);
  RUN_TEST(test::bitchat::test_clear_cache);
  RUN_TEST(test::bitchat::test_signature_null_parameters);
  RUN_TEST(test::bitchat::test_find_nonexistent_peer);
  RUN_TEST(test::bitchat::test_get_entry_by_index);

  // Run all tests from Story 3.3
  RUN_TEST(test::bitchat::test_build_announce_payload);
  RUN_TEST(test::bitchat::test_build_announce_message);
  RUN_TEST(test::bitchat::test_announce_builder_peer_id);
  RUN_TEST(test::bitchat::test_builder_valid_with_required_fields);
  RUN_TEST(test::bitchat::test_builder_includes_noise_pubkey);
  RUN_TEST(test::bitchat::test_builder_returns_zero_for_invalid);
  RUN_TEST(test::bitchat::test_builder_truncates_long_name);

  // Run all tests from Story 4.1
  RUN_TEST(test::bitchat::test_register_channel);
  RUN_TEST(test::bitchat::test_lookup_by_name);
  RUN_TEST(test::bitchat::test_lookup_by_key);
  RUN_TEST(test::bitchat::test_unregister_channel);
  RUN_TEST(test::bitchat::test_update_existing_channel);
  RUN_TEST(test::bitchat::test_registry_max_size);
  RUN_TEST(test::bitchat::test_derive_key_from_hashtag);
  RUN_TEST(test::bitchat::test_register_default_mesh_channel);
  RUN_TEST(test::bitchat::test_clear_registry);
  RUN_TEST(test::bitchat::test_get_channel_by_index);
  RUN_TEST(test::bitchat::test_null_parameters_registry);
  RUN_TEST(test::bitchat::test_find_nonexistent_channel);

  // Run all tests from Story 4.2
  RUN_TEST(test::bitchat::test_mesh_channel_auto_registration);
  RUN_TEST(test::bitchat::test_mesh_key_derivation);
  RUN_TEST(test::bitchat::test_mesh_channel_bidirectional_lookup);
  RUN_TEST(test::bitchat::test_mesh_registration_idempotent);

  // Run all tests from Story 4.3
  RUN_TEST(test::bitchat::test_cli_channel_add);
  RUN_TEST(test::bitchat::test_cli_channel_remove);
  RUN_TEST(test::bitchat::test_cli_channel_list);
  RUN_TEST(test::bitchat::test_cli_channel_add_invalid_key);
  RUN_TEST(test::bitchat::test_cli_channel_default_shows_list);
  RUN_TEST(test::bitchat::test_cli_channel_help);

  // Run all tests from Story 4.4
  RUN_TEST(test::bitchat::test_derive_hashtag_key_with_prefix);
  RUN_TEST(test::bitchat::test_derive_hashtag_key_without_prefix);
  RUN_TEST(test::bitchat::test_different_hashtags_different_keys);
  RUN_TEST(test::bitchat::test_normalize_hashtag_adds_prefix);
  RUN_TEST(test::bitchat::test_normalize_hashtag_preserves_prefix);
  RUN_TEST(test::bitchat::test_hashtag_key_consistent);
  RUN_TEST(test::bitchat::test_hashtag_null_parameters);
  RUN_TEST(test::bitchat::test_hashtag_empty_string);

  // Run all tests from Story 5.1
  RUN_TEST(test::bitchat::test_receive_message_packet);
  RUN_TEST(test::bitchat::test_parse_message_with_all_tlvs);
  RUN_TEST(test::bitchat::test_parse_unsupported_version);
  RUN_TEST(test::bitchat::test_parse_truncated_message);
  RUN_TEST(test::bitchat::test_supported_message_types);
  RUN_TEST(test::bitchat::test_parse_empty_payload);
  RUN_TEST(test::bitchat::test_parse_truncated_tlv);

  // Run all tests from Story 5.2
  RUN_TEST(test::bitchat::test_encapsulate_message_basic);
  RUN_TEST(test::bitchat::test_encapsulation_header_format);
  RUN_TEST(test::bitchat::test_encapsulation_encrypts_payload);
  RUN_TEST(test::bitchat::test_encapsulate_large_message_fragments);
  RUN_TEST(test::bitchat::test_fragment_header_format);
  RUN_TEST(test::bitchat::test_small_message_no_fragmentation);
  RUN_TEST(test::bitchat::test_encapsulate_null_parameters);
  RUN_TEST(test::bitchat::test_decapsulate_recover_original);

  // Run all tests from Story 5.3
  RUN_TEST(test::bitchat::test_route_to_registered_channel);
  RUN_TEST(test::bitchat::test_route_to_different_channels);
  RUN_TEST(test::bitchat::test_unknown_channel_handling);
  RUN_TEST(test::bitchat::test_default_channel_for_unknown);
  RUN_TEST(test::bitchat::test_fragmentation_with_channel_routing);
  RUN_TEST(test::bitchat::test_channel_name_normalization);
  RUN_TEST(test::bitchat::test_null_empty_channel_name);
  RUN_TEST(test::bitchat::test_multiple_messages_multiple_channels);

  // Run all tests from Story 5.4
  RUN_TEST(test::bitchat::test_dm_to_known_contact);
  RUN_TEST(test::bitchat::test_dm_encapsulation_format);
  RUN_TEST(test::bitchat::test_dm_to_unknown_recipient);
  RUN_TEST(test::bitchat::test_dm_without_recipient_flag);
  RUN_TEST(test::bitchat::test_large_dm_fragmentation);
  RUN_TEST(test::bitchat::test_multiple_dm_recipients);
  RUN_TEST(test::bitchat::test_dm_with_sender_verification);

  // Run all tests from Story 6.1
  RUN_TEST(test::bitchat::test_receive_encapsulated_bitchat);
  RUN_TEST(test::bitchat::test_ignore_non_bitchat_messages);
  RUN_TEST(test::bitchat::test_wrong_channel_key);
  RUN_TEST(test::bitchat::test_packet_too_short);
  RUN_TEST(test::bitchat::test_unsupported_version);
  RUN_TEST(test::bitchat::test_detect_bitchat_magic);
  RUN_TEST(test::bitchat::test_multiple_channels_decapsulation);

  // Run all tests from Story 6.2
  RUN_TEST(test::bitchat::test_receive_dm_from_known_contact);
  RUN_TEST(test::bitchat::test_dm_from_non_bitchat_source);
  RUN_TEST(test::bitchat::test_dm_from_unknown_contact);
  RUN_TEST(test::bitchat::test_dm_wrong_recipient_key);
  RUN_TEST(test::bitchat::test_dm_packet_too_short);
  RUN_TEST(test::bitchat::test_dm_sender_info_extraction);
  RUN_TEST(test::bitchat::test_multiple_dm_senders);

  // Run all tests from Story 6.3
  RUN_TEST(test::bitchat::test_format_group_message);
  RUN_TEST(test::bitchat::test_format_with_special_chars);
  RUN_TEST(test::bitchat::test_format_long_message);
  RUN_TEST(test::bitchat::test_format_dm_message);
  RUN_TEST(test::bitchat::test_format_preserves_timestamp);
  RUN_TEST(test::bitchat::test_format_empty_content);
  RUN_TEST(test::bitchat::test_format_null_content);
  RUN_TEST(test::bitchat::test_format_with_angle_brackets);

  // Run all tests from Story 6.4
  RUN_TEST(test::bitchat::test_detect_duplicate_message);
  RUN_TEST(test::bitchat::test_new_message_not_duplicate);
  RUN_TEST(test::bitchat::test_different_senders_same_msgid);
  RUN_TEST(test::bitchat::test_cache_eviction);
  RUN_TEST(test::bitchat::test_clear_old_entries);
  RUN_TEST(test::bitchat::test_cache_size_limit);
  RUN_TEST(test::bitchat::test_clear_all_entries);
  RUN_TEST(test::bitchat::test_message_hash_consistency);

  // Run all tests from Story 7.1
  RUN_TEST(test::bitchat::test_sync_time_from_bitchat);
  RUN_TEST(test::bitchat::test_small_time_difference_no_sync);
  RUN_TEST(test::bitchat::test_use_bitchat_timestamp_for_meshcore);
  RUN_TEST(test::bitchat::test_invalid_timestamp_handling);
  RUN_TEST(test::bitchat::test_time_difference_calculation);
  RUN_TEST(test::bitchat::test_custom_sync_threshold);
  RUN_TEST(test::bitchat::test_future_timestamp_rejection);
  RUN_TEST(test::bitchat::test_timestamp_formatting);

  // Run all tests from Story 7.2
  RUN_TEST(test::bitchat::test_compress_decompress_roundtrip);
  RUN_TEST(test::bitchat::test_small_data_compression);
  RUN_TEST(test::bitchat::test_compression_buffer_too_small);
  RUN_TEST(test::bitchat::test_decompression_buffer_too_small);
  RUN_TEST(test::bitchat::test_is_compressed_check);
  RUN_TEST(test::bitchat::test_compression_ratio);
  RUN_TEST(test::bitchat::test_compress_empty_data);
  RUN_TEST(test::bitchat::test_large_data_compression);

  // Run all tests from Story 7.3
  RUN_TEST(test::bitchat::test_reassemble_fragments);
  RUN_TEST(test::bitchat::test_incomplete_fragments);
  RUN_TEST(test::bitchat::test_fragment_timeout);
  RUN_TEST(test::bitchat::test_duplicate_fragment);
  RUN_TEST(test::bitchat::test_clear_message_fragments);
  RUN_TEST(test::bitchat::test_multiple_messages);
  RUN_TEST(test::bitchat::test_reassembly_buffer_too_small);
  RUN_TEST(test::bitchat::test_clear_all_fragments);

  // Run all tests from Story 7.4
  RUN_TEST(test::bitchat::test_sign_message);
  RUN_TEST(test::bitchat::test_verify_valid_signature);
  RUN_TEST(test::bitchat::test_reject_invalid_signature);
  RUN_TEST(test::bitchat::test_sign_verify_roundtrip);
  RUN_TEST(test::bitchat::test_different_messages_different_signatures);
  RUN_TEST(test::bitchat::test_same_message_different_keys);
  RUN_TEST(test::bitchat::test_null_parameters);
  RUN_TEST(test::bitchat::test_empty_message);

  // Run all tests from Story 8.1
  RUN_TEST(test::bitchat::test_route_public_message_to_bitchat);
  RUN_TEST(test::bitchat::test_default_target_channel);
  RUN_TEST(test::bitchat::test_configurable_target_channel);
  RUN_TEST(test::bitchat::test_unknown_target_channel);
  RUN_TEST(test::bitchat::test_empty_public_message);
  RUN_TEST(test::bitchat::test_long_public_message);
  RUN_TEST(test::bitchat::test_get_target_channel);
  RUN_TEST(test::bitchat::test_multiple_public_messages);

  // Run all tests from Story 9.1
  RUN_TEST(test::bitchat::test_vector_message_type);
  RUN_TEST(test::bitchat::test_vector_announce_type);
  RUN_TEST(test::bitchat::test_vector_leave_type);
  RUN_TEST(test::bitchat::test_fuzz_random_data);
  RUN_TEST(test::bitchat::test_fuzz_truncated_packets);
  RUN_TEST(test::bitchat::test_fuzz_corrupted_data);
  RUN_TEST(test::bitchat::test_encapsulation_vectors);
  RUN_TEST(test::bitchat::test_roundtrip_vectors);
  RUN_TEST(test::bitchat::test_edge_zero_length_payload);

  // Run all tests from Story 9.2 (Integration)
  RUN_TEST(test::bitchat::test_integration_alice_to_bob);
  RUN_TEST(test::bitchat::test_integration_loop_prevention);
  RUN_TEST(test::bitchat::test_integration_channel_routing);

  // Run all tests from Story 10.1 (BitchatBridge — both relay directions)
  // BitChat → MeshCore
  RUN_TEST(test::bitchat::test_bridge_initialization);
  RUN_TEST(test::bitchat::test_mesh_channel_ready_after_begin); // REGRESSION guard (channel secret)
  RUN_TEST(test::bitchat::test_bitchat_message_relayed_to_mesh_after_begin); // REGRESSION guard (encap+relay)

  RUN_TEST(test::bitchat::test_multiple_bitchat_messages_each_relayed);
  RUN_TEST(test::bitchat::test_init_mesh_channel_idempotent);
  // MeshCore → BitChat
  RUN_TEST(test::bitchat::test_mesh_to_bitchat_accepts_mesh_channel_msg);
  RUN_TEST(test::bitchat::test_mesh_to_bitchat_drops_non_mesh_channel);
  RUN_TEST(test::bitchat::test_mesh_to_bitchat_loop_prevention);
  RUN_TEST(test::bitchat::test_mesh_to_bitchat_skips_bitchat_origin_prefix);
  RUN_TEST(test::bitchat::test_bridge_loop_processing);
  // computeChannelHash — routing byte
  RUN_TEST(test::bitchat::test_compute_channel_hash_produces_correct_routing_byte);
  RUN_TEST(test::bitchat::test_compute_channel_hash_deterministic);
  RUN_TEST(test::bitchat::test_compute_channel_hash_different_secrets_differ);
  // onMeshcoreEncapsulatedMessage — component path
  RUN_TEST(test::bitchat::test_encapsulated_message_decapsulates_with_mesh_key);
  RUN_TEST(test::bitchat::test_encapsulated_non_bitchat_packet_rejected);
  // derivePeerId — wrapper behaviour
  RUN_TEST(test::bitchat::test_derive_peer_id_null_key_returns_zero);
  RUN_TEST(test::bitchat::test_derive_peer_id_unique_per_key);

  // ButtonBLEController Tests
  RUN_TEST(test::button_ble::test_controller_initialization_creates_button);
  RUN_TEST(test::button_ble::test_controller_initializes_in_meshcore_mode);
  RUN_TEST(test::button_ble::test_controller_registers_quintuple_press_callback);
  RUN_TEST(test::button_ble::test_quintuple_press_toggles_to_bitchat_mode);
  RUN_TEST(test::button_ble::test_quintuple_press_toggles_back_to_meshcore_mode);
  RUN_TEST(test::button_ble::test_mode_switch_calls_serial_interface);
  RUN_TEST(test::button_ble::test_meshcore_mode_led_slow_blink);
  RUN_TEST(test::button_ble::test_bitchat_mode_led_fast_blink);
  RUN_TEST(test::button_ble::test_led_blinks_three_times);
  RUN_TEST(test::button_ble::test_meshcore_mode_low_tone);
  RUN_TEST(test::button_ble::test_bitchat_mode_high_tone);
  RUN_TEST(test::button_ble::test_buzzer_enable_pin_set);
  RUN_TEST(test::button_ble::test_loop_updates_button);
  RUN_TEST(test::button_ble::test_multiple_switches_in_sequence);
  RUN_TEST(test::button_ble::test_controller_active_after_begin);
  RUN_TEST(test::button_ble::test_indicate_methods_work_correctly);

  UNITY_END();

  return 0;
}
