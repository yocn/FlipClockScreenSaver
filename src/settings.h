#pragma once
#include <cstdint>

namespace flip {
struct Options { bool seconds=true, twelve=false, date=false, drift=true, weekday=false; };
inline Options decodeOptions(std::uint32_t flags) {
    return {!(flags&1),bool(flags&2),bool(flags&4),!(flags&8),bool(flags&16)};
}
inline std::uint32_t encodeOptions(const Options& options) {
    return (options.seconds?0u:1u) | (options.twelve?2u:0u)
        | (options.date?4u:0u) | (options.drift?0u:8u) | (options.weekday?16u:0u);
}
}
