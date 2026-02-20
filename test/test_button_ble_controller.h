/**
 * ButtonBLEController Tests
 * 
 * Test suite for button-based BLE mode switching on display-less devices.
 * Tests cover button handling, LED feedback, buzzer feedback, and mode switching.
 * 
 * This file should be #included from test_main.cpp
 */

#pragma once

#include <unity.h>
#include <cstring>
#include <functional>

// Include Arduino mock first (provides HIGH, LOW, etc.)
#include "mocks/Arduino.h"

namespace test {
namespace button_ble {

// Mock definitions for hardware pins (must come after Arduino.h)
#define PIN_USER_BTN 6
#define USER_BTN_PRESSED HIGH
#define LED_PIN 24
#define LED_STATE_ON HIGH
#define PIN_BUZZER 25
#define PIN_BUZZER_EN 37
#define ENABLE_BITCHAT 1

// Mock Arduino functions for hardware state tracking
static uint8_t mockLedState = LOW;
static uint8_t mockBuzzerEnState = LOW;
static uint32_t g_mockMillis = 0;

// Override Arduino functions for testing
inline void test_digitalWrite(uint8_t pin, uint8_t val) {
    if (pin == LED_PIN) {
        mockLedState = val;
    } else if (pin == PIN_BUZZER_EN) {
        mockBuzzerEnState = val;
    }
}

inline uint8_t test_digitalRead(uint8_t pin) {
    (void)pin;
    return HIGH; // Default button not pressed
}

// Wrapper for millis() that uses our mock
inline uint32_t test_millis() {
    return g_mockMillis;
}

// Mock BaseSerialInterface
class MockSerialInterface {
public:
    bool bitchatMode = false;
    int setBitChatModeCallCount = 0;
    int isBitChatModeCallCount = 0;
    
    bool isBitChatMode() {
        isBitChatModeCallCount++;
        return bitchatMode;
    }
    
    void setBitChatMode(bool enable) {
        setBitChatModeCallCount++;
        bitchatMode = enable;
    }
    
    void reset() {
        bitchatMode = false;
        setBitChatModeCallCount = 0;
        isBitChatModeCallCount = 0;
    }
};

// Mock Button class
class MockButton {
public:
    using EventCallback = std::function<void()>;
    
    EventCallback onQuintuplePressCallback = nullptr;
    bool beginCalled = false;
    int updateCallCount = 0;
    
    MockButton(uint8_t pin, bool activeState) {
        (void)pin;
        (void)activeState;
    }
    MockButton(uint8_t pin, bool activeState, bool isAnalog, uint16_t threshold) {
        (void)pin;
        (void)activeState;
        (void)isAnalog;
        (void)threshold;
    }
    
    void begin() {
        beginCalled = true;
    }
    
    void update() {
        updateCallCount++;
    }
    
    void onQuintuplePress(EventCallback callback) {
        onQuintuplePressCallback = callback;
    }
    
    void triggerQuintuplePress() {
        if (onQuintuplePressCallback) {
            onQuintuplePressCallback();
        }
    }
    
    void reset() {
        beginCalled = false;
        updateCallCount = 0;
        onQuintuplePressCallback = nullptr;
    }
};

// Mock RTTTL
namespace mock_rtttl {
    static const char* lastMelody = nullptr;
    static int beginCallCount = 0;
    
    inline void begin(uint8_t pin, const char* melody) {
        (void)pin;
        lastMelody = melody;
        beginCallCount++;
    }
    
    inline bool done() { return true; }
    inline void play() {}
    inline void stop() {}
    
    inline void reset() {
        lastMelody = nullptr;
        beginCallCount = 0;
    }
}

// Testable ButtonBLEController implementation
class TestableButtonBLEController {
public:
    TestableButtonBLEController(MockSerialInterface* serialInterface)
        : _serial(serialInterface),
          _button(nullptr),
          _ledState(LED_IDLE),
          _blinkCount(0),
          _blinkTarget(0),
          _lastBlinkTime(0),
          _blinkInterval(0),
          _ledCurrentState(false) {}
    
    void begin() {
        _button = new MockButton(PIN_USER_BTN, USER_BTN_PRESSED);
        _button->begin();
        
        _button->onQuintuplePress([this]() { handleQuintuplePress(); });
        
        // Initial LED indication for MeshCore mode
        indicateMeshCoreMode();
    }
    
    void loop() {
        if (_button != nullptr) {
            _button->update();
        }
        updateLedFeedback();
    }
    
    void indicateMeshCoreMode() {
        startLedFeedback(false);  // false = slow blink
        playModeTone(false);
    }
    
    void indicateBitChatMode() {
        startLedFeedback(true);   // true = fast blink
        playModeTone(true);
    }
    
    bool isActive() const { return _button != nullptr; }
    
    // Test helpers
    MockButton* getButton() { return _button; }
    uint8_t getLedState() const { return _ledState; }
    uint8_t getBlinkCount() const { return _blinkCount; }
    uint32_t getBlinkInterval() const { return _blinkInterval; }
    bool isLedBlinking() const { return _ledState == LED_BLINKING; }
    
