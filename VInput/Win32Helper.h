#pragma once
#include "pch.h"
#include <filesystem>
#include <vector>
#include <io.h>
#include <fcntl.h>
#include <source_location>
#include <SetupAPI.h>
#include <functional>
#include <iostream>

namespace VInput::Win32 {
	enum DriverErrorStatus : int
	{
		DRIVER_INSTALLED = 0,
		DRIVER_FOUND,
		DRIVER_NOT_FOUND,
		DRIVER_INCOMPATIBLE,
	};

	class DevInfoSet {
	public:
		explicit DevInfoSet(HDEVINFO h = INVALID_HANDLE_VALUE) : h_(h) {}
		~DevInfoSet() {
			if (h_ != INVALID_HANDLE_VALUE)
				SetupDiDestroyDeviceInfoList(h_);
		}
		DevInfoSet(const DevInfoSet&) = delete;
		DevInfoSet& operator=(const DevInfoSet&) = delete;
		HDEVINFO get() const { return h_; }
		bool valid() const { return h_ != INVALID_HANDLE_VALUE; }
	private:
		HDEVINFO h_;
	};

	struct Win32Result {
		bool success = true;
		bool needReboot = false;
		DWORD error = ERROR_SUCCESS;
		uint32_t errorLine = 0;
	};

	static void MakeResult(Win32Result& result, bool success, DWORD error, bool needReboot = false, const std::source_location& loc = std::source_location::current()) {
		result.success = result.success && success;
		result.needReboot = result.needReboot || needReboot;
		if (!success && (result.errorLine == UINT32_MAX)) {
			result.errorLine = loc.line();
			result.error = error;
		}
	}

	static void MergeResult(const Win32Result& input, Win32Result & final) {
		final.success = final.success && input.success;
		final.needReboot = final.needReboot || input.needReboot;
		if (!input.success && (final.errorLine == UINT32_MAX)) {
			final.errorLine = input.errorLine;
			final.error = input.error;
		}
	}

	static void EnableUtf16Output() {
		if (_setmode(_fileno(stdout), _O_U16TEXT) == -1)
			std::wcerr << L"Failed to set stdout mode" << std::endl;
		if (_setmode(_fileno(stderr), _O_U16TEXT) == -1)
			std::wcerr << L"Failed to set stderr mode" << std::endl;
	}

	bool IsAdministrator();
	bool IsFileExists(const std::wstring& path);
	std::filesystem::path GetAbsolutePath(const std::wstring& path);
	std::filesystem::path GetWindowsInfDirectory();
	std::filesystem::path GetSystem32Directory();
	VInput::Win32::Win32Result DeleteSysFile(const std::wstring& fileName);
	int Exec(const std::wstring& command, std::wstring* exec_stdout);
	HMODULE GetSelfModule();
	bool WriteResourceToFile(HMODULE mod, int resId, const std::filesystem::path& path);
	std::vector<std::wstring> GetDeviceInterfacePaths(const GUID& guid);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="targetHwId"></param>
	/// <param name="filterInstanceId">可以是ROOT\\DEVICE，也可以是HID的HID\\VID_046D&PID_C231，取决于设备类型</param>
	/// <returns></returns>
	bool IsDeviceExists(const std::wstring_view targetHwId, const std::wstring_view filterInstanceId = {});
	VInput::Win32::Win32Result CreateRootDevice(const std::wstring_view inf, const std::wstring_view rootDeviceId, const std::wstring_view hardwareId);
	VInput::Win32::Win32Result RemoveRootDevice(const std::wstring_view rootDeviceId);
	bool ScanHardware();
	VInput::Win32::Win32Result DeleteServiceEntry(const wchar_t* name);
	VInput::Win32::Win32Result DeleteRegKey(HKEY root, std::wstring_view parentKeyPath, std::wstring_view deletedKeyName, REGSAM view = KEY_WOW64_64KEY);
	VInput::Win32::Win32Result DeleteRegKey(HKEY root, const std::wstring& subkey, REGSAM view = KEY_WOW64_64KEY);
#pragma pack(push, 4)
	struct MouseAcceleration
	{
		INT xThreshold;
		INT yThreshold;
		INT Acceleration; // 控制面板中的“提高指针精确度”选项，默认为1， 0是关闭
	};

	struct MouseSpeedInfo
	{
		INT speed; // [1-20], 默认是10
		MouseAcceleration acceleration;
	};
#pragma pack(pop)

	MouseSpeedInfo SetSafeMouseSpeed();
	void RecoveryMouseSpeed(MouseSpeedInfo& speedInfo);
}