#include "pch.h"
#include "LgDriver.h"
#include "resource.h"
#include "StringHelper.h"
#include "StopwatchHelper.h"

namespace VInput::Lg {

    DEFINE_GUID(GUID_LGHUB_XLCORE_INTERFACE, 0x1ABC05C0, 0xC378, 0x41B9, 0x9C, 0xEF, 0xDF, 0x1A, 0xBA, 0x82, 0xB0, 0x15);

    constexpr std::wstring_view kVirtualBusHardwareId       = L"ROOT\\LGHUBVirtualBus";
    constexpr std::wstring_view kVirtualDeviceHardwareId    = L"LGHUBDevice\\LGHUBVirtualDevice";
    constexpr std::wstring_view kVirtualJoystickHardwareId  = L"LGHUBDevice\\VID_046D&PID_C2AB";
    constexpr uint16_t kLogitechVendorId                    = 0x046D;
    constexpr uint16_t kVirtualKeyboardProductId            = 0xC232;
    constexpr uint16_t kVirtualMouseProductId               = 0xC231;
    constexpr uint16_t kVirtualJoystickProductId            = 0xC2AB;
    constexpr std::wstring_view kVirtualKeyboardHardwareId  = L"LGHUBDevice\\VID_046D&PID_C232";
    constexpr std::wstring_view kVirtualMouseHardwareId     = L"LGHUBDevice\\VID_046D&PID_C231";
    constexpr std::wstring_view mouseHardwareId             = L"HID\\VID_046D&PID_C231";


    constexpr DWORD IOCTL_LGHUB_PLUG_DEVICE = 0x002A2000;
    constexpr DWORD IOCTL_LGHUB_UNPLUG_DEVICE = 0x002A2004;


    struct VirtualDeviceDefinition
    {
        VirtualDeviceType deviceType;
        std::uint16_t vendorId;
        std::uint16_t productId;
        std::wstring_view hardwareId;
        std::span<const std::uint8_t> reportDescriptor;
    };

#pragma pack(push, 1)
    struct PlugRequest
    {
        std::uint32_t magic;                 // +0x00，调用方设置为 0xB7
        std::uint32_t deviceIdOut;           // +0x04，驱动成功后写回
        std::uint32_t initializedFlag;       // +0x08，驱动覆盖为 1
        std::uint32_t hardwareIdSize;        // +0x0C，单位：字节
        std::uint8_t hardwareId[0x80];       // +0x10..+0x8F, UTF-16 MULTI_SZ，末尾两个 WCHAR NUL
        std::uint16_t productId;             // +0x90，PID
        std::uint16_t vendorId;              // +0x92，VID

        std::uint32_t deviceType;            // +0x94 enum class VirtualDeviceType, 0 = Keyboard, 1 = Mouse, 2 = Other
        std::uint16_t version;               // +0x98

        /*
         * 驱动会把这三个值当作内核对象指针，并在非零时调用 ObfReferenceObject。
         * 用户态调用时必须全部保持为 0。
         */
        std::uint64_t object0;                // +0x9A
        std::uint64_t object1;                // +0xA2
        std::uint64_t object2;                // +0xAA

        std::uint32_t reportDescriptorSize;  // +0xB2，单位：字节
    };

    struct UnplugRequest
    {
        std::uint32_t size;
        std::uint32_t deviceIdOut;
        std::uint32_t deviceType;
        std::uint32_t reserved0;
        std::uint32_t reserved1;
    };

    struct MouseInputReport
    {
        std::uint8_t buttons;
        std::int8_t x;
        std::int8_t y;
        std::int8_t wheel;
        std::int8_t horizontalWheel;
    };

    struct KeyboardInputReport {
        uint8_t modifiers;  // byte 0: LCTRL=1, LSHIFT=2, LALT=4, LWIN=8,
        // RCTRL=16, RSHIFT=32, RALT=64, RWIN=128
        uint8_t reserved;   // byte 1: always 0 (HID spec)
        uint8_t key0;       // bytes 2..7: HID Usage id (a=0x04, ..., f1=0x3A, ...)
        uint8_t key1;		// see HID Keyboard/KeypadPage(0x07)
        uint8_t key2;
        uint8_t key3;
        uint8_t key4;
        uint8_t key5;
    };

#pragma pack(pop)

