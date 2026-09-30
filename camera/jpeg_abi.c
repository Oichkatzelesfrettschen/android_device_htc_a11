/* OpenSSL envelope encryption for the HTC JPEG codec over BoringSSL. */
#include <limits.h>
#include <openssl/evp.h>
#include <openssl/mem.h>
#include <openssl/rand.h>

int a11_EVP_SealInit(EVP_CIPHER_CTX *context, const EVP_CIPHER *cipher,
        unsigned char **envelopes, int *envelope_lengths, unsigned char *iv,
        EVP_PKEY **public_keys, int key_count)
{
    unsigned char key[EVP_MAX_KEY_LENGTH];
    int result = 0;
    unsigned key_length;

    if (EVP_EncryptInit_ex(context, cipher, NULL, NULL, NULL) != 1)
        return 0;
    if (key_count <= 0)
        return 1;
    key_length = EVP_CIPHER_CTX_key_length(context);
    if (key_length > sizeof(key) || RAND_bytes(key, key_length) != 1)
        goto out;
    if (EVP_CIPHER_CTX_iv_length(context) > 0 &&
            RAND_bytes(iv, EVP_CIPHER_CTX_iv_length(context)) != 1)
        goto out;

    for (int key_index = 0; key_index < key_count; key_index++) {
        EVP_PKEY_CTX *key_context = EVP_PKEY_CTX_new(public_keys[key_index], NULL);
        size_t envelope_length = (size_t)EVP_PKEY_size(public_keys[key_index]);
        if (key_context == NULL)
            goto out;
        int sealed = EVP_PKEY_encrypt_init(key_context) > 0 &&
                EVP_PKEY_encrypt(key_context, envelopes[key_index], &envelope_length,
                        key, key_length) > 0;
        EVP_PKEY_CTX_free(key_context);
        if (!sealed || envelope_length > INT_MAX)
            goto out;
        envelope_lengths[key_index] = (int)envelope_length;
    }
    if (EVP_EncryptInit_ex(context, NULL, NULL, key, iv) == 1)
        result = key_count;
out:
    OPENSSL_cleanse(key, sizeof(key));
    return result;
}

int a11_EVP_SealFinal(EVP_CIPHER_CTX *context, unsigned char *output, int *length)
{
    int result = EVP_EncryptFinal_ex(context, output, length);
    if (result == 1)
        result = EVP_EncryptInit_ex(context, NULL, NULL, NULL, NULL);
    return result;
}

int a11_EVP_OpenInit(EVP_CIPHER_CTX *context, const EVP_CIPHER *cipher,
        const unsigned char *envelope, int envelope_length,
        const unsigned char *iv, EVP_PKEY *private_key)
{
    unsigned char *key = NULL;
    EVP_PKEY_CTX *key_context = NULL;
    size_t key_capacity = 0;
    size_t key_length;
    int result = 0;

    if (EVP_DecryptInit_ex(context, cipher, NULL, NULL, NULL) != 1)
        return 0;
    if (private_key == NULL)
        return 1;
    if (envelope_length <= 0)
        return 0;
    key_context = EVP_PKEY_CTX_new(private_key, NULL);
    if (key_context == NULL || EVP_PKEY_decrypt_init(key_context) <= 0 ||
            EVP_PKEY_decrypt(key_context, NULL, &key_capacity,
                    envelope, (size_t)envelope_length) <= 0)
        goto out;
    /* BoringSSL requires RSA_size bytes even for a short unpadded cipher key. */
    key = OPENSSL_malloc(key_capacity);
    if (key == NULL)
        goto out;
    key_length = key_capacity;
    if (EVP_PKEY_decrypt(key_context, key, &key_length,
                envelope, (size_t)envelope_length) <= 0 || key_length > INT_MAX ||
            EVP_CIPHER_CTX_set_key_length(context, (int)key_length) != 1)
        goto out;
    result = EVP_DecryptInit_ex(context, NULL, NULL, key, iv);
out:
    EVP_PKEY_CTX_free(key_context);
    if (key != NULL) {
        OPENSSL_cleanse(key, key_capacity);
        OPENSSL_free(key);
    }
    return result;
}

int a11_EVP_OpenFinal(EVP_CIPHER_CTX *context, unsigned char *output, int *length)
{
    int result = EVP_DecryptFinal_ex(context, output, length);
    if (result == 1)
        result = EVP_DecryptInit_ex(context, NULL, NULL, NULL, NULL);
    return result;
}
