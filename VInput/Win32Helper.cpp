#include "pch.h"
#include "Win32Helper.h"
#include <devpkey.h>
#include "StringHelper.h"
#include <thread>


namespace VInput::Win32 {

    bool IsAdministrator() {
        SID_IDENTIFIER_AUTHORITY nt = SECURITY_NT_AUTHORITY;
        PSID sid = nullptr;
        BOOL isAdmin = FALSE;
        if (!AllocateAndInitializeSid(&nt, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sid))
            return false;
        const BOOL sucess = CheckTokenMembership(nullptr, sid, &isAdmin);
        FreeSid(sid);
        return sucess && isAdmin;
    }

    bool IsFileExists(const std::wstring& path) {
        DWORD a = GetFileAttributesW(path.c_str());
        return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
    }

    std::filesystem::path GetAbsolutePath(const std::wstring& path) {
        return std::filesystem::absolute(path).lexically_normal();
    }

    std::filesystem::path GetWindowsInfDirectory() {
        std::vector<wchar_t> buffer(MAX_PATH, '\0');
        
        UINT size = GetWindowsDirectoryW(buffer.data(), MAX_PATH);
        if (size <= MAX_PATH) {
            return std::filesystem::path(buffer.data()) / L"INF";
        }
        else if (size > MAX_PATH) {
            buffer.resize(size, '\0');
            if (size == GetWindowsDirectoryW(buffer.data(), size)) {
                return std::filesystem::path(buffer.data()) / L"INF";
            }
        }
        return L"C:\\Windows\\INF";
    }

    std::filesystem::path GetSystem32Directory() {
        std::vector<wchar_t> buffer(MAX_PATH, '\0');

        UINT size = GetSystemDirectoryW(buffer.data(), MAX_PATH);
        if (size <= MAX_PATH) {
            return std::filesystem::path(buffer.data());
        }
        else if (size > MAX_PATH) {
            buffer.resize(size, '\0');
            if (size == GetSystemDirectoryW(buffer.data(), size)) {
                return std::filesystem::path(buffer.data());
            }
        }
        return L"C:\\Windows\\System32";
    }

    static bool EnableTakeOwnershipPrivilege()
    {
        HANDLE token = nullptr;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
            return false;
        }

        LUID luid{};
        const bool found = LookupPrivilegeValueW(nullptr, SE_TAKE_OWNERSHIP_NAME, &luid) != FALSE;

        TOKEN_PRIVILEGES privileges{};
        privileges.PrivilegeCount = 1;
        privileges.Privileges[0].Luid = luid;
        privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        SetLastError(ERROR_SUCCESS);
        const bool adjusted = found && AdjustTokenPrivileges(token, FALSE, &privileges, 0, nullptr, nullptr) != FALSE;
        const DWORD error = GetLastError();

        CloseHandle(token);

        if (!adjusted || error != ERROR_SUCCESS) {
            SetLastError(error);
            return false;
        }

