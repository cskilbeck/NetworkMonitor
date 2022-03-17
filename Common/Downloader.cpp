//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

LOG_Context("::Downloader");

//////////////////////////////////////////////////////////////////////

struct status_message
{
    int status_code;
    char const *name;
    char const *message;
};

status_message status_messages[] = { { 100, "CONTINUE", "The request can be continued" },
    { 101, "SWITCH_PROTOCOLS", "The server has switched protocols in an upgrade header" },

    { 200, "OK", "The request completed successfully" },
    { 201, "CREATED", "The request has been fulfilled and resulted in the creation of a new resource" },
    { 202, "ACCEPTED", "The request has been accepted for processing, but the processing has not been completed" },
    { 203,
        "PARTIAL",
        "The returned meta information in the entity-header is not the definitive set available from the originating "
        "server" },
    { 204, "NO_CONTENT", "The server has fulfilled the request, but there is no new information to send back" },
    { 205,
        "RESET_CONTENT",
        "The request has been completed, and the client program should reset the document view that caused the request "
        "to be sent to allow the user to easily initiate another "
        "input action" },
    { 206, "PARTIAL_CONTENT", "The server has fulfilled the partial GET request for the resource" },
    { 207,
        "WEBDAV_MULTI_STATUS",
        "During a World Wide Web Distributed Authoring and Versioning (WebDAV) operation, this indicates multiple "
        "status codes for a single response. The response body contains "
        "Extensible Markup Language (XML) that describes the status codes. For more information, see HTTP Extensions "
        "for Distributed Authoring" },

    { 300, "ambiguous", "The requested resource is available at one or more locations" },
    { 301,
        "moved",
        "The requested resource has been assigned to a new permanent Uniform Resource Identifier (URI), and any future "
        "references to this resource should be done using one of the "
        "returned URIs" },
    { 302, "redirect", "The requested resource resides temporarily under a different URI" },
    { 303,
        "redirect method",
        "The response to the request can be found under a different URI and should be retrieved using a GET HTTP verb "
        "on that resource" },
    { 304, "not modified", "The requested resource has not been modified" },
    { 305, "use proxy", "The requested resource must be accessed through the proxy given by the location field" },
    { 307, "redirect keep verb", "The redirected request keeps the same HTTP verb. HTTP/1.1 behavior" },

    { 400, "bad request", "The request could not be processed by the server due to invalid syntax" },
    { 401, "denied", "The requested resource requires user authentication" },
    { 402, "payment required", "Not implemented in the HTTP protocol" },
    { 403, "forbidden", "The server understood the request, but cannot fulfill it" },
    { 404, "not found", "The server has not found anything that matches the requested URI" },
    { 405, "bad method", "The HTTP verb used is not allowed" },
    { 406, "none acceptable", "No responses acceptable to the client were found" },
    { 407, "proxy auth required", "Proxy authentication required" },
    { 408, "request timeout", "The server timed out waiting for the request" },
    { 409,
        "conflict",
        "The request could not be completed due to a conflict with the current state of the resource. The user should "
        "resubmit with more information" },
    { 410, "gone", "The requested resource is no longer available at the server, and no forwarding address is known" },
    { 411, "length required", "The server cannot accept the request without a defined content length" },
    { 412,
        "precondition failed",
        "The precondition given in one or more of the request header fields evaluated to false when it was tested on "
        "the server" },
    { 413,
        "request too large",
        "The server cannot process the request because the request entity is larger than the server is able to "
        "process" },
    { 414,
        "uri too long",
        "The server cannot service the request because the request URI is longer than the server can interpret" },
    { 415,
        "unsupported media",
        "The server cannot service the request because the entity of the request is in a format not supported by the "
        "requested resource for the requested method" },
    { 449, "retry with", "The request should be retried after doing the appropriate action" },

    { 500,
        "server error",
        "The server encountered an unexpected condition that prevented it from fulfilling the request" },
    { 501, "not supported", "The server does not support the functionality required to fulfill the request" },
    { 502,
        "bad gateway",
        "The server, while acting as a gateway or proxy, received an invalid response from the upstream server it "
        "accessed in attempting to fulfill the request" },
    { 503, "service unavailable", "The service is temporarily overloaded" },
    { 504, "gateway timeout", "The request was timed out waiting for a gateway" },
    { 505,
        "version not supported",
        "The server does not support the HTTP protocol version that was used in the request message" } };