    enum class MouseButton : std::uint8_t
    {
        None = 0x00,
        Left = 0x01,
        Right = 0x02,
        Middle = 0x04,
        X1 = 0x08,
        X2 = 0x10,
        Button6 = 0x20,
        Button7 = 0x40,
        Button8 = 0x80
    };

    constexpr std::array<std::uint8_t, 63> kKeyboardReportDescriptor =
    {
        0x05, 0x01,
        0x09, 0x06,
        0xA1, 0x01,
        0x05, 0x07,
        0x19, 0xE0, 0x29, 0xE7,
        0x15, 0x00, 0x25, 0x01,
        0x75, 0x01, 0x95, 0x08,
        0x81, 0x02,
        0x95, 0x01, 0x75, 0x08,
        0x81, 0x01,
        0x95, 0x05, 0x75, 0x01,
        0x05, 0x08,
        0x19, 0x01, 0x29, 0x05,
        0x91, 0x02,
        0x95, 0x01, 0x75, 0x03,
        0x91, 0x01,
        0x95, 0x06, 0x75, 0x08,
        0x15, 0x00, 0x25, 0xE7,
        0x05, 0x07,
        0x19, 0x00, 0x29, 0xE7,
        0x81, 0x00,
        0xC0
    };

    constexpr std::array<std::uint8_t, 71> kMouseReportDescriptor =
    {
        0x05, 0x01,             // Usage Page (Generic Desktop)
        0x09, 0x02,             // Usage (Mouse)
        0xA1, 0x01,             // Collection (Application)
        0xA1, 0x02,             //   Collection (Logical)
        0x05, 0x09,             //     Usage Page (Buttons)
        0x19, 0x01, 0x29, 0x08, //     Usage Min-Max: 1..8 (8 buttons)
        0x15, 0x00, 0x25, 0x01,
        0x75, 0x01, 0x95, 0x08,
        0x81, 0x02,             //     Input (8 button bits)
        0x05, 0x01,             //     Usage Page (Generic Desktop)
        0x09, 0x01,             //     Usage (Pointer)
        0xA1, 0x00,             //     Collection (Physical)
        0x15, 0x81, 0x25, 0x7F,
        0x75, 0x08, 0x95, 0x02,
        0x09, 0x30, 0x09, 0x31,
        0x81, 0x06,             //       Input (x, y — relative)
        0xC0,                   //     End Collection (Physical)
        0x09, 0x38,             //     Usage (Wheel)
        0x95, 0x01, 0x81, 0x06, //     Input (wheel)
        0x05, 0x0C,             //     Usage Page (Consumer)
        0x09, 0x01,             //     Usage (Consumer Control)
        0xA1, 0x01,             //     Collection (Application)
        0x15, 0x81, 0x25, 0x7F,
        0x0A, 0x38, 0x02,       //       Usage (AC Pan)
        0x95, 0x01, 0x81, 0x06, //       Input (horizontal wheel)
        0xC0,                   //     End Collection
        0xC0,                   //   End Collection (Logical)
        0xC0                    // End Collection (Application)
    };

    constexpr std::array<std::uint8_t, 71> kJoystickReportDescriptor =
    {
        0x05, 0x01,
        0x09, 0x04,
        0xA1, 0x01,
        0x05, 0x09,
        0x19, 0x01,
        0x29, 0x10,
        0x15, 0x00,
        0x25, 0x01,
        0x75, 0x01,
        0x95, 0x10,
        0x81, 0x02,
        0x05, 0x01,
        0x09, 0x30,
        0x09, 0x31,
        0x16, 0x00, 0x80,
        0x26, 0xFF, 0x7F,
        0x75, 0x10,
        0x95, 0x02,
        0x81, 0x02,
        0x09, 0x39,
        0x15, 0x00,
        0x25, 0x07,
        0x35, 0x00,
        0x46, 0x3B, 0x01,
        0x65, 0x14,
        0x75, 0x04,
        0x95, 0x01,
        0x81, 0x42,
        0x65, 0x00,
        0x75, 0x04,
        0x95, 0x01,
        0x81, 0x01,
        0xC0
    };

    inline constexpr VirtualDeviceDefinition kKeyboardDefinition
    {
        VirtualDeviceType::Keyboard,
        kLogitechVendorId,
        kVirtualKeyboardProductId,
        kVirtualKeyboardHardwareId,
        kKeyboardReportDescriptor
    };

