#include "pch.h"
#include "InfHelper.h"
#include "Win32Helper.h"
#include "StringHelper.h"
#include <SetupAPI.h>
#include <algorithm>
#include <climits>
#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <ranges>
#include <newdev.h>
#include <functional>
#include <initializer_list>
#include "StopwatchHelper.h"

#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Newdev.lib")

namespace VInput::Inf {
    static std::vector<std::wstring> EnumeratePublishedInfs(const std::filesystem::path& infDirectory) {
        const std::wstring searchPattern = infDirectory.native() + L"\\oem*.inf"; // C:\Windows\INF
        WIN32_FIND_DATAW findData{};
        auto fileHandle = VInput::Win32::SafePtr<HANDLE, FindClose>{ FindFirstFileExW(searchPattern.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH) };
        if (!fileHandle)
            return {};

        std::vector<std::wstring> result;
        do {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                result.push_back(findData.cFileName);
        } while (FindNextFileW(fileHandle.get(), &findData));
        return result;
    }

    static std::optional<std::filesystem::path> GetDriverStoreInfPath(const std::wstring_view publishedInfName) {
        DWORD requiredCharacters = 0;
        std::vector<wchar_t> buffer(MAX_PATH, L'\0');
        if (!SetupGetInfDriverStoreLocationW(publishedInfName.data(), nullptr, nullptr, buffer.data(), static_cast<DWORD>(buffer.size()), &requiredCharacters)) {
            const DWORD error = GetLastError();
            if (error == ERROR_INSUFFICIENT_BUFFER) {
                buffer.resize(requiredCharacters);
                requiredCharacters = 0;
                if (!SetupGetInfDriverStoreLocationW(publishedInfName.data(), nullptr, nullptr, buffer.data(), static_cast<DWORD>(buffer.size()), &requiredCharacters))
                    return std::nullopt;
                return std::filesystem::path(buffer.data());
            }
            return std::nullopt;
        }
        return std::filesystem::path(buffer.data());
    }

    static DWORD GetInfStringField(INFCONTEXT &context, DWORD fieldIndex, std::vector<wchar_t> &buffer) {
        DWORD returnedCharacters = 0;
        if (!SetupGetStringFieldW(&context, fieldIndex, buffer.data(), static_cast<DWORD>(buffer.size()), &returnedCharacters)) {
            const DWORD error = GetLastError();
            if (error == ERROR_INSUFFICIENT_BUFFER) {
                buffer.resize(returnedCharacters);
                returnedCharacters = 0;
                if (!SetupGetStringFieldW(&context, fieldIndex, buffer.data(), static_cast<DWORD>(buffer.size()), &returnedCharacters))
                    return 0;
                return returnedCharacters;
            }
            return 0;
        }
        return returnedCharacters;
    }

    static DriverPackageVersion GetInfVersionField(HINF infHandle) {
        const wchar_t* section = L"Version";
        INFCONTEXT context{};
        if (!SetupFindFirstLineW(infHandle, section, nullptr, &context))
            return {};

        DriverPackageVersion version;
        std::vector<wchar_t> keyBuffer(static_cast<std::size_t>(MAX_PATH) + 1, L'\0');
        std::vector<wchar_t> valueBuffer(static_cast<std::size_t>(MAX_PATH) + 1, L'\0');


        do {
            if (GetInfStringField(context, 0, keyBuffer) != 0) {
                std::wstring key(keyBuffer.data());

                if (VInput::String::EqualsIgnoreCase(key, L"Class")) {
                    if (GetInfStringField(context, 1, valueBuffer) != 0) {
                        std::wstring value(valueBuffer.data());
                        version.className = value;
                    }
                }
                else if (VInput::String::EqualsIgnoreCase(key, L"Provider")) {
                    if (GetInfStringField(context, 1, valueBuffer) != 0) {
                        std::wstring value(valueBuffer.data());
                        version.provider = value;
                    }
                }
                else if (VInput::String::EqualsIgnoreCase(key, L"ClassGuid")) {
                    if (GetInfStringField(context, 1, valueBuffer) != 0) {
                        std::wstring value(valueBuffer.data());
                        version.classGuid = value;
                    }
                }
                else if (VInput::String::EqualsIgnoreCase(key, L"CatalogFile")) {
                    if (GetInfStringField(context, 1, valueBuffer) != 0) {
                        std::wstring value(valueBuffer.data());
                        version.catalogFile = value;
                    }
                }
                else if (VInput::String::EqualsIgnoreCase(key, L"DriverVer")) {
                    if (GetInfStringField(context, 1, valueBuffer) != 0) {
                        std::wstring value(valueBuffer.data());
                        version.driverVersion = value;
                    }
                    memset(valueBuffer.data(), 0, valueBuffer.size() * sizeof(wchar_t));
                    if (GetInfStringField(context, 2, valueBuffer) != 0) {
                        std::wstring value(valueBuffer.data());
                        version.driverVersion = version.driverVersion + L"," + value;
                    }
                }
            }
            std::fill(keyBuffer.begin(), keyBuffer.end(), L'\0');
            std::fill(valueBuffer.begin(), valueBuffer.end(), L'\0');
        } while (SetupFindNextLine(&context, &context));
        return version;
    }

