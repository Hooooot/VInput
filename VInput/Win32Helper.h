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
#include <windows.h>
#include <memory>
#include <utility>

namespace VInput::Win32 {
	enum DriverErrorStatus : int
	{
		DRIVER_INSTALLED = 0,
		DRIVER_FOUND,
		DRIVER_NOT_FOUND,
		DRIVER_INCOMPATIBLE,
	};

	template <typename T, auto Deleter>
	class SafePtr
	{
	public:
		SafePtr() noexcept = default;
		explicit SafePtr(T ptr) noexcept : m_ptr(ptr) {}
		~SafePtr() noexcept { reset(); }

		SafePtr(const SafePtr&) = delete;
		SafePtr& operator=(const SafePtr&) = delete;

		SafePtr(SafePtr&& o) noexcept
			: m_ptr(std::exchange(o.m_ptr, nullptr)) {
		}

		SafePtr& operator=(SafePtr&& o) noexcept
		{
			if (this != &o)
				reset(std::exchange(o.m_ptr, nullptr));
			return *this;
		}

		[[nodiscard]] T get() const noexcept { return m_ptr; }

		[[nodiscard]] explicit operator bool() const noexcept
		{
			return m_ptr != nullptr && m_ptr != (T)INVALID_HANDLE_VALUE;
		}

		[[nodiscard]] T release() noexcept
		{
			return std::exchange(m_ptr, nullptr);
		}

		void reset(T ptr = nullptr) noexcept
		{
			if (ptr != m_ptr)
			{
				if (m_ptr != nullptr && m_ptr != (T)INVALID_HANDLE_VALUE)
					Deleter(m_ptr);
				m_ptr = ptr;
			}
		}

	private:
		T m_ptr = nullptr;
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
		if (!success && (result.errorLine == 0)) {
			result.errorLine = loc.line();
			result.error = error;
		}
	}

	static void MergeResult(const Win32Result& input, Win32Result & final) {
		final.success = final.success && input.success;
		final.needReboot = final.needReboot || input.needReboot;
		if (!input.success && (final.errorLine == 0)) {
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
	std::wstring GetDeviceInterfacePath(const GUID& guid, const std::wstring_view filterPrefix);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="targetHwId"></param>
	/// <param name="filterInstanceId">可以是ROOT\\DEVICE，也可以是HID的HID\\VID_046D&PID_C231，取决于设备类型</param>
	/// <returns></returns>
	bool IsDeviceExists(const std::wstring_view targetHwId, const std::wstring_view filterInstanceId = {});

	/// <summary>
	/// 创建设备并绑定到inf指定的驱动
	/// </summary>
	/// <param name="inf"></param>
	/// <param name="rootDeviceInstanceId">不得包含ROOT\\，例如原本路径为ROOT\\DEVICE_ID，则应当输入DEVICE_ID</param>
	/// <param name="rootDeviceHardwareId">inf绑定的硬件ID</param>
	/// <returns></returns>
	VInput::Win32::Win32Result CreateRootDevice(const std::wstring_view inf, const std::wstring_view rootDeviceInstanceId, const std::wstring_view rootDeviceHardwareId);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="rootDeviceInstanceId">需要带ROOT\\，不要带\\000这种序号</param>
	/// <returns></returns>
	VInput::Win32::Win32Result RemoveRootDevice(const std::wstring_view rootDeviceInstanceId);
	bool ScanHardware();
	VInput::Win32::Win32Result DeleteServiceEntry(const wchar_t* name);
	VInput::Win32::Win32Result DeleteRegKey(HKEY root, std::wstring_view parentKeyPath, std::wstring_view deletedKeyName, REGSAM view = KEY_WOW64_64KEY);
	VInput::Win32::Win32Result DeleteRegKey(HKEY root, const std::wstring& subkey, REGSAM view = KEY_WOW64_64KEY);
	VInput::Win32::Win32Result AddRegKeyDWORD(HKEY root, std::wstring_view parentKeyPath, std::wstring_view dwordName, UINT32 dwordData, REGSAM view = KEY_WOW64_64KEY);

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