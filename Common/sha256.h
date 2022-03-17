/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#pragma once

using SHA_LONG = uint32_t;

constexpr int SHA_LBLOCK = 16;
constexpr int SHA_CBLOCK = SHA_LBLOCK * 4;
constexpr int SHA_LAST_BLOCK = SHA_CBLOCK - 8;
constexpr int SHA_DIGEST_LENGTH = 20;
constexpr int SHA256_CBLOCK = SHA_LBLOCK * 4;

typedef struct SHA256state_st
{
    SHA_LONG h[8];
    SHA_LONG Nl, Nh;
    SHA_LONG data[SHA_LBLOCK];
    unsigned int num, md_len;
} SHA256_CTX;

int SHA256_Init(SHA256_CTX *c);
int SHA256_Update(SHA256_CTX *c, const void *data, size_t len);
int SHA256_Final(unsigned char *md, SHA256_CTX *c);
void SHA256_Transform(SHA256_CTX *c, const unsigned char *data);

constexpr int SHA256_DIGEST_LENGTH = 32;
