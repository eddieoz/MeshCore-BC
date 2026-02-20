# BitChat Payloads

This document defines the payload structures for BitChat messages transported through MeshCore.

## Important Notes

- All multi-byte integers are **big-endian** unless specified otherwise
- Peer IDs are 8 bytes, **little-endian** encoding
- Timestamps are Unix milliseconds (64-bit, big-endian)
- Text payloads use UTF-8 encoding

## ANNOUNCE Payload

Node announcement payload uses TLV (Type-Length-Value) encoding:

| Field | TLV Type | Size | Description |
|-------|----------|------|-------------|
| nickname | `0x01` | 1-32 bytes | Node display name (null-terminated) |
| noise_pubkey | `0x02` | 32 bytes | Curve25519 public key for Noise protocol |
| ed25519_pubkey | `0x03` | 32 bytes | Ed25519 public key for signing |

### ANNOUNCE TLV Format

```
[0x01] [length:2] [nickname bytes]
[0x02] [length:2] [32-byte noise key]
[0x03] [length:2] [32-byte ed25519 key]
```

### ANNOUNCE Example

```
TLV Data:
  01 00 0D 4D 65 73 68 4E 6F 64 65 31 32 33 00  ; nickname="MeshNode123"
  03 00 20 A1B2C3... (32 bytes)                  ; ed25519_pubkey
```

## MESSAGE Payload (Group)

Group message payload contains channel information and text content:

| Field | TLV Type | Required | Description |
|-------|----------|----------|-------------|
| text | `0x10` | Yes | Message text content (UTF-8) |
| channel | `0x11` | No | Channel name (default: global) |
| timestamp | `0x12` | No | Override timestamp |
| reply_to | `0x13` | No | Message ID being replied to |

### Group Message Format

Plain text format (broadcast compatible):
```
#channel: <sender_nickname> <message_text>
```

Or without channel prefix:
```
<sender_nickname>: <message_text>
```

### MESSAGE TLV Example

```
TLV Data:
  11 00 05 6D 65 73 68 00     ; channel="mesh"
  10 00 0C 48 65 6C 6C 6F 20 57 6F 72 6C 64 21  ; text="Hello World!"
```

## MESSAGE Payload (Direct/DM)

Direct messages use the same TLV format but with the `HAS_RECIPIENT` flag set and `recipient_id` field in the header.

### DM Header Flags

```
flags = HAS_RECIPIENT (0x01) | HAS_SIGNATURE (0x02) [optional]
```

### DM Payload

| Field | TLV Type | Description |
|-------|----------|-------------|
| text | `0x10` | Message text (required) |

DMs do not include channel TLV as they are peer-to-peer.

## Fragment Payload

Fragment messages (`FRAGMENT_NEW` type `0x20` or `FRAGMENT` type `0xFF`) carry a fragment header followed by payload data:

### Fragment Header

| Field | Size | Description |
|-------|------|-------------|
| original_message_id | 4 bytes | ID of original message |
| fragment_index | 1 byte | 0-based fragment index |
| total_fragments | 1 byte | Total number of fragments |
| fragment_data | variable | Fragment payload data |

### Fragment Structure

```
[message_id:4] [index:1] [total:1] [data...]
```

## Channel-Related Payloads

### CHANNEL Message (Type `0x05`)

Used for channel operations:

| Sub-type | Value | Description |
|----------|-------|-------------|
| JOIN | `0x01` | Join a channel |
| LEAVE | `0x02` | Leave a channel |
| LIST | `0x03` | List available channels |

### JOIN/LEAVE Payload

| Field | TLV Type | Description |
|-------|----------|-------------|
| channel_name | `0x11` | Name of channel to join/leave |

## Noise Protocol Payloads

### NOISE_HANDSHAKE (Type `0x10`)

Noise protocol XX pattern handshake message:

| Field | Size | Description |
|-------|------|-------------|
| ephemeral_key | 32 bytes | Ephemeral Curve25519 public key |
| static_key | 32 bytes | Static Curve25519 public key (encrypted) |
| payload | variable | Optional encrypted payload |

### NOISE_ENCRYPTED (Type `0x11`)

Encrypted message payload:

| Field | Size | Description |
|-------|------|-------------|
| nonce | 12 bytes | AES-GCM nonce |
| ciphertext | variable | Encrypted data |
| tag | 16 bytes | AES-GCM authentication tag |

## Signature Payload

Signatures can be included either:
1. In the 64-byte trailing field (when `HAS_SIGNATURE` flag set)
2. As a TLV (`0x20`) within the payload

### Signature TLV Format

```
[0x20] [0x0040] [64-byte Ed25519 signature]
```

### Signing Data Format

The signature covers the following serialized data:

| Field | Size | Notes |
|-------|------|-------|
| version | 1 | Message version |
| type | 1 | Message type |
| ttl | 1 | Always 0 for signing |
| timestamp | 8 | Big-endian |
| flags | 1 | With HAS_SIGNATURE cleared |
| payload_length | 2 | Big-endian |
| sender_id | 8 | Big-endian byte order |
| recipient_id | 8 | Only if HAS_RECIPIENT |
| payload | variable | Raw TLV bytes |

## Compression

When `IS_COMPRESSED` flag is set, the payload is zlib/deflate compressed:

```
compressed_payload = zlib_compress(raw_payload)
```

Maximum compressed size: 245 bytes (BitChat wire limit)
Maximum decompressed size: 2048 bytes

## TLV Type Reference

| Type | Name | Used In |
|------|------|---------|
| `0x01` | NICKNAME | ANNOUNCE |
| `0x02` | NOISE_PUBKEY | ANNOUNCE |
| `0x03` | ED25519_PUBKEY | ANNOUNCE |
| `0x10` | MESSAGE_TEXT | MESSAGE |
| `0x11` | CHANNEL_NAME | MESSAGE, CHANNEL |
| `0x12` | TIMESTAMP | MESSAGE |
| `0x13` | REPLY_TO | MESSAGE |
| `0x20` | SIGNATURE | Any (alternative location) |

## Size Limits

| Field | Maximum Size |
|-------|-------------|
| Nickname | 32 bytes |
| Channel name | 32 bytes |
| Message text | 1024 bytes (uncompressed) |
| Payload (total) | 2048 bytes |
| Wire payload | 245 bytes (compressed) |
| Signature | 64 bytes |

## MeshCore → BitChat Payload Translation

When converting MeshCore messages to BitChat format:

### Group Messages

MeshCore format: `[timestamp:4] [txt_type:1] [text]`

BitChat format: 
- TLV `0x10` with formatted text: `<sender>: <original_text>`
- TLV `0x11` with channel name (if available)
- Timestamp from original message

### Direct Messages

MeshCore format: Encrypted E2E payload

BitChat format:
- Decrypt E2E payload
- Extract text content
- Create BitChat DM with `HAS_RECIPIENT` flag
- Set recipient_id from contact mapping

## BitChat → MeshCore Payload Translation

When converting BitChat messages to MeshCore format:

1. Parse TLV payload
2. Extract text content
3. Prepend `📱` prefix for loop prevention
4. Format: `📱 <sender_nickname>: <message_text>`
5. Send as `PAYLOAD_TYPE_GRP_TXT`

## Implementation Reference

- `src/helpers/bitchat/BitchatMessageParser.h` - Payload parsing
- `src/helpers/bitchat/MessageFormatter.h` - Payload formatting
- `src/helpers/bitchat/Compression.h` - Compression handling