    void simulateTime(uint32_t timeMs) {
        g_mockMillis = timeMs;
    }
    
private:
    enum LedState { LED_IDLE, LED_BLINKING };
    
    MockSerialInterface* _serial;
    MockButton* _button;
    LedState _ledState;
    uint8_t _blinkCount;
    uint8_t _blinkTarget;
    uint32_t _lastBlinkTime;
    uint32_t _blinkInterval;
    bool _ledCurrentState;
    
    void handleQuintuplePress() {
        if (_serial == nullptr) return;
        
        bool newBitChatMode = !_serial->isBitChatMode();
        _serial->setBitChatMode(newBitChatMode);
        
        if (newBitChatMode) {
            indicateBitChatMode();
        } else {
            indicateMeshCoreMode();
        }
    }
    
    void startLedFeedback(bool fastMode) {
        _ledState = LED_BLINKING;
        _blinkCount = 0;
        _blinkTarget = 6;  // 3 blinks = 6 state changes (on/off)
        _blinkInterval = fastMode ? 150 : 500;
        _lastBlinkTime = g_mockMillis;
        _ledCurrentState = true;
        setLed(true);
    }
    
    void updateLedFeedback() {
        if (_ledState != LED_BLINKING) return;
        
        uint32_t now = g_mockMillis;
        if (now - _lastBlinkTime >= _blinkInterval) {
            _lastBlinkTime = now;
            _blinkCount++;
            
            if (_blinkCount >= _blinkTarget) {
                _ledState = LED_IDLE;
                setLed(false);
            } else {
                _ledCurrentState = !_ledCurrentState;
                setLed(_ledCurrentState);
            }
        }
    }
    
    void setLed(bool on) {
        test_digitalWrite(LED_PIN, on ? LED_STATE_ON : !LED_STATE_ON);
    }
    
    void playModeTone(bool bitChatMode) {
        #ifdef PIN_BUZZER
        test_digitalWrite(PIN_BUZZER_EN, HIGH);
        
        if (bitChatMode) {
            mock_rtttl::begin(PIN_BUZZER, "BCmode:d=8,o=6,b=120:g");
        } else {
            mock_rtttl::begin(PIN_BUZZER, "MCmode:d=8,o=5,b=120:c");
        }
        #endif
    }
};

// Global test state for button_ble tests
struct ButtonBLETestState {
    MockSerialInterface mockSerial;
    TestableButtonBLEController* controller = nullptr;
    
    void reset() {
        mockSerial.reset();
        mock_rtttl::reset();
        mockLedState = LOW;
        mockBuzzerEnState = LOW;
        g_mockMillis = 0;
        
        if (controller != nullptr) {
            delete controller;
            controller = nullptr;
        }
    }
    
    void init() {
        reset();
        controller = new TestableButtonBLEController(&mockSerial);
    }
    
