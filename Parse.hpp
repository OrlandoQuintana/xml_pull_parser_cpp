// Parse.hpp

#pragma once

#include <charconv>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace xmlparse {

inline std::string_view trim_view(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }

    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }

    return s;
}

inline bool iequals_ascii(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto ca = static_cast<unsigned char>(a[i]);
        const auto cb = static_cast<unsigned char>(b[i]);

        if (std::tolower(ca) != std::tolower(cb)) {
            return false;
        }
    }

    return true;
}

template <typename T>
std::optional<T> parse_integer(std::string_view s) {
    s = trim_view(s);

    T value{};
    const char* begin = s.data();
    const char* end = s.data() + s.size();

    auto [ptr, ec] = std::from_chars(begin, end, value);

    if (ec == std::errc{} && ptr == end) {
        return value;
    }

    return std::nullopt;
}

inline std::optional<int> parse_int(std::string_view s) {
    return parse_integer<int>(s);
}

inline std::optional<std::int32_t> parse_i32(std::string_view s) {
    return parse_integer<std::int32_t>(s);
}

inline std::optional<std::int64_t> parse_i64(std::string_view s) {
    return parse_integer<std::int64_t>(s);
}

inline std::optional<std::uint32_t> parse_u32(std::string_view s) {
    return parse_integer<std::uint32_t>(s);
}

inline std::optional<std::uint64_t> parse_u64(std::string_view s) {
    return parse_integer<std::uint64_t>(s);
}

inline std::optional<unsigned long long> parse_ull(std::string_view s) {
    return parse_integer<unsigned long long>(s);
}

inline std::optional<double> parse_double(std::string_view s) {
    s = trim_view(s);

#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
    double value{};
    const char* begin = s.data();
    const char* end = s.data() + s.size();

    auto [ptr, ec] = std::from_chars(begin, end, value);

    if (ec == std::errc{} && ptr == end) {
        return value;
    }

    return std::nullopt;
#else
    try {
        return std::stod(std::string{s});
    } catch (...) {
        return std::nullopt;
    }
#endif
}

inline std::optional<float> parse_float(std::string_view s) {
    if (auto value = parse_double(s)) {
        return static_cast<float>(*value);
    }

    return std::nullopt;
}

inline std::optional<bool> parse_bool(std::string_view s) {
    s = trim_view(s);

    if (s == "1" || iequals_ascii(s, "true")) {
        return true;
    }

    if (s == "0" || iequals_ascii(s, "false")) {
        return false;
    }

    return std::nullopt;
}

inline std::string parse_string(std::string_view s) {
    return std::string{trim_view(s)};
}

} // namespace xmlparse