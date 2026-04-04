#pragma once

/**
 * Story 2.1: Configuration Storage
 * 
 * BitChat Configuration Persistence
 * - Runtime enable/disable flag
 * - Persistent storage in filesystem
 * - Loaded on boot, saved on change
 * 
 * Story 2.3: Compile-time control via ENABLE_BITCHAT flag
 */

#ifdef ENABLE_BITCHAT

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace mesh {
namespace bitchat {

// Config file path
#define BITCHAT_CONFIG_PATH "/config/bitchat.cfg"
#define BITCHAT_CONFIG_MAGIC 0x4243  // "BC" in hex
#define BITCHAT_CONFIG_VERSION 1

/**
 * BitChat Configuration Structure
 * 
 * Stored in flash filesystem for persistence across reboots
 */
struct BitchatConfig {
    uint16_t magic;           // BITCHAT_CONFIG_MAGIC
    uint8_t version;          // Config format version
    uint8_t enabled;          // 1 = enabled, 0 = disabled
    uint32_t lastModified;    // Timestamp of last change
    
    // Padding for future expansion (64 bytes total)
    uint8_t reserved[56];
    
    BitchatConfig() 
        : magic(BITCHAT_CONFIG_MAGIC)
        , version(BITCHAT_CONFIG_VERSION)
        , enabled(0)  // Disabled by default
        , lastModified(0) 
    {
        memset(reserved, 0, sizeof(reserved));
    }
    
    bool isValid() const {
        return magic == BITCHAT_CONFIG_MAGIC && version == BITCHAT_CONFIG_VERSION;
    }
};

static_assert(sizeof(BitchatConfig) == 64, "BitchatConfig must be 64 bytes");

/**
 * Configuration Manager
 * 
 * Handles loading/saving BitChat configuration from filesystem
 */
class BitchatConfigManager {
public:
    BitchatConfigManager();
    
    // Initialize and load config from filesystem
    bool begin();
    
    // Check if BitChat is enabled
    bool isEnabled() const;
    
    // Enable/disable BitChat
    void setEnabled(bool enabled);
    
    // Save current config to filesystem
    bool save();
    
    // Load config from filesystem (returns false if not found/invalid)
    bool load();
    
    // Reset to defaults
    void reset();
    
    // Get last modified timestamp
    uint32_t getLastModified() const;
    
    // Direct access to config (for testing)
    const BitchatConfig& getConfig() const { return _config; }
    
private:
    BitchatConfig _config;
    bool _initialized;
    
    // Get current timestamp (platform-specific)
    uint32_t getCurrentTimestamp() const;
    
    // Filesystem operations (platform-specific)
    bool writeFile(const char* path, const uint8_t* data, size_t len);
    bool readFile(const char* path, uint8_t* data, size_t len);
    bool ensureDirectory(const char* path);
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
