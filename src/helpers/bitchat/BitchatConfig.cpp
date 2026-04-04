#include "BitchatConfig.h"

#ifdef ENABLE_BITCHAT

#ifdef NATIVE_TEST
  #include <cstdio>
  #include <fstream>
  #include <sys/stat.h>
#else
  #ifdef ESP32
    #include <SPIFFS.h>
    #include <Arduino.h>
  #elif defined(NRF52_PLATFORM)
    #include <InternalFileSystem.h>
    #include <Arduino.h>
    using namespace Adafruit_LittleFS_Namespace;
  #endif
#endif

namespace mesh {
namespace bitchat {

// Native test filesystem simulation
#ifdef NATIVE_TEST
static const char* NATIVE_CONFIG_DIR = "/tmp/meshcore_test";
static const char* NATIVE_CONFIG_FILE = "/tmp/meshcore_test/bitchat.cfg";
#endif

BitchatConfigManager::BitchatConfigManager()
    : _initialized(false)
{
}

bool BitchatConfigManager::begin() {
    if (_initialized) {
        return true;
    }
    
    // Try to load existing config
    if (!load()) {
        // No valid config found, use defaults
        reset();
        save();
    }
    
    _initialized = true;
    return true;
}

bool BitchatConfigManager::isEnabled() const {
    return _config.enabled != 0;
}

void BitchatConfigManager::setEnabled(bool enabled) {
    _config.enabled = enabled ? 1 : 0;
    _config.lastModified = getCurrentTimestamp();
}

bool BitchatConfigManager::save() {
    _config.lastModified = getCurrentTimestamp();
    
#ifdef NATIVE_TEST
    // Ensure directory exists
    ensureDirectory(NATIVE_CONFIG_DIR);
    
    // Write config file
    FILE* fp = fopen(NATIVE_CONFIG_FILE, "wb");
    if (!fp) {
        return false;
    }
    size_t written = fwrite(&_config, 1, sizeof(_config), fp);
    fclose(fp);
    return written == sizeof(_config);
    
#elif defined(ESP32)
    ensureDirectory("/config");
    
    File file = SPIFFS.open(BITCHAT_CONFIG_PATH, "wb");
    if (!file) {
        return false;
    }
    size_t written = file.write((uint8_t*)&_config, sizeof(_config));
    file.close();
    return written == sizeof(_config);
    
#elif defined(NRF52_PLATFORM)
    ensureDirectory("/config");
    
    File file(InternalFS.open(BITCHAT_CONFIG_PATH, FILE_O_WRITE));
    if (!file) {
        return false;
    }
    size_t written = file.write((uint8_t*)&_config, sizeof(_config));
    file.close();
    return written == sizeof(_config);
#else
    return false;
#endif
}

bool BitchatConfigManager::load() {
#ifdef NATIVE_TEST
    FILE* fp = fopen(NATIVE_CONFIG_FILE, "rb");
    if (!fp) {
        return false;
    }
    
    BitchatConfig tempConfig;
    size_t read = fread(&tempConfig, 1, sizeof(tempConfig), fp);
    fclose(fp);
    
    if (read != sizeof(tempConfig) || !tempConfig.isValid()) {
        return false;
    }
    
    _config = tempConfig;
    return true;
    
#elif defined(ESP32)
    if (!SPIFFS.exists(BITCHAT_CONFIG_PATH)) {
        return false;
    }
    
    File file = SPIFFS.open(BITCHAT_CONFIG_PATH, "rb");
    if (!file) {
        return false;
    }
    
    BitchatConfig tempConfig;
    size_t read = file.read((uint8_t*)&tempConfig, sizeof(tempConfig));
    file.close();
    
    if (read != sizeof(tempConfig) || !tempConfig.isValid()) {
        return false;
    }
    
    _config = tempConfig;
    return true;
    
#elif defined(NRF52_PLATFORM)
    if (!InternalFS.exists(BITCHAT_CONFIG_PATH)) {
        return false;
    }
    
    File file(InternalFS.open(BITCHAT_CONFIG_PATH, FILE_O_READ));
    if (!file) {
        return false;
    }
    
    BitchatConfig tempConfig;
    size_t read = file.read((uint8_t*)&tempConfig, sizeof(tempConfig));
    file.close();
    
    if (read != sizeof(tempConfig) || !tempConfig.isValid()) {
        return false;
    }
    
    _config = tempConfig;
    return true;
#else
    return false;
#endif
}

void BitchatConfigManager::reset() {
    _config = BitchatConfig();
    _config.lastModified = getCurrentTimestamp();
}

uint32_t BitchatConfigManager::getLastModified() const {
    return _config.lastModified;
}

uint32_t BitchatConfigManager::getCurrentTimestamp() const {
#ifdef NATIVE_TEST
    // Use a simple counter for native tests
    static uint32_t counter = 1000;
    return counter++;
#else
    return millis() / 1000;  // Convert to seconds
#endif
}

bool BitchatConfigManager::ensureDirectory(const char* path) {
#ifdef NATIVE_TEST
    struct stat st;
    if (stat(path, &st) != 0) {
        // Directory doesn't exist, create it
        #ifdef _WIN32
        return _mkdir(path) == 0;
        #else
        return mkdir(path, 0755) == 0;
        #endif
    }
    return true;
    
#elif defined(ESP32)
    if (!SPIFFS.exists(path)) {
        return SPIFFS.mkdir(path);
    }
    return true;
    
#elif defined(NRF52_PLATFORM)
    if (!InternalFS.exists(path)) {
        return InternalFS.mkdir(path);
    }
    return true;
#else
    return false;
#endif
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
