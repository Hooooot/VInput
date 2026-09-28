// VInputTests.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <windows.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <stdio.h>

#include "LgDriver.h"
#include "UvDriver.h"
#include "RzDriver.h"
#include "StopwatchHelper.h"
#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

#define UVHID_VID       0x9512
#define UVHID_PID       0x9512
#define UVHID_BUF_SIZE  65 


HANDLE OpenUvhidDevice() {
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);
    HDEVINFO devInfo = SetupDiGetClassDevs(
        &hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    SP_DEVICE_INTERFACE_DATA iface = { sizeof(iface) };
    for (DWORD i = 0;
        SetupDiEnumDeviceInterfaces(devInfo, NULL, &hidGuid, i, &iface);
        i++)
    {
        DWORD needed = 0;
        SetupDiGetDeviceInterfaceDetail(devInfo, &iface, NULL, 0, &needed, NULL);
        auto* det = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(needed);
        if (!det) continue;
        det->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        if (!SetupDiGetDeviceInterfaceDetail(devInfo, &iface, det, needed, NULL, NULL)) {
            free(det); continue;
        }
        HANDLE h = CreateFile(det->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL);
        free(det);
        if (h == INVALID_HANDLE_VALUE) continue;

        HIDD_ATTRIBUTES a = { sizeof(a) };
        if (HidD_GetAttributes(h, &a)
            && a.VendorID == UVHID_VID
            && a.ProductID == UVHID_PID) {
            printf("Opened uvhid \n");
            SetupDiDestroyDeviceInfoList(devInfo);
            return h;
        }
        CloseHandle(h);
    }
    SetupDiDestroyDeviceInfoList(devInfo);
    return INVALID_HANDLE_VALUE;
}


int main()
{
    VInput::Win32::EnableUtf16Output();
    VInput::Stopwatch::Stopwatch stopwatch;
    
    
    
    //VInput::Lg::LgDriver driver;
    //stopwatch.Start();
    //driver.Install(nullptr);
    //stopwatch.Stop();
    //std::wcout << std::format(L"Install: {:.3f}ms\n", stopwatch.ElapsedMilliseconds());

    //stopwatch.Restart();
    //driver.Uninstall();
    //stopwatch.Stop();
    //std::wcout << std::format(L"Uninstall: {:.3f}ms\n", stopwatch.ElapsedMilliseconds());

    VInput::Rz::RzDriver driver;
    stopwatch.Start();
    driver.Install(nullptr);
    stopwatch.Stop();
    std::wcout << std::format(L"Install: {:.3f}ms\n", stopwatch.ElapsedMilliseconds());

    stopwatch.Restart();
    driver.Uninstall();
    stopwatch.Stop();
    std::wcout << std::format(L"Uninstall: {:.3f}ms\n", stopwatch.ElapsedMilliseconds());


    //VInput::Uv::UvDriver driver;
    //stopwatch.Start();
    //driver.Install(nullptr);
    //stopwatch.Stop();
    //std::wcout << std::format(L"Install: {:.3f}ms\n", stopwatch.ElapsedMilliseconds());

    //stopwatch.Restart();
    //driver.Uninstall();
    //stopwatch.Stop();
    //std::wcout << std::format(L"Uninstall: {:.3f}ms\n", stopwatch.ElapsedMilliseconds());



    //HANDLE hDev = OpenUvhidDevice();
    //CloseHandle(hDev);

    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
