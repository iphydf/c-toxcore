/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2018 The TokTok team.
 * Copyright © 2013 Tox project.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Functions for the core crypto.
 */
namespace crypto {

/**
 * The number of bytes in a signature.
 */
#define SIGNATURE_SIZE 64

/**
 * The number of bytes in a Tox public key used for signatures.
 */
#define SIGN_PUBLIC_KEY_SIZE 32

/**
 * The number of bytes in a Tox secret key used for signatures.
 */
#define SIGN_SECRET_KEY_SIZE 64

/**
 * The number of bytes in a Tox public key used for encryption.
 */
#define PUBLIC_KEY_SIZE 32

/**
 * The number of bytes in a Tox secret key used for encryption.
 */
#define SECRET_KEY_SIZE 32

/**
 * The number of bytes in a shared key computed from public and secret keys.
 */
#define SHARED_KEY_SIZE 32

/**
 * The number of bytes in a symmetric key.
 */
#define SYMMETRIC_KEY_SIZE SHARED_KEY_SIZE

/**
 * The number of bytes needed for the MAC (message authentication code) in an
 * encrypted message.
 */
#define MAC_SIZE 16

/**
 * The number of bytes in a nonce used for encryption/decryption.
 */
#define NONCE_SIZE 24

/**
 * The number of bytes in a SHA256 hash.
 */
#define SHA256_SIZE 32

/**
 * The number of bytes in a SHA512 hash.
 */
#define SHA512_SIZE 64

/**
 * A `memcmp`-like function whose running time does not depend on the input
 * bytes, only on the input length. Useful to compare sensitive data where
 * timing attacks could reveal that data.
 *
 * This means for instance that comparing "aaaa" and "aaaa" takes 4 time, and
 * "aaaa" and "baaa" also takes 4 time. With a regular `memcmp`, the latter may
 * take 1 time, because it immediately knows that the two strings are not equal.
 */
int32_t memcmp(const uint8_t *p1, const uint8_t *p2, size_t length);

/**
 * A `bzero`-like function which won't be optimised away by the compiler. Some
 * compilers will inline `bzero` or `memset` if they can prove that there will
 * be no reads to the written data. Use this function if you want to be sure the
 * memory is indeed zeroed.
 */
void memzero(void *data, size_t length);

/**
 * Compute a SHA256 hash (32 bytes).
 */
void sha256(uint8_t hash[SHA256_SIZE], const uint8_t data[length]);

/**
 * Compute a SHA512 hash (64 bytes).
 */
void sha512(uint8_t hash[SHA512_SIZE], const uint8_t data[length]);

/**
 * Compare 2 public keys of length #PUBLIC_KEY_SIZE, not vulnerable to
 * timing attacks.
 *
 * @return 0 if both mem locations of length are equal, -1 if they are not.
 */
int32_t public_key_cmp(
    const uint8_t pk1[PUBLIC_KEY_SIZE],
    const uint8_t pk2[PUBLIC_KEY_SIZE]);

namespace random {

/**
 * Return a random 8 bit integer.
 */
uint8_t u08(void);

/**
 * Return a random 16 bit integer.
 */
uint16_t u16(void);

/**
 * Return a random 32 bit integer.
 */
uint32_t u32(void);

/**
 * Return a random 64 bit integer.
 */
uint64_t u64(void);

/**
 * Fill the given nonce with random bytes.
 */
void nonce(uint8_t nonce[NONCE_SIZE]);

/**
 * Fill an array of bytes with random values.
 */
void bytes(uint8_t bytes[length]);

}

/**
 * Return a value between 0 and upper_bound using a uniform distribution.
 */
uint32_t random_int_range(uint32_t upper_bound);

/**
 * Check if a Tox public key #PUBLIC_KEY_SIZE is valid or not. This
 * should only be used for input validation.
 *
 * @return false if it isn't, true if it is.
 */
bool public_key_valid(const uint8_t public_key[PUBLIC_KEY_SIZE]);

/**
 * Extended keypair: curve + ed. Encryption keys are derived from the signature keys.
 * Used for group chats and group DHT announcements.
 * pk and sk must have room for at least EXT_PUBLIC_KEY bytes each.
 */
int32_t create_extended_keypair(uint8_t *pk, uint8_t *sk);

/**
 * Generate a new random keypair. Every call to this function is likely to
 * generate a different keypair.
 */
int32_t crypto_new_keypair(
    uint8_t public_key[PUBLIC_KEY_SIZE],
    uint8_t secret_key[SECRET_KEY_SIZE]);

/**
 * Derive the public key from a given secret key.
 */
void crypto_derive_public_key(
    uint8_t public_key[PUBLIC_KEY_SIZE],
    const uint8_t secret_key[SECRET_KEY_SIZE]);

/**
 * Encrypt plain text of the given length to encrypted of length +
 * #MAC_SIZE using the public key (#PUBLIC_KEY_SIZE bytes) of the
 * receiver and the secret key of the sender and a #NONCE_SIZE byte
 * nonce.
 *
 * @return -1 if there was a problem, length of encrypted data if everything
 * was fine.
 */
int32_t encrypt_data(
    const uint8_t public_key[PUBLIC_KEY_SIZE],
    const uint8_t secret_key[SECRET_KEY_SIZE],
    const uint8_t nonce[NONCE_SIZE],
    const uint8_t plain[length],
    uint8_t *encrypted);


/**
 * Decrypt encrypted text of the given length to plain text of the given length
 * - #MAC_SIZE using the public key (#PUBLIC_KEY_SIZE bytes) of
 * the sender, the secret key of the receiver and a #NONCE_SIZE byte
 * nonce.
 *
 * @return -1 if there was a problem (decryption failed), length of plain text
 * data if everything was fine.
 */
int32_t decrypt_data(
    const uint8_t public_key[PUBLIC_KEY_SIZE],
    const uint8_t secret_key[SECRET_KEY_SIZE],
    const uint8_t nonce[NONCE_SIZE],
    const uint8_t encrypted[length],
    uint8_t *plain);

/**
 * Fast encrypt/decrypt operations.
 *
 * Use if this is not a one-time communication. #encrypt_precompute does the
 * shared-key generation once so it does not have to be performed on every
 * encrypt/decrypt.
 */
int32_t encrypt_precompute(
    const uint8_t public_key[PUBLIC_KEY_SIZE],
    const uint8_t secret_key[SECRET_KEY_SIZE],
    uint8_t shared_key[SHARED_KEY_SIZE]);

/**
 * Encrypts plain of length length to encrypted of length + #MAC_SIZE
 * using a shared key #SYMMETRIC_KEY_SIZE big and a #NONCE_SIZE
 * byte nonce.
 *
 * @return -1 if there was a problem, length of encrypted data if everything
 * was fine.
 */
int32_t encrypt_data_symmetric(
    const uint8_t shared_key[SHARED_KEY_SIZE],
    const uint8_t nonce[NONCE_SIZE],
    const uint8_t plain[length],
    uint8_t *encrypted);

/**
 * Decrypts encrypted of length length to plain of length length -
 * #MAC_SIZE using a shared key #SHARED_KEY_SIZE big and a
 * #NONCE_SIZE byte nonce.
 *
 * @return -1 if there was a problem (decryption failed), length of plain data
 * if everything was fine.
 */
int32_t decrypt_data_symmetric(
    const uint8_t shared_key[SHARED_KEY_SIZE],
    const uint8_t nonce[NONCE_SIZE],
    const uint8_t encrypted[length],
    uint8_t *plain);

/**
 * Increment the given nonce by 1 in big endian (rightmost byte incremented
 * first).
 */
void increment_nonce(uint8_t nonce[NONCE_SIZE]);

/**
 * Increment the given nonce by a given number.
 *
 * The number should be in host byte order.
 */
void increment_nonce_number(uint8_t nonce[NONCE_SIZE], uint32_t host_order_num);

/**
 * Fill a key #SYMMETRIC_KEY_SIZE big with random bytes.
 */
void new_symmetric_key(uint8_t key[SYMMETRIC_KEY_SIZE]);

}
