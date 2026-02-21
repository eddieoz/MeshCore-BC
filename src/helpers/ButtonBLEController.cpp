#include "ButtonBLEController.h"

// Only compile if we have button support and BitChat enabled
#if defined(PIN_USER_BTN) && defined(ENABLE_BITCHAT)

#include "Button.h"  // From ui-orig folder (included in build path)

// LED blink timing
#define MESHCORE_BLINK_INTERVAL_MS  500  // Slow blink for MeshCore (500ms on/off)
#define BITCHAT_BLINK_INTERVAL_MS   150  // Fast blink for BitChat (150ms on/off)
#define LED_FEEDBACK_BLINK_COUNT    3    // Number of blinks for feedback

// Buzzer melodies for mode indication
#define MESHCORE_MODE_TONE  "MCmode:d=8,o=5,b=120:c"      // Low tone for MeshCore
#define BITCHAT_MODE_TONE   "BCmode:d=8,o=6,b=120:g"      // High tone for BitChat

ButtonBLEController::ButtonBLEController(BaseSerialInterface* serialInterface)
    : _serial(serialInterface),
      _button(nullptr),
      _ledState(LED_IDLE),
      _blinkCount(0),
      _blinkTarget(0),
      _lastBlinkTime(0),
      _blinkInterval(0),
      _ledCurrentState(false)
{
}

void ButtonBLEController::begin() {
    // Only initialize if we have a button pin and BLE interface
    #ifdef PIN_USER_BTN
    if (_serial != nullptr) {
        _button = new Button(PIN_USER_BTN, USER_BTN_PRESSED);
        _button->begin();
        
        // Set up triple press callback
        _button->onTriplePress([this]() { handleTriplePress(); });
        
        Serial.println("[ButtonBLEController] Initialized - 3x press to toggle mode");
        
        // Initial LED indication for MeshCore mode (boot state)
        indicateMeshCoreMode();
    }
    #endif
    
    // Initialize LED pin if available
    #ifdef LED_PIN
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    #endif
    
    #ifdef PIN_STATUS_LED
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);
    #endif
}

void ButtonBLEController::loop() {
    // Update button state
    if (_button != nullptr) {
        _button->update();
    }
    
    // Update LED feedback
    updateLedFeedback();
}

void ButtonBLEController::handleTriplePress() {
    if (_serial == nullptr) return;
    
    // Toggle BitChat mode
    bool newBitChatMode = !_serial->isBitChatMode();
    _serial->setBitChatMode(newBitChatMode);
    
    Serial.print("[ButtonBLEController] Mode switched to: ");
    Serial.println(newBitChatMode ? "BitChat" : "MeshCore");
    
    // Provide feedback
    if (newBitChatMode) {
        indicateBitChatMode();
    } else {
        indicateMeshCoreMode();
    }
}

void ButtonBLEController::indicateMeshCoreMode() {
    Serial.println("[ButtonBLEController] Indicating MeshCore mode (3 slow blinks)");
    startLedFeedback(false);  // false = slow blink
    playModeTone(false);      // false = MeshCore tone
}

void ButtonBLEController::indicateBitChatMode() {
    Serial.println("[ButtonBLEController] Indicating BitChat mode (3 fast blinks)");
    startLedFeedback(true);   // true = fast blink
    playModeTone(true);       // true = BitChat tone
}

void ButtonBLEController::startLedFeedback(bool fastMode) {
    _ledState = LED_BLINKING;
    _blinkCount = 0;
    _blinkTarget = LED_FEEDBACK_BLINK_COUNT * 2;  // *2 because on+off = 1 blink
    _blinkInterval = fastMode ? BITCHAT_BLINK_INTERVAL_MS : MESHCORE_BLINK_INTERVAL_MS;
    _lastBlinkTime = millis();
    _ledCurrentState = true;  // Start with LED ON
    setLed(true);
}

void ButtonBLEController::updateLedFeedback() {
    if (_ledState != LED_BLINKING) return;
    
    uint32_t now = millis();
    if (now - _lastBlinkTime >= _blinkInterval) {
        _lastBlinkTime = now;
        _blinkCount++;
        
        if (_blinkCount >= _blinkTarget) {
            // Finished blinking
            _ledState = LED_IDLE;
            setLed(false);  // Turn LED off
        } else {
            // Toggle LED
            _ledCurrentState = !_ledCurrentState;
            setLed(_ledCurrentState);
        }
    }
}

void ButtonBLEController::setLed(bool on) {
    #ifdef LED_PIN
    digitalWrite(LED_PIN, on ? LED_STATE_ON : !LED_STATE_ON);
    #endif
    
    #ifdef PIN_STATUS_LED
    digitalWrite(PIN_STATUS_LED, on ? HIGH : LOW);
    #endif
}

void ButtonBLEController::playModeTone(bool bitChatMode) {
    #ifdef PIN_BUZZER
    #ifdef PIN_BUZZER_EN
    // Enable buzzer if required (T1000-E specific)
    pinMode(PIN_BUZZER_EN, OUTPUT);
    digitalWrite(PIN_BUZZER_EN, HIGH);
    #endif
    
    // Play appropriate tone
    if (bitChatMode) {
        // High tone for BitChat
        // rtttl::begin(PIN_BUZZER, BITCHAT_MODE_TONE);
    } else {
        // Low tone for MeshCore
        // rtttl::begin(PIN_BUZZER, MESHCORE_MODE_TONE);
    }
    
    // Note: The tone will play non-blocking. We don't wait for it to finish.
    #endif
}

#else  // !defined(PIN_USER_BTN) || !defined(ENABLE_BITCHAT)

// Stub implementation when not supported
ButtonBLEController::ButtonBLEController(BaseSerialInterface* serialInterface)
    : _serial(serialInterface), _button(nullptr) {}
void ButtonBLEController::begin() {}
void ButtonBLEController::loop() {}
void ButtonBLEController::indicateMeshCoreMode() {}
void ButtonBLEController::indicateBitChatMode() {}

#endif  // PIN_USER_BTN && ENABLE_BITCHAT
