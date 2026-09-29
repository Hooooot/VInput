// VInputTests.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#define DLLAPI

#include <iostream>
#include <windows.h>
#include <devpkey.h>
#include <initguid.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <stdio.h>

#include "Api.h"
#include "LgDriver.h"
#include "LgDevice.h"
#include "UvDriver.h"
#include "UvDevice.h"
#include "RzDriver.h"
#include "RzDevice.h"

#include "StopwatchHelper.h"
#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

int wmain()
{
    VInput::Win32::EnableUtf16Output();
    VInput::Stopwatch::Stopwatch stopwatch;

    //VInput::Lg::LgDriver driver;
    //VInput::Lg::LgDevice device;

    VInput::Rz::RzDriver driver;
    VInput::Rz::RzDevice device;

    //VInput::Uv::UvDriver driver;
    //VInput::Uv::UvDevice device;

    stopwatch.Start();
    driver.Install(nullptr);
    stopwatch.Stop();
    std::wcout << std::format(L"Install: {:.3f}ms\n", stopwatch.ElapsedMilliseconds());

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    device.Initialize();
    device.KeyboardClick(VK_NUMPAD1);
    device.MouseMoveTo(100, 100);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    device.Shutdown();

    stopwatch.Restart();
    driver.Uninstall();
    stopwatch.Stop();
    std::wcout << std::format(L"Uninstall: {:.3f}ms\n", stopwatch.ElapsedMilliseconds());

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
