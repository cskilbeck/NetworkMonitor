#pragma once

#include "Proxy.h"

//////////////////////////////////////////////////////////////////////

struct UrlComponents
{
    UrlComponents();
    bool Init(wchar const *uri);

    static const int max_buffer_size = 1024;

    URL_COMPONENTS components;

    wstr scheme;
    wstr hostname;
    wstr username;
    wstr password;
    wstr path;
    wstr extrainfo;

    bool valid;
};

//////////////////////////////////////////////////////////////////////
// Base downloader

class Downloader
{
    LOG_Context("Downloader");

public:
    Downloader();
    virtual ~Downloader();

    virtual bool Start(wchar const *url);
    bool Join(DWORD milliseconds = INFINITE) const;
    bool Abort();
    void Terminate();

    bool Succeeded() const
    {
        return StatusCode() < 400;
    }

    int StatusCode() const
    {
        return status_code;
    }

    wstr const &Url() const
    {
        return url;
    }

    wstr const &Filename() const
    {
        return url_components.path;
    }

    virtual DWORD DoDownload();

    virtual void OnSuccess()
    {
    }

    virtual void OnFailure()
    {
    }

    HANDLE ThreadHandle() const
    {
        return thread_handle;
    }

    wstr const &ErrorMessage() const
    {
        return error_message;
    }

    bool &CacheEnabled()
    {
        return cache_enabled;
    }

    enum Status { invalid = 0, not_started = 1, in_progress = 2, aborted = 3, failed = 4, succeeded = 5 };

    Status status = Status::not_started;

protected:
    virtual bool OnReceiveData(byte *data, size_t size) = 0;
    virtual bool OnReceiveComplete() = 0;

    wstr error_message;

private:
    int status_code = 0;
    char *headers = null;
    DWORD headers_size = 0;
    DWORD content_length = 0;
    HANDLE thread_handle = INVALID_HANDLE_VALUE;
    DWORD thread_id = 0;
    HINTERNET hSession = null;
    HINTERNET hConnect = null;
    HINTERNET hRequest = null;
    bool cache_enabled = true;
    ProxyResolver proxy;
    UrlComponents url_components;
    wstr url;
    MemoryBuffer<char> response;
};

//////////////////////////////////////////////////////////////////////
// Download to memory

class MemoryDownloader : public Downloader, MemoryBuffer<byte>
{
    LOG_Context("MemoryDownloader");

public:
    MemoryDownloader();
    ~MemoryDownloader() override;

    byte *Data() const
    {
        return data;
    }

    size_t Size() const
    {
        return size;
    }

protected:
    bool OnReceiveData(byte *data, size_t size) override;
    bool OnReceiveComplete() override;
};

//////////////////////////////////////////////////////////////////////
// Download a file

class FileDownloader : public Downloader
{
    LOG_Context("FileDownloader");

public:
    FileDownloader();
    ~FileDownloader() override;

    bool Init(wchar const *file_name);
    void SetChecksum(Hash::Checksum *new_checksum, byte *hash);
    bool CheckChecksum() const;
    void CloseFileHandle();

    wstr filename;
    HANDLE file_handle = INVALID_HANDLE_VALUE;
    byte required_checksum[32]; // might be 16 (MD5) or 20 (SHA1) or 32 (SHA256)
    Hash::Checksum *checksum = null;

protected:
    bool OnReceiveData(byte *data, size_t size) override;
    bool OnReceiveComplete() override;
};