    inline constexpr VirtualDeviceDefinition kMouseDefinition
    {
        VirtualDeviceType::Mouse,
        kLogitechVendorId,
        kVirtualMouseProductId,
        kVirtualMouseHardwareId,
        kMouseReportDescriptor
    };

    inline constexpr VirtualDeviceDefinition kJoystickDefinition
    {
        VirtualDeviceType::Joystick,
        kLogitechVendorId,
        kVirtualJoystickProductId,
        kVirtualJoystickHardwareId,
        kJoystickReportDescriptor
    };

    [[nodiscard]] static constexpr const VirtualDeviceDefinition& GetVirtualDeviceDefinition(VirtualDeviceType deviceType)
    {
        switch (deviceType)
        {
        case VirtualDeviceType::Keyboard:
            return kKeyboardDefinition;

        case VirtualDeviceType::Mouse:
            return kMouseDefinition;

        case VirtualDeviceType::Joystick:
            return kJoystickDefinition;
        }

        throw std::invalid_argument("Invalid VirtualDeviceType");
    }


    static bool SendBufferedIoctl(HANDLE deviceHandle, DWORD ioctlCode, void* inputBuffer, DWORD inputSize, void* outputBuffer, DWORD outputSize, DWORD* bytesReturned)
    {
        if (deviceHandle == INVALID_HANDLE_VALUE)
            return false;

        DWORD localBytesReturned = 0;
        BOOL result = DeviceIoControl(deviceHandle, ioctlCode, inputBuffer, inputSize, outputBuffer, outputSize, &localBytesReturned, nullptr);

        if (!result)
        {
            return false;
        }

        if (bytesReturned != nullptr)
            *bytesReturned = localBytesReturned;

        return true;
    }


    static bool PlugVirtualDevice(HANDLE deviceHandle, VirtualDeviceType deviceType, std::uint32_t* deviceIdOut)
    {
        if (deviceHandle == INVALID_HANDLE_VALUE)
            return false;

        const auto& deviceDef = GetVirtualDeviceDefinition(deviceType);
        const std::size_t hardwareIdTextBytes = deviceDef.hardwareId.size() * sizeof(wchar_t);
        const std::size_t hardwareIdSize = hardwareIdTextBytes + 2 * sizeof(wchar_t);
        const std::size_t totalSize = sizeof(PlugRequest) + deviceDef.reportDescriptor.size();

        // 驱动分发函数要求总长度至少为 0xBB。
        if (totalSize < 0xBB)
            return false;

        if (totalSize > std::numeric_limits<DWORD>::max())
            return false;

        std::vector<std::uint8_t> buffer(totalSize, 0);
        PlugRequest* request = reinterpret_cast<PlugRequest*>(buffer.data());
        request->magic = 0xB7;
        request->deviceIdOut = 0;
        request->initializedFlag = 0;
        request->hardwareIdSize = static_cast<std::uint32_t>(hardwareIdSize);
        std::memcpy(request->hardwareId, deviceDef.hardwareId.data(), hardwareIdTextBytes);
        request->productId = deviceDef.productId;
        request->vendorId = deviceDef.vendorId;

        request->deviceType = static_cast<std::uint32_t>(deviceDef.deviceType);
        request->version = 0x0100;

        // 用户态必须保持为零。
        request->object0 = 0;
        request->object1 = 0;
        request->object2 = 0;

        request->reportDescriptorSize = static_cast<std::uint32_t>(deviceDef.reportDescriptor.size());

        std::memcpy(buffer.data() + sizeof(PlugRequest), deviceDef.reportDescriptor.data(), deviceDef.reportDescriptor.size());

        DWORD bytesReturned = 0;

        if (!SendBufferedIoctl(deviceHandle, IOCTL_LGHUB_PLUG_DEVICE, buffer.data(), static_cast<DWORD>(buffer.size()), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesReturned)) {
            return false;
        }

        if (deviceIdOut != nullptr)
            *deviceIdOut = request->deviceIdOut;

        //std::wcout << L"Plug 成功：deviceType=" << request->deviceType << L"，driver deviceId=" << request->deviceIdOut << L"，bytesReturned=" << bytesReturned << std::endl;
        return true;
    }

