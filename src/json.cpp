#include "ste.hpp"
#include <sstream>
namespace ste {
std::string quote(const std::string &s) {
    std::string out = "\"";
    constexpr char digits[] = "0123456789abcdef";
    for (unsigned char c : s) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (c < 32) {
                out += "\\u00";
                out += digits[static_cast<unsigned>(c) >> 4U];
                out += digits[static_cast<unsigned>(c) & 15U];
            } else
                out += static_cast<char>(c);
        }
    }
    return out + '"';
}
namespace {
std::string array(const std::vector<std::string> &values) {
    std::string s = "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i)
            s += ',';
        s += quote(values[i]);
    }
    return s + "]";
}
std::string optional(const std::string &s) {
    return s.empty() ? "null" : quote(s);
}
} // namespace
std::string json(const Result &r) {
    std::ostringstream o;
    o << "{\"schema_version\":1,\"application_version\":" << quote(version)
      << ",\"service\":{\"name\":" << quote(r.name) << ",\"display_name\":" << quote(r.display)
      << ",\"account\":" << quote(r.account) << ",\"service_type\":" << r.service_type
      << ",\"start_type\":" << r.start << ",\"start_type_name\":" << quote(start_name(r.start))
      << ",\"delayed_auto_start\":" << (r.delayed_known ? (r.delayed ? "true" : "false") : "null")
      << ",\"current_state\":" << (r.state ? std::to_string(r.state) : "null")
      << ",\"current_state_name\":" << quote(state_name(r.state))
      << "},\"trigger_count\":" << r.triggers.size() << ",\"triggers\":[";
    for (size_t i = 0; i < r.triggers.size(); ++i) {
        const auto &t = r.triggers[i];
        if (i)
            o << ',';
        o << "{\"index\":" << i + 1 << ",\"action\":" << t.action
          << ",\"action_name\":" << quote(t.action_name) << ",\"type\":" << t.type
          << ",\"type_name\":" << quote(t.type_name) << ",\"subtype_guid\":" << optional(t.guid)
          << ",\"subtype_name\":" << optional(t.subtype_name)
          << ",\"explanation\":" << quote(t.explanation) << ",\"data_items\":[";
        for (size_t j = 0; j < t.data.size(); ++j) {
            const auto &d = t.data[j];
            if (j)
                o << ',';
            o << "{\"type\":" << d.type << ",\"type_name\":" << quote(d.name)
              << ",\"byte_count\":" << d.hex.size() / 2 << ",\"raw_hex\":" << quote(d.hex)
              << ",\"strings\":" << array(d.strings) << ",\"unsigned_value\":" << optional(d.number)
              << ",\"warnings\":" << array(d.warnings) << '}';
        }
        o << "],\"warnings\":" << array(t.warnings) << '}';
    }
    o << "],\"warnings\":" << array(r.warnings) << "}\n";
    return o.str();
}
std::string json_error(int code, DWORD win32, const std::string &message) {
    return "{\"schema_version\":1,\"application_version\":" + quote(version) +
           ",\"error\":{\"exit_code\":" + std::to_string(code) +
           ",\"win32_code\":" + (win32 ? std::to_string(win32) : "null") +
           ",\"message\":" + quote(message) + "}}\n";
}
} // namespace ste
