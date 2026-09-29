# =============================================================================
# VInput.py - Python wrapper for VInput.dll
#
# Version    : 2.0.0.0
# Source     : Api.h
# Generated  : 2026-09-30 01:52:05
# Generator  : gen_vinput_py.ps1
# Platform   : Windows x64 only
#
# This file is auto-generated. Do not edit manually.
# Any changes will be overwritten on the next build.
# =============================================================================

import ctypes
from ctypes import c_bool, c_int, c_byte, c_char_p, c_void_p
import os

__version__ = "2.0.0.0"

# =============================================================================
# Virtual Key constants
# =============================================================================

# Mouse buttons
VK_LBUTTON          = 0x01
VK_RBUTTON          = 0x02
VK_MBUTTON          = 0x04    # NOT contiguous with L & RBUTTON
VK_XBUTTON1         = 0x05    # NOT contiguous with L & RBUTTON
VK_XBUTTON2         = 0x06    # NOT contiguous with L & RBUTTON

# Keyboard keys
VK_BACK             = 0x08
VK_TAB              = 0x09
VK_RETURN           = 0x0D
VK_ESCAPE           = 0x1B
VK_SPACE            = 0x20
VK_PRIOR            = 0x21
VK_NEXT             = 0x22
VK_END              = 0x23
VK_HOME             = 0x24
VK_LEFT             = 0x25
VK_UP               = 0x26
VK_RIGHT            = 0x27
VK_DOWN             = 0x28
VK_SELECT           = 0x29
VK_PRINT            = 0x2A
VK_EXECUTE          = 0x2B
VK_SNAPSHOT         = 0x2C
VK_INSERT           = 0x2D
VK_DELETE           = 0x2E
VK_HELP             = 0x2F    # VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
VK_0                = 0x30
VK_1                = 0x31
VK_2                = 0x32
VK_3                = 0x33
VK_4                = 0x34
VK_5                = 0x35
VK_6                = 0x36
VK_7                = 0x37
VK_8                = 0x38
VK_9                = 0x39    # 0x3A - 0x40 : unassigned, VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
VK_A                = 0x41
VK_B                = 0x42
VK_C                = 0x43
VK_D                = 0x44
VK_E                = 0x45
VK_F                = 0x46
VK_G                = 0x47
VK_H                = 0x48
VK_I                = 0x49
VK_J                = 0x4A
VK_K                = 0x4B
VK_L                = 0x4C
VK_M                = 0x4D
VK_N                = 0x4E
VK_O                = 0x4F
VK_P                = 0x50
VK_Q                = 0x51
VK_R                = 0x52
VK_S                = 0x53
VK_T                = 0x54
VK_U                = 0x55
VK_V                = 0x56
VK_W                = 0x57
VK_X                = 0x58
VK_Y                = 0x59
VK_Z                = 0x5A
VK_LWIN             = 0x5B
VK_RWIN             = 0x5C
VK_APPS             = 0x5D
VK_SLEEP            = 0x5F
VK_NUMPAD0          = 0x60
VK_NUMPAD1          = 0x61
VK_NUMPAD2          = 0x62
VK_NUMPAD3          = 0x63
VK_NUMPAD4          = 0x64
VK_NUMPAD5          = 0x65
VK_NUMPAD6          = 0x66
VK_NUMPAD7          = 0x67
VK_NUMPAD8          = 0x68
VK_NUMPAD9          = 0x69
VK_MULTIPLY         = 0x6A
VK_ADD              = 0x6B
VK_SEPARATOR        = 0x6C
VK_SUBTRACT         = 0x6D
VK_DECIMAL          = 0x6E
VK_DIVIDE           = 0x6F
VK_F1               = 0x70
VK_F2               = 0x71
VK_F3               = 0x72
VK_F4               = 0x73
VK_F5               = 0x74
VK_F6               = 0x75
VK_F7               = 0x76
VK_F8               = 0x77
VK_F9               = 0x78
VK_F10              = 0x79
VK_F11              = 0x7A
VK_F12              = 0x7B
VK_F13              = 0x7C
VK_F14              = 0x7D
VK_F15              = 0x7E
VK_F16              = 0x7F
VK_F17              = 0x80
VK_F18              = 0x81
VK_F19              = 0x82
VK_F20              = 0x83
VK_F21              = 0x84
VK_F22              = 0x85
VK_F23              = 0x86
VK_F24              = 0x87
VK_LSHIFT           = 0xA0
VK_RSHIFT           = 0xA1
VK_LCONTROL         = 0xA2
VK_RCONTROL         = 0xA3
VK_LMENU            = 0xA4
VK_RMENU            = 0xA5
VK_OEM_1            = 0xBA    # ';:' for US
VK_OEM_PLUS         = 0xBB    # '+' any country
VK_OEM_COMMA        = 0xBC    # ',' any country
VK_OEM_MINUS        = 0xBD    # '-' any country
VK_OEM_PERIOD       = 0xBE    # '.' any country
VK_OEM_2            = 0xBF    # '/?' for US
VK_OEM_3            = 0xC0    # '`~' for US
VK_OEM_4            = 0xDB    # '[{' for US
VK_OEM_5            = 0xDC    # '\|' for US
VK_OEM_6            = 0xDD    # ']}' for US
VK_OEM_7            = 0xDE    # ''"' for US
VK_OEM_8            = 0xDF

