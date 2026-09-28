#pragma once
#include "pch.h"
#include <string>
#include "Win32Helper.h"
#include "Common.h"
#include <mutex>

namespace VInput::Uv {
	constexpr std::wstring_view vender = L"Unified Intents AB";


	enum class VirtualDeviceType : std::uint32_t
	{
		Keyboard = 0,
		Mouse = 1,
		Joystick = 2 // No matching device type
	};

	class UvDriver : public VInput::IDriver {
	private:
		std::wstring infPath_;

	public:
		UvDriver() {}

		// 通过 IDriver 继承
		VInput::Win32::DriverErrorStatus GetDriverStatus() override;
		int Uninstall() override;
		int Install(const char* utf8DriverDirectory) override;
	};
}