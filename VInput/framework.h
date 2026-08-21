#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>

#include <initguid.h>
#include <shellapi.h>
#include <Knownfolders.h>
#include <bcrypt.h>
#include <cwctype>
#include <Shlobj.h>
#include <SetupAPI.h>
#include <functional>
#include <newdev.h>
#include <cfgmgr32.h>
#include <AclAPI.h>

#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Bcrypt.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Advapi32.lib")