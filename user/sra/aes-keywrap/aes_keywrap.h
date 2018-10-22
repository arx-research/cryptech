/*
 * Implementation of RFC 5649 variant of AES Key Wrap, using Cryptlib
 * to supply the AES ECB encryption and decryption functions.
 */

#ifndef _AES_KEYWRAP_
#define _AES_KEYWRAP_

/*
 * Input and output buffers can overlap (we use memmove()), but be
 * warned that failures can occur after we've started writing to the
 * output buffer, so if the input and output buffers do overlap, the
 * input may have been overwritten by the time the failure occurs.
 */

typedef enum {
  AES_KEY_WRAP_OK,                      /* Success */
  AES_KEY_WRAP_BAD_ARGUMENTS,           /* Null pointers or similar */
  AES_KEY_WRAP_ENCRYPTION_FAILED,       /* cryptEncrypt() failed */
  AES_KEY_WRAP_DECRYPTION_FAILED,       /* cryptDecrypt() failed */
  AES_KEY_WRAP_BAD_MAGIC,               /* MSB(32,A) != 0xA65959A6 */
  AES_KEY_WRAP_BAD_LENGTH,              /* LSB(32,A) out of range */
  AES_KEY_WRAP_BAD_PADDING              /* Nonzero padding detected */
} aes_key_wrap_status_t;
  
extern aes_key_wrap_status_t aes_key_wrap(const CRYPT_CONTEXT kek,
                                          const unsigned char * const plaintext,
                                          const size_t plaintext_length,
                                          unsigned char *cyphertext,
                                          size_t *ciphertext_length);

extern aes_key_wrap_status_t aes_key_unwrap(const CRYPT_CONTEXT kek,
                                            const unsigned char * const ciphertext,
                                            const size_t ciphertext_length,
                                            unsigned char *plaintext,
                                            size_t *plaintext_length);

extern const char *
aes_key_wrap_error_string(const aes_key_wrap_status_t code);

/*
 * AES_KEY_WRAP_CIPHERTEXT_SIZE() tells you how big the ciphertext
 * will be for a given plaintext size.
 */

#define AES_KEY_WRAP_CIPHERTEXT_SIZE(_plaintext_length_) \
  ((size_t) (((_plaintext_length_) + 15) & ~7))

#endif

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
