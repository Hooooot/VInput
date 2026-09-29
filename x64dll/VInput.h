#pragma once

/*
 * Windows Virtual Keys, Standard Set
 */

/*
Mouse event
*/
#define VK_LBUTTON        0x01
#define VK_RBUTTON        0x02
#define VK_MBUTTON        0x04    /* NOT contiguous with L & RBUTTON */
#define VK_XBUTTON1       0x05    /* NOT contiguous with L & RBUTTON */
#define VK_XBUTTON2       0x06    /* NOT contiguous with L & RBUTTON */

/*
Keyboard event
*/
#define VK_BACK           0x08
#define VK_TAB            0x09

#define VK_RETURN         0x0D

#define VK_ESCAPE         0x1B

#define VK_SPACE          0x20
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_SELECT         0x29
#define VK_PRINT          0x2A
#define VK_EXECUTE        0x2B
#define VK_SNAPSHOT       0x2C
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_HELP           0x2F

 /*
 VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
 */
#define VK_0              0x30
#define VK_1              0x31
#define VK_2              0x32
#define VK_3              0x33
#define VK_4              0x34
#define VK_5              0x35
#define VK_6              0x36
#define VK_7              0x37
#define VK_8              0x38
#define VK_9              0x39

/*
0x3A - 0x40 : unassigned, VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
*/
#define VK_A              0x41
#define VK_B              0x42
#define VK_C              0x43
#define VK_D              0x44
#define VK_E              0x45
#define VK_F              0x46
#define VK_G              0x47
#define VK_H              0x48
#define VK_I              0x49
#define VK_J              0x4A
#define VK_K              0x4B
#define VK_L              0x4C
#define VK_M              0x4D
#define VK_N              0x4E
#define VK_O              0x4F
#define VK_P              0x50
#define VK_Q              0x51
#define VK_R              0x52
#define VK_S              0x53
#define VK_T              0x54
#define VK_U              0x55
#define VK_V              0x56
#define VK_W              0x57
#define VK_X              0x58
#define VK_Y              0x59
#define VK_Z              0x5A

#define VK_LWIN           0x5B
#define VK_RWIN           0x5C
#define VK_APPS           0x5D

#define VK_SLEEP          0x5F

#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69
#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F
#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B
#define VK_F13            0x7C
#define VK_F14            0x7D
#define VK_F15            0x7E
#define VK_F16            0x7F
#define VK_F17            0x80
#define VK_F18            0x81
#define VK_F19            0x82
#define VK_F20            0x83
#define VK_F21            0x84
#define VK_F22            0x85
#define VK_F23            0x86
#define VK_F24            0x87

#define VK_LSHIFT         0xA0
#define VK_RSHIFT         0xA1
#define VK_LCONTROL       0xA2
#define VK_RCONTROL       0xA3
#define VK_LMENU          0xA4
#define VK_RMENU          0xA5

#define VK_OEM_1          0xBA   // ';:' for US
#define VK_OEM_PLUS       0xBB   // '+' any country
#define VK_OEM_COMMA      0xBC   // ',' any country
#define VK_OEM_MINUS      0xBD   // '-' any country
#define VK_OEM_PERIOD     0xBE   // '.' any country
#define VK_OEM_2          0xBF   // '/?' for US
#define VK_OEM_3          0xC0   // '`~' for US

#define VK_OEM_4          0xDB  //  '[{' for US
#define VK_OEM_5          0xDC  //  '\|' for US
#define VK_OEM_6          0xDD  //  ']}' for US
#define VK_OEM_7          0xDE  //  ''"' for US
#define VK_OEM_8          0xDF

#ifndef STDCALL
#define STDCALL             __stdcall
#endif

#ifndef DLLAPI
#define DLLAPI              __declspec(dllexport)
#endif

#ifndef FALSE
#define FALSE               0
#endif
#ifndef TRUE
#define TRUE                1
#endif
#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

typedef int                 BOOL;
typedef const char*		    UTF8_STRING;
typedef signed int		    INT32;
typedef signed char			INT8;
typedef unsigned char		UINT8;

#ifdef __cplusplus
extern "C" {
#endif

    /// <summary>
    /// install and initizlize the available driver and device environment. It should be called every time the DLL is loaded.
    /// </summary>
    /// <param name="driverPath">nullptr(0 or None): use the built-in driver in the DLL.</param>
    /// <returns></returns>
    DLLAPI BOOL STDCALL Initialize(UTF8_STRING driverPath);
    /// <summary>
    /// install and initizlize the Razer driver and device environment. It should be called every time the DLL is loaded.
    /// </summary>
    /// <param name="driverPath">nullptr(0 or None): use the built-in driver in the DLL.</param>
    /// <returns></returns>
    DLLAPI BOOL STDCALL InitializeRazer(UTF8_STRING driverPath);
    /// <summary>
    /// install and initizlize the Logitech driver and device environment. It should be called every time the DLL is loaded.
    /// </summary>
    /// <param name="driverPath">nullptr(0 or None): use the built-in driver in the DLL.</param>
    /// <returns></returns>
    DLLAPI BOOL STDCALL InitializeLogitech(UTF8_STRING driverPath);
    /// <summary>
    /// install and initizlize the uvhid driver and device environment. It should be called every time the DLL is loaded.
    /// </summary>
    /// <param name="driverPath">nullptr(0 or None): use the built-in driver in the DLL.</param>
    /// <returns></returns>
    DLLAPI BOOL STDCALL InitializeUvhid(UTF8_STRING driverPath);

    DLLAPI BOOL STDCALL MouseMove(INT32 dx, INT32 dy);
    DLLAPI BOOL STDCALL MouseMoveTo(INT32 x, INT32 y);
    DLLAPI BOOL STDCALL MousePress(UINT8 vk_button);
    DLLAPI BOOL STDCALL MouseRelease(UINT8 vk_button);
    DLLAPI BOOL STDCALL MouseClick(UINT8 vk_button);
    DLLAPI BOOL STDCALL MouseMoveToClick(INT32 x, INT32 y, UINT8 vk_button);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="delta">delta > 0: scroll up, delta < 0: scroll down</param>
	/// <returns></returns>
	DLLAPI BOOL STDCALL MouseWheel(INT8 delta);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="horizontalDelta">horizontalDelta > 0: scroll right, horizontalDelta < 0: scroll left</param>
	/// <returns></returns>
	DLLAPI BOOL STDCALL MouseHorizontalWheel(INT8 horizontalDelta);
    DLLAPI BOOL STDCALL MouseReleaseAll();

    DLLAPI BOOL STDCALL KeyboardPress(UINT8 vk_key);
    DLLAPI BOOL STDCALL KeyboardRelease(UINT8 vk_key);
    DLLAPI BOOL STDCALL KeyboardClick(UINT8 vk_key);
    DLLAPI BOOL STDCALL KeyboardPressHID(UINT8 page07_usage);
    DLLAPI BOOL STDCALL KeyboardReleaseHID(UINT8 page07_usage);
    DLLAPI BOOL STDCALL KeyboardClickHID(UINT8 page07_usage);
    DLLAPI BOOL STDCALL KeyboardReleaseAll();

    /// <summary>
    /// Shutdown the driver and device environment. Will uninstall the device and driver.
    /// Call only when you need to uninstall the driver, otherwise it is recommended not to call to speed up DLL initialization time.
    /// Even if you don't call it, there won't be a resource leak at the end of the program
    /// </summary>
    DLLAPI void STDCALL Shutdown(void);

#ifdef __cplusplus
}
#endif