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

//////////////////////////////////////////////////////////////////////
// direct 3d/2d/write

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>

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

#include "Resource.h"
#include "Types.h"
#include "Debug.h"
#include "Ansi.h"
#include "Log.h"
#include "Util.h"
#include "Defer.h"
#include "D3D.h"
#include "Settings.h"
#include "Globals.h"
#include "InterfaceTypes.h"
#include "Network.h"
#include "Graph.h"
