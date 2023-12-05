// utility.hh
// Copyright (C) 2023 by Shawn Yarbrough

#pragma once

#include <string>
#include <string_view>

namespace utility {

inline std::string_view strip(std::string_view text) {
    std::string_view::size_type i1, i2;
    for (i1 = 0; i1 < text.size() && std::isspace(text[i1]); ++i1);
    for (i2 = text.size(); i2 > i1 && std::isspace(text[i2-1]); --i2);
    return text.substr(i1, i2-i1);
}

inline std::string strip(const std::string& text) {
    std::string::size_type i1, i2;
    for (i1 = 0; i1 < text.size() && std::isspace(text[i1]); ++i1);
    for (i2 = text.size(); i2 > i1 && std::isspace(text[i2-1]); --i2);
    return text.substr(i1, i2-i1);
}

}    // namespace utility
