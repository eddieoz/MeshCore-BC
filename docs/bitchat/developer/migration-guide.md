# BitChat Migration Guide

> **Purpose**: Guide for porting BitChat integration to new MeshCore firmware versions  
> **Source Version**: 1.14.1  
> **Approach**: Subclassing Architecture & Separate Examples

---

## Quick Migration Checklist

Use this checklist when migrating BitChat to a new MeshCore version:

- [ ] Copy `src/helpers/bitchat/` directory
- [ ] Copy `src/helpers/ble/` directory  
- [ ] Copy `examples/bitchat_companion/` directory
- [ ] Copy `test/bitchat/` directory
- [ ] Modify `src/helpers/BaseChatMesh.h` (protected section)
- [ ] Modify `src/helpers/BaseSerialInterface.h` (virtual methods)
- [ ] Update `platformio.ini` source filters
- [ ] Add `ENABLE_BITCHAT=1` to build flags
- [ ] Run tests: `pio test -e native_bitchat`
- [ ] Build target: `pio run -e seeed-wio-tracker-l1`
- [ ] Verify no core file modifications beyond the 2 required

---

## Migration Philosophy

### Why Subclassing Architecture Works

The 1.14.1 implementation uses **Subclassing Architecture** which isolates BitChat functionality:

```
┌─────────────────────────────────────────────────────────────┐
│  Traditional Approach (1.13.0)                              │
│  ├── Modify BaseChatMesh.cpp/h extensively                  │
│  ├── Modify companion_radio example                         │
│  ├── Complex git merges for each new version                │
│  └── High risk of conflicts                                 │
├─────────────────────────────────────────────────────────────┤
│  Subclassing Architecture (1.14.1) - RECOMMENDED            │
│  ├── Create BitchatMesh subclass                            │
│  ├── Create separate bitchat_companion example              │
│  ├── Copy 2 directories to new version                      │
│  ├── Apply 2 minimal patches to core                        │
│  └── Clean, conflict-free integration                       │
└─────────────────────────────────────────────────────────────┘
```

---

## Step-by-Step Migration

### Step 1: Copy Source Directories

From the source version (e.g., 1.14.1), copy these directories:

```bash
# Source: feature/bitchat-1.14.1 branch
# Target: new version branch (e.g., feature/bitchat-1.15.0)

# 1. Core BitChat implementation
cp -r src/helpers/bitchat/ \
      new-version/src/helpers/

# 2. BLE service layer
cp -r src/helpers/ble/ \
      new-version/src/helpers/

# 3. Standalone example
cp -r examples/bitchat_companion/ \
      new-version/examples/

# 4. Test suite
cp -r test/bitchat/ \
      new-version/test/
```

### Step 2: Apply Core Modifications

Modify exactly **2 files** in the new version:

#### File 1: `src/helpers/BaseChatMesh.h`

**Change**: Move channel members from `private` to `protected`

```cpp
// BEFORE (private section):
class BaseChatMesh : public mesh::Mesh {
  ContactInfo contacts[MAX_CONTACTS];
  int num_contacts;
  int sort_array[MAX_CONTACTS];
  int matching_peer_indexes[MAX_SEARCH_RESULTS];
  unsigned long txt_send_timeout;
#ifdef MAX_GROUP_CHANNELS
  ChannelDetails channels[MAX_GROUP_CHANNELS];  // <-- MOVE THESE
  int num_channels;                             // <-- MOVE THESE
#endif
  mesh::Packet* _pendingLoopback;
  // ...

// AFTER (protected section):
class BaseChatMesh : public mesh::Mesh {
  ContactInfo contacts[MAX_CONTACTS];
  int num_contacts;
  int sort_array[MAX_CONTACTS];
  int matching_peer_indexes[MAX_SEARCH_RESULTS];
  unsigned long txt_send_timeout;
  mesh::Packet* _pendingLoopback;
  // ...

protected:
#ifdef MAX_GROUP_CHANNELS
  ChannelDetails channels[MAX_GROUP_CHANNELS];  // <-- NOW PROTECTED
  int num_channels;                             // <-- NOW PROTECTED
#endif
```

**Why**: Allows `BitchatMesh` subclass to access channel management.

#### File 2: `src/helpers/BaseSerialInterface.h`

**Change**: Add virtual methods for BitChat mode

