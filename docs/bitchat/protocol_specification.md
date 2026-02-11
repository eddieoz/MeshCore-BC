# BitChat Protocol Specification

## Overview

This document specifies the BitChat wire protocol used for communication between the BitChat Android app and MeshCore devices via BLE.

## Message Structure

All BitChat messages follow a common header format:

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

BitChat payloads use Type-Length-Value (TLV) encoding:

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

BitChat peer IDs are derived from Ed25519 public keys:

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

BitChat uses Unix timestamps in milliseconds. Devices synchronize time via:

1. Receiving ANNOUNCE or MESSAGE with timestamp
2. Validating timestamp (2020+ and not >30 minutes future)
3. Calculating offset from local clock
4. Using offset for subsequent message timestamps

Time sync threshold: 30 seconds difference triggers update.

## BLE Transport

BitChat messages are sent over BLE as GATT notifications on the BitChat service characteristic. Messages may be fragmented if larger than MTU (typically 185-512 bytes).

See [BLE Service Specification](./ble_service.md) for GATT service details.
