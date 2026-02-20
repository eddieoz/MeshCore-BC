#include "BitchatCLI.h"
#include "BitchatConfig.h"
#include "ChannelRegistry.h"

#ifdef ENABLE_BITCHAT

#ifdef NATIVE_TEST
  #include <cstdio>
  #include <cstdarg>
#else
  #ifdef ESP32
    #include <Arduino.h>
  #elif defined(NRF52_PLATFORM)
    #include <Arduino.h>
  #endif
#endif

namespace mesh {
namespace bitchat {

BitchatCLI::BitchatCLI()
    : _config(nullptr)
    , _channels(nullptr)
{
}

void BitchatCLI::begin(BitchatConfigManager* configManager, ChannelRegistry* channelRegistry) {
    _config = configManager;
    _channels = channelRegistry;
}

void BitchatCLI::setOutputCallback(CLIOutputCallback callback) {
    _output = callback;
}

bool BitchatCLI::isBitchatCommand(const char* cmdLine) {
    if (!cmdLine) return false;
    
    // Skip leading whitespace
    while (*cmdLine == ' ' || *cmdLine == '\t') cmdLine++;
    
    return (strncmp(cmdLine, "bitchat", 7) == 0 && 
            (cmdLine[7] == ' ' || cmdLine[7] == '\0' || cmdLine[7] == '\n' || cmdLine[7] == '\r'));
}

bool BitchatCLI::processCommand(const char* cmdLine) {
    if (!cmdLine || !_config) return false;
    
    // Skip leading whitespace
    while (*cmdLine == ' ' || *cmdLine == '\t') cmdLine++;
    
    // Check if it's a bitchat command
    if (strncmp(cmdLine, "bitchat", 7) != 0) {
        return false;
    }
    
    // Move past "bitchat"
    cmdLine += 7;
    
    // Skip whitespace
    while (*cmdLine == ' ' || *cmdLine == '\t') cmdLine++;
    
    // Get subcommand
    if (*cmdLine == '\0' || *cmdLine == '\n' || *cmdLine == '\r') {
        // Just "bitchat" - show help
        cmdHelp();
        return true;
    }
    
    // Parse subcommand
    if (strncmp(cmdLine, "enable", 6) == 0) {
        cmdEnable();
    } else if (strncmp(cmdLine, "disable", 7) == 0) {
        cmdDisable();
    } else if (strncmp(cmdLine, "status", 6) == 0) {
        cmdStatus();
    } else if (strncmp(cmdLine, "channel", 7) == 0) {
        // Channel subcommand
        cmdLine += 7;
        while (*cmdLine == ' ' || *cmdLine == '\t') cmdLine++;
        cmdChannel(cmdLine);
    } else if (strncmp(cmdLine, "help", 4) == 0) {
        cmdHelp();
    } else {
        println("Unknown bitchat command. Type 'bitchat help' for usage.");
    }
    
    return true;
}

void BitchatCLI::cmdEnable() {
    if (_config->isEnabled()) {
        println("BitChat is already enabled.");
        return;
    }
    
    _config->setEnabled(true);
    if (_config->save()) {
        println("BitChat enabled. Restart device to activate.");
        println("(Or use 'bitchat status' to verify)");
    } else {
        println("ERROR: Failed to save config!");
    }
}

void BitchatCLI::cmdDisable() {
    if (!_config->isEnabled()) {
        println("BitChat is already disabled.");
        return;
    }
    
    _config->setEnabled(false);
    if (_config->save()) {
        println("BitChat disabled. Restart device to deactivate.");
        println("(Or use 'bitchat status' to verify)");
    } else {
        println("ERROR: Failed to save config!");
    }
}

void BitchatCLI::cmdStatus() {
    println("=== BitChat Status ===");
    
    if (_config->isEnabled()) {
        println("Status: ENABLED");
    } else {
        println("Status: DISABLED");
    }
    
    char buf[64];
    snprintf(buf, sizeof(buf), "Config: valid (version %d)", BITCHAT_CONFIG_VERSION);
    println(buf);
    
    uint32_t lastMod = _config->getLastModified();
    if (lastMod > 0) {
        snprintf(buf, sizeof(buf), "Last modified: %lu", (unsigned long)lastMod);
        println(buf);
    }
    
    println("");
    println("Note: Changes take effect after restart.");
}

void BitchatCLI::cmdHelp() {
    println(getHelpText());
}

const char* BitchatCLI::getHelpText() {
    return 
        "=== BitChat Commands ===\n"
        "bitchat enable   - Enable BitChat service\n"
        "bitchat disable  - Disable BitChat service\n"
        "bitchat status   - Show current status\n"
        "bitchat channel  - Manage channel mappings\n"
        "bitchat help     - Show this help\n"
        "\n"
        "Use 'bitchat channel help' for channel commands.\n"
        "\n"
        "BitChat allows Android BitChat app to connect\n"
        "via BLE alongside MeshCore app.";
}

void BitchatCLI::print(const char* text) {
    if (_output) {
        _output(text);
    }
#ifdef NATIVE_TEST
    else {
        printf("%s", text);
    }
#endif
}

void BitchatCLI::println(const char* text) {
    if (_output) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s\n", text);
        _output(buf);
    }
#ifdef NATIVE_TEST
    else {
        printf("%s\n", text);
    }
#endif
}

// Story 4.3: Channel management commands
void BitchatCLI::cmdChannel(const char* subcmd) {
    if (!_channels) {
        println("ERROR: Channel registry not initialized.");
        return;
    }
    
    // Parse channel subcommand
    if (strncmp(subcmd, "add", 3) == 0) {
        subcmd += 3;
        while (*subcmd == ' ' || *subcmd == '\t') subcmd++;
        cmdChannelAdd(subcmd);
    } else if (strncmp(subcmd, "remove", 6) == 0) {
        subcmd += 6;
        while (*subcmd == ' ' || *subcmd == '\t') subcmd++;
        cmdChannelRemove(subcmd);
    } else if (strncmp(subcmd, "list", 4) == 0) {
        cmdChannelList();
    } else if (strncmp(subcmd, "help", 4) == 0) {
        cmdChannelHelp();
    } else {
        cmdChannelList();  // Default to list
    }
}

void BitchatCLI::cmdChannelAdd(const char* args) {
    // Parse: <name> <hex_key>
    char name[32];
    char hexKey[65];  // 64 hex chars + null
    
    // Simple parsing - assumes format "name hexkey"
    int nameLen = 0;
    while (*args && *args != ' ' && *args != '\t' && nameLen < 31) {
        name[nameLen++] = *args++;
    }
    name[nameLen] = '\0';
    
    while (*args == ' ' || *args == '\t') args++;
    
    int keyLen = 0;
    while (*args && *args != ' ' && *args != '\t' && keyLen < 64) {
        hexKey[keyLen++] = *args++;
    }
    hexKey[keyLen] = '\0';
    
    if (nameLen == 0 || keyLen == 0) {
        println("Usage: bitchat channel add <name> <hex_key>");
        return;
    }
    
    // Parse hex key
    uint8_t key[32];
    if (keyLen != 64) {
        println("ERROR: Key must be 64 hex characters (32 bytes).");
        return;
    }
    
    for (int i = 0; i < 32; i++) {
        unsigned int byte;
        sscanf(&hexKey[i*2], "%2x", &byte);
        key[i] = (uint8_t)byte;
    }
    
    if (_channels->registerChannel(name, key)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Channel '%s' registered.", name);
        println(buf);
    } else {
        println("ERROR: Failed to register channel (registry full?).");
    }
}

void BitchatCLI::cmdChannelRemove(const char* name) {
    if (!name || *name == '\0') {
        println("Usage: bitchat channel remove <name>");
        return;
    }
    
    if (_channels->unregisterChannel(name)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Channel '%s' removed.", name);
        println(buf);
    } else {
        char buf[64];
        snprintf(buf, sizeof(buf), "ERROR: Channel '%s' not found.", name);
        println(buf);
    }
}

void BitchatCLI::cmdChannelList() {
    println("=== BitChat Channels ===");
    
    size_t count = _channels->getChannelCount();
    if (count == 0) {
        println("No channels registered.");
        return;
    }
    
    char buf[80];
    snprintf(buf, sizeof(buf), "Registered channels: %zu", count);
    println(buf);
    println("");
    
    for (size_t i = 0; i < 8; i++) {
        const ChannelMapping* ch = _channels->getChannel(i);
        if (ch) {
            snprintf(buf, sizeof(buf), "  %s", ch->name);
            println(buf);
        }
    }
}

void BitchatCLI::cmdChannelHelp() {
    println("=== BitChat Channel Commands ===");
    println("bitchat channel add <name> <hex_key>  - Add channel mapping");
    println("bitchat channel remove <name>         - Remove channel mapping");
    println("bitchat channel list                  - List all channels");
    println("bitchat channel help                  - Show this help");
    println("");
    println("Example: bitchat channel add general a1b2c3...");
}

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
