#pragma once
#include "pch.h"
#include <format>
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <tchar.h>

namespace VInput::String {

    template<typename... Args>
    void MakeMsg(std::wstring* msg,
        std::basic_format_string<wchar_t, std::type_identity_t<Args>...> format,
        Args&&... args)
    {
        if (msg != nullptr)
        {
            *msg = std::format(format, std::forward<Args>(args)...);
        }
    }

    std::wstring ToLower(std::wstring_view s);
    std::wstring ToUpper(std::wstring s);
    std::wstring Trim(std::wstring s);
    bool EqualsIgnoreCase(std::wstring_view a, std::wstring_view b);
    void ConvertUTF8ToUTF16(std::string_view utf8, std::wstring& utf16);
    bool StartsWithIgnoreCase(std::wstring_view value, std::wstring_view prefix);
    bool ContainsIgnoreCase(const std::vector<std::wstring>& paths, std::wstring_view path);
    bool ContainsIgnoreCase(const std::wstring_view str1, const std::wstring_view str2);
    //bool ContainsIgnoreCase(const std::wstring& str1, const std::wstring& str2);
}

