#include "pch.h"
#include "Api.h"
#include "Common.h"
#include "LgDevice.h"
#include "LgDriver.h"
#include "RzDevice.h"
#include "RzDriver.h"

class Manager {
private:
    std::unique_ptr<VInput::IDriver> driver_ = nullptr;
    std::unique_ptr<VInput::AbsDevice> device_ = nullptr;

public:
    Manager() {}
    ~Manager() {
        if (device_ != nullptr)
            device_ = nullptr;

        if (driver_ != nullptr)
            driver_ = nullptr;
    }

    BOOL InitializeLogitech(UTF8_STRING driverPath) noexcept {
        try {
            if (driver_ == nullptr) {
                device_ = nullptr;
                driver_ = std::make_unique<VInput::Lg::LgDriver>();
                if (driver_->Install(driverPath) == 0) {
                    device_ = std::make_unique<VInput::Lg::LgDevice>();
                    return device_->Initialize();
                }
                driver_ = nullptr;
            }
        }
        catch (...) {}
        return false;
    }

    BOOL InitializeRazer(UTF8_STRING driverPath) noexcept {
        try {
            if (driver_ == nullptr) {
                device_ = nullptr;
                driver_ = std::make_unique<VInput::Rz::RzDriver>();
                if (driver_->Install(driverPath) == 0) {
                    device_ = std::make_unique<VInput::Rz::RzDevice>();
                    return device_->Initialize();
                }
                driver_ = nullptr;
            }
        }
        catch (...) {}
        return false;
    }

    VInput::IDriver* Driver() const {
        return driver_.get();
    }

    VInput::AbsDevice* Device() const {
        return device_.get();
    }

    void Shutdown() noexcept {
        try {
            if (device_ != nullptr) {
                try {
                    device_->Shutdown();
                }
                catch (...) {}
                device_ = nullptr;
            }

            if (driver_ != nullptr) {
                try {
                    driver_->Uninstall();
                }
                catch (...) {}
                driver_ = nullptr;
            }
        }
        catch (...) {}
    }
};

static Manager manager{};


DLLAPI BOOL STDCALL Initialize(UTF8_STRING driverPath)
{
    if (manager.InitializeLogitech(driverPath))
        return true;

    else if (manager.InitializeRazer(driverPath))
        return true;

	return false;
}

DLLAPI BOOL STDCALL InitializeRazer(UTF8_STRING driverPath)
{
    return manager.InitializeRazer(driverPath);
}

DLLAPI BOOL STDCALL InitializeLogitech(UTF8_STRING driverPath)
{
    return manager.InitializeLogitech(driverPath);
}

DLLAPI BOOL STDCALL MouseMove(INT32 dx, INT32 dy)
{
    try {
        return manager.Device()->MouseMove(dx, dy);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL MouseMoveTo(INT32 x, INT32 y)
{
    try {
        return manager.Device()->MouseMoveTo(x, y);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL MousePress(UINT8 vk_button)
{
    try {
        return manager.Device()->MousePress(vk_button);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL MouseRelease(UINT8 vk_button)
{
    try {
        return manager.Device()->MouseRelease(vk_button);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL MouseClick(UINT8 vk_button)
{
    try {
        return manager.Device()->MouseClick(vk_button);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL MouseMoveToClick(INT32 x, INT32 y, UINT8 vk_button)
{
    try {
        return manager.Device()->MouseMoveToClick(x, y, vk_button);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL MouseWheel(INT8 delta)
{
    try {
        return manager.Device()->MouseWheel(delta);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL MouseHorizontalWheel(INT8 horizontalDelta)
{
    try {
        return manager.Device()->MouseHorizontalWheel(horizontalDelta);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL MouseReleaseAll()
{
    try {
        return manager.Device()->MouseReleaseAll();
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL KeyboardPress(UINT8 vk_key)
{
    try {
        return manager.Device()->KeyboardPress(vk_key);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL KeyboardRelease(UINT8 vk_key)
{
    try {
        return manager.Device()->KeyboardRelease(vk_key);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL KeyboardClick(UINT8 vk_key)
{
    try {
        return manager.Device()->KeyboardClick(vk_key);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL KeyboardPressHID(UINT8 page07_usage)
{
    try {
        return manager.Device()->KeyboardPressHID(page07_usage);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL KeyboardReleaseHID(UINT8 page07_usage)
{
    try {
        return manager.Device()->KeyboardReleaseHID(page07_usage);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL KeyboardClickHID(UINT8 page07_usage)
{
    try {
        return manager.Device()->KeyboardClickHID(page07_usage);
    }
    catch (...) {}
    return false;
}

DLLAPI BOOL STDCALL KeyboardReleaseAll()
{
    try {
        return manager.Device()->KeyboardReleaseAll();
    }
    catch (...) {}
    return false;
}

DLLAPI void STDCALL Shutdown(void)
{
    manager.Shutdown();
}