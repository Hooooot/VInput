# VInput.py - Python 封装库 for VInput.dll
# x64 only

import ctypes
from ctypes import c_bool, c_int, c_byte, c_char_p, c_void_p
import os

# ---------- 常量定义（与Windows Virtual key定义一致）----------


# 鼠标按键
VK_LBUTTON   = 0x01
VK_RBUTTON   = 0x02
VK_MBUTTON   = 0x04
VK_XBUTTON1  = 0x05  # Back
VK_XBUTTON2  = 0x06  # Forward

# 键盘常用键
VK_BACK      = 0x08
VK_TAB       = 0x09
VK_RETURN    = 0x0D
VK_ESCAPE    = 0x1B
VK_SPACE     = 0x20
VK_PRIOR     = 0x21  # Page Up
VK_NEXT      = 0x22  # Page Down
VK_END       = 0x23
VK_HOME      = 0x24
VK_LEFT      = 0x25
VK_UP        = 0x26
VK_RIGHT     = 0x27
VK_DOWN      = 0x28
VK_INSERT    = 0x2D
VK_DELETE    = 0x2E
VK_0         = 0x30
VK_1         = 0x31
VK_2         = 0x32
VK_3         = 0x33
VK_4         = 0x34
VK_5         = 0x35
VK_6         = 0x36
VK_7         = 0x37
VK_8         = 0x38
VK_9         = 0x39

# 与ASCII一致
VK_A         = 0x41
VK_B         = 0x42
VK_C         = 0x43
VK_D         = 0x44
VK_E         = 0x45
VK_F         = 0x46
VK_G         = 0x47
VK_H         = 0x48
VK_I         = 0x49
VK_J         = 0x4A
VK_K         = 0x4B
VK_L         = 0x4C
VK_M         = 0x4D
VK_N         = 0x4E
VK_O         = 0x4F
VK_P         = 0x50
VK_Q         = 0x51
VK_R         = 0x52
VK_S         = 0x53
VK_T         = 0x54
VK_U         = 0x55
VK_V         = 0x56
VK_W         = 0x57
VK_X         = 0x58
VK_Y         = 0x59
VK_Z         = 0x5A

VK_LWIN      = 0x5B
VK_RWIN      = 0x5C
VK_APPS      = 0x5D
VK_NUMPAD0   = 0x60
VK_NUMPAD1   = 0x61
VK_NUMPAD2   = 0x62
VK_NUMPAD3   = 0x63
VK_NUMPAD4   = 0x64
VK_NUMPAD5   = 0x65
VK_NUMPAD6   = 0x66
VK_NUMPAD7   = 0x67
VK_NUMPAD8   = 0x68
VK_NUMPAD9   = 0x69

VK_F1        = 0x70
VK_F2        = 0x71
VK_F3        = 0x72
VK_F4        = 0x73
VK_F5        = 0x74
VK_F6        = 0x75
VK_F7        = 0x76
VK_F8        = 0x77
VK_F9        = 0x78
VK_F10       = 0x79
VK_F11       = 0x7A
VK_F12       = 0x7B
VK_F13       = 0x7C
VK_F14       = 0x7D
VK_F15       = 0x7E
VK_F16       = 0x7F
VK_F17       = 0x80
VK_F18       = 0x81
VK_F19       = 0x82
VK_F20       = 0x83
VK_F21       = 0x84
VK_F22       = 0x85
VK_F23       = 0x86
VK_F24       = 0x87

VK_LSHIFT    = 0xA0
VK_RSHIFT    = 0xA1
VK_LCONTROL  = 0xA2
VK_RCONTROL  = 0xA3
VK_LMENU     = 0xA4  # Left Alt
VK_RMENU     = 0xA5  # Right Alt


# ---------- 加载 DLL ----------
_dll = None

def _load_dll(dll_path=None):
    """
    加载 VInput.dll。
    :param dll_path: DLL 的完整路径。若为 None，则从系统 PATH 或当前目录加载。
    :return: 加载后的 DLL 对象。
    :raises OSError: 如果 DLL 无法加载。
    """
    global _dll
    if _dll is not None:
        return _dll

    base_dir = os.path.dirname(os.path.abspath(__file__))
    dll_path = os.path.join(base_dir, "VInput.dll")

    if not os.path.exists(dll_path):
        print(f"错误：DLL 文件不存在 -> {dll_path}")
    else:
        try:
            _dll = ctypes.WinDLL(dll_path)
        except OSError as e:
            raise OSError(f"无法加载 VInput.dll: {e}。请确认文件存在且路径正确，并以管理员身份运行。")
    return _dll