```cpp
class BaseSerialInterface {
protected:
  BaseSerialInterface() { }

public:
  virtual void enable() = 0;
  virtual void disable() = 0;
  virtual bool isEnabled() const = 0;
  virtual bool isConnected() const = 0;
  virtual bool isWriteBusy() const = 0;
  virtual size_t writeFrame(const uint8_t src[], size_t len) = 0;
  virtual size_t checkRecvFrame(uint8_t dest[]) = 0;

  // ADD THESE TWO METHODS:
  virtual bool isBitChatMode() const { return false; }
  virtual void setBitChatMode(bool enable) { (void)enable; }
};
```

**Why**: Allows `SerialBLEInterface` to track BitChat mode state.

### Step 3: Update Build Configuration

#### Option A: Using variants/ (Recommended)

Create or update variant configuration:

```ini
; variants/wio-tracker-l1-bitchat/platformio.ini

[env:seeed-wio-tracker-l1-bitchat]
extends = nrf52_base
board = seeed_wio_tracker_l1
build_flags = ${nrf52_base.build_flags}
  -D BLE_PIN_CODE=123456
  -D DISPLAY_CLASS=CustomTT3Driver
  -D ENABLE_BITCHAT=1          ; <-- ADD THIS
  -D LORA_FREQ=869.525
lib_deps = ${nrf52_base.lib_deps}
  https://github.com/oltaco/CustomLFS @ 0.2.1
```

#### Option B: Global platformio.ini

Add to `[arduino_base]` section:

```ini
[arduino_base]
build_src_filter =
  +<*.cpp>
  +<helpers/*.cpp>
  +<helpers/radiolib/*.cpp>
  +<helpers/bitchat/*.cpp>    ; <-- ADD THIS
  +<helpers/ble/*.cpp>        ; <-- ADD THIS
  +<helpers/bridges/BridgeBase.cpp>
  +<helpers/ui/MomentaryButton.cpp>
```

Add to `[nrf52_base]` section:

```ini
[nrf52_base]
build_flags = ${arduino_base.build_flags}
  -D NRF52_PLATFORM
  -D ENABLE_BITCHAT=1         ; <-- ADD THIS
```

### Step 4: Verify Compilation

```bash
# Run native tests
pio test -e native_bitchat

# Build target hardware
pio run -e seeed-wio-tracker-l1
```

Expected output:
```
Environment           Status    Duration
--------------------  --------  ------------
native_bitchat        PASSED    00:00:03.077
seeed-wio-tracker-l1  SUCCESS   00:00:20.619

RAM:   [======    ]  60.4% (used 142xxx bytes from 235520 bytes)
Flash: [=======   ]  68.0% (used 482xxx bytes from 708608 bytes)
```

### Step 5: Handle API Changes

If MeshCore core API changed between versions:

#### Check 1: BaseChatMesh API
```cpp
// Verify these methods still exist with same signatures:
virtual void onChannelMessageRecv(const mesh::GroupChannel& channel, 
                                   mesh::Packet* pkt, 
                                   uint32_t timestamp, 
                                   const char *text);
ChannelDetails* addChannel(const char* name, const char* psk_base64);
bool getChannel(int idx, ChannelDetails& dest);
```

#### Check 2: Mesh Class API
```cpp
// Verify these methods in BitchatBridge:
mesh::Packet* createGroupDatagram(uint8_t type, 
                                   const mesh::GroupChannel& channel, 
                                   uint8_t* data, size_t len);
void sendFlood(mesh::Packet* packet);
```

#### Check 3: Identity API
```cpp
// Verify signature method:
void sign(uint8_t* signature, const uint8_t* data, size_t len);
```

### Step 6: Test Suite Verification

```bash
# Full test suite
pio test -e native_bitchat -v

# Should show:
# ================ 228 test cases: 228 succeeded ================
```

---

## Handling Version-Specific Changes

### Scenario 1: New MeshCore Feature Conflicts

**Example**: MeshCore 1.15.0 adds new `onChannelMessageRecv` parameter

**Solution**:
```cpp
// In BitchatMesh.h, update override signature:
void onChannelMessageRecv(const mesh::GroupChannel& channel, 
                           mesh::Packet* pkt, 
                           uint32_t timestamp, 
                           const char *text,
                           uint8_t new_param) override;  // Add new param
```

### Scenario 2: BaseChatMesh Member Changes

**Example**: `channels` array renamed to `group_channels`

**Solution**:
```cpp
// In BitchatMesh.cpp, update references:
#ifdef MAX_GROUP_CHANNELS
  if (num_channels < MAX_GROUP_CHANNELS && secret_len <= 32) {
    auto dest = &group_channels[num_channels];  // Updated name
    // ...
  }
#endif
```

### Scenario 3: New Platform Support

**Example**: Adding RP2040 support

