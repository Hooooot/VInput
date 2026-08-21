#pragma once

#include "Win32Helper.h"
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <vector>
#include <functional>

namespace VInput::Inf
{
    struct DriverPackageVersion
    {
        std::wstring provider;
        std::wstring className;
        std::wstring classGuid;
        std::wstring driverVersion;
        std::wstring catalogFile;
    };

    struct DriverPackageInfo
    {
        std::wstring publishedName;
        std::filesystem::path publishedInfPath;
        std::wstring originalName;
        std::filesystem::path driverStoreInfPath;
        VInput::Inf::DriverPackageVersion version;
    };

    struct RequiredInf {
        std::wstring_view originalName;
        std::wstring* outputPath;
    };

    std::vector<VInput::Inf::DriverPackageInfo> GetDriverPackageInfos(std::function<bool(const VInput::Inf::DriverPackageInfo&)> filter = [](const VInput::Inf::DriverPackageInfo&) { return true; });
    VInput::Win32::DriverErrorStatus GetVenderDriverStatus(std::vector<VInput::Inf::DriverPackageInfo>& infos, std::initializer_list<RequiredInf> requiredInfs, const std::wstring_view vender);
    VInput::Win32::Win32Result InstallInfPackage(const std::wstring& infPath);
    VInput::Win32::Win32Result UninstallInfPackage(const std::wstring& infPath);
    void PrintPackage(const VInput::Inf::DriverPackageInfo& package);
    void PrintPackages(const std::vector<VInput::Inf::DriverPackageInfo>& packages);
}