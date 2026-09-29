#include "pch.h"
#include "UvDriver.h"
#include "Win32Helper.h"
#include "InfHelper.h"
#include "resource.h"
#include "StringHelper.h"
#include "StopwatchHelper.h"


namespace VInput::Uv {
    // {745A17A0-74D3-11D0-B6FE-00A0C90F57DA}
    //DEFINE_GUID(GUID_UVHID_INTERFACE, 0x745A17A0, 0x74D3, 0x11D0, 0xB6, 0xFE, 0x00, 0xA0, 0xC9, 0x0F, 0x57, 0xDA);

    constexpr std::wstring_view kRootDeviceInstanceId = L"ROOT\\uvhid";
    constexpr std::wstring_view kRootDeviceHardwareId = L"HID\\uvhid";
    constexpr std::wstring_view controllerHardwareId = L"HID\\UVHID&Col01";


    static std::filesystem::path ReleaseBuildinDriver() {
        constexpr std::pair<int, const wchar_t*> driverFiles[] = {
            { IDR_UVHID_CAT, L"uvhid.cat" },
            { IDR_UVHID_INF, L"uvhid.inf" },
            { IDR_UVHID_SYS, L"uvhid.sys" },
        };

        auto tempPath = std::filesystem::temp_directory_path();
        if (std::filesystem::exists(tempPath)) {
            auto mod = VInput::Win32::GetSelfModule();
            tempPath = tempPath / L"VInput" / L"UV_DRIVER";
            if (std::filesystem::exists(tempPath)) {
                for (const auto& [resId, filename] : driverFiles) {
                    if (!std::filesystem::exists(tempPath / filename)) {
                        VInput::Win32::WriteResourceToFile(mod, resId, tempPath / filename);
                    }
                }
            }
            else
            {
                std::filesystem::create_directories(tempPath);
                for (const auto& [resId, filename] : driverFiles) {
                    VInput::Win32::WriteResourceToFile(mod, resId, tempPath / filename);
                }
            }
            return tempPath;
        }
        return {};
    }


    static int InternalUninstall(const std::wstring& inf)
    {
        VInput::Win32::Win32Result r;
        MergeResult(VInput::Win32::RemoveRootDevice(kRootDeviceInstanceId), r);
        if (r.error || r.needReboot)
            return r.error;
        if (!inf.empty())
            MergeResult(VInput::Inf::UninstallInfPackage(inf), r);
        MergeResult(VInput::Win32::DeleteServiceEntry(L"uvhid"), r);

        std::wcout << L"Uninstall Result: " << (r.success ? L"SUCCESS" : L"FAILED")
            << L" , need reboot: " << (r.needReboot ? L"YES" : L"NO")
            << L" , error code: " << r.error
            << L" , error line: " << r.errorLine
            << L"\n";
        return r.error;
    }

    static bool IsDriverReady()
    {
        return VInput::Win32::IsDeviceExists(controllerHardwareId, controllerHardwareId);
    }

    VInput::Win32::DriverErrorStatus UvDriver::GetDriverStatus()
    {
        auto infos = VInput::Inf::GetDriverPackageInfos([](const auto& info) {
            return VInput::String::ContainsIgnoreCase(info.version.provider, VInput::Uv::vender);
            });
        return VInput::Inf::GetVenderDriverStatus(infos, {
            { L"uvhid.inf", &infPath_ },
            }, VInput::Uv::vender);
    }

    int UvDriver::Uninstall()
    {
        auto status = GetDriverStatus();
        if (status == VInput::Win32::DriverErrorStatus::DRIVER_NOT_FOUND)
            return 0;
        auto error = InternalUninstall(infPath_);
        if (!error) {
            auto tempPath = std::filesystem::temp_directory_path() / L"VInput" / L"UV_DRIVER";
            if (std::filesystem::exists(tempPath)) {
                std::error_code ec;
                std::filesystem::remove_all(tempPath, ec);
                if (ec.value() == 0) {
                    auto appTempPath = std::filesystem::temp_directory_path() / L"VInput";
                    if (std::filesystem::directory_iterator(appTempPath) == std::filesystem::directory_iterator()) {
                        std::filesystem::remove_all(appTempPath);
                    }
                }
            }
        }
        return 0;
    }

    int UvDriver::Install(const char* utf8DriverDirectory)
    {
        if (IsDriverReady())
            return 0;

        auto status = GetDriverStatus();
        if (status == VInput::Win32::DriverErrorStatus::DRIVER_FOUND) {
            auto uninstallStatus = InternalUninstall(infPath_);
            if (uninstallStatus != 0)
                return uninstallStatus;
        }
        else if (status == VInput::Win32::DriverErrorStatus::DRIVER_INCOMPATIBLE)
            return status;

        VInput::Win32::Win32Result r;
        if (status != VInput::Win32::DriverErrorStatus::DRIVER_INSTALLED) {
            std::filesystem::path driverDirectory;
            if (utf8DriverDirectory == nullptr) {
                driverDirectory = ReleaseBuildinDriver();
                if (driverDirectory.empty())
                    return -1;
            }
            else {
                std::wstring utf16Path;
                VInput::String::ConvertUTF8ToUTF16(utf8DriverDirectory, utf16Path);
                driverDirectory = std::filesystem::path(utf16Path);
            }
            infPath_ = driverDirectory / L"uvhid.inf";
            if (r.success)
                MergeResult(VInput::Inf::InstallInfPackage(infPath_), r);
            if (!r.success)
                InternalUninstall(infPath_);
        }

        MergeResult(VInput::Win32::CreateRootDevice(infPath_, kRootDeviceInstanceId.substr(5), kRootDeviceHardwareId), r);

        if (r.success) {
            VInput::Stopwatch::Stopwatch stop;
            stop.Start();
            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                if (stop.ElapsedMilliseconds() >= 1000)
                    break;
            } while (!IsDriverReady());
            stop.Stop();
            std::wcout << std::format(L"IsDriverReady 耗时: {:.3f} ms\n", stop.ElapsedMilliseconds());
        }

        std::wcout << L"Install Result: " << (r.success ? L"SUCCESS" : L"FAILED")
            << L" , need reboot: " << (r.needReboot ? L"YES" : L"NO")
            << L" , error code: " << r.error
            << L" , error line: " << r.errorLine
            << L"\n";

        return r.error;
    }
}