wstr GetStatusMessage(int code)
{
    for(auto const &m : status_messages) {
        if(m.status_code == code) {
            return StrToWide($("%d %s: %s", m.status_code, m.name, m.message));
        }
    }
    return $(L"%s:UNKNOWN", code);
}

//////////////////////////////////////////////////////////////////////

Downloader::Downloader()
{
}

//////////////////////////////////////////////////////////////////////

Downloader::~Downloader()
{
    // https://msdn.microsoft.com/en-us/library/windows/desktop/aa384090(v=vs.85).aspx
    // "An application should never WinHttpCloseHandle call on a synchronous request. This can create a race condition.
    // See HINTERNET Handles in WinHTTP for more information."

    // if(hRequest != null) {
    //    WinHttpCloseHandle(hRequest);
    //}
    if(hConnect != null) {
        WinHttpCloseHandle(hConnect);
    }
    if(hSession != null) {
        WinHttpCloseHandle(hSession); // this closes the child handles
    }
}

//////////////////////////////////////////////////////////////////////

DWORD WINAPI downloader_thread_routine(void *param)
{
    return static_cast<Downloader *>(param)->DoDownload();
}

//////////////////////////////////////////////////////////////////////

DWORD Downloader::DoDownload()
{
    LOG_Debug(L"DoDownload(%s)", url.c_str());

    wchar const *user_agent = L"curl/7.55.1";

    hSession = WinHttpOpen(user_agent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, null, null, 0);
    if(hSession == null) {
        error_message = WindowsError(L"Can't open HTTP library");
        return 0;
    }

    DWORD dwError = proxy.ResolveProxy(hSession, url);

    if(dwError != ERROR_SUCCESS) {
        LOG_Error(L"Proxy Resolution failed with Error: 0x%x\n", dwError);
        error_message = WindowsError(L"Can't resolve proxy for %s", url.c_str());
        return 0;
    }

    hConnect = WinHttpConnect(hSession, url_components.components.lpszHostName, url_components.components.nPort, 0);
    if(hConnect == null) {
        error_message = WindowsError(L"Can't connect HTTP library");
        return 0;
    }

    DWORD flags = 0;
    if(_wcsnicmp(L"https", url_components.scheme, 5) == 0) {
        flags |= WINHTTP_FLAG_SECURE;
    }

    if(!cache_enabled) {
        flags |= WINHTTP_FLAG_REFRESH;
    }

    wchar const *accept_types[] = { L"*/*", null };
    wchar const *get = L"GET";
    wstr url = $(L"%s%s", url_components.path.c_str(), url_components.extrainfo.c_str());

    hRequest = WinHttpOpenRequest(hConnect, get, url.c_str(), null, null, accept_types, flags);
    if(hRequest == null) {
        error_message = WindowsError(L"Can't open HTTP request");
        return 0;
    }

    // read the content
    size_t buffer_size = 0;
    byte *read_buffer = null;
    DWORD state = 0;
    DWORD error;
    DWORD request_error = ERROR_SUCCESS;

    while(state == 0) {
        proxy.ResetProxyCursor();

        // keep trying proxies until they're exhausted or succeeded
        while(true) {
            error = proxy.SetNextProxySetting(hRequest, request_error);
            if(error == ERROR_NO_MORE_ITEMS) {
                error = request_error;
                break;
            }
            if(error != ERROR_SUCCESS) {
                break;
            }
            if(WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                error = ERROR_SUCCESS;
                break;
            }
            request_error = GetLastError();
        }

        if(error != ERROR_SUCCESS) {
            error_message = WindowsError(L"Can't send request");
            break;
        }

        if(!WinHttpReceiveResponse(hRequest, null)) {
            error_message = WindowsError(L"Can't receive response");
            break;
        }

        DWORD dwSize = sizeof(status_code);
        DWORD query_flags = WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER;
        if(!WinHttpQueryHeaders(hRequest, query_flags, null, &status_code, &dwSize, null)) {
            error_message = WindowsError(L"Can't query headers (1)");
            break;
        }

        DWORD content_length_size = sizeof(content_length);
        if(!WinHttpQueryHeaders(hRequest,
               WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
               null,
               &content_length,
               &content_length_size,
               null)) {
            LOG_Warn(L"Can't get content length for %s", Url().c_str());
        }

        LOG_Debug(L"Status code %d for %s", status_code, url.c_str());

        while(true) {
            DWORD data_available;
            if(!WinHttpQueryDataAvailable(hRequest, &data_available)) {
                error_message = WindowsError(L"Can't query data availability");
                break;
            }
            if(data_available == 0) {
                state = 1;
                break;
            }
            if(buffer_size < data_available) {
                delete[] read_buffer;
                buffer_size = data_available;
                read_buffer = new byte[buffer_size];
            }
            DWORD received_len;
            if(!WinHttpReadData(hRequest, static_cast<LPVOID>(read_buffer), data_available, &received_len)) {
                error_message = WindowsError(L"Can't read data");
                break;
            }

            if(status_code >= 400) {
                response.add(reinterpret_cast<char *>(read_buffer), received_len);
            }
            else if(!OnReceiveData(read_buffer, received_len)) {
                status = Status::failed;
                OnFailure();
                return 0;
            }
        }
    }

    delete[] read_buffer;

    if(status_code >= 400) {
        response.add(0); // null terminate the response string
        error_message = $(L"%s", GetStatusMessage(status_code).c_str());
    }

    if(state == 1 && status_code < 400) {
        LOG_Debug(L"Finished downloading %s", Url().c_str());
        if(OnReceiveComplete()) {
            status = Status::succeeded;
            OnSuccess();
            return state;
        }
    }
    status = Status::failed;
    OnFailure();
    return state;
}