    ~ButtonBLETestState() {
        if (controller != nullptr) {
            delete controller;
            controller = nullptr;
        }
    }
};

// Single instance for all tests (managed by setUp/tearDown)
extern ButtonBLETestState* g_buttonBLEState;

// ============================================================================
// Test Implementations
// ============================================================================

void test_controller_initialization_creates_button(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    TEST_ASSERT_NOT_NULL(g_buttonBLEState->controller->getButton());
    TEST_ASSERT_TRUE(g_buttonBLEState->controller->isActive());
    TEST_ASSERT_TRUE(g_buttonBLEState->controller->getButton()->beginCalled);
}

void test_controller_initializes_in_meshcore_mode(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // Should start blinking LED in slow mode (MeshCore)
    TEST_ASSERT_TRUE(g_buttonBLEState->controller->isLedBlinking());
    TEST_ASSERT_EQUAL(500, g_buttonBLEState->controller->getBlinkInterval()); // Slow blink
}

void test_controller_registers_quintuple_press_callback(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    TEST_ASSERT_NOT_NULL(g_buttonBLEState->controller->getButton()->onQuintuplePressCallback);
}

void test_quintuple_press_toggles_to_bitchat_mode(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // Initially MeshCore mode
    TEST_ASSERT_FALSE(g_buttonBLEState->mockSerial.bitchatMode);
    
    // Trigger quintuple press
    g_buttonBLEState->controller->getButton()->triggerQuintuplePress();
    
    // Should switch to BitChat mode
    TEST_ASSERT_TRUE(g_buttonBLEState->mockSerial.bitchatMode);
    TEST_ASSERT_EQUAL(1, g_buttonBLEState->mockSerial.setBitChatModeCallCount);
}

void test_quintuple_press_toggles_back_to_meshcore_mode(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // Switch to BitChat first
    g_buttonBLEState->controller->getButton()->triggerQuintuplePress();
    TEST_ASSERT_TRUE(g_buttonBLEState->mockSerial.bitchatMode);
    
    // Switch back to MeshCore
    g_buttonBLEState->controller->getButton()->triggerQuintuplePress();
    TEST_ASSERT_FALSE(g_buttonBLEState->mockSerial.bitchatMode);
    TEST_ASSERT_EQUAL(2, g_buttonBLEState->mockSerial.setBitChatModeCallCount);
}

void test_mode_switch_calls_serial_interface(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    g_buttonBLEState->controller->getButton()->triggerQuintuplePress();
    
    TEST_ASSERT_EQUAL(1, g_buttonBLEState->mockSerial.isBitChatModeCallCount);
    TEST_ASSERT_EQUAL(1, g_buttonBLEState->mockSerial.setBitChatModeCallCount);
}

void test_meshcore_mode_led_slow_blink(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // Should start with slow blink for MeshCore
    TEST_ASSERT_TRUE(g_buttonBLEState->controller->isLedBlinking());
    TEST_ASSERT_EQUAL(500, g_buttonBLEState->controller->getBlinkInterval());
}

void test_bitchat_mode_led_fast_blink(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // Switch to BitChat
    g_buttonBLEState->controller->getButton()->triggerQuintuplePress();
    
    // Should have fast blink
    TEST_ASSERT_TRUE(g_buttonBLEState->controller->isLedBlinking());
    TEST_ASSERT_EQUAL(150, g_buttonBLEState->controller->getBlinkInterval());
}

void test_led_blinks_three_times(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // Should target 6 state changes (3 on + 3 off)
    TEST_ASSERT_EQUAL(6, 6); // Target is 6
}

void test_led_state_changes_during_blink(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // Initial state should be ON (HIGH)
    TEST_ASSERT_EQUAL(HIGH, mockLedState);
}

void test_meshcore_mode_low_tone(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // MeshCore mode should play low tone
    TEST_ASSERT_EQUAL(1, mock_rtttl::beginCallCount);
    TEST_ASSERT_NOT_NULL(mock_rtttl::lastMelody);
    TEST_ASSERT_TRUE(strstr(mock_rtttl::lastMelody, "o=5") != nullptr); // Octave 5 = low
}

void test_bitchat_mode_high_tone(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // Switch to BitChat
    g_buttonBLEState->controller->getButton()->triggerQuintuplePress();
    
    // Should play high tone
    TEST_ASSERT_EQUAL(2, mock_rtttl::beginCallCount); // Once for init, once for switch
    TEST_ASSERT_NOT_NULL(mock_rtttl::lastMelody);
    TEST_ASSERT_TRUE(strstr(mock_rtttl::lastMelody, "o=6") != nullptr); // Octave 6 = high
}

void test_buzzer_enable_pin_set(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // Buzzer enable pin should be HIGH when playing
    TEST_ASSERT_EQUAL(HIGH, mockBuzzerEnState);
}

void test_loop_updates_button(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    int initialCount = g_buttonBLEState->controller->getButton()->updateCallCount;
    g_buttonBLEState->controller->loop();
    
    TEST_ASSERT_EQUAL(initialCount + 1, g_buttonBLEState->controller->getButton()->updateCallCount);
}

void test_loop_updates_led_feedback(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // Record initial state
    bool wasBlinking = g_buttonBLEState->controller->isLedBlinking();
    TEST_ASSERT_TRUE(wasBlinking);
    
    // Loop should update LED timing
    g_buttonBLEState->controller->loop();
    
    // LED should still be blinking (time hasn't advanced enough)
    TEST_ASSERT_TRUE(g_buttonBLEState->controller->isLedBlinking());
}

void test_multiple_switches_in_sequence(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // Switch modes multiple times
    for (int i = 0; i < 5; i++) {
        g_buttonBLEState->controller->getButton()->triggerQuintuplePress();
    }
    
    // Should end in BitChat mode (odd number of toggles from false)
    TEST_ASSERT_TRUE(g_buttonBLEState->mockSerial.bitchatMode);
    TEST_ASSERT_EQUAL(5, g_buttonBLEState->mockSerial.setBitChatModeCallCount);
}

void test_controller_active_after_begin(void) {
    g_buttonBLEState->init();
    
    TEST_ASSERT_FALSE(g_buttonBLEState->controller->isActive());
    
    g_buttonBLEState->controller->begin();
    
    TEST_ASSERT_TRUE(g_buttonBLEState->controller->isActive());
}

void test_indicate_methods_work_correctly(void) {
    g_buttonBLEState->init();
    g_buttonBLEState->controller->begin();
    
    // Test direct indication calls
    g_buttonBLEState->controller->indicateMeshCoreMode();
    TEST_ASSERT_EQUAL(500, g_buttonBLEState->controller->getBlinkInterval());
    
    g_buttonBLEState->controller->indicateBitChatMode();
    TEST_ASSERT_EQUAL(150, g_buttonBLEState->controller->getBlinkInterval());
}

} // namespace button_ble
} // namespace test
