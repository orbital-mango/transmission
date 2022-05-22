// This file Copyright © 2019-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#include <algorithm>
#include <iterator>
#include <string>
#include <string_view>

#include <event2/util.h> // evutil_ascii_strncasecmp

#include <fmt/core.h>
#include <fmt/format.h>

#include "file-info.h"
#include "utils.h"

using namespace std::literals;

static void appendSanitizedComponent(std::string_view in, tr_pathbuf& out)
{
    // remove leading and trailing spaces
    in = tr_strvStrip(in);

    // remove trailing periods
    while (tr_strvEndsWith(in, '.'))
    {
        in.remove_suffix(1);
    }

    // if `in` begins with any of these prefixes, insert a leading character
    // to ensure that it _doesn't_ start with that prefix
    // https://docs.microsoft.com/en-us/windows/desktop/FileIO/naming-a-file
    static auto constexpr ReservedNames = std::array<std::string_view, 44>{
        "AUX"sv,   "AUX."sv,  "COM1"sv,  "COM1."sv, "COM2"sv,  "COM2."sv, "COM3"sv,  "COM3."sv, "COM4"sv,  "COM4."sv, "COM5"sv,
        "COM5."sv, "COM6"sv,  "COM6."sv, "COM7"sv,  "COM7."sv, "COM8"sv,  "COM8."sv, "COM9"sv,  "COM9."sv, "CON"sv,   "CON."sv,
        "LPT1"sv,  "LPT1."sv, "LPT2"sv,  "LPT2."sv, "LPT3"sv,  "LPT3."sv, "LPT4"sv,  "LPT4."sv, "LPT5"sv,  "LPT5."sv, "LPT6"sv,
        "LPT6."sv, "LPT7"sv,  "LPT7."sv, "LPT8"sv,  "LPT8."sv, "LPT9"sv,  "LPT9."sv, "NUL"sv,   "NUL."sv,  "PRN"sv,   "PRN."sv,
    };
    auto in_lower = tr_pathbuf{};
    std::transform(std::begin(in), std::end(in), std::back_inserter(in_lower), [](auto ch) { return tolower(ch); });
    auto const in_lower_sv = std::string_view{ std::data(in_lower), std::size(in_lower) };
    if (std::any_of(
            std::begin(ReservedNames),
            std::end(ReservedNames),
            [in_lower_sv](auto const& prefix) { return tr_strvStartsWith(in_lower_sv, prefix); }))
    {
        out.append("_"sv);
    }

    // munge banned characters
    // https://docs.microsoft.com/en-us/windows/desktop/FileIO/naming-a-file
    std::transform(
        std::begin(in),
        std::end(in),
        std::back_inserter(out),
        [](auto ch) { return !tr_strvContains("<>:\"/\\|?*"sv, ch) ? ch : '_'; });
}

void tr_file_info::sanitizePath(std::string_view in, tr_pathbuf& append_me)
{
    auto segment = std::string_view{};
    while (tr_strvSep(&in, &segment, '/'))
    {
        appendSanitizedComponent(segment, append_me);
        append_me.append("/"sv);
    }

    if (auto const n = std::size(append_me); n > 0)
    {
        append_me.resize(n - 1); // remove trailing slash
    }
}

std::string tr_file_info::sanitizePath(std::string_view in)
{
    auto buf = tr_pathbuf{};
    sanitizePath(in, buf);
    return std::string{ buf.sv() };
}