# ---------- 函数声明 ----------
def _declare_funcs(dll):
    """
    为所有导出函数设置参数类型和返回类型。
    必须在调用任何函数之前执行。
    """
    # 初始化
    dll.Initialize.argtypes = [c_char_p]
    dll.Initialize.restype = c_bool

    dll.InitializeRazer.argtypes = [c_char_p]
    dll.InitializeRazer.restype = c_bool

    dll.InitializeLogitech.argtypes = [c_char_p]
    dll.InitializeLogitech.restype = c_bool

    # 鼠标
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

    # 键盘
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

    # Shutdown
    dll.Shutdown.argtypes = []
    dll.Shutdown.restype = None

    return dll

# ---------- 公共 API ----------
def initialize(driver_path=None):
    """
    初始化驱动和设备环境。
    :param driver_path: 驱动文件路径（字符串或 None）。若 None 则使用内置驱动。
    :return: True 表示成功，False 表示失败。
    """
    dll = _load_dll()
    _declare_funcs(dll)
    # 转换路径为 bytes 或 None
    if driver_path is None:
        arg = None
    else:
        if isinstance(driver_path, str):
            arg = driver_path.encode('utf-8')
        else:
            arg = driver_path
    return dll.Initialize(arg)

def initialize_razer(driver_path=None):
    """初始化雷蛇驱动。参数同 initialize。"""
    dll = _load_dll()
    _declare_funcs(dll)
    if driver_path is None:
        arg = None
    else:
        if isinstance(driver_path, str):
            arg = driver_path.encode('utf-8')
        else:
            arg = driver_path
    return dll.InitializeRazer(arg)

def initialize_logitech(driver_path=None):
    """初始化罗技驱动。参数同 initialize。"""
    dll = _load_dll()
    _declare_funcs(dll)
    if driver_path is None:
        arg = None
    else:
        if isinstance(driver_path, str):
            arg = driver_path.encode('utf-8')
        else:
            arg = driver_path
    return dll.InitializeLogitech(arg)

def shutdown():
    """卸载驱动和设备环境（通常不需要显式调用）。"""
    dll = _load_dll()
    _declare_funcs(dll)
    dll.Shutdown()

# 鼠标操作
def mouse_move(dx, dy):
    """相对移动鼠标。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseMove(c_int(dx), c_int(dy))

def mouse_move_to(x, y):
    """移动到绝对坐标。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseMoveTo(c_int(x), c_int(y))

def mouse_press(button):
    """按下鼠标按键（不释放）。button 为 VK_* 常量。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MousePress(c_byte(button))

def mouse_release(button):
    """释放鼠标按键。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseRelease(c_byte(button))

def mouse_click(button):
    """单击鼠标按键（按下并释放）。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseClick(c_byte(button))

def mouse_move_to_click(x, y, button):
    """移动到指定坐标并单击。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseMoveToClick(c_int(x), c_int(y), c_byte(button))

def mouse_wheel(delta):
    """滚动垂直滚轮。delta>0 向上，<0 向下。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseWheel(c_byte(delta))

def mouse_horizontal_wheel(delta):
    """滚动水平滚轮。delta>0 向右，<0 向左。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseHorizontalWheel(c_byte(delta))

def mouse_release_all():
    """释放所有鼠标按键。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.MouseReleaseAll()

# 键盘操作
def keyboard_press(key):
    """按下键盘按键（不释放）。key 为 VK_* 常量。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardPress(c_byte(key))

def keyboard_release(key):
    """释放键盘按键。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardRelease(c_byte(key))

def keyboard_click(key):
    """单击键盘按键（按下并释放）。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardClick(c_byte(key))

def keyboard_press_hid(usage):
    """按下 HID 键盘用法（page 0x07 的 usage）。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardPressHID(c_byte(usage))

def keyboard_release_hid(usage):
    """释放 HID 键盘用法。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardReleaseHID(c_byte(usage))

def keyboard_click_hid(usage):
    """单击 HID 键盘用法。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardClickHID(c_byte(usage))

def keyboard_release_all():
    """释放所有键盘按键。"""
    dll = _load_dll()
    _declare_funcs(dll)
    return dll.KeyboardReleaseAll()