**Solution**:
1. Create `src/helpers/rp2040/SerialBLEInterface.cpp/h` if needed
2. Add RP2040-specific BLE initialization in `BitchatBLEService`
3. Update `examples/bitchat_companion/main.cpp` with RP2040 sections

---

## Verification Script

Create a verification script for automated migration testing:

```bash
#!/bin/bash
# verify-migration.sh

echo "=== BitChat Migration Verification ==="

# 1. Check source files exist
echo "Checking source files..."
[ -d "src/helpers/bitchat" ] || { echo "❌ Missing src/helpers/bitchat"; exit 1; }
[ -d "src/helpers/ble" ] || { echo "❌ Missing src/helpers/ble"; exit 1; }
[ -d "examples/bitchat_companion" ] || { echo "❌ Missing examples/bitchat_companion"; exit 1; }
echo "✅ Source files present"

# 2. Check core modifications
echo "Checking core modifications..."
grep -q "protected:" src/helpers/BaseChatMesh.h && \
grep -q "ChannelDetails channels" src/helpers/BaseChatMesh.h && \
echo "✅ BaseChatMesh.h modified correctly" || \
echo "❌ BaseChatMesh.h missing modifications"

grep -q "isBitChatMode()" src/helpers/BaseSerialInterface.h && \
echo "✅ BaseSerialInterface.h modified correctly" || \
echo "❌ BaseSerialInterface.h missing modifications"

# 3. Run tests
echo "Running tests..."
pio test -e native_bitchat --silent
[ $? -eq 0 ] && echo "✅ Tests passing" || echo "❌ Tests failed"

# 4. Build target
echo "Building target..."
pio run -e seeed-wio-tracker-l1 --silent
[ $? -eq 0 ] && echo "✅ Build successful" || echo "❌ Build failed"

echo "=== Verification Complete ==="
```

---

## Troubleshooting

### Issue: Compilation Error - `channels` is private

```
error: 'channels' is a private member of 'BaseChatMesh'
```

**Solution**: Ensure `BaseChatMesh.h` has channels in `protected:` section.

### Issue: Link Error - Undefined reference to `BitchatBridge`

```
undefined reference to `BitchatBridge::BitchatBridge(...)`
```

**Solution**: Add to `platformio.ini`:
```ini
build_src_filter = 
  +<helpers/bitchat/*.cpp>
```

### Issue: Test Failures

```
FAILED: test_bitchat_bridge
```

**Solution**: 
1. Check mocks are up to date in `test/mocks/`
2. Verify `NATIVE_TEST` flag is defined for native environment
3. Check for API signature changes

### Issue: Stack Overflow on Hardware

```
Device freezes during message processing
```

**Solution**: Verify no large stack allocations:
```cpp
// Check BitchatBridge.h for member variables:
BitchatMessage _tempLoopMessage;      // Should be member, not stack
uint8_t _signingBuffer[2048 + 64];    // Should be member, not stack
```

---

## Migration Timeline Example

### Migrating from 1.14.1 to 1.15.0

| Phase | Task | Duration | Owner |
|-------|------|----------|-------|
| 1 | Copy source directories | 10 min | Developer |
| 2 | Apply core modifications | 15 min | Developer |
| 3 | Update build configuration | 10 min | Developer |
| 4 | Verify compilation | 5 min | CI/CD |
| 5 | Run test suite | 5 min | CI/CD |
| 6 | Hardware testing | 2 hours | QA |
| 7 | Documentation update | 30 min | Developer |

**Total estimated time**: ~3 hours (mostly hardware testing)

---

## Best Practices

### 1. Maintain Test Coverage

After migration, ensure all 228 tests still pass:
```bash
pio test -e native_bitchat
# Expect: 228 test cases: 228 succeeded
```

### 2. Verify No Core Drift

Check that only 2 core files are modified:
```bash
git diff main --stat -- '*.cpp' '*.h' | grep -v "bitchat\|ble\/"
# Should only show:
# src/helpers/BaseChatMesh.h
# src/helpers/BaseSerialInterface.h
```

### 3. Memory Budget

Monitor memory usage after migration:
```
RAM:   Should be < 70%
Flash: Should be < 75%
```

### 4. Version Documentation

Update version references:
```cpp
// In examples/bitchat_companion/MyMesh.h
#define FIRMWARE_VERSION "v1.15.0"  // Update version
```

---

## References

- [Integration Guide](./integration-guide-1.14.1.md)
- [Code Review Report](./code-review-1.14.1.md)
- [Architecture Overview](../architecture.md)
- [Build Configuration](../build_configuration.md)

---

*Last Updated: April 2026*  
*Applies to: MeshCore 1.14.1 and later*