# =============================================================================
# DLL loading
# =============================================================================
_dll = None
_funcs_declared = False

def _load_dll(dll_path=None):
    """Load VInput.dll from script directory or given path."""
    global _dll
    if _dll is not None:
        return _dll

    if dll_path is None:
        base_dir = os.path.dirname(os.path.abspath(__file__))
        dll_path = os.path.join(base_dir, "VInput.dll")

    if not os.path.exists(dll_path):
        raise OSError(f"DLL not found: {dll_path}")

    _dll = ctypes.WinDLL(dll_path)
    return _dll

# =============================================================================
# Function signatures (argtypes / restype)
# =============================================================================
def _declare_funcs(dll):
    """Set argtypes and restype for all exported functions."""
    global _funcs_declared
    if _funcs_declared:
        return

    dll.Initialize.argtypes = [c_char_p]
    dll.Initialize.restype = c_bool
    dll.InitializeRazer.argtypes = [c_char_p]
    dll.InitializeRazer.restype = c_bool
    dll.InitializeLogitech.argtypes = [c_char_p]
    dll.InitializeLogitech.restype = c_bool
    dll.InitializeUvhid.argtypes = [c_char_p]
    dll.InitializeUvhid.restype = c_bool
    dll.MouseMove.argtypes = [c_int, c_int]
    dll.MouseMove.restype = c_bool
    dll.MouseMoveTo.argtypes = [c_int, c_int]
    dll.MouseMoveTo.restype = c_bool
    dll.MousePress.argtypes = [c_byte]
    dll.MousePress.restype = c_bool
    dll.MouseRelease.argtypes = [c_byte]
    dll.MouseRelease.restype = c_bool
    dll.MouseClick.argtypes = [c_byte]
    dll.MouseClick.restype = c_bool
    dll.MouseMoveToClick.argtypes = [c_int, c_int, c_byte]
    dll.MouseMoveToClick.restype = c_bool
    dll.MouseWheel.argtypes = [c_byte]
    dll.MouseWheel.restype = c_bool
    dll.MouseHorizontalWheel.argtypes = [c_byte]
    dll.MouseHorizontalWheel.restype = c_bool
    dll.MouseReleaseAll.argtypes = []
    dll.MouseReleaseAll.restype = c_bool
    dll.KeyboardPress.argtypes = [c_byte]
    dll.KeyboardPress.restype = c_bool
    dll.KeyboardRelease.argtypes = [c_byte]
    dll.KeyboardRelease.restype = c_bool
    dll.KeyboardClick.argtypes = [c_byte]
    dll.KeyboardClick.restype = c_bool
    dll.KeyboardPressHID.argtypes = [c_byte]
    dll.KeyboardPressHID.restype = c_bool
    dll.KeyboardReleaseHID.argtypes = [c_byte]
    dll.KeyboardReleaseHID.restype = c_bool
    dll.KeyboardClickHID.argtypes = [c_byte]
    dll.KeyboardClickHID.restype = c_bool
    dll.KeyboardReleaseAll.argtypes = []
    dll.KeyboardReleaseAll.restype = c_bool
    dll.Shutdown.argtypes = []
    dll.Shutdown.restype = None
    _funcs_declared = True

# =============================================================================
# Public API
# =============================================================================

# DLLAPI BOOL STDCALL Initialize(UTF8_STRING driverPath);
def initialize(driverPath):
    """
    install and initizlize the available driver and device environment. It should be called every time the DLL is loaded.

    Args:
        driverPath: nullptr(0 or None): use the built-in driver in the DLL.
    """
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.Initialize(driverPath.encode('utf-8') if isinstance(driverPath, str) else driverPath)

# DLLAPI BOOL STDCALL InitializeRazer(UTF8_STRING driverPath);
def initialize_razer(driverPath):
    """
    install and initizlize the Razer driver and device environment. It should be called every time the DLL is loaded.

    Args:
        driverPath: nullptr(0 or None): use the built-in driver in the DLL.
    """
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.InitializeRazer(driverPath.encode('utf-8') if isinstance(driverPath, str) else driverPath)

