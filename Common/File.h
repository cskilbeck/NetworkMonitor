#pragma once

//////////////////////////////////////////////////////////////////////

bool ChecksumFile(wchar const *filename, uint64 &file_size, Hash::Checksum *checksum);
HRESULT SaveFile(wchar const *filename, byte *data, uint64 size);
HRESULT LoadFile(wchar const *filename, MemoryBuffer<byte> &data);
HRESULT CreateEmptyFile(wchar const *filename);
bool LaunchExecutable(wstr const &executable, wstr const &working_folder, wstr &parameters);
wstr GetTemporaryFilename(wchar const *prefix);
wstr SetFileExtension(wstr const &f);
bool DeleteFolder(wchar const *path);
bool EnsureFolderExists(wchar const *path);

//////////////////////////////////////////////////////////////////////

inline bool PathExists(wchar const *path)
{
    return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
}

//////////////////////////////////////////////////////////////////////

inline bool FileExists(wchar const *path)
{
    DWORD r = GetFileAttributes(path);
    return r != INVALID_FILE_ATTRIBUTES && (r & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

//////////////////////////////////////////////////////////////////////

inline bool FolderExists(wchar const *path)
{
    DWORD r = GetFileAttributes(path);
    return r != INVALID_FILE_ATTRIBUTES && (r & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

//////////////////////////////////////////////////////////////////////

inline uint64 FileSize(wchar const *path)
{
    WIN32_FILE_ATTRIBUTE_DATA a{ 0 };
    if(!GetFileAttributesEx(path, GetFileExInfoStandard, &a)) {
        return 0;
    }
    if(a.dwFileAttributes == INVALID_FILE_ATTRIBUTES || (a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return 0;
    }
    return ((uint64)a.nFileSizeHigh << 32) | a.nFileSizeLow;
}

//////////////////////////////////////////////////////////////////////

inline bool IsFolder(wchar const *path)
{
    return FolderExists(path);
}

//////////////////////////////////////////////////////////////////////

inline HRESULT CreateFolder(wchar const *name)
{
    int r = SHCreateDirectory(null, name);
    return (r == ERROR_SUCCESS || r == ERROR_ALREADY_EXISTS || r == ERROR_FILE_EXISTS) ? S_OK : HRESULT_FROM_WIN32(r);
}

//////////////////////////////////////////////////////////////////////

inline wstr PathJoin(wstr const &a, wstr const &b)
{
    wchar output[4096];
    PathCombine(output, a, b); // TODO (chs): security, PathCch not available on XP
    return output;
}

//////////////////////////////////////////////////////////////////////

inline bool RelativePath(wstr const &from, wstr const &to, wstr &result)
{
    wchar output[4096];
    DWORD from_flags = FILE_ATTRIBUTE_NORMAL;
    DWORD to_flags = FILE_ATTRIBUTE_NORMAL;
    if(IsFolder(from)) {
        from_flags = FILE_ATTRIBUTE_DIRECTORY;
    }
    if(IsFolder(to)) {
        to_flags = FILE_ATTRIBUTE_DIRECTORY;
    }
    if(!PathRelativePathTo(output, from, from_flags, to, to_flags)) {
        return false;
    }
    wchar *result_ptr = output;
    if(output[0] == '.' && output[1] == '\\') {
        result_ptr += 2;
    }
    result = result_ptr;
    return true;
}

//////////////////////////////////////////////////////////////////////

inline wstr MakePathCanonical(wstr const &path)
{
    wchar buffer[8192];
    PathCanonicalize(buffer, path); // TODO (chs): security, PathCch not available on XP
    return buffer;
}

//////////////////////////////////////////////////////////////////////

inline wstr GetCurrentFolder()
{
    wstr s;
    DWORD l = GetCurrentDirectoryW(0, NULL);
    s.resize(l);
    GetCurrentDirectoryW(l, s.buffer());
    return s;
}

//////////////////////////////////////////////////////////////////////

struct PathComponents
{
    wchar drive[_MAX_DRIVE];
    wchar dir[_MAX_DIR];
    wchar name[_MAX_FNAME];
    wchar ext[_MAX_EXT];
};

//////////////////////////////////////////////////////////////////////

inline PathComponents SplitPath(wstr const &path, PathComponents &pc)
{
    _wsplitpath_s(path.c_str(), pc.drive, pc.dir, pc.name, pc.ext);
    return pc;
}

//////////////////////////////////////////////////////////////////////

inline wstr GetDrive(wstr const &path)
{
    PathComponents pc;
    return SplitPath(path, pc).drive;
}

//////////////////////////////////////////////////////////////////////

inline wstr GetPath(wstr const &path)
{
    PathComponents pc;
    return SplitPath(path, pc).dir;
}

//////////////////////////////////////////////////////////////////////

inline wstr GetPathFromFilename(wstr const &filename)
{
    return PathJoin(GetDrive(filename), GetPath(filename));
}

//////////////////////////////////////////////////////////////////////

inline wstr GetFilenameOnly(wstr const &path)
{
    PathComponents pc;
    return SplitPath(path, pc).name;
}

//////////////////////////////////////////////////////////////////////

inline wstr GetExtension(wstr const &path)
{
    PathComponents pc;
    return SplitPath(path, pc).ext;
}

//////////////////////////////////////////////////////////////////////

inline wstr GetFilename(wstr const &path)
{
    return GetFilenameOnly(path) + GetExtension(path);
}

//////////////////////////////////////////////////////////////////////

inline wstr SetFileExtension(wstr const &f, wstr const &ext)
{
    PathComponents pc;
    SplitPath(f, pc);
    wstr e = ext.duplicate();
    if(!e.empty() && e.c_str()[0] != '.') {
        e = $(L".%s", ext.c_str());
    }
    return $(L"%s%s%s%s", pc.drive, pc.dir, pc.name, e.c_str());
}

//////////////////////////////////////////////////////////////////////

inline bool CreateFolderForFile(wstr const &filename)
{
    return SUCCEEDED(CreateFolder(GetPathFromFilename(filename)));
}