    static bool UnplugVirtualDevice(HANDLE deviceHandle, VirtualDeviceType deviceType, std::uint32_t* deviceIdOut)
    {
        if (deviceHandle == INVALID_HANDLE_VALUE)
            return false;

        UnplugRequest request{};
        request.size = static_cast<std::uint32_t>(sizeof(request));
        request.deviceIdOut = 0;
        request.deviceType = static_cast<std::uint32_t>(deviceType);

        DWORD bytesReturned = 0;
        const bool success = SendBufferedIoctl(deviceHandle, IOCTL_LGHUB_UNPLUG_DEVICE, &request, static_cast<DWORD>(sizeof(request)), &request, static_cast<DWORD>(sizeof(request)), &bytesReturned);

        if (!success)
            return false;

        if (deviceIdOut != nullptr)
            *deviceIdOut = request.deviceIdOut;

        //std::wcout << L"Unplug 成功：deviceType=" << request.deviceType << L"，driver deviceId=" << request.deviceIdOut << L"，bytesReturned=" << bytesReturned << std::endl;
        return true;
    }

    static HANDLE OpenDeviceHandle() {
        const std::vector<std::wstring> paths = VInput::Win32::GetDeviceInterfacePaths(GUID_LGHUB_XLCORE_INTERFACE);
        if (paths.empty())
            return INVALID_HANDLE_VALUE;

        for (const std::wstring& path : paths) {
            HANDLE handle = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (handle != INVALID_HANDLE_VALUE)
                return handle;
        }

        return INVALID_HANDLE_VALUE;
    }