        return true;
    }

    static DWORD GrantAdministratorsDeleteAccess(const std::filesystem::path& path)
    {
        BYTE sidBuffer[SECURITY_MAX_SID_SIZE]{};
        DWORD sidSize = sizeof(sidBuffer);

        if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, sidBuffer, &sidSize)) {
            return GetLastError();
        }

        DWORD error = SetNamedSecurityInfoW(
            const_cast<wchar_t*>(path.c_str()),
            SE_FILE_OBJECT,
            OWNER_SECURITY_INFORMATION,
            sidBuffer,
            nullptr,
            nullptr,
            nullptr
        );

        if (error != ERROR_SUCCESS) {
            return error;
        }

        EXPLICIT_ACCESSW access{};
        access.grfAccessPermissions = DELETE;
        access.grfAccessMode = GRANT_ACCESS;
        access.grfInheritance = NO_INHERITANCE;
        BuildTrusteeWithSidW(&access.Trustee, sidBuffer);

        PACL oldDacl = nullptr;
        PACL newDacl = nullptr;
        PSECURITY_DESCRIPTOR descriptor = nullptr;

        error = GetNamedSecurityInfoW(
            const_cast<wchar_t*>(path.c_str()),
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            nullptr,
            nullptr,
            &oldDacl,
            nullptr,
            &descriptor
        );

        if (error == ERROR_SUCCESS) {
            error = SetEntriesInAclW(1, &access, oldDacl, &newDacl);
        }

        if (error == ERROR_SUCCESS) {
            error = SetNamedSecurityInfoW(
                const_cast<wchar_t*>(path.c_str()),
                SE_FILE_OBJECT,
                DACL_SECURITY_INFORMATION,
                nullptr,
                nullptr,
                newDacl,
                nullptr
            );
        }

        LocalFree(newDacl);
        LocalFree(descriptor);
        return error;
    }

    VInput::Win32::Win32Result DeleteSysFile(const std::wstring& fileName)
    {
        VInput::Win32::Win32Result r;
        auto system32 = GetSystem32Directory();
        auto sysFile = system32 / L"drivers" / fileName;
        if (!std::filesystem::exists(sysFile))
            return r;

        if (DeleteFileW(sysFile.c_str()))
            return r;

        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            return r;
        }

        if (error == ERROR_ACCESS_DENIED) {
            if (!EnableTakeOwnershipPrivilege()) {
                MakeResult(r, false, GetLastError());
                return r;
            }

            error = GrantAdministratorsDeleteAccess(sysFile);

            if (error != ERROR_SUCCESS) {
                MakeResult(r, false, error);
                return r;
            }

            if (DeleteFileW(sysFile.c_str())) {
                return r;
            }

            error = GetLastError();
        }

        if ((error == ERROR_ACCESS_DENIED || error == ERROR_SHARING_VIOLATION) && MoveFileExW(sysFile.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT)) {
            r.needReboot = true;
            return r;
        }

        MakeResult(r, false, error);
        return r;
    }


    int Exec(const std::wstring& command, std::wstring* exec_stdout = nullptr) {
        const std::wstring fullCommand = command + TEXT(" 2>&1");
        FILE* pipe = _tpopen(fullCommand.c_str(), TEXT("r"));
        if (pipe == nullptr) {
            if (exec_stdout != nullptr)
                exec_stdout->append(TEXT("PIPE ERROR"));
            return -1;
        }

        std::array<wchar_t, 4096> buffer{};
        while (_fgetts(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
            if (exec_stdout != nullptr)
                exec_stdout->append(buffer.data());
        }
        return _pclose(pipe);
    }

    HMODULE GetSelfModule() {
        HMODULE h = nullptr;
        GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<const wchar_t*>(&GetSelfModule), &h);
        return h;
    }

    bool WriteResourceToFile(HMODULE mod, int resId, const std::filesystem::path& path) {
        const HRSRC resource = FindResource(mod, MAKEINTRESOURCE(resId), RT_RCDATA);
        if (!resource) {
            return false;
        }

        const DWORD size = SizeofResource(mod, resource);
        const HGLOBAL handle = LoadResource(mod, resource);
        const void* data = handle ? LockResource(handle) : nullptr;

        if (!handle || (size != 0 && !data)) {
            return false;
        }

        const auto parent = path.parent_path();
        if (parent.empty()) {
            if (!std::filesystem::create_directories(parent)) {
                return false;
            }
        }

        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file || (size != 0 && !file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size)))) {
            return false;
        }

        return true;
    }

    template<typename T>
    static std::optional<T> ParseRegistryInteger(const std::vector<std::byte>& data) noexcept
    {
        static_assert(std::is_trivially_copyable_v<T>);

        if (data.size() != sizeof(T))
            return std::nullopt;

        T value{};
        std::memcpy(&value, data.data(), sizeof(value));
        return value;
    }

    static std::optional<std::wstring> ParseRegistryString(const std::vector<std::byte>& data)
    {
        if (data.empty())
            return std::wstring{};

        if (data.size() % sizeof(wchar_t) != 0)
            return std::nullopt;

        const wchar_t* characters = reinterpret_cast<const wchar_t*>(data.data());
        std::size_t characterCount = data.size() / sizeof(wchar_t);

        while (characterCount != 0 && characters[characterCount - 1] == L'\0')
            --characterCount;
        return std::wstring(characters, characterCount);
    }

    static std::optional<std::vector<std::wstring>> ParseRegistryMultiString(const std::vector<std::byte>& data)
    {
        if (data.empty())
            return std::vector<std::wstring>{};

        if (data.size() % sizeof(wchar_t) != 0)
            return std::nullopt;

        const wchar_t* characters = reinterpret_cast<const wchar_t*>(data.data());
        const std::size_t characterCount = data.size() / sizeof(wchar_t);
        std::vector<std::wstring> values;
        std::size_t offset = 0;

        while (offset < characterCount)
        {
            std::size_t length = 0;
            while (offset + length < characterCount && characters[offset + length] != L'\0')
                ++length;
            if (offset + length >= characterCount)
                return std::nullopt;

            if (length == 0)
                return values;

            values.emplace_back(characters + offset, length);
            offset += length + 1;
        }
        return values;
    }

    static std::optional<bool> GetDeviceBoolProperty(HDEVINFO deviceInfoSet, SP_DEVINFO_DATA& deviceInfoData, const DEVPROPKEY& propertyKey)
    {
        DEVPROPTYPE propertyType = DEVPROP_TYPE_EMPTY;
        DWORD requiredSize = 1;
        std::vector<std::byte> buffer(requiredSize);
        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &propertyKey, &propertyType,
            reinterpret_cast<PBYTE>(buffer.data()), static_cast<DWORD>(buffer.size()), &requiredSize, 0)) {
            return std::nullopt;
        }

        const bool* b = reinterpret_cast<const bool*>(buffer.data());
        return *b;
    }

    static std::optional<uint32_t> GetDeviceDwordProperty(HDEVINFO deviceInfoSet, SP_DEVINFO_DATA& deviceInfoData, const DEVPROPKEY& propertyKey)
    {
        DEVPROPTYPE propertyType = DEVPROP_TYPE_EMPTY;
        DWORD requiredSize = 4;
        uint32_t result = 0;
        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &propertyKey, &propertyType, (BYTE *) & result, sizeof(result), &requiredSize, 0)) {
            return std::nullopt;
        }
        return result;
    }

    static std::optional<std::wstring> GetDeviceStringProperty(HDEVINFO deviceInfoSet, SP_DEVINFO_DATA& deviceInfoData, const DEVPROPKEY& propertyKey)
    {
        DEVPROPTYPE propertyType = DEVPROP_TYPE_EMPTY;
        DWORD requiredSize = 0;

        SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &propertyKey, &propertyType, nullptr, 0, &requiredSize, 0);

        const DWORD queryError = GetLastError();

        if (queryError != ERROR_INSUFFICIENT_BUFFER || requiredSize < sizeof(wchar_t))
            return std::nullopt;


        std::vector<std::byte> buffer(requiredSize);
        DWORD returnedSize = 0;

        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &propertyKey, &propertyType, reinterpret_cast<PBYTE>(buffer.data()), static_cast<DWORD>(buffer.size()), &returnedSize, 0))
            return std::nullopt;

        if (propertyType != DEVPROP_TYPE_STRING)
            return std::nullopt;

        return ParseRegistryString(buffer);
    }

    static std::optional<std::vector<std::wstring>> GetDeviceStringListProperty(HDEVINFO deviceInfoSet, SP_DEVINFO_DATA& deviceInfoData, const DEVPROPKEY& propertyKey)
    {
        DEVPROPTYPE propertyType = DEVPROP_TYPE_EMPTY;
        DWORD requiredSize = 0;

        SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &propertyKey, &propertyType, nullptr, 0, &requiredSize, 0);

        const DWORD queryError = GetLastError();

        if (queryError != ERROR_INSUFFICIENT_BUFFER || requiredSize < sizeof(wchar_t))
            return std::nullopt;


        std::vector<std::byte> buffer(requiredSize);
        DWORD returnedSize = 0;

        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &propertyKey, &propertyType, reinterpret_cast<PBYTE>(buffer.data()), static_cast<DWORD>(buffer.size()), &returnedSize, 0))
            return std::nullopt;

        if (propertyType != DEVPROP_TYPE_STRING_LIST)
            return std::nullopt;

        return ParseRegistryMultiString(buffer);
    }

    static std::wstring GetDeviceDisplayName(HDEVINFO deviceInfoSet, SP_DEVINFO_DATA& deviceInfoData)
    {
        if (const auto friendlyName = GetDeviceStringProperty(deviceInfoSet, deviceInfoData, DEVPKEY_Device_FriendlyName))
            return *friendlyName;

        if (const auto description = GetDeviceStringProperty(deviceInfoSet, deviceInfoData, DEVPKEY_Device_DeviceDesc))
            return *description;

        return L"<未知设备>";
    }

    static std::wstring GetDeviceInstanceId(HDEVINFO set, SP_DEVINFO_DATA& data) {
        DWORD required = 0;
        SetupDiGetDeviceInstanceIdW(set, &data, nullptr, 0, &required);
        if (!required)
            return {};
        std::vector<wchar_t> buf(required + 1);
        if (!SetupDiGetDeviceInstanceIdW(set, &data, buf.data(), static_cast<DWORD>(buf.size()), nullptr))
            return {};
        return buf.data();
    }

    std::vector<std::wstring> GetDeviceInterfacePaths(const GUID& guid) {
        std::vector<std::wstring> paths;

        VInput::Win32::DevInfoSet deviceInfoSet(SetupDiGetClassDevsW(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
        if (!deviceInfoSet.valid())
            return paths;

        for (DWORD index = 0;; ++index) {
            SP_DEVICE_INTERFACE_DATA interfaceData{};
            interfaceData.cbSize = sizeof(interfaceData);

            if (!SetupDiEnumDeviceInterfaces(deviceInfoSet.get(), nullptr, &guid, index, &interfaceData))
                break;

            DWORD requiredSize = 0;
            SetupDiGetDeviceInterfaceDetailW(deviceInfoSet.get(), &interfaceData, nullptr, 0, &requiredSize, nullptr);

            if (requiredSize == 0 || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                continue;

            std::vector<std::byte> buffer(requiredSize);
            SP_DEVICE_INTERFACE_DETAIL_DATA_W* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buffer.data());
            detail->cbSize = sizeof(*detail);
            if (!SetupDiGetDeviceInterfaceDetailW(deviceInfoSet.get(), &interfaceData, detail, requiredSize, nullptr, nullptr))
                continue;

            const std::wstring path(detail->DevicePath);

            if (!VInput::String::ContainsIgnoreCase(paths, path))
                paths.push_back(path);
        }

        return paths;
    }

    static std::vector<std::wstring> GetMultiSzProperty(HDEVINFO set, SP_DEVINFO_DATA& data, DWORD prop) {
        DWORD type = 0, required = 0;
        SetupDiGetDeviceRegistryPropertyW(set, &data, prop, &type, nullptr, 0, &required);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || !required)
            return {};
        std::vector<BYTE> buf(required + 2 * sizeof(wchar_t), 0);
        if (!SetupDiGetDeviceRegistryPropertyW(set, &data, prop, &type, buf.data(), required, nullptr))
            return {};
        const wchar_t* p = reinterpret_cast<const wchar_t*>(buf.data());
        std::vector<std::wstring> out;
        if (type == REG_SZ) {
            if (*p)
                out.emplace_back(p);
            return out;
        }
        if (type != REG_MULTI_SZ)
            return out;
        while (*p) {
            out.emplace_back(p);
            p += wcslen(p) + 1;
        }
        return out;
    }

    //static bool Matches(HDEVINFO set, SP_DEVINFO_DATA& data, const std::vector<std::wstring>& ids) {
    //	const auto instance = GetDeviceInstanceId(set, data);
    //	auto hardware = GetMultiSzProperty(set, data, SPDRP_HARDWAREID);
    //	auto compatible = GetMultiSzProperty(set, data, SPDRP_COMPATIBLEIDS);
    //	for (const auto& wanted : ids) {
    //		if (VInput::String::StartsWithIgnoreCase(instance, wanted))
    //			return true;
    //		for (const auto& id : hardware)
    //			if (VInput::String::EqualsIgnoreCase(id, wanted))
    //				return true;
    //		for (const auto& id : compatible)
    //			if (VInput::String::EqualsIgnoreCase(id, wanted))
    //				return true;
    //	}
    //	return false;
    //}

    static std::wstring GetDeviceInstanceId(const std::wstring_view rootBus) {
        if (rootBus.empty())
            return {};

        ULONG listSize = 0;
        if (CM_Get_Device_ID_List_SizeW(&listSize, nullptr, CM_GETIDLIST_FILTER_PRESENT) != CR_SUCCESS)
            return {};

        std::vector<wchar_t> buffer(listSize);
        if (CM_Get_Device_ID_ListW(nullptr, buffer.data(), listSize, CM_GETIDLIST_FILTER_PRESENT) != CR_SUCCESS)
            return {};

        for (wchar_t* instanceId = buffer.data(); *instanceId; instanceId += wcslen(instanceId) + 1) {
            if (VInput::String::StartsWithIgnoreCase(instanceId, rootBus))
                return std::wstring(instanceId);
        }
        return {};
    }

    bool IsDeviceExists(const std::wstring_view targetHwId, const std::wstring_view filterInstanceId) {
        ULONG listSize = 0;
        if (CM_Get_Device_ID_List_SizeW(&listSize, nullptr, CM_GETIDLIST_FILTER_PRESENT) != CR_SUCCESS)
            return false;

        std::vector<wchar_t> buffer(listSize);
        if (CM_Get_Device_ID_ListW(nullptr, buffer.data(), listSize, CM_GETIDLIST_FILTER_PRESENT) != CR_SUCCESS)
            return false;

        for (wchar_t* instanceId = buffer.data(); *instanceId; instanceId += wcslen(instanceId) + 1) {
            if (!filterInstanceId.empty()) {
                if (!VInput::String::StartsWithIgnoreCase(instanceId, filterInstanceId))
                    continue;
            }

            DEVINST devInst;
            CONFIGRET cr = CM_Locate_DevNodeW(&devInst, instanceId, CM_LOCATE_DEVNODE_NORMAL);
            if (cr != CR_SUCCESS)
                continue;

            DEVPROPTYPE propType;
            ULONG size = MAX_PATH;
            std::vector<BYTE> propBuffer(MAX_PATH);
            cr = CM_Get_DevNode_PropertyW(devInst, &DEVPKEY_Device_HardwareIds, &propType, propBuffer.data(), &size, 0);
            if (cr != CR_SUCCESS) {
                if (cr == CR_BUFFER_SMALL) {
                    propBuffer.resize(size);
                    cr = CM_Get_DevNode_PropertyW(devInst, &DEVPKEY_Device_HardwareIds, &propType, propBuffer.data(), &size, 0);
                }
                else
                    continue;
            }
            if (cr != CR_SUCCESS || propType != DEVPROP_TYPE_STRING_LIST)
                continue;

            wchar_t* p = reinterpret_cast<wchar_t*>(propBuffer.data());
            while (*p) {
                if (wcscmp(p, targetHwId.data()) == 0) {
                    return true;
                }
                p += wcslen(p) + 1;
            }
        }
        return false;
    }

    VInput::Win32::Win32Result CreateRootDevice(const std::wstring_view infFullPath, const std::wstring_view rootDeviceId, const std::wstring_view hardwareId)
    {
        VInput::Win32::Win32Result r;

        if (hardwareId.empty()) {
            MakeResult(r, false, ERROR_INVALID_PARAMETER);
            return r;
        }
        std::wstring rootId;
        rootId.reserve(5 + rootDeviceId.size());
        rootId.append(L"ROOT\\");
        rootId.append(rootDeviceId);
        if (IsDeviceExists(hardwareId, rootId))
            return r;

        if (!VInput::Win32::IsFileExists(std::wstring{ infFullPath })) {
            MakeResult(r, false, ERROR_FILE_NOT_FOUND);
            return r;
        }

        GUID classGuid{};
        wchar_t className[MAX_CLASS_NAME_LEN]{};

        if (!SetupDiGetINFClassW(infFullPath.data(), &classGuid, className, ARRAYSIZE(className), nullptr)) {
            MakeResult(r, false, GetLastError());
            return r;
        }

        DevInfoSet set(SetupDiCreateDeviceInfoList(&classGuid, nullptr));

        if (!set.valid()) {
            MakeResult(r, false, GetLastError());
            return r;
        }

        SP_DEVINFO_DATA data{};
        data.cbSize = sizeof(data);

        if (!SetupDiCreateDeviceInfoW(set.get(), rootDeviceId.data(), &classGuid, nullptr, nullptr, DICD_GENERATE_ID, &data)) {
            MakeResult(r, false, GetLastError());
            return r;
        }

        std::vector<wchar_t> multi(hardwareId.size() + 2, L'\0');
        std::copy(hardwareId.begin(), hardwareId.end(), multi.begin());

        if (!SetupDiSetDeviceRegistryPropertyW(set.get(), &data, SPDRP_HARDWAREID, reinterpret_cast<const BYTE*>(multi.data()),
            static_cast<DWORD>(multi.size() * sizeof(wchar_t)))) {
            MakeResult(r, false, GetLastError());
            return r;
        }

        if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set.get(), &data)) {
            MakeResult(r, false, GetLastError());
            return r;
        }

        BOOL reboot = FALSE;
        if (!UpdateDriverForPlugAndPlayDevicesW(nullptr, hardwareId.data(), infFullPath.data(), 0, &reboot)) {
            const DWORD installError = GetLastError();
            BOOL undoReboot = FALSE;
            DiUninstallDevice(GetDesktopWindow(), set.get(), &data, 0, &undoReboot);
            MakeResult(r, false, installError, reboot || undoReboot);
            return r;
        }
        return r;
    }

    VInput::Win32::Win32Result RemoveRootDevice(const std::wstring_view rootDeviceId) {
        VInput::Win32::Win32Result r;
        if (rootDeviceId.empty())
            return r;

        ULONG listSize = 0;
        CONFIGRET cr = CM_Get_Device_ID_List_SizeW(&listSize, nullptr, CM_GETIDLIST_FILTER_PRESENT);
        if (cr != CR_SUCCESS) {
            MakeResult(r, false, CM_MapCrToWin32Err(cr, ERROR_GEN_FAILURE));
            return r;
        }

        std::vector<wchar_t> buffer(listSize);
        cr = CM_Get_Device_ID_ListW(L"ROOT", buffer.data(), listSize, CM_GETIDLIST_FILTER_ENUMERATOR | CM_GETIDLIST_FILTER_PRESENT);
        if (cr != CR_SUCCESS) {
            MakeResult(r, false, CM_MapCrToWin32Err(cr, ERROR_GEN_FAILURE));
            return r;
        }
        std::vector<std::wstring_view> instances;
        for (wchar_t* instanceId = buffer.data(); *instanceId; instanceId += wcslen(instanceId) + 1) {
            if (VInput::String::StartsWithIgnoreCase(instanceId, rootDeviceId))
                instances.push_back(instanceId);
        }

        for (const auto& id : instances) {
            DevInfoSet one(SetupDiCreateDeviceInfoList(nullptr, nullptr));
            if (!one.valid()) {
                DWORD e = GetLastError();
                MakeResult(r, false, e);
                continue;
            }
            SP_DEVINFO_DATA data{};
            data.cbSize = sizeof(data);
            if (!SetupDiOpenDeviceInfoW(one.get(), id.data(), nullptr, 0, &data)) {
                DWORD e = GetLastError();
                if (e == ERROR_NO_SUCH_DEVINST || e == ERROR_NOT_FOUND)
                    continue;
                MakeResult(r, false, e);
                continue;
            }
            BOOL reboot = FALSE;
            if (!DiUninstallDevice(GetDesktopWindow(), one.get(), &data, 0, &reboot)) {
                DWORD e = GetLastError();
                MakeResult(r, false, e);
                continue;
            }
            r.needReboot = r.needReboot || reboot;
        }
        return r;
    }

    bool ScanHardware() {
        DEVINST root = 0;
        CONFIGRET cr = CM_Locate_DevNodeW(&root, nullptr, CM_LOCATE_DEVNODE_NORMAL);
        if (cr != CR_SUCCESS) {
            return false;
        }
        cr = CM_Reenumerate_DevNode(root, CM_REENUMERATE_NORMAL);
        if (cr != CR_SUCCESS) {
            return false;
        }
        return true;
    }

    VInput::Win32::Win32Result DeleteServiceEntry(const wchar_t* name) {
        VInput::Win32::Win32Result r;
        auto deleter = [](SC_HANDLE handle) noexcept { if (handle != nullptr) CloseServiceHandle(handle); };

        std::unique_ptr<std::remove_pointer_t<SC_HANDLE>, decltype(deleter)> scm(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));

        if (!scm) {
            MakeResult(r, false, GetLastError());
            return r;
        }

        std::unique_ptr<std::remove_pointer_t<SC_HANDLE>, decltype(deleter)> svc(OpenServiceW(scm.get(), name, SERVICE_QUERY_STATUS | SERVICE_STOP | DELETE));

        if (!svc) {
            DWORD e = GetLastError();
            if (e == ERROR_SERVICE_DOES_NOT_EXIST) {
                return r;
            }
            if (e == ERROR_SERVICE_MARKED_FOR_DELETE) {
                r.needReboot = true;
                return r;
            }
            MakeResult(r, false, e);
            return r;
        }
        SERVICE_STATUS_PROCESS ssp{};
        DWORD needed = 0;
        if (QueryServiceStatusEx(svc.get(), SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &needed)) {
            if (ssp.dwCurrentState != SERVICE_STOPPED && ssp.dwCurrentState != SERVICE_STOP_PENDING) {
                SERVICE_STATUS ss{};
                ControlService(svc.get(), SERVICE_CONTROL_STOP, &ss);
            }
        }
        if (!DeleteService(svc.get())) {
            DWORD e = GetLastError();
            if (e == ERROR_SERVICE_MARKED_FOR_DELETE) {
                VInput::Win32::MakeResult(r, true, e);
                return r;
            }
            MakeResult(r, false, e);
            return r;
        }
        return r;
    }

    VInput::Win32::Win32Result DeleteRegKey(HKEY root, std::wstring_view parentKeyPath, std::wstring_view deletedKeyName, REGSAM view)
    {
        VInput::Win32::Win32Result result;
        constexpr REGSAM requiredAccess = DELETE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE;
        auto deleter = [](HKEY key) { if (key) RegCloseKey(key); };
        HKEY key = nullptr;
        LONG status = RegOpenKeyExW(root, parentKeyPath.data(), 0, requiredAccess | view, &key);
        std::unique_ptr<std::remove_pointer_t<HKEY>, decltype(deleter)> parentKey(key);

        if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND)
            return result;

        if (status != ERROR_SUCCESS) {
            MakeResult(result, false, status);
            return result;
        }
        status = RegDeleteTreeW(parentKey.get(), deletedKeyName.data());
        if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND && status != ERROR_PATH_NOT_FOUND) {
            MakeResult(result, false, status);
        }
        return result;
    }


    VInput::Win32::Win32Result DeleteRegKey(HKEY root, const std::wstring& subkey, REGSAM view) {
        auto deleter = [](HKEY key) { if (key) RegCloseKey(key); };
        VInput::Win32::Win32Result r;
        HKEY key = nullptr;
        LONG st = RegOpenKeyExW(root, subkey.data(), 0, KEY_READ | view, &key);
        std::unique_ptr<std::remove_pointer_t<HKEY>, decltype(deleter)> safeKey(key);
        if (st == ERROR_FILE_NOT_FOUND)
            return r;
        if (st != ERROR_SUCCESS) {
            MakeResult(r, false, st);
            return r;
        }
        std::wstring path(subkey);
        auto pos = path.find_last_of(L'\\');
        if (pos == std::wstring::npos) {
            MakeResult(r, false, ERROR_INVALID_PARAMETER);
            return r;
        }
        const auto parent = path.substr(0, pos);
        const auto leaf = path.substr(pos + 1);
        HKEY parentKey = nullptr;
        st = RegOpenKeyExW(root, parent.c_str(), 0, KEY_WRITE | DELETE | view, &parentKey);
        std::unique_ptr<std::remove_pointer_t<HKEY>, decltype(deleter)> safeParentKey(parentKey);
        if (st != ERROR_SUCCESS) {
            MakeResult(r, false, st);
            return r;
        }
        st = RegDeleteTreeW(safeParentKey.get(), leaf.c_str());
        if (st != ERROR_SUCCESS && st != ERROR_FILE_NOT_FOUND) {
            MakeResult(r, false, st);
            return r;
        }
        return r;
    }


    /// <summary>
    /// 控制面板中的“选择指针移动速度”选项， [1, 20], 默认为10
    /// </summary>
    /// <returns></returns>
    static bool SetMouseSpeed(int speed) {
        if (speed < 1 || speed > 20)
            return false;
        return SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(INT_PTR)speed, SPIF_SENDCHANGE);
    }

    static int GetMouseSpeed() {
        INT speed = 0;
        if (!SystemParametersInfo(SPI_GETMOUSESPEED, 0, &speed, 0))
            return -1;
        return static_cast<int>(speed);
    }

    static MouseAcceleration GetMouseAcceleration() {
        MouseAcceleration mouseParams{};
        if (!SystemParametersInfo(SPI_GETMOUSE, 0, &mouseParams, 0))
            return { 0, 0, -1 };

        return mouseParams;
    }

    static bool SetMouseAcceleration(MouseAcceleration acceleration) {
        if (acceleration.Acceleration < 0 || acceleration.Acceleration > 2)
            return false;
        return SystemParametersInfo(SPI_SETMOUSE, 0, &acceleration, 0);
    }

    MouseSpeedInfo SetSafeMouseSpeed() {
        MouseSpeedInfo speedInfo{};
        speedInfo.speed = GetMouseSpeed();
        if (speedInfo.speed != 10)
            SetMouseSpeed(10);
        speedInfo.acceleration = GetMouseAcceleration();
        if (speedInfo.acceleration.Acceleration != 0)
            SetMouseAcceleration({ 0, 0, 0 });

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return speedInfo;
    }

    void RecoveryMouseSpeed(MouseSpeedInfo& speedInfo) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (speedInfo.speed != 10)
            SetMouseSpeed(speedInfo.speed);
        if (speedInfo.acceleration.Acceleration != 0)
            SetMouseAcceleration(speedInfo.acceleration);

        speedInfo.speed = -1;
        speedInfo.acceleration.Acceleration = -1;
    }
}