    static std::optional<DriverPackageInfo> AnalyzePublishedInf(const std::filesystem::path& infDirectory, const std::wstring& publishedName) {
        DriverPackageInfo package;
        package.publishedName = publishedName;
        package.publishedInfPath = infDirectory / publishedName;

        const std::optional<std::filesystem::path> storeInfPath = GetDriverStoreInfPath(publishedName);
        if (storeInfPath.has_value()) {
            package.driverStoreInfPath = *storeInfPath;
            package.originalName = storeInfPath->filename().wstring();
        }

        UINT errorLine = 0;
        auto infHandle = VInput::Win32::SafePtr<HINF, SetupCloseInfFile>(SetupOpenInfFileW(package.publishedInfPath.c_str(), nullptr, INF_STYLE_WIN4, &errorLine));
        if (!infHandle)
            return std::nullopt;

        package.version = GetInfVersionField(infHandle.get());
        return package;
    }

    static std::optional<DriverPackageInfo> AnalyzeInf(const std::filesystem::path& infDirectory, const std::wstring& infName) {
        DriverPackageInfo package;
        package.publishedName = infName;
        package.publishedInfPath = infDirectory / infName;

        UINT errorLine = 0;
        auto infHandle = VInput::Win32::SafePtr<HINF, SetupCloseInfFile>(SetupOpenInfFileW(package.publishedInfPath.c_str(), nullptr, INF_STYLE_WIN4, &errorLine));
        if (!infHandle)
            return std::nullopt;

        package.version = GetInfVersionField(infHandle.get());
        return package;
    }

    /// <summary>
    /// 首次运行大约300毫秒，再次运行约30毫秒，Windows重启后重置
    /// </summary>
    /// <param name="filter"></param>
    /// <returns></returns>
    std::vector<VInput::Inf::DriverPackageInfo> GetDriverPackageInfos(std::function<bool(const VInput::Inf::DriverPackageInfo&)> filter) {
        std::filesystem::path infDirectory = VInput::Win32::GetWindowsInfDirectory();
        std::vector<std::wstring> publishedNames = EnumeratePublishedInfs(infDirectory);
        std::vector<VInput::Inf::DriverPackageInfo> infPackages;

        for (const std::wstring& publishedName : publishedNames) {
            const std::optional<VInput::Inf::DriverPackageInfo> package = AnalyzePublishedInf(infDirectory, publishedName);
            if (package.has_value() && filter(*package)) {
                infPackages.push_back(*package);
            }
        }
        return infPackages;
    }

