# Bitchat Protocol Specification

## Overview

This document specifies the Bitchat wire protocol used for communication between the Bitchat Android app and MeshCore devices via BLE.

## Message Structure

All Bitchat messages follow a common header format:

| Field | Size (bytes) | Description |
|-------|--------------|-------------|
| version | 1 | Protocol version (currently 1) |
| type | 1 | Message type (see below) |
| ttl | 1 | Time-to-live hop counter |
| timestamp | 8 | Unix timestamp in milliseconds (big-endian) |
| flags | 1 | Bit flags (see below) |
| payload_length | 2 | Length of payload data (big-endian) |
| sender_id | 8 | 8-byte sender peer ID (little-endian) |
| recipient_id | 8 | Optional recipient ID for DMs (little-endian) |
| payload | variable | Message payload (see TLV format) |
| signature | 64 | Ed25519 signature (optional) |

**Total header size without optional fields**: 14 bytes

## Message Types

| Value | Name | Description |
|-------|------|-------------|
| `0x01` | ANNOUNCE | Node announcement with identity info |
| `0x02` | MESSAGE | Chat message (group or direct) |
| `0x03` | LEAVE | Channel leave notification |
| `0x04` | IDENTITY | Identity update |
| `0x05` | CHANNEL | Channel-related operation |
| `0x06` | PING | Keepalive ping |
| `0x07` | PONG | Keepalive pong |
| `0x10` | NOISE_HANDSHAKE | Noise protocol handshake |
| `0x11` | NOISE_ENCRYPTED | Noise-encrypted message |
| `0x20` | FRAGMENT_NEW | First fragment of multi-part message |
| `0x21` | REQUEST_SYNC | Request message sync |
| `0x22` | FILE_TRANSFER | File transfer metadata |
| `0xFF` | FRAGMENT | Continuation fragment |

## Flags

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 0 | `0x01` | HAS_RECIPIENT | Message has recipient_id field (DM) |
| 1 | `0x02` | HAS_SIGNATURE | Message includes 64-byte signature |
| 2 | `0x04` | IS_COMPRESSED | Payload is zlib-compressed |

## Payload TLV Format

Bitchat payloads use Type-Length-Value (TLV) encoding:

| Field | Size (bytes) | Description |
|-------|--------------|-------------|
| type | 1 | TLV type identifier |
| length | 2 | Value length (big-endian) |
| value | variable | Payload data |

### TLV Types

| Value | Name | Description |
|-------|------|-------------|
| `0x10` | MESSAGE_TEXT | Message text content (UTF-8) |
| `0x11` | CHANNEL_NAME | Channel name for group messages |
| `0x12` | TIMESTAMP | Override timestamp |
| `0x13` | REPLY_TO | Reference to message being replied to |
| `0x20` | SIGNATURE | Ed25519 signature (alternative location) |

## ANNOUNCE Message

ANNOUNCE messages broadcast node identity information.

### ANNOUNCE Payload TLVs

| TLV Type | Required | Description |
|----------|----------|-------------|
| `0x01` | Yes | Nickname (max 31 chars) |
| `0x02` | No | Noise protocol public key (32 bytes) |
| `0x03` | No | Ed25519 public key (32 bytes) |

### ANNOUNCE Example

```
Header:
  version:      0x01
  type:         0x01 (ANNOUNCE)
  ttl:          0x08
  timestamp:    0x0000017A... (64-bit big-endian ms)
  flags:        0x00
  payload_len:  0x0025 (37 bytes)
  sender_id:    0xA1B2C3D4E5F6... (8 bytes little-endian)

Payload TLVs:
  [0x01 0x0010 "MeshNode12345678"]     ; Nickname TLV
  [0x03 0x0020 <32-byte Ed25519 key>] ; Public key TLV
```

## MESSAGE Type

MESSAGE type carries chat messages.

### Group Message Payload

| TLV | Description |
|-----|-------------|
| `0x10` (MESSAGE_TEXT) | Message content (required) |
| `0x11` (CHANNEL_NAME) | Channel name (optional, default: global) |

### Direct Message Payload

| TLV | Description |
|-----|-------------|
| `0x10` (MESSAGE_TEXT) | Message content (required) |

Flags: `HAS_RECIPIENT` must be set, `recipient_id` field present.

## Peer ID Derivation

Bitchat peer IDs are derived from Ed25519 public keys:

```
peer_id = first 8 bytes of Ed25519 public key
          interpreted as little-endian uint64_t
```

Example:
```
Ed25519 pubkey:  A1B2C3D4E5F6... (32 bytes)
Peer ID bytes:   D4C3B2A1F6E5... (first 8 bytes, little-endian)
Peer ID value:   0xE5F6A1B2C3D4 (uint64_t)
```

See [BitchatIdentity.h](../../src/helpers/bitchat/BitchatIdentity.h) for implementation.

## Signing Process

Messages are signed with Ed25519 using the following serialization:

1. Create signing buffer excluding signature field
2. Set `HAS_SIGNATURE` flag to 0 for signing
3. Force TTL to 0 (matches Android `AppConstants.SYNC_TTL_HOPS`)
4. Apply PKCS#7 padding to optimal block size (256, 512, 1024, or 2048)
5. Sign padded data with Ed25519 private key

### Signing Buffer Format

| Field | Size | Notes |
|-------|------|-------|
| version | 1 | Message version |
| type | 1 | Message type |
| ttl | 1 | Always 0 for signing |
| timestamp | 8 | Big-endian |
| flags | 1 | With HAS_SIGNATURE cleared |
| payload_length | 2 | Big-endian |
| sender_id | 8 | Big-endian byte order |
| recipient_id | 8 | Only if HAS_RECIPIENT set |
| payload | variable | Raw payload bytes |

