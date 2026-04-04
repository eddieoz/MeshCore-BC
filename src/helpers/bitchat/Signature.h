#pragma once

/**
 * Story 7.4: Signature Verification and Generation
 * 
 * Handles BitChat Ed25519 signatures
 * - Sign messages with Ed25519 private key
 * - Verify signatures with Ed25519 public key
 * - Per BitChat spec: TTL=0, PKCS#7 padding
 */

#ifdef ENABLE_BITCHAT

#include <cstdint>
#include <cstring>

namespace mesh {
namespace bitchat {

// Ed25519 signature size
#define SIGNATURE_SIZE 64

// Ed25519 public key size
#define ED25519_PUBKEY_SIZE 32

// Ed25519 private key size
#define ED25519_PRIVKEY_SIZE 64

/**
 * Signature Manager
 * 
 * Signs and verifies BitChat messages using Ed25519
 * 
 * NOTE: This is a stub implementation for the native test environment.
 * In production with actual Ed25519 crypto library, this would use real signatures.
 */
class SignatureManager {
public:
    SignatureManager();
    
    /**
     * Sign message data
     * 
     * @param message Message data to sign
     * @param messageLen Message length
     * @param privateKey Ed25519 private key (64 bytes)
     * @param signature Output signature (64 bytes)
     * @return true if signed successfully
     */
    bool sign(const uint8_t* message, size_t messageLen,
              const uint8_t* privateKey,
              uint8_t* signature);
    
    /**
     * Verify message signature
     * 
     * @param message Message data
     * @param messageLen Message length
     * @param signature Signature (64 bytes)
     * @param publicKey Ed25519 public key (32 bytes)
     * @return true if signature valid
     */
    bool verify(const uint8_t* message, size_t messageLen,
                const uint8_t* signature,
                const uint8_t* publicKey);
    
    /**
     * Prepare message for signing (per BitChat spec)
     * - Uses TTL=0 in signature data
     * - Applies PKCS#7 padding
     * 
     * @param original Original message
     * @param originalLen Original length
     * @param prepared Output buffer
     * @param preparedLen Prepared length
     * @param preparedBufSize Output buffer size
     * @return true if prepared successfully
     */
    bool prepareForSigning(const uint8_t* original, size_t originalLen,
                           uint8_t* prepared, size_t* preparedLen,
                           size_t preparedBufSize);

private:
    // Stub: Compute a simple hash of message + key as "signature"
    void computeStubSignature(const uint8_t* message, size_t messageLen,
                              const uint8_t* key, size_t keyLen,
                              uint8_t* signature);
    
    // Stub: Verify by recomputing
    bool verifyStubSignature(const uint8_t* message, size_t messageLen,
                             const uint8_t* signature,
                             const uint8_t* key, size_t keyLen);
};

} // namespace bitchat
} // namespace mesh

#endif // ENABLE_BITCHAT
