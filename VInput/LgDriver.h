#pragma once
#include "pch.h"
#include <string>
#include "Win32Helper.h"
#include "Common.h"
#include <mutex>

namespace VInput::Lg {
	constexpr std::wstring_view vender = L"Logitech";


	enum class VirtualDeviceType : std::uint32_t
	{
		Keyboard = 0,
		Mouse = 1,
		Joystick = 2 // No matching device type
	};

	class LgDriver : public VInput::IDriver {
	private:
		std::wstring busInfPath_;
		std::wstring hidInfPath_;

	public:
		LgDriver() {}

		// 通过 IDriver 继承
		VInput::Win32::DriverErrorStatus GetDriverStatus() override;
		int Uninstall() override;
		int Install(const char* utf8DriverDirectory) override;
	};
}