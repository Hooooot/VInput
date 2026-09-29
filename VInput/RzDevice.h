#pragma once
#include "Common.h"
#include <ntddkbd.h>
#include <ntddmou.h>
#include <mutex>

namespace VInput::Rz {

	constexpr GUID GUID_RZ_CONTROL_INTERFACE = { 0xE3BE005D, 0xD130, 0x4910, {0x88, 0xFF, 0x09, 0xAE, 0x02, 0xF6, 0x80, 0xE9} };
	constexpr std::wstring_view RZ_CONTROL_INTERFACE_PATH = L"\\\\?\\RZCONTROL#VID_1532&PID_0306";

	enum class RzInputType : std::uint32_t {
		Keyboard = 1,
		Mouse = 2
	};

	struct RzInputPacket {
		std::uint32_t target;
		RzInputType type;

		union {
			KEYBOARD_INPUT_DATA keyboard;
			MOUSE_INPUT_DATA mouse;
		};
	};

	struct KeyboardCode {
		USHORT makeCode;
		USHORT prefixFlags;
	};

	class RzDevice : public VInput::AbsDevice {
	public:
		RzDevice() {}
		~RzDevice() {
			if (deviceHandle_ != INVALID_HANDLE_VALUE)
			{
				MouseReleaseAll();
				KeyboardReleaseAll();
				CloseHandle(deviceHandle_);
			}
		}

		// 通过 AbsDevice 继承
		bool Initialize() override;
		void Shutdown() noexcept override;
		bool SendMouseReport(const uint8_t buttons, const int8_t dx, const int8_t dy, const int8_t wheel, const int8_t horizontalWheel) override;
		bool SendKeyboardReport(const uint8_t modifiers, const uint8_t keys[6]) override;

	private:
		bool SendPacketUnlocked(RzInputPacket& packet);
		bool SendKeyboardEventUnlocked(KeyboardCode& code, const bool pressed);
	};
}