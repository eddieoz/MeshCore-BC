#pragma once

#include <Arduino.h>
#include "BaseSerialInterface.h"

// Forward declaration of Button class
class Button;

/**
 * ButtonBLEController - BLE Mode Switching for Button-Only Devices
 * 
 * This class provides button-based BLE mode switching (MeshCore <-> BitChat)
 * for devices without displays. It uses multi-click detection and provides
 * LED/buzzer feedback to indicate the current mode.
 * 
 * Features:
 * - Quintuple (5x) press to toggle between MeshCore and BitChat modes
 * - LED feedback: 3 blinks (slow = MeshCore, fast = BitChat)
 * - Buzzer feedback: Different tones for each mode (if buzzer available)
 * 
 * Target devices: T1000-E, RAK WisMesh Tag, and other button-only devices
 */
class ButtonBLEController {
public:
    ButtonBLEController(BaseSerialInterface* serialInterface);
    
    void begin();
    void loop();
    
    // Mode indicator methods
    void indicateMeshCoreMode();
    void indicateBitChatMode();
    
    // Check if controller is active (has required hardware)
    bool isActive() const { return _button != nullptr; }

private:
    BaseSerialInterface* _serial;
    Button* _button;
    
    // LED feedback state
    enum LedState { LED_IDLE, LED_BLINKING };
    LedState _ledState;
    uint8_t _blinkCount;
    uint8_t _blinkTarget;
    uint32_t _lastBlinkTime;
    uint32_t _blinkInterval;
    bool _ledCurrentState;
    
    // Button callback handler
    void handleQuintuplePress();
    
    // LED feedback helpers
    void startLedFeedback(bool fastMode);
    void updateLedFeedback();
    void setLed(bool on);
    
    // Buzzer feedback
    void playModeTone(bool bitChatMode);
};
