#pragma once

#include "md5.h"
#include "sha256.h"

namespace Hash
{
    struct Checksum
    {
        virtual void Init() = 0;
        virtual void Update(byte *data, uint32_t len) = 0;
        virtual void Final() = 0;
        virtual int Size() = 0;
        virtual byte const *Result() = 0;
        virtual wchar const *Name() const = 0;
    };

    struct SHA256 : Checksum
    {
        static constexpr int size = 32;
        static constexpr char const *name = "sha256";
        static constexpr wchar const *wname = L"sha256";

        void Init() override
        {
            SHA256_Init(&context);
            finalized = false;
        }

        void Update(byte *data, uint32_t len) override
        {
            SHA256_Update(&context, data, len);
        }

        void Final() override
        {
            if(!finalized) {
                finalized = true;
                SHA256_Final(hash, &context);
            }
        }

        int Size() override
        {
            return size;
        }

        byte const *Result() override
        {
            Final();
            return hash;
        }

        wchar const *Name() const override
        {
            return wname;
        }

    private:
        bool finalized = false;
        SHA256_CTX context;
        byte hash[size];
    };

    struct MD5 : Checksum
    {
        static constexpr int size = 16;
        static constexpr char const *name = "hash"; // _not_ called 'md5', 'hash' is backwards compatible from when md5
                                                    // was the only type of checksum available
        static constexpr wchar const *wname = L"hash";

        void Init() override
        {
            MD5_Init(&context);
            finalized = false;
        }

        void Update(byte *data, uint32_t len) override
        {
            MD5_Update(&context, data, len);
        }

        void Final() override
        {
            if(!finalized) {
                finalized = true;
                MD5_Final(hash, &context);
            }
        }

        int Size() override
        {
            return size;
        }

        byte const *Result() override
        {
            Final();
            return hash;
        }

        wchar const *Name() const override
        {
            return wname;
        }

    private:
        bool finalized = false;
        MD5_CTX context;
        byte hash[size];
    };

} // namespace Hash
