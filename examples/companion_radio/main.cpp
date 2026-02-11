#include <Arduino.h> // needed for PlatformIO
#include <Mesh.h>
#include "MyMesh.h"

// Believe it or not, this std C function is busted on some platforms!
static uint32_t _atoi(const char *sp) {
  uint32_t n = 0;
  while (*sp && *sp >= '0' && *sp <= '9') {
    n *= 10;
    n += (*sp++ - '0');
  }
  return n;
}

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
#include <InternalFileSystem.h>
#if defined(QSPIFLASH)
#include <CustomLFS_QSPIFlash.h>
DataStore store(InternalFS, QSPIFlash, rtc_clock);
#else
#if defined(EXTRAFS)
#include <CustomLFS.h>
CustomLFS ExtraFS(0xD4000, 0x19000, 128);
DataStore store(InternalFS, ExtraFS, rtc_clock);
#else
DataStore store(InternalFS, rtc_clock);
#endif
#endif
#elif defined(RP2040_PLATFORM)
#include <LittleFS.h>
DataStore store(LittleFS, rtc_clock);
#elif defined(ESP32)
#include <SPIFFS.h>
DataStore store(SPIFFS, rtc_clock);
#endif

#ifdef ESP32
#ifdef WIFI_SSID
#include <helpers/esp32/SerialWifiInterface.h>
SerialWifiInterface serial_interface;
#ifndef TCP_PORT
#define TCP_PORT 5000
#endif
#elif defined(BLE_PIN_CODE)
#include <helpers/esp32/SerialBLEInterface.h>
SerialBLEInterface serial_interface;
#elif defined(SERIAL_RX)
#include <helpers/ArduinoSerialInterface.h>
ArduinoSerialInterface serial_interface;
HardwareSerial companion_serial(1);
#else
#include <helpers/ArduinoSerialInterface.h>
ArduinoSerialInterface serial_interface;
#endif
#elif defined(RP2040_PLATFORM)
#if defined(SERIAL_RX)
#include <helpers/ArduinoSerialInterface.h>
ArduinoSerialInterface serial_interface;
HardwareSerial companion_serial(1);
#else
#include <helpers/ArduinoSerialInterface.h>
ArduinoSerialInterface serial_interface;
#endif
#elif defined(NRF52_PLATFORM)
#ifdef BLE_PIN_CODE
#include <helpers/nrf52/SerialBLEInterface.h>
SerialBLEInterface serial_interface;
#else
#include <helpers/ArduinoSerialInterface.h>
ArduinoSerialInterface serial_interface;
#endif
#elif defined(STM32_PLATFORM)
#include <helpers/ArduinoSerialInterface.h>
ArduinoSerialInterface serial_interface;
#else
#error "need to define a serial interface"
#endif

/* GLOBAL OBJECTS */
#ifdef DISPLAY_CLASS
#include "UITask.h"
UITask ui_task(&board, &serial_interface);
#endif

StdRNG fast_rng;
SimpleMeshTables tables;
MyMesh the_mesh(radio_driver, fast_rng, rtc_clock, tables, store
#ifdef DISPLAY_CLASS
                ,
                &ui_task
#endif
);

// BitChat bridge - instantiated with mesh and identity references
// This follows Story 10.2 architecture
#if defined(ENABLE_BITCHAT) && (defined(ESP32) || defined(NRF52_PLATFORM))
#include <helpers/bitchat/BitchatBridge.h>
BitchatBridge bitchat_bridge(the_mesh, the_mesh.self_id, the_mesh.getNodeName());
#endif

/* END GLOBAL OBJECTS */

void halt() {
  while (1)
    ;
}

