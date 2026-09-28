#include "pch.h"
#include "RzDriver.h"
#include "StopwatchHelper.h"
#include "StringHelper.h"
#include "InfHelper.h"
#include "Win32Helper.h"
#include "resource.h"
#include <vector>


namespace VInput::Rz {

    constexpr std::wstring_view kRzVirtualBusId = L"ROOT\\RzDev0306VBus"; // 此驱动ID无要求

    constexpr std::wstring_view kRzVirtualDeviceId = L"RAZER\\VirtualBus\\VID_1532&PID_0306";

    constexpr std::wstring_view mouseHardwareId = L"HID\\VID_1532&PID_0306&MI_00&Col01";

    constexpr GUID GUID_RZ_CONTROL_INTERFACE = { 0xE3BE005D, 0xD130, 0x4910, {0x88, 0xFF, 0x09, 0xAE, 0x02, 0xF6, 0x80, 0xE9} };


    static std::filesystem::path ReleaseBuildinDriver() {
        constexpr std::pair<int, const wchar_t*> driverFiles[] = {
            { IDR_RZ_COMMON_CAT,   L"rzcommon.cat"         },
            { IDR_RZ_COMMON_SYS,   L"RzCommon.sys"         },
            { IDR_RZ_COMMON_INF,   L"RzCommonU.inf"        },
                                                           
            { IDR_RZ_DEV_0306_SYS, L"RzDev_0306.sys"       },
                                                           
            { IDR_RZ_DEV_VBUS_CAT, L"rzdev_0306_vbus.cat"  },
            { IDR_RZ_DEV_VBUS_INF, L"RzDevU_0306_VBus.inf" },
                                                           
            { IDR_RZ_DEV_VCON_CAT, L"rzdev_0306_vcon.cat"  },
            { IDR_RZ_DEV_VCON_INF, L"RzDevU_0306_VCon.inf" },
                                                           
            { IDR_RZ_DEV_VKBD_CAT, L"rzdev_0306_vkbd.cat"  },
            { IDR_RZ_DEV_VKBD_INF, L"RzDevU_0306_VKbd.inf" },
                                                           
            { IDR_RZ_DEV_VMOU_CAT, L"rzdev_0306_vmou.cat"  },
            { IDR_RZ_DEV_VMOU_INF, L"RzDevU_0306_VMou.inf" },

            { IDR_RZ_DEV_KBD_CAT,  L"rzdev_0306_kbd.cat"   },
            { IDR_RZ_DEV_KBD_INF,  L"RzDevU_0306_Kbd.inf"  },

            { IDR_RZ_DEV_MOU_CAT,  L"rzdev_0306_mou.cat"   },
            { IDR_RZ_DEV_MOU_INF,  L"RzDevU_0306_Mou.inf"  },
        };

        auto tempPath = std::filesystem::temp_directory_path();
        if (std::filesystem::exists(tempPath)) {
            auto mod = VInput::Win32::GetSelfModule();
            tempPath = tempPath / L"VInput" / L"RZ_DRIVER";
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

    static bool IsDriverReady()
    {
        return VInput::Win32::GetDeviceInterfacePaths(GUID_RZ_CONTROL_INTERFACE).size() > 0;
    }

    static int InternalUninstall(const std::wstring& vbusInf, const std::wstring& vconInf,
        const std::wstring& vkbdInf, const std::wstring& vmouInf, const std::wstring& commInf,
        const std::wstring& kbdInf, const std::wstring& mouInf)
    {
        VInput::Win32::Win32Result r;
        MergeResult(VInput::Win32::RemoveRootDevice(kRzVirtualBusId), r);
        if (r.error || r.needReboot)
            return r.error;
        VInput::Stopwatch::Stopwatch stop;
        stop.Start();
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            if (stop.ElapsedMilliseconds() >= 1000)
                break;
        } while (IsDriverReady());
        stop.Stop();
        //std::wcout << std::format(L"IsDriverReady 耗时: {:.3f} ms\n", stop.ElapsedMilliseconds());

        if (!kbdInf.empty())
            MergeResult(VInput::Inf::UninstallInfPackage(kbdInf), r);
        if (!mouInf.empty())
            MergeResult(VInput::Inf::UninstallInfPackage(mouInf), r);
        if (!vconInf.empty())
            MergeResult(VInput::Inf::UninstallInfPackage(vconInf), r);
        if (!vkbdInf.empty())
            MergeResult(VInput::Inf::UninstallInfPackage(vkbdInf), r);
        if (!vmouInf.empty())
            MergeResult(VInput::Inf::UninstallInfPackage(vmouInf), r);
        if (!vbusInf.empty())
            MergeResult(VInput::Inf::UninstallInfPackage(vbusInf), r);
        if (!commInf.empty())
            MergeResult(VInput::Inf::UninstallInfPackage(commInf), r);

        if (!r.error && !r.needReboot) {
            MergeResult(VInput::Win32::DeleteServiceEntry(L"RzCommon"), r);
            MergeResult(VInput::Win32::DeleteServiceEntry(L"RzDev_0306"), r);
            if (!r.error && !r.needReboot) {
                VInput::Win32::DeleteSysFile(L"RzCommon.sys");
                VInput::Win32::DeleteSysFile(L"RzDev_0306.sys");
                VInput::Win32::DeleteRegKey(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\DeviceClasses", L"{E3BE005D-D130-4910-88FF-09AE02F680E9}");
            }
        }

        //std::wcout << L"Uninstall Result: " << (r.success ? L"SUCCESS" : L"FAILED")
        //    << L" , need reboot: " << (r.needReboot ? L"YES" : L"NO")
        //    << L" , error code: " << r.error
        //    << L" , error line: " << r.errorLine
        //    << L"\n";
        return r.error;
    }

    VInput::Win32::DriverErrorStatus RzDriver::GetDriverStatus() {
        auto infos = VInput::Inf::GetDriverPackageInfos([](const auto& info) {
            return VInput::String::ContainsIgnoreCase(info.version.provider, VInput::Rz::vender);
            });
        return VInput::Inf::GetVenderDriverStatus(infos, {
            { L"RzDevU_0306_Kbd.inf",  &kbdInfPath_  },
            { L"RzDevU_0306_Mou.inf",  &mouInfPath_  },
            { L"RzDevU_0306_VCon.inf", &vconInfPath_ },
            { L"RzDevU_0306_VKbd.inf", &vkbdInfPath_ },
            { L"RzDevU_0306_VMou.inf", &vmouInfPath_ },
            { L"RzDevU_0306_VBus.inf", &vbusInfPath_ },
            { L"RzCommonU.inf",        &commInfPath_ },
        }, VInput::Rz::vender);
    }

    int RzDriver::Uninstall()
    {
        auto status = GetDriverStatus();
        if (status == VInput::Win32::DriverErrorStatus::DRIVER_NOT_FOUND)
            return 0;

        auto error = InternalUninstall(vbusInfPath_, vconInfPath_, vkbdInfPath_, vmouInfPath_, commInfPath_, kbdInfPath_, mouInfPath_);
        if (!error) {
            auto tempPath = std::filesystem::temp_directory_path() / L"VInput" / L"RZ_DRIVER";
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
        return error;
    }

    int RzDriver::Install(const char* utf8DriverDirectory)
    {
        if (IsDriverReady())
            return 0;

        auto status = GetDriverStatus();
        if (status == VInput::Win32::DriverErrorStatus::DRIVER_FOUND) {
            auto uninstallStatus = InternalUninstall(vbusInfPath_, vconInfPath_, vkbdInfPath_, vmouInfPath_, commInfPath_, kbdInfPath_, mouInfPath_);
            if (uninstallStatus != 0)
                return uninstallStatus;
        }

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

            commInfPath_ = driverDirectory / L"RzCommonU.inf";
            vmouInfPath_ = driverDirectory / L"RzDevU_0306_VMou.inf";
            vkbdInfPath_ = driverDirectory / L"RzDevU_0306_VKbd.inf";
            vconInfPath_ = driverDirectory / L"RzDevU_0306_VCon.inf";
            mouInfPath_  = driverDirectory / L"RzDevU_0306_Mou.inf";
            kbdInfPath_  = driverDirectory / L"RzDevU_0306_Kbd.inf";
            vbusInfPath_ = driverDirectory / L"RzDevU_0306_VBus.inf";
            if (r.success)
                MergeResult(VInput::Inf::InstallInfPackage(commInfPath_), r);
            if (r.success)
                MergeResult(VInput::Inf::InstallInfPackage(vmouInfPath_), r);
            if (r.success)
                MergeResult(VInput::Inf::InstallInfPackage(vkbdInfPath_), r);
            if (r.success)
                MergeResult(VInput::Inf::InstallInfPackage(vconInfPath_), r);
            if (r.success)
                MergeResult(VInput::Inf::InstallInfPackage(mouInfPath_), r);
            if (r.success)
                MergeResult(VInput::Inf::InstallInfPackage(kbdInfPath_), r);
        }

        if (r.success)
            MergeResult(VInput::Win32::CreateRootDevice(vbusInfPath_, kRzVirtualBusId.substr(5), kRzVirtualDeviceId), r);

        if (!r.success)
            InternalUninstall(vbusInfPath_, vconInfPath_, vkbdInfPath_, vmouInfPath_, commInfPath_, kbdInfPath_, mouInfPath_);

        if (r.success) {
            VInput::Stopwatch::Stopwatch stop;
            stop.Start();
            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                if (stop.ElapsedMilliseconds() >= 1000)
                    break;
            } while (!IsDriverReady());
            stop.Stop();
            //std::wcout << std::format(L"IsDriverReady 耗时: {:.3f} ms\n", stop.ElapsedMilliseconds());
        }

        //std::wcout << L"Install Result: " << (r.success ? L"SUCCESS" : L"FAILED")
        //    << L" , need reboot: " << (r.needReboot ? L"YES" : L"NO")
        //    << L" , error code: " << r.error
        //    << L" , error line: " << r.errorLine
        //    << L"\n";
        return r.error;
    }

}


