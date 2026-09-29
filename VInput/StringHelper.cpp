#include "pch.h"
#include "StringHelper.h"
#include <format>
#include <filesystem>
#include <tchar.h>
#include <cwctype>

namespace VInput::String {
    std::wstring Trim(std::wstring_view s) {
        auto begin = std::find_if_not(s.begin(), s.end(), [](wchar_t ch) { return _istspace(ch) != 0; });
        if (begin == s.end()) {
            return {};
        }
        auto end = std::find_if_not(s.rbegin(), s.rend(), [](wchar_t ch) { return _istspace(ch) != 0; }).base();
        return std::wstring(begin, end);
    }

    std::wstring ToLower(std::wstring_view s) {
        std::wstring result(s);
        std::transform(s.begin(), s.end(), result.begin(), [](wchar_t c) {
            return static_cast<wchar_t>(std::_totlower(c));
            });
        return result;
    }

    std::wstring ToUpper(std::wstring_view s) {
        std::wstring result(s);
        std::transform(s.begin(), s.end(), result.begin(), [](wchar_t c) {
            return static_cast<wchar_t>(std::_totupper(c));
            });
        return result;
    }

    bool EqualsIgnoreCase(std::wstring_view left, std::wstring_view right) {
        if (left.size() != right.size())
            return false;

        if (left.empty())
            return right.empty();

        if (left.size() > static_cast<std::size_t>(INT_MAX) ||
            right.size() > static_cast<std::size_t>(INT_MAX))
            return false;

        const int compareResult = CompareStringOrdinal(
            left.data(),
            static_cast<int>(left.size()),
            right.data(),
            static_cast<int>(right.size()),
            TRUE);
        return compareResult == CSTR_EQUAL;
    }

    void ConvertUTF8ToUTF16(std::string_view utf8, std::wstring& utf16) {
        utf16.clear();
        if (utf8.empty())
            return;
        if (utf8.size() > static_cast<size_t>(INT_MAX))
            return;

        const int wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
        if (wideLen <= 0)
            return;

        utf16.resize(wideLen);
        const int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            utf8.data(), static_cast<int>(utf8.size()), utf16.data(), wideLen);
        if (written != wideLen)
            utf16.clear();
    }

    bool StartsWithIgnoreCase(std::wstring_view value, std::wstring_view prefix) {
        if (value.size() < prefix.size())
            return false;

        if (prefix.empty())
            return true;

        if (prefix.size() > static_cast<size_t>(INT_MAX))
            return false;

        const int result = CompareStringOrdinal(
            value.data(),
            static_cast<int>(prefix.size()),
            prefix.data(),
            static_cast<int>(prefix.size()),
            TRUE);
        return result == CSTR_EQUAL;
    }

    bool ContainsIgnoreCase(const std::vector<std::wstring>& paths, std::wstring_view path)
    {
        for (const std::wstring& existing : paths)
        {
            const int result = CompareStringOrdinal(existing.data(), static_cast<int>(existing.size()), path.data(), static_cast<int>(path.size()), TRUE);
            if (result == CSTR_EQUAL)
                return true;
        }
        return false;
    }

    bool ContainsIgnoreCase(const std::wstring_view str1, const std::wstring_view str2) {
        if (str2.empty())
            return true;
        auto cmp = [](wchar_t a, wchar_t b) { return std::towlower(a) == std::towlower(b); };
        auto result = std::ranges::search(str1, str2, cmp);
        return !result.empty();
    }

    //bool ContainsIgnoreCase(const std::wstring& str1, const std::wstring& str2) {
    //    if (str2.empty())
    //        return true;
    //    auto cmp = [](wchar_t a, wchar_t b) { return std::towlower(a) == std::towlower(b); };
    //    auto result = std::ranges::search(str1, str2, cmp);
    //    return !result.empty();
    //}
}