void setup() {
  Serial.begin(115200);

  board.begin();

#ifdef DISPLAY_CLASS
  DisplayDriver *disp = NULL;
  if (display.begin()) {
    disp = &display;
    disp->startFrame();
#ifdef ST7789
    disp->setTextSize(2);
#endif
    disp->drawTextCentered(disp->width() / 2, 28, "Loading...");
    disp->endFrame();
  }
#endif

  if (!radio_init()) {
    halt();
  }

  fast_rng.begin(radio_get_rng_seed());

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  InternalFS.begin();
#if defined(QSPIFLASH)
  if (!QSPIFlash.begin()) {
    MESH_DEBUG_PRINTLN("CustomLFS_QSPIFlash: failed to initialize");
  } else {
    MESH_DEBUG_PRINTLN("CustomLFS_QSPIFlash: initialized successfully");
  }
#else
#if defined(EXTRAFS)
  ExtraFS.begin();
#endif
#endif
  store.begin();
  the_mesh.begin(
#ifdef DISPLAY_CLASS
      disp != NULL
#else
      false
#endif
  );

#ifdef BLE_PIN_CODE
  char dev_name[32 + 16];
  sprintf(dev_name, "%s%s", BLE_NAME_PREFIX, the_mesh.getNodeName());
  serial_interface.begin(BLE_NAME_PREFIX, dev_name, the_mesh.getBLEPin());

  // Initialize BitChat bridge for nRF52
  // On nRF52, BitChat service is added to the same Bluefruit stack as MeshCore UART
  #ifdef ENABLE_BITCHAT
  Serial.println("[BitChat] Initializing bridge...");
  bitchat_bridge.begin();
  Serial.println("[BitChat] Bridge begin() called");
  
  #ifdef NRF52_PLATFORM
    // For nRF52: Add BitChat BLE service to existing Bluefruit instance
    Serial.println("[BitChat] nRF52 platform detected, initializing BLE service...");
    if (bitchat_bridge.beginStandalone(the_mesh.getNodeName())) {
      Serial.println("[BitChat] BLE service initialized");
      // Store BitChat service for mode switching
      mesh::ble::BitchatBLEService &bitchatService = bitchat_bridge.getBLEService();
      serial_interface.setBitChatService(&bitchatService.getNRF52Service());
      Serial.println("[BitChat] Service registered for mode switching");
    } else {
      Serial.println("[BitChat] ERROR: BLE service initialization failed");
    }
  #else
    // For ESP32: BitChat attaches to existing BLE server
    if (bitchat_bridge.beginStandalone(the_mesh.getNodeName())) {
      Serial.println("[BitChat] BLE service attached");
    }
  #endif
  
  the_mesh.initBitchat(&bitchat_bridge);
  Serial.println("[BitChat] Bridge fully initialized and registered with mesh");
  #endif
#else
  // USB Serial mode - BitChat can use BLE standalone
  serial_interface.begin(Serial);

  #ifdef ENABLE_BITCHAT
  Serial.println("Initializing BitChat bridge...");
  bitchat_bridge.begin();
  if (bitchat_bridge.beginStandalone(the_mesh.getNodeName())) {
    Serial.println("BitChat BLE service started (standalone mode)");
  }
  the_mesh.initBitchat(&bitchat_bridge);
  Serial.println("BitChat bridge initialized");
  #endif
#endif
  the_mesh.startInterface(serial_interface);

#elif defined(RP2040_PLATFORM)
  LittleFS.begin();
  store.begin();
  the_mesh.begin(
#ifdef DISPLAY_CLASS
      disp != NULL
#else
      false
#endif
  );

#if defined(SERIAL_RX)
  companion_serial.setPins(SERIAL_RX, SERIAL_TX);
  companion_serial.begin(115200);
  serial_interface.begin(companion_serial);
#else
  serial_interface.begin(Serial);
#endif
  the_mesh.startInterface(serial_interface);

#elif defined(ESP32)
  SPIFFS.begin(true);
  store.begin();
  the_mesh.begin(
#ifdef DISPLAY_CLASS
      disp != NULL
#else
      false
#endif
  );

#ifdef WIFI_SSID
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  serial_interface.begin(TCP_PORT);
#elif defined(BLE_PIN_CODE)
  char dev_name[32 + 16] = "@@MAC";  // Use MAC address as default
  serial_interface.begin(BLE_NAME_PREFIX, dev_name, the_mesh.getBLEPin());

  // Initialize BitChat bridge for ESP32
  // On ESP32, BitChat attaches to existing BLE server
  #ifdef ENABLE_BITCHAT
  Serial.println("[BitChat] Initializing bridge...");
  bitchat_bridge.begin();
  
  if (serial_interface.getBLEServer() != nullptr) {
    // Initialize BitChat BLE service on ESP32
    mesh::ble::BitchatBLEService &bitchatService = bitchat_bridge.getBLEService();
    
    // Register BitChat service with SerialBLEInterface for mode switching
    bitchatService.initESP32(the_mesh.getNodeName());
    serial_interface.setBitChatService(bitchatService.getESP32Service());
    
    Serial.println("[BitChat] BLE service registered for mode switching");
  }
  
  the_mesh.initBitchat(&bitchat_bridge);
  Serial.println("[BitChat] Bridge fully initialized and registered with mesh");
  #endif
#elif defined(SERIAL_RX)
  companion_serial.setPins(SERIAL_RX, SERIAL_TX);
  companion_serial.begin(115200);
  serial_interface.begin(companion_serial);
#else
  serial_interface.begin(Serial);
#endif
  the_mesh.startInterface(serial_interface);
#else
#error "need to define filesystem"
#endif

  sensors.begin();

#ifdef DISPLAY_CLASS
  ui_task.begin(disp, &sensors, the_mesh.getNodePrefs());
#endif
}

void loop() {
  the_mesh.loop();
  sensors.loop();
#ifdef DISPLAY_CLASS
  ui_task.loop();
#endif
  rtc_clock.tick();

#if defined(ENABLE_BITCHAT) && (defined(ESP32) || defined(NRF52_PLATFORM))
  bitchat_bridge.loop();
#endif
}
