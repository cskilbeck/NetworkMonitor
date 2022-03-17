#include "stdafx.h"

LOG_Context("File");

//////////////////////////////////////////////////////////////////////
// ChecksumFile - get MD5 of a file

bool ChecksumFile(wchar const *filename, uint64 &file_size, Hash::Checksum *checksum)
{
    HANDLE f = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, 0, null);
    if(f == INVALID_HANDLE_VALUE) {
        ReportWindowsError(L"Can't open %s", filename);
        return false;
    }
    defer(CloseHandle(f));

    checksum->Init();

    constexpr size_t buffer_size = 1048576; // 1 meg chunks

    byte *buffer = (byte *)malloc(buffer_size);
    defer(free(buffer));

    if(!GetFileSizeEx(f, (LARGE_INTEGER *)&file_size)) {
        ReportWindowsError(L"Can't get file size of %s", filename);
        return false;
    }

    uint64 remaining = file_size;

    while(remaining != 0) {
        uint64 read_size = remaining;
        if(read_size > buffer_size) {
            read_size = buffer_size;
        }

        DWORD got;
        if(!ReadFile(f, buffer, (DWORD)read_size, &got, null) || got != read_size) {
            ReportWindowsError(L"Can't read from %s", filename);
            return false;
        }
        checksum->Update(buffer, (uint)read_size);
        remaining -= read_size;
    }
    checksum->Final();
    return true;
}

//////////////////////////////////////////////////////////////////////
// Save a file, overwriting without prompting if it exists already

HRESULT SaveFile(wchar const *filename, byte *data, uint64 size)
{
    HANDLE h = CreateFile(filename, GENERIC_WRITE, 0, null, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, null);
    if(h == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }
    defer(CloseHandle(h));
    while(size > 0) {
        uint32 write_size = 16384;
        if(size < write_size) {
            write_size = (uint32)size;
        }
        DWORD wrote;
        if(!WriteFile(h, data, write_size, &wrote, null) || wrote != write_size) {
            return GetLastError();
        }
        data += write_size;
        size -= write_size;
    }
    return S_OK;
}

//////////////////////////////////////////////////////////////////////

HRESULT CreateEmptyFile(wchar const *filename)
{
    if(!CreateFolderForFile(filename)) {
        return GetLastError();
    }
    HANDLE h = CreateFile(filename, GENERIC_WRITE, 0, null, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, null);
    if(h == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }
    CloseHandle(h);
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// just load a whole file into a memory buffer

HRESULT LoadFile(wchar const *filename, MemoryBuffer<byte> &data)
{
    HANDLE f = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, 0, null);
    if(f == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }
    defer(CloseHandle(f));
    uint64 file_size;
    if(!GetFileSizeEx(f, (LARGE_INTEGER *)&file_size)) {
        return GetLastError();
    }
    data.set_length((size_t)file_size);
    uint64 remaining = file_size;
    constexpr size_t chunk_size = 16384;
    byte *buffer = data.data;
    while(remaining != 0) {
        uint64 read_size = remaining;
        if(read_size > chunk_size) {
            read_size = chunk_size;
        }
        DWORD got;
        if(!ReadFile(f, buffer, (DWORD)read_size, &got, null) || got != read_size) {
            return GetLastError();
        }
        buffer += read_size;
        remaining -= read_size;
    }
    return S_OK;
}

//////////////////////////////////////////////////////////////////////

bool LaunchExecutable(wstr const &executable, wstr const &working_folder, wstr &parameters)
{
    LOG_Info(L"Launching %s", executable.c_str());
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    bool rc = CreateProcessW(
        executable, parameters.buffer(), null, null, false, DETACHED_PROCESS, null, working_folder, &si, &pi);
    if(rc) {
        LOG_Info(L"Launched, Process ID: %08x, Thread ID: %08x", pi.dwProcessId, pi.dwThreadId);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    else {
        LOG_Error(L"Failed to launch %s", executable.c_str());
    }
    return rc;
}

//////////////////////////////////////////////////////////////////////

wstr GetTemporaryFilename(wchar const *prefix)
{
    wstr temp_path(MAX_PATH + 1);
    GetTempPath(MAX_PATH, temp_path.buffer());
    wstr temp_filename(MAX_PATH + 1);
    GetTempFileName(temp_path, prefix, 0, temp_filename.buffer());
    return temp_filename;
}

//////////////////////////////////////////////////////////////////////

bool DeleteFolder(wchar const *path)
{
    // create double null-terminated wchar string
    wstr p(path);
    size_t l = p.size();
    std::vector<wchar> buf(l + 2);
    memcpy(&buf[0], path, (l + 1) * sizeof(wchar));
    buf[l + 1] = 0;
    SHFILEOPSTRUCT o = { 0 };
    o.wFunc = FO_DELETE;
    o.pFrom = buf.data();
    o.pTo = null;
    o.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NO_UI;
    return SHFileOperation(&o) == 0; // care if this fails?
}

//////////////////////////////////////////////////////////////////////

bool EnsureFolderExists(wchar const *path)
{
    if(!FolderExists(path)) {
        LOG_Debug(L"Creating folder %s", path);
        if(!SUCCEEDED(CreateFolder(path))) {
            DWORD e = GetLastError();
            LOG_Error(L"Error creating %s", path);
            SetLastError(e);
            return false;
        }
    }
    return true;
}
