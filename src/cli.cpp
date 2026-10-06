#include "ste.hpp"
namespace ste {
Options parse(const std::vector<std::wstring> &args) {
    Options o;
    for (const auto &a : args) {
        if (a == L"--json") {
            if (o.json)
                throw std::invalid_argument("Duplicate --json");
            o.json = true;
        } else if (a == L"--help") {
            if (o.help)
                throw std::invalid_argument("Duplicate --help");
            o.help = true;
        } else if (a == L"--version") {
            if (o.version)
                throw std::invalid_argument("Duplicate --version");
            o.version = true;
        } else if (a.empty() || a.front() == L'-' || !o.name.empty())
            throw std::invalid_argument("Expected one canonical service name");
        else
            o.name = a;
    }
    if (o.help || o.version) {
        if (args.size() != 1)
            throw std::invalid_argument("--help and --version must be used alone");
    } else if (o.name.empty() || o.name.size() > 256 ||
               o.name.find_first_of(L"/\\") != std::wstring::npos)
        throw std::invalid_argument(
            "Provide a canonical local service name (maximum 256 characters)");
    return o;
}
} // namespace ste
