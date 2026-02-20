# ButtonBLEController Test Suite

This document describes the test suite for the ButtonBLEController class, which provides button-based BLE mode switching for display-less devices.

## Overview

The test suite covers:
- **Initialization**: Controller setup and button registration
- **Mode Switching**: Quintuple press toggles between MeshCore and BitChat modes
- **LED Feedback**: Visual indication with different blink patterns
- **Buzzer Feedback**: Audio indication with different tones
- **Edge Cases**: Multiple switches, state management

## Test Structure

```
test/
├── test_button_ble_controller.h      # Main test file (header)
├── test_main.cpp                      # Test runner (includes button_ble tests)
└── BUTTON_BLE_CONTROLLER_TESTS.md     # This documentation
```

## Test Categories

### 1. Initialization Tests (3 tests)

| Test | Description |
|------|-------------|
| `test_controller_initialization_creates_button` | Verifies Button instance is created |
| `test_controller_initializes_in_meshcore_mode` | Verifies initial state is MeshCore |
| `test_controller_registers_quintuple_press_callback` | Verifies callback is registered |

### 2. Mode Switching Tests (3 tests)

| Test | Description |
|------|-------------|
| `test_quintuple_press_toggles_to_bitchat_mode` | 5x press switches to BitChat |
| `test_quintuple_press_toggles_back_to_meshcore_mode` | 5x press switches back |
| `test_mode_switch_calls_serial_interface` | Verifies SerialBLEInterface calls |

### 3. LED Feedback Tests (5 tests)

| Test | Description |
|------|-------------|
| `test_meshcore_mode_led_slow_blink` | MeshCore = 500ms blink interval |
| `test_bitchat_mode_led_fast_blink` | BitChat = 150ms blink interval |
| `test_led_blinks_three_times` | Exactly 3 blinks for feedback |
| `test_led_turns_off_after_blinking` | LED off after feedback complete |
| `test_led_state_changes_during_blink` | Verifies LED state transitions |

### 4. Buzzer Feedback Tests (3 tests)

| Test | Description |
|------|-------------|
| `test_meshcore_mode_low_tone` | MeshCore = Octave 5 (low) tone |
| `test_bitchat_mode_high_tone` | BitChat = Octave 6 (high) tone |
| `test_buzzer_enable_pin_set` | Buzzer enable pin activated |

### 5. Loop Update Tests (2 tests)

| Test | Description |
|------|-------------|
| `test_loop_updates_button` | Button state updated each loop |
| `test_loop_updates_led_feedback` | LED timing updated each loop |

### 6. Edge Case Tests (3 tests)

| Test | Description |
|------|-------------|
| `test_multiple_switches_in_sequence` | Rapid mode switching works |
| `test_controller_active_after_begin` | isActive() returns correct state |
| `test_indicate_methods_work_correctly` | Direct indication calls work |

## Running the Tests

### Prerequisites

```bash
# Install PlatformIO
pip install platformio

# Install Unity test framework (via PlatformIO)
pio pkg install -g --tool throwtheswitch/Unity@^2.6.1
```

### Run All Tests

```bash
# Run all tests including ButtonBLEController tests
pio test -e native_test
```

### Run Specific Test File

```bash
# Run only ButtonBLEController tests
pio test -e native_test --filter "test_button_ble_controller"
```

### Run with Verbose Output

```bash
# Detailed test output
pio test -e native_test -v
```

## Test Implementation Details

### Mock Objects

The tests use mock implementations to avoid hardware dependencies:

#### MockSerialInterface
```cpp
class MockSerialInterface {
    bool bitchatMode = false;
    int setBitChatModeCallCount = 0;
    int isBitChatModeCallCount = 0;
    // ...
};
```

#### MockButton
```cpp
class MockButton {
    EventCallback onQuintuplePressCallback = nullptr;
    bool beginCalled = false;
    int updateCallCount = 0;
    // ...
};
```

### Timing Simulation

Tests use a simulated millis() counter:
```cpp
static uint32_t mockMillis = 0;
void simulateTime(uint32_t timeMs) {
    mockMillis = timeMs;
}
```

### Hardware State Tracking

Tests track hardware state changes:
```cpp
static uint8_t mockLedState = LOW;
static uint8_t mockBuzzerEnState = LOW;
```

## Expected Test Output

```
Verbose mode can be enabled via -v, --verbose option
Collected 19 tests

test/ButtonBLEController/test_button_ble_controller.cpp:157
test_controller_initialization_creates_button        PASSED

test/ButtonBLEController/test_button_ble_controller.cpp:165
test_controller_initializes_in_meshcore_mode         PASSED

...

-----------------------
19 Tests 0 Failures 0 Ignored 
OK
```

## Adding New Tests

To add a new test:

1. Add test function to `test_button_ble_controller.cpp`:
```cpp
void test_your_new_test() {
    // Setup
    controller->begin();
    
    // Execute
    // ... test code ...
    
    // Verify
    TEST_ASSERT_EQUAL(expected, actual);
}
```

2. Register test in `test_main.cpp`:
```cpp
RUN_TEST(test::button_ble::test_your_new_test);
```

3. Update the header file `test_button_ble_controller.h` with the new test function declaration.

3. Run tests to verify:
```bash
pio test -e native_test
```

## Continuous Integration

Add to your CI pipeline:

```yaml
# Example GitHub Actions
- name: Run ButtonBLEController Tests
  run: |
    pip install platformio
    pio test -e native_test
```

## Debugging Failed Tests

### Enable Verbose Output
```bash
pio test -e native_test -v --verbose
```

### Run Single Test
```cpp
// In test_main.cpp, comment out other tests:
RUN_TEST(test::button_ble::test_specific_failing_test);
```

### Add Debug Output
```cpp
void test_something() {
    Serial.println("Debug: Starting test");
    // ...
    Serial.print("Debug: Value = ");
    Serial.println(value);
}
```

## Test Coverage

Current coverage areas:
- ✅ Initialization
- ✅ Mode switching logic
- ✅ LED feedback timing
- ✅ Buzzer feedback tones
- ✅ Loop updates
- ✅ Edge cases

Areas for future tests:
- ⏳ Long press behavior (if implemented)
- ⏳ Double/triple press handling
- ⏳ Button debouncing
- ⏳ Concurrent mode switches
- ⏳ Memory leak detection
- ⏳ Power consumption during feedback

## Related Documentation

- [Button BLE Controller Feature](../../docs/button_ble_controller.md)
- [BitChat Compatibility](../../docs/bitchat/compatibility_devices.md)
- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity)