    static std::filesystem::path ReleaseBuildinDriver() {
        constexpr std::pair<int, const wchar_t*> driverFiles[] = {
            { IDR_LOGI_JOY_BUS_ENUM_CAT, L"logi_joy_bus_enum.cat" },
            { IDR_LOGI_JOY_BUS_ENUM_INF, L"logi_joy_bus_enum.inf" },
            { IDR_LOGI_JOY_BUS_ENUM_SYS, L"logi_joy_bus_enum.sys" },

            { IDR_LOGI_JOY_VIR_HID_CAT,  L"logi_joy_vir_hid.cat"  },
            { IDR_LOGI_JOY_VIR_HID_INF,  L"logi_joy_vir_hid.inf"  },
            { IDR_LOGI_JOY_VIR_HID_SYS,  L"logi_joy_vir_hid.sys"  },

            { IDR_LOGI_JOY_XLCORE_SYS,   L"logi_joy_xlcore.sys"   },
        };

        auto tempPath = std::filesystem::temp_directory_path();
        if (std::filesystem::exists(tempPath)) {
            auto mod = VInput::Win32::GetSelfModule();
            tempPath = tempPath / L"VInput" / L"LG_DRIVER";
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
        return VInput::Win32::IsDeviceExists(mouseHardwareId, mouseHardwareId.substr(0, 21));
    }

    static int InternalUninstall(const std::wstring& busInf, const std::wstring& hidInf)
    {
        VInput::Win32::Win32Result r;
        if (r.success) {
            HANDLE deviceHandle = OpenDeviceHandle();
            if (deviceHandle != INVALID_HANDLE_VALUE) {
                r.success = UnplugVirtualDevice(deviceHandle, VirtualDeviceType::Keyboard, nullptr);
                r.success = UnplugVirtualDevice(deviceHandle, VirtualDeviceType::Mouse, nullptr);
                CloseHandle(deviceHandle);

                VInput::Stopwatch::Stopwatch stop;
                stop.Start();
                do {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    if (stop.ElapsedMilliseconds() >= 1000)
                        break;
                } while (IsDriverReady());
                stop.Stop();
                //std::wcout << std::format(L"IsDriverReady 耗时: {:.3f} ms\n", stop.ElapsedMilliseconds());
            }
        }

        MergeResult(VInput::Win32::RemoveRootDevice(kVirtualBusHardwareId), r);
        if (r.error || r.needReboot)
            return r.error;

        if (!hidInf.empty())
            MergeResult(VInput::Inf::UninstallInfPackage(hidInf), r);
        if (!busInf.empty())
            MergeResult(VInput::Inf::UninstallInfPackage(busInf), r);

        if (!r.error && !r.needReboot) {
            MergeResult(VInput::Win32::DeleteServiceEntry(L"logi_joy_vir_hid"), r);
            MergeResult(VInput::Win32::DeleteServiceEntry(L"logi_joy_bus_enum"), r);
            MergeResult(VInput::Win32::DeleteServiceEntry(L"logi_joy_xlcore"), r);
            VInput::Win32::DeleteSysFile(L"logi_joy_vir_hid.sys");
            MergeResult(VInput::Win32::DeleteRegKey(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM", L"VID_046D&PID_C2AB"), r);
            MergeResult(VInput::Win32::DeleteRegKey(HKEY_CURRENT_USER, L"System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\DirectInput", L"VID_046D&PID_C232"), r);
            MergeResult(VInput::Win32::DeleteRegKey(HKEY_CURRENT_USER, L"System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\DirectInput", L"VID_046D&PID_C231"), r);
            MergeResult(VInput::Win32::DeleteRegKey(HKEY_CURRENT_USER, L"System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\DirectInput", L"VID_046D&PID_C2AB"), r);
        }

        std::wcout << L"Uninstall Result: " << (r.success ? L"SUCCESS" : L"FAILED")
            << L" , need reboot: " << (r.needReboot ? L"YES" : L"NO")
            << L" , error code: " << r.error
            << L" , error line: " << r.errorLine
            << L"\n";
        return r.error;
    }

    VInput::Win32::DriverErrorStatus LgDriver::GetDriverStatus() {
        auto infos = VInput::Inf::GetDriverPackageInfos([](const auto& info) {
            return VInput::String::ContainsIgnoreCase(info.version.provider, VInput::Lg::vender);
            });
        return VInput::Inf::GetVenderDriverStatus(infos, {
            { L"logi_joy_bus_enum.inf", &busInfPath_ },
            { L"logi_joy_vir_hid.inf",  &hidInfPath_ },
            }, VInput::Lg::vender);
    }

    int LgDriver::Uninstall()
    {
        auto status = GetDriverStatus();
        if (status == VInput::Win32::DriverErrorStatus::DRIVER_NOT_FOUND)
            return 0;

        auto error = InternalUninstall(busInfPath_, hidInfPath_);
        if (!error) {
            auto tempPath = std::filesystem::temp_directory_path() / L"VInput" / L"LG_DRIVER";
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

    int LgDriver::Install(const char* utf8DriverDirectory)
    {
        if (IsDriverReady())
            return 0;

        auto status = GetDriverStatus();
        if (status == VInput::Win32::DriverErrorStatus::DRIVER_FOUND) {
            auto uninstallStatus = InternalUninstall(busInfPath_, hidInfPath_);
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
            busInfPath_ = driverDirectory / L"logi_joy_bus_enum.inf";
            hidInfPath_ = driverDirectory / L"logi_joy_vir_hid.inf";
            if (r.success)
                MergeResult(VInput::Inf::InstallInfPackage(hidInfPath_), r);
        }
        if (r.success)
            MergeResult(VInput::Win32::CreateRootDevice(busInfPath_, kVirtualBusHardwareId.substr(5), kVirtualBusHardwareId), r);

        if (!r.success)
            InternalUninstall(busInfPath_, hidInfPath_);

        if (r.success) {
            HANDLE deviceHandle = OpenDeviceHandle();
            if (deviceHandle == INVALID_HANDLE_VALUE)
                VInput::Win32::MakeResult(r, false, 1, false);
            if (r.success)
                r.success = PlugVirtualDevice(deviceHandle, VirtualDeviceType::Keyboard, nullptr);
            if (r.success)
                r.success = PlugVirtualDevice(deviceHandle, VirtualDeviceType::Mouse, nullptr);
            VInput::Stopwatch::Stopwatch stop;
            stop.Start();
            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                if (stop.ElapsedMilliseconds() >= 1000)
                    break;
            } while (!IsDriverReady());
            stop.Stop();
            std::wcout << std::format(L"IsDriverReady 耗时: {:.3f} ms\n", stop.ElapsedMilliseconds());
            if (deviceHandle != INVALID_HANDLE_VALUE && deviceHandle != nullptr)
                CloseHandle(deviceHandle);
        }

        std::wcout << L"Install Result: " << (r.success ? L"SUCCESS" : L"FAILED")
            << L" , need reboot: " << (r.needReboot ? L"YES" : L"NO")
            << L" , error code: " << r.error
            << L" , error line: " << r.errorLine
            << L"\n";

        return r.error;
    }
}