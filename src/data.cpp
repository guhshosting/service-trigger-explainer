#include "ste.hpp"
#include <cstring>
namespace ste {
Data decode_data(DWORD type, const std::vector<BYTE> &bytes) {
    Data d;
    d.type = type;
    constexpr char digits[] = "0123456789abcdef";
    for (BYTE b : bytes) {
        d.hex.push_back(digits[static_cast<unsigned>(b) >> 4U]);
        d.hex.push_back(digits[static_cast<unsigned>(b) & 15U]);
    }
    if (bytes.size() > 1024)
        d.warnings.push_back("Data exceeds the documented 1024-byte limit.");
    switch (type) {
    case SERVICE_TRIGGER_DATA_TYPE_BINARY:
        d.name = "Binary";
        break;
    case SERVICE_TRIGGER_DATA_TYPE_STRING: {
        d.name = "UTF-16 string/multi-string";
        if (bytes.empty() || bytes.size() % 2) {
            d.warnings.push_back("Invalid UTF-16 byte count.");
            break;
        }
        std::wstring part;
        bool terminated = false;
        bool end_seen = false;
        for (size_t i = 0; i < bytes.size(); i += 2) {
            const wchar_t c = static_cast<wchar_t>(static_cast<unsigned>(bytes[i]) |
                                                   (static_cast<unsigned>(bytes[i + 1]) << 8U));
            if (c == 0) {
                if (terminated)
                    end_seen = true;
                if (!part.empty()) {
                    try {
                        d.strings.push_back(utf8(part));
                    } catch (const std::runtime_error &) {
                        d.warnings.push_back("Invalid UTF-16 sequence; consult raw hex.");
                    }
                    part.clear();
                }
                terminated = true;
            } else {
                if (end_seen) {
                    d.warnings.push_back(
                        "Nonzero data follows a multi-string terminator; raw hex retained.");
                    end_seen = false;
                }
                part.push_back(c);
                terminated = false;
            }
        }
        if (d.strings.size() > 1 && !end_seen)
            d.warnings.push_back("Multi-string lacks a double null terminator.");
        if (!terminated) {
            d.warnings.push_back("String is not null-terminated; bounded contents retained.");
            try {
                d.strings.push_back(utf8(part));
            } catch (const std::runtime_error &) {
                d.warnings.push_back("Invalid UTF-16 sequence; consult raw hex.");
            }
        }
        break;
    }
    case SERVICE_TRIGGER_DATA_TYPE_LEVEL:
        d.name = "ETW level";
        if (bytes.size() == 1)
            d.number = std::to_string(bytes[0]);
        else
            d.warnings.push_back("Level requires exactly one byte.");
        break;
    case SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ANY:
    case SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ALL: {
        d.name = type == SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ANY ? "ETW keyword-any mask"
                                                               : "ETW keyword-all mask";
        if (bytes.size() != 8)
            d.warnings.push_back("Keyword mask requires exactly eight bytes.");
        else {
            uint64_t v = 0;
            for (size_t i = 0; i < 8; ++i)
                v |= static_cast<uint64_t>(bytes[i]) << (8 * i);
            d.number = std::to_string(v);
        }
        break;
    }
    default:
        d.name = "Unknown";
        d.warnings.push_back("Unknown data type; preserved as hex.");
        break;
    }
    return d;
}
} // namespace ste
