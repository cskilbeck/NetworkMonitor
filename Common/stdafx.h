//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

//////////////////////////////////////////////////////////////////////
// windows

#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <combaseapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <TimeAPI.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <strsafe.h>
#include <commctrl.h>
#include <ColorDlg.h>
#include <commdlg.h>
#include <winhttp.h>

#include <Shlwapi.h>
#pragma warning(push)
#pragma warning(disable : 4091)
#include <Shlobj.h>
#pragma warning(pop)
#include <mmsystem.h>

//////////////////////////////////////////////////////////////////////
// cruntime

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

//////////////////////////////////////////////////////////////////////
// std

#include <vector>
#include <string>
#include <map>
#include <list>
#include <algorithm>
#include <thread>
#include <mutex>

//////////////////////////////////////////////////////////////////////
// local

#include "Types.h"
#include "Debug.h"
#include "Ansi.h"
#include "Log.h"
#include "Util.h"
#include "Defer.h"
#include "D3D.h"
#include "md5.h"
#include "sha256.h"
#include "MemoryBuffer.h"
#include "Checksum.h"
#include "File.h"
#include "InterfaceTypes.h"
#include "Network.h"
#include "Proxy.h"
#include "Downloader.h"
#include "json_parser.h"
