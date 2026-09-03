# Cryptography in brazier
## Overview
*brazier* provides a lightweight *RSA* and *DH* implementations for cryptography (encryption & decryption, and key exchange) using 256-bit integers. It does **not** have a keygen, being designed for use with existing *RSA* keys.

## RSA

*RSA (Rivest-Shamir-Adleman)* is an asymmetric encryption algorithm that uses two keys:

- Public key: Used for encryption, and can be shared freely.
- Private key: Used for decryption, and must be kept secret.

*RSA* uses two large prime numbers and a public & private exponents to mathematically encrypt and decrypt **strings**.

### Usage

To begin with encryption, one needs to call *RSA* and provide it with `p`, `q` and `r` (two primes and public exponent) values, which will begin key evaluation:

```cpp
RSA rsa(p, q, r);
```

This will create key pairs which can be accessed using methods `rsa.getPublicKey()` and `rsa.getPrivateKey()`.

To encrypt any data, simply refer to methods `rsa.encrypt()` and `rsa.decrypt()` like so:

```cpp
std::string session_key = "TOP_SECRET";

crypt_str encrypted_key = rsa.encrypt(session_key); // returns std::vector of 256-bit integers
std::string decrypted_key = rsa.decrypt(session_key);
```

The function only works with `std::string`, and will return an encrypted string `crypt_str`, which is a vector of 256-bit integets `std::vector<int256_t>`

## Diffie-Hellman Key Exchange

*Diffie-Hellman (DH)* is a key exchange protocol that allows two parties to securely establish a shared secret over an insecure channel. It uses assymmetric cryptography, but is **not** an encryption algorithm itself, rather generating a shared secret for use in symmetric encryption.

### Usage

To begin a DH key exchange, one needs to call `DiffieHellman` for both parties, and provide it with `prime`, `generator` and `private_key` values:

```cpp
DiffieHellman alice(prime, generator, private_key);
DiffieHellman bob(prime, generator, private_key);
```

This will generate a public key which can be accessed using method `getPublicKey()` and shared with the other party.

To compute a shared secret, call `evaluateSharedSecret()` with the partner's public key:

```cpp
alice.evaluateSharedSecret(bob.getPublicKey());
```

Once the shared secret is established, use methods `encrypt()` and `decrypt()`:

```cpp
std::string message = "SECRET_MESSAGE";

crypt_str encrypted = alice.encrypt(message);
std::string decrypted = bob.decrypt(encrypted);
```

The function only works with `std::string`, and will return an encrypted string `crypt_str`, which is a vector of 256-bit integets `std::vector<int256_t004331