## Time Synchronization

Bitchat uses Unix timestamps in milliseconds. Devices synchronize time via:

1. Receiving ANNOUNCE or MESSAGE with timestamp
2. Validating timestamp (2020+ and not >30 minutes future)
3. Calculating offset from local clock
4. Using offset for subsequent message timestamps

Time sync threshold: 30 seconds difference triggers update.

## Hashtag Channels

Bitchat supports public hashtag channels (e.g., `#mesh`). These channels use a deterministic key derivation mechanism based on the channel name.

### #mesh Channel

The `#mesh` channel is the primary public channel for Bitchat-MeshCore interoperability.

#### Channel Secret Derivation

The `#mesh` channel secret is derived using **the first 16 bytes of SHA256** over the UTF-8 encoded channel name string (including the `#` prefix):

```
channel_secret = first_16_bytes(SHA256("#mesh"))
```

#### #mesh Secret (Test Vector)

```
Input:          "#mesh" (5 bytes: 0x23 0x6D 0x65 0x73 0x68)
SHA256:         0x5B664CDE0B08B220612113DB980650F3F3500698DB13216120B2080BDE4C665B
First 16 bytes: 0x5B664CDE0B08B220612113DB980650F3
```

The 16-byte channel secret: **`5B664CDE0B08B220612113DB980650F3`**

#### Firmware Implementation

The MeshCore firmware computes and stores the `#mesh` channel secret:

```cpp
// In BitchatBridge::computeMeshSecret()
const char* hashtag = "#mesh";
uint8_t sha256_result[32];
mesh::Utils::sha256(sha256_result, 32, (const uint8_t*)hashtag, strlen(hashtag));
memcpy(_meshChannelSecret, sha256_result, 16);  // First 16 bytes only
```

#### Channel Verification

When receiving a group message, the firmware verifies it belongs to the `#mesh` channel by comparing the first 16 bytes of the channel secret:

```cpp
bool BitchatBridge::isMeshChannel(const mesh::GroupChannel& channel) const {
    if (!_hasMeshSecret) return false;
    // Compare first 16 bytes (hashtag channel secret length)
    return (memcmp(channel.secret, _meshChannelSecret, 16) == 0);
}
```

Only messages from the `#mesh` channel are forwarded to the Bitchat BLE interface.

#### MyMesh Channel Initialization

The `#mesh` channel is automatically initialized when `ENABLE_BITCHAT` is defined:

```cpp
// In MyMesh::begin()
#ifdef ENABLE_BITCHAT
    addHashtagChannel("mesh");  // Adds #mesh channel
#endif
```

The `addHashtagChannel()` method derives the secret using the same `first_16_bytes(SHA256("#mesh"))` computation, ensuring the firmware can both send and receive on this channel.

#### Channel Hash

MeshCore computes the channel hash for routing using SHA256 of the channel secret:

```
channel_hash = first_byte(SHA256(channel_secret))
```

For `#mesh`:
```
channel_hash = first_byte(SHA256(0x5B664CDE0B08B220612113DB980650F3))
             = 0xB0 (first byte of hash)
```

**⚠️ Important Implementation Detail**: In MeshCore, `GroupChannel.hash` is only 1 byte (`PATH_HASH_SIZE = 1`), while `GroupChannel.secret` is 32 bytes (`PUB_KEY_SIZE`). When initializing the channel, only copy 1 byte to the hash field:

```cpp
// CORRECT: Only set hash[0]
_meshChannel.hash[0] = 0xB0;

// INCORRECT: This would overflow into the secret field!
// const uint8_t hash[8] = {...};
// memcpy(_meshChannel.hash, hash, 8); // ❌ Overwrites secret bytes 1-7
```

Similarly, when caching a channel from an incoming message, only copy the hash byte:

```cpp
// CORRECT: Only copy the hash
_meshChannel.hash[0] = channel.hash[0];

// INCORRECT: This overwrites our carefully set secret!
// _meshChannel = channel; // ❌ Overwrites entire secret
```

### Hashtag Channel Name Constraints

Per the MeshCore protocol, hashtag channel names must follow these rules:

- Start with `#`
- Lowercase alphanumeric characters (`a-z`, `0-9`) and hyphens (`-`)
- No leading, trailing, or consecutive hyphens
- Maximum 30 characters (including `#`)

Examples of valid names:
- `#mesh` ✅
- `#general` ✅
- `#off-topic` ✅

Examples of invalid names:
- `#Mesh` ❌ (uppercase)
- `#-test` ❌ (leading hyphen)
- `#test--channel` ❌ (consecutive hyphens)
- `#test_channel` ❌ (underscore, not hyphen)

### Custom Hashtag Channels

Additional hashtag channels can be derived using the same mechanism:

```
channel_secret = first_16_bytes(SHA256("#<channel_name>"))
```

For example:
- `#general` → `first_16_bytes(SHA256("#general"))`
- `#offtopic` → `first_16_bytes(SHA256("#offtopic"))`

**Note**: The current firmware implementation focuses on the `#mesh` channel. Support for additional hashtag channels would require extending the channel registry.

## BLE Transport

Bitchat messages are sent over BLE as GATT notifications on the Bitchat service characteristic. Messages may be fragmented if larger than MTU (typically 185-512 bytes).

See [BLE Service Specification](./ble_service.md) for GATT service details.
