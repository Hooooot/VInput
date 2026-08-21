#pragma once
#include "pch.h"
#include <string>
#include "Win32Helper.h"
#include "Common.h"

namespace VInput::Rz {
	constexpr std::wstring_view vender = L"Razer";


	class RzDriver : public VInput::IDriver {
	private:
		std::wstring mouInfPath_;
		std::wstring kbdInfPath_;
		std::wstring vmouInfPath_;
		std::wstring vkbdInfPath_;
		std::wstring vconInfPath_;
		std::wstring commInfPath_;
		std::wstring vbusInfPath_;

	public:
		RzDriver() { }

		// 通过 IDriver 继承
		VInput::Win32::DriverErrorStatus GetDriverStatus() override;
		int Uninstall() override;
		int Install(const char* utf8DriverDirectory) override;
	};
}