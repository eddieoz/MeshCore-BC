# BitChat Encapsulation in MeshCore

This document specifies how BitChat messages are encapsulated within standard MeshCore packets for transport over the mesh network.

## Overview

BitChat messages are encapsulated using `PAYLOAD_TYPE_GRP_DATA` (0x06) MeshCore packets. This allows BitChat messages to be transported through the mesh without requiring any modifications to repeaters or room servers.

## Encapsulation Strategy

```
┌─────────────────────────────────────────────────────────────┐
│ MeshCore PAYLOAD_TYPE_GRP_DATA Packet                       │
├─────────────────────────────────────────────────────────────┤
│ Channel Hash (1 byte)                                       │
│ MAC + Encrypted Data:                                       │
│   ┌─────────────────────────────────────────────────────┐  │
│   │ BitChat Encapsulation Header (14 bytes):           │  │
│   │   - Magic: "BC\x00\x00" (4 bytes)                  │  │
│   │   - Version: 0x01 (1 byte)                         │  │
│   │   - Flags: compression, fragmentation (1 byte)     │  │
│   │   - Original Length (2 bytes)                      │  │
│   │   - Message ID (4 bytes)                           │  │
│   │   - Fragment Info (2 bytes)                        │  │
│   ├─────────────────────────────────────────────────────┤  │
│   │ BitChat Serialized Message Data (up to 170 bytes)  │  │
│   └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
        ↑
        │
   Encrypted with channel secret
   Repeaters forward without decryption
```

## Encapsulation Header Format

| Field | Size (bytes) | Description |
|-------|--------------|-------------|
| magic | 4 | `0x42430000` (ASCII "BC\0\0") |
| version | 1 | Encapsulation format version (1) |
| flags | 1 | Compression and fragmentation flags |
| original_len | 2 | Original BitChat message length before encapsulation (little-endian) |
| message_id | 4 | Unique message identifier for reassembly (little-endian) |
| fragment_num | 1 | Fragment number (0 = unfragmented, 1+ = fragment index) |
| total_fragments | 1 | Total fragments (1 = unfragmented) |

**Total header size**: 14 bytes

## Flags

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 0 | `0x01` | COMPRESSED | BitChat payload is zlib compressed |
| 1 | `0x02` | FRAGMENTED | Message is fragmented across multiple packets |

## Complete Encapsulated Structure

| Field | Size (bytes) | Description |
|-------|--------------|-------------|
| channel_hash | 1 | First byte of SHA256(channel_secret) |
| cipher_mac | 2 | MAC for encrypted data |
| encapsulation_header | 14 | BitChat encapsulation header |
| bitchat_message | 0-170 | Serialized BitChat message (or fragment) |

**Maximum BitChat data per packet**: 170 bytes (184 - 14 header)

## Fragmentation

For BitChat messages larger than 170 bytes, fragmentation is used:

### Fragment Calculation

```
max_fragment_data = 170 bytes
fragments_needed = ceil(original_size / 170)
```

### Fragment Header

Each fragment uses the same encapsulation header with:
- `FRAGMENTED` flag set
- `fragment_num`: 1-based index of this fragment
- `total_fragments`: Total number of fragments
- `message_id`: Same across all fragments of a message

### Fragment Example

```
Original message: 400 bytes

Fragment 1:
  - fragment_num: 1
  - total_fragments: 3
  - data: bytes 0-169 (170 bytes)

Fragment 2:
  - fragment_num: 2
  - total_fragments: 3
  - data: bytes 170-339 (170 bytes)

Fragment 3:
  - fragment_num: 3
  - total_fragments: 3
  - data: bytes 340-399 (60 bytes)
```

### Maximum Fragmented Message Size

```
max_fragments = 4 (implementation limit)
max_message_size = 4 × 170 = 680 bytes
```

Note: BitChat's native maximum is 2048 bytes (2KB). Large messages may require application-layer handling.

## Encryption

The encapsulation header and BitChat message are encrypted together using the MeshCore group channel encryption:

```
encrypted_data = AES-CTR(encapsulation_header || bitchat_message, 
                         key=channel_secret, 
                         iv=derived_from_packet)
```

Encryption is handled automatically by MeshCore's `createGroupDatagram()` function.

## Decapsulation Process

When receiving a `PAYLOAD_TYPE_GRP_DATA` packet:

1. Decrypt payload using channel secret
2. Check first 4 bytes for magic `0x42430000`
3. If magic matches, process as BitChat encapsulated
4. Extract encapsulation header fields
5. If fragmented, buffer for reassembly
6. Deserialize BitChat message from data portion
7. Forward to BitChat BLE service

### Decapsulation Status Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | SUCCESS | Successfully decapsulated |
| 1 | NOT_BITCHAT | No BitChat magic header |
| 2 | INVALID_PACKET | Packet too short or malformed |
| 3 | VERSION_MISMATCH | Unsupported encapsulation version |
| 4 | DECRYPTION_FAILED | Decryption error |
| 5 | DESERIALIZATION_FAILED | Failed to deserialize message |

## Repeater Transparency

The encapsulation strategy ensures repeaters forward packets transparently:

1. **No decryption required**: Repeaters don't have channel secrets
2. **Standard packet type**: Uses `PAYLOAD_TYPE_GRP_DATA` (0x06)
3. **Same routing**: Follows same flood/direct routing as other packets
4. **Loop prevention**: Uses existing MeshCore `hasSeen()` mechanism

## Compatibility with Existing MeshCore Nodes

| Node Type | Behavior |
|-----------|----------|
| Repeater | Forwards without decryption |
| Room Server | Forwards without decryption |
| Companion (no BitChat) | Decrypts but ignores (no BitChat magic) |
| Companion (with BitChat) | Decrypts and forwards to app |

## Implementation Reference

See source files:
- `src/helpers/bitchat/BitchatMessageEncapsulator.h` - Encapsulation
- `src/helpers/bitchat/MessageDecapsulator.h` - Decapsulation
- `src/helpers/bitchat/FragmentReassembly.h` - Fragment handling

## Size Calculations

### Unfragmented Message

```
MeshCore payload limit:        184 bytes
Encapsulation header:           14 bytes
Available for BitChat:         170 bytes
Overhead:                        8.2%
```

### Fragmented Message (worst case)

```
170 bytes per fragment
14 bytes header per fragment
Efficiency: 92% (170/184)
```

### Channel Hash Collision Probability

```
1 byte hash space: 256 values
Random collision probability: 1/256 ≈ 0.4%
Acceptable for mesh routing (duplicate suppression)
```