//////////////////////////////////////////////////////////////////////

bool Downloader::Start(wchar const *url)
{
    this->url = url;
    LOG_Debug(L"Start: \"%s\"", url);
    if(!url_components.Init(url)) {
        error_message = WindowsError(L"Bad URL: %s", url);
        status = Status::failed;
        return false;
    }
    status = Status::in_progress;
    thread_handle = CreateThread(null, 0, downloader_thread_routine, static_cast<void *>(this), 0, &thread_id);
    if(thread_handle == INVALID_HANDLE_VALUE) {
        error_message = WindowsError(L"Can't create thread");
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////

bool Downloader::Join(DWORD milliseconds) const
{
    if(thread_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    return WaitForSingleObject(thread_handle, milliseconds) != WAIT_TIMEOUT;
}

//////////////////////////////////////////////////////////////////////

bool Downloader::Abort()
{
    if(!Join(500)) {
        Terminate();
        return true; // true means it got zapped
    }
    return false;
}

//////////////////////////////////////////////////////////////////////

void Downloader::Terminate()
{
    status = Status::aborted;
    TerminateThread(thread_handle, 0);
}

//////////////////////////////////////////////////////////////////////

FileDownloader::FileDownloader()
    : Downloader()
    , required_checksum{}
{
}

//////////////////////////////////////////////////////////////////////

bool FileDownloader::Init(wchar const *file_name)
{
    filename = file_name;
    return true;
}

//////////////////////////////////////////////////////////////////////

void FileDownloader::SetChecksum(Hash::Checksum *new_checksum, byte *hash)
{
    new_checksum->Init();
    checksum = new_checksum;
    memcpy(required_checksum, hash, new_checksum->Size());
    LOG_Debug(L"Setting checksum for %s to %s",
        filename.c_str(),
        BytesToHexString(required_checksum, new_checksum->Size()).c_str());
}

//////////////////////////////////////////////////////////////////////

bool FileDownloader::CheckChecksum() const
{
    if(checksum != null) {
        byte const *result = checksum->Result();
        bool match = memcmp(result, required_checksum, checksum->Size()) == 0;
        if(!match) {
            LOG_Error(L"Checksum for %s (%s) does not match (%s)",
                filename.c_str(),
                BytesToHexString(result, checksum->Size()).c_str(),
                BytesToHexString(required_checksum, checksum->Size()).c_str());
        }
        return match;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////

bool FileDownloader::OnReceiveData(byte *data, size_t size)
{
    if(file_handle == INVALID_HANDLE_VALUE) {
        if(!CreateFolderForFile(filename)) {
            error_message = WindowsError(L"Can't create folder %s", GetPath(filename).c_str());
            return false;
        }
        // extra step here to delete existing file
        // because CreateFile (CREATE_ALWAYS) doesn't fail as it should
        // when the file is locked by another process
        if(FileExists(filename) && !DeleteFile(filename)) {
            error_message = WindowsError(L"Can't create file %s", GetFilename(filename).c_str());
            LOG_Error(error_message.c_str());
            return false;
        }
        LOG_Debug(L"Creating %s", filename.c_str());
        file_handle = CreateFile(filename, GENERIC_WRITE, 0, null, CREATE_ALWAYS, 0, null);
        if(file_handle == INVALID_HANDLE_VALUE) {
            error_message = WindowsError(L"Can't create file %s", GetFilename(filename).c_str());
            LOG_Error(error_message.c_str());
            return false;
        }
    }
    DWORD wrote;
    if(!WriteFile(file_handle, data, static_cast<DWORD>(size), &wrote, null) || wrote != size) {
        error_message = WindowsError(L"Can't write to file %s", GetFilename(filename).c_str());
        CloseFileHandle();
        return false;
    }
    if(checksum != null) {
        checksum->Update(data, static_cast<uint32>(size));
    }
    return true;
}

//////////////////////////////////////////////////////////////////////

void FileDownloader::CloseFileHandle()
{
    if(file_handle != INVALID_HANDLE_VALUE) {
        LOG_Debug(L"Closing %s", filename.c_str());
        CloseHandle(file_handle);
        file_handle = INVALID_HANDLE_VALUE;
    }
}

//////////////////////////////////////////////////////////////////////

bool FileDownloader::OnReceiveComplete()
{
    bool rc = CheckChecksum();
    if(!rc) {
        LOG_Error(L"Checksum doesn't match for %s", Url().c_str());
        error_message = $(L"checksum mismatch for file: %s", GetFilename(filename).c_str());
    }
    CloseFileHandle();
    return rc;
}

//////////////////////////////////////////////////////////////////////

FileDownloader::~FileDownloader()
{
    CloseFileHandle();
    if(checksum != null) {
        delete checksum;
        checksum = null;
    }
}

//////////////////////////////////////////////////////////////////////

bool MemoryDownloader::OnReceiveData(byte *data, size_t size)
{
    add(data, size);
    return true;
}

//////////////////////////////////////////////////////////////////////

bool MemoryDownloader::OnReceiveComplete()
{
    // make sure it's null terminated
    // this might mean there are two zeros on the end, no biggie
    add(0);
    return true;
}

//////////////////////////////////////////////////////////////////////

MemoryDownloader::MemoryDownloader()
    : Downloader()
{
}

//////////////////////////////////////////////////////////////////////

MemoryDownloader::~MemoryDownloader()
{
}

//////////////////////////////////////////////////////////////////////

bool UrlComponents::Init(wchar const *uri)
{
    if(uri != null) {
        valid = WinHttpCrackUrl(uri, 0, 0, (URL_COMPONENTS *)&components);
    }

    // this is the sort of thing that makes you want to use curl
    // winhttp fails if path is an empty string
    if(valid && components.lpszUrlPath[0] == 0) {
        components.lpszUrlPath[0] = '/';
        components.lpszUrlPath[1] = 0;
        components.dwUrlPathLength = 1;
    }
    return valid;
}

//////////////////////////////////////////////////////////////////////

UrlComponents::UrlComponents()
    : scheme(max_buffer_size)
    , hostname(max_buffer_size)
    , username(max_buffer_size)
    , password(max_buffer_size)
    , path(max_buffer_size)
    , extrainfo(max_buffer_size)
    , valid(false)
{
    memset(&components, 0, sizeof(URL_COMPONENTS));
    components.dwStructSize = sizeof(URL_COMPONENTS);
    components.lpszScheme = scheme.buffer();
    components.dwSchemeLength = max_buffer_size;
    components.lpszHostName = hostname.buffer();
    components.dwHostNameLength = max_buffer_size;
    components.lpszUserName = username.buffer();
    components.dwUserNameLength = max_buffer_size;
    components.lpszPassword = password.buffer();
    components.dwPasswordLength = max_buffer_size;
    components.lpszUrlPath = path.buffer();
    components.dwUrlPathLength = max_buffer_size;
    components.lpszExtraInfo = extrainfo.buffer();
    components.dwExtraInfoLength = max_buffer_size;
}

//////////////////////////////////////////////////////////////////////