    VInput::Win32::DriverErrorStatus GetVenderDriverStatus(std::vector<VInput::Inf::DriverPackageInfo> &infos, std::initializer_list<RequiredInf> requiredInfs, const std::wstring_view vender) {
        for (const RequiredInf& requiredInf : requiredInfs) {
            if (requiredInf.outputPath != nullptr) {
                requiredInf.outputPath->clear();
            }
        }

        auto vendoerPackages = infos | std::ranges::views::filter([&vender](const VInput::Inf::DriverPackageInfo& package){
                return VInput::String::ContainsIgnoreCase(package.version.provider, vender);
            });
        bool incompatible = false;
        std::size_t matchedCount = 0;
        for (const auto &package : vendoerPackages) {
            for (const RequiredInf& requiredInf : requiredInfs) {
                if (requiredInf.outputPath == nullptr || !requiredInf.outputPath->empty())
                    continue;

                if (!VInput::String::EqualsIgnoreCase(package.originalName, requiredInf.originalName)) {
                    incompatible = true;
                    continue;
                }

                *requiredInf.outputPath = package.publishedInfPath;
                ++matchedCount;
            }

            if (matchedCount == requiredInfs.size())
                return VInput::Win32::DriverErrorStatus::DRIVER_INSTALLED;
        }
        if (incompatible)
            return VInput::Win32::DriverErrorStatus::DRIVER_INCOMPATIBLE;
        if (matchedCount != 0)
            return VInput::Win32::DriverErrorStatus::DRIVER_FOUND;
        return VInput::Win32::DriverErrorStatus::DRIVER_NOT_FOUND;
    }


    VInput::Win32::Win32Result InstallInfPackage(const std::wstring& infPath) {
        VInput::Win32::Win32Result r;
        if (!VInput::Win32::IsFileExists(infPath)) {
            VInput::Win32::MakeResult(r, false, ERROR_FILE_NOT_FOUND);
            return r;
        }
        BOOL reboot = FALSE;
        const auto path = VInput::Win32::GetAbsolutePath(infPath);
        if (!DiInstallDriverW(nullptr, path.c_str(), 0, &reboot)) {
            VInput::Win32::MakeResult(r, false, GetLastError());
            return r;
        }
        r.needReboot = reboot != FALSE;
        return r;
    }

    VInput::Win32::Win32Result UninstallInfPackage(const std::wstring& infPath) {
        VInput::Win32::Win32Result r;
        if (!VInput::Win32::IsFileExists(infPath)) {
            VInput::Win32::MakeResult(r, false, ERROR_FILE_NOT_FOUND);
            return r;
        }
        const auto path = VInput::Win32::GetAbsolutePath(infPath);
        BOOL reboot = FALSE;
        if (!DiUninstallDriverW(nullptr, path.c_str(), 0, &reboot)) {
            DWORD e = GetLastError();
            if (e == ERROR_FILE_NOT_FOUND || e == ERROR_NOT_FOUND) {
                return r;
            }
            MakeResult(r, false, e);
            return r;
        }
        r.needReboot = reboot != FALSE;
        return r;
    }


    template<typename T>
    concept PrintableField = requires(T v) {
        { v.empty() } -> std::convertible_to<bool>;
        { std::wcout << v } -> std::same_as<std::wostream&>;
    };

    template<PrintableField T>
    void PrintInfField(const wchar_t* fieldName, const T& value) {
        std::wcout << fieldName << L": ";
        if (value.empty())
            std::wcout << L"<未知>";
        else
            std::wcout << value;

        std::wcout << L'\n';
    }

    void PrintPackage(const VInput::Inf::DriverPackageInfo& package) {
        std::wcout << L"================================================================================\n";
        PrintInfField(L"Published INF ", package.publishedName);
        PrintInfField(L"Original INF  ", package.originalName);
        PrintInfField(L"Provider      ", package.version.provider);
        PrintInfField(L"Class         ", package.version.className);
        PrintInfField(L"Class GUID    ", package.version.classGuid);
        PrintInfField(L"DriverVer     ", package.version.driverVersion);
        PrintInfField(L"Catalog       ", package.version.catalogFile);
        PrintInfField(L"Published     ", package.publishedInfPath);
        PrintInfField(L"Driver Store  ", package.driverStoreInfPath);
    }

    void PrintPackages(const std::vector<VInput::Inf::DriverPackageInfo>& packages) {
        for (const VInput::Inf::DriverPackageInfo& package : packages) {
            PrintPackage(package);
        }
    }
}