# DLLAPI BOOL STDCALL InitializeLogitech(UTF8_STRING driverPath);
def initialize_logitech(driverPath):
    """
    install and initizlize the Logitech driver and device environment. It should be called every time the DLL is loaded.

    Args:
        driverPath: nullptr(0 or None): use the built-in driver in the DLL.
    """
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.InitializeLogitech(driverPath.encode('utf-8') if isinstance(driverPath, str) else driverPath)

# DLLAPI BOOL STDCALL InitializeUvhid(UTF8_STRING driverPath);
def initialize_uvhid(driverPath):
    """
    install and initizlize the uvhid driver and device environment. It should be called every time the DLL is loaded.

    Args:
        driverPath: nullptr(0 or None): use the built-in driver in the DLL.
    """
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.InitializeUvhid(driverPath.encode('utf-8') if isinstance(driverPath, str) else driverPath)

# DLLAPI BOOL STDCALL MouseMove(INT32 dx, INT32 dy);
def mouse_move(dx, dy):
    """MouseMove"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseMove(c_int(dx), c_int(dy))

# DLLAPI BOOL STDCALL MouseMoveTo(INT32 x, INT32 y);
def mouse_move_to(x, y):
    """MouseMoveTo"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseMoveTo(c_int(x), c_int(y))

# DLLAPI BOOL STDCALL MousePress(UINT8 vk_button);
def mouse_press(vk_button):
    """MousePress"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MousePress(c_byte(vk_button))

# DLLAPI BOOL STDCALL MouseRelease(UINT8 vk_button);
def mouse_release(vk_button):
    """MouseRelease"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseRelease(c_byte(vk_button))

# DLLAPI BOOL STDCALL MouseClick(UINT8 vk_button);
def mouse_click(vk_button):
    """MouseClick"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseClick(c_byte(vk_button))

# DLLAPI BOOL STDCALL MouseMoveToClick(INT32 x, INT32 y, UINT8 vk_button);
def mouse_move_to_click(x, y, vk_button):
    """MouseMoveToClick"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseMoveToClick(c_int(x), c_int(y), c_byte(vk_button))

# DLLAPI BOOL STDCALL MouseWheel(INT8 delta);
def mouse_wheel(delta):
    """

    Args:
        delta: delta > 0: scroll up, delta < 0: scroll down
    """
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseWheel(c_byte(delta))

# DLLAPI BOOL STDCALL MouseHorizontalWheel(INT8 horizontalDelta);
def mouse_horizontal_wheel(horizontalDelta):
    """

    Args:
        horizontalDelta: horizontalDelta > 0: scroll right, horizontalDelta < 0: scroll left
    """
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseHorizontalWheel(c_byte(horizontalDelta))

# DLLAPI BOOL STDCALL MouseReleaseAll(void);
def mouse_release_all():
    """MouseReleaseAll"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseReleaseAll()

# DLLAPI BOOL STDCALL KeyboardPress(UINT8 vk_key);
def keyboard_press(vk_key):
    """KeyboardPress"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardPress(c_byte(vk_key))

# DLLAPI BOOL STDCALL KeyboardRelease(UINT8 vk_key);
def keyboard_release(vk_key):
    """KeyboardRelease"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardRelease(c_byte(vk_key))

# DLLAPI BOOL STDCALL KeyboardClick(UINT8 vk_key);
def keyboard_click(vk_key):
    """KeyboardClick"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardClick(c_byte(vk_key))

# DLLAPI BOOL STDCALL KeyboardPressHID(UINT8 page07_usage);
def keyboard_press_hid(page07_usage):
    """KeyboardPressHID"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardPressHID(c_byte(page07_usage))

# DLLAPI BOOL STDCALL KeyboardReleaseHID(UINT8 page07_usage);
def keyboard_release_hid(page07_usage):
    """KeyboardReleaseHID"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardReleaseHID(c_byte(page07_usage))

# DLLAPI BOOL STDCALL KeyboardClickHID(UINT8 page07_usage);
def keyboard_click_hid(page07_usage):
    """KeyboardClickHID"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardClickHID(c_byte(page07_usage))

# DLLAPI BOOL STDCALL KeyboardReleaseAll(void);
def keyboard_release_all():
    """KeyboardReleaseAll"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardReleaseAll()

# DLLAPI void STDCALL Shutdown(void);
def shutdown():
    """
    Shutdown the driver and device environment. Will uninstall the device and driver.
    Call only when you need to uninstall the driver, otherwise it is recommended not to call to speed up DLL initialization time.
    Even if you don't call it, there won't be a resource leak at the end of the program
    """
    dll = _load_dll()
    _declare_funcs(dll)
    dll.Shutdown()
