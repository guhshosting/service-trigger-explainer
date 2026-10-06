#include "ste.hpp"
#include <sstream>
namespace ste {
std::string human(const Result &r) {
    std::ostringstream o;
    o << "Service: " << quote(r.name) << "\nDisplay name: " << quote(r.display)
      << "\nStart type: " << start_name(r.start) << " (" << r.start << ")"
      << (r.delayed ? "; delayed auto-start" : "")
      << (!r.delayed_known ? "; delayed auto-start unavailable" : "")
      << "\nAccount: " << quote(r.account) << "\nCurrent state: " << state_name(r.state)
      << "\nTriggers: " << r.triggers.size() << "\n";
    if (r.triggers.empty())
        o << "No service triggers are configured.\n";
    for (size_t i = 0; i < r.triggers.size(); ++i) {
        const auto &t = r.triggers[i];
        o << "\nTrigger " << i + 1 << "\n  Action: " << t.action_name << " (" << t.action
          << ")\n  Type: " << t.type_name << " (" << t.type
          << ")\n  Subtype GUID: " << (t.guid.empty() ? "Absent" : t.guid)
          << "\n  Subtype name: " << (t.subtype_name.empty() ? "Unknown" : t.subtype_name)
          << "\n  Meaning: " << t.explanation << '\n';
        for (size_t j = 0; j < t.data.size(); ++j) {
            const auto &d = t.data[j];
            o << "  Data " << j + 1 << ": " << d.name << " (" << d.type << "), " << d.hex.size() / 2
              << " bytes\n    Raw hex: " << d.hex << '\n';
            for (const auto &s : d.strings)
                o << "    String: " << quote(s) << '\n';
            if (!d.number.empty())
                o << "    Unsigned value: " << d.number << '\n';
            for (const auto &w : d.warnings)
                o << "    Warning: " << w << '\n';
        }
        for (const auto &w : t.warnings)
            o << "  Warning: " << w << '\n';
    }
    for (const auto &w : r.warnings)
        o << "\nNote: " << w << '\n';
    return o.str();
}
} // namespace ste
