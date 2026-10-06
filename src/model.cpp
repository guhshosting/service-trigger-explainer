#include "ste.hpp"
#include <cstring>
#include <limits>
namespace ste {
std::string utf8(const std::wstring &value) {
    if (value.empty())
        return {};
    if (value.size() > static_cast<size_t>((std::numeric_limits<int>::max)()))
        throw std::runtime_error("String too large");
    const int n = static_cast<int>(value.size());
    const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), n, nullptr,
                                           0, nullptr, nullptr);
    if (!length)
        throw std::runtime_error("Invalid UTF-16");
    std::string out(static_cast<size_t>(length), '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), n, out.data(), length,
                             nullptr, nullptr))
        throw std::runtime_error("UTF-8 conversion failed");
    return out;
}
void BufferView::require(const void *p, size_t count, size_t width) const {
    if (count == 0)
        return;
    const auto b = reinterpret_cast<uintptr_t>(base_), a = reinterpret_cast<uintptr_t>(p);
    if (!base_ || !p || !width || a < b || a - b > size_ || count > (size_ - (a - b)) / width)
        throw ApiError("Malformed SCM buffer", ERROR_INVALID_DATA);
}
std::wstring BufferView::string(const wchar_t *p) const {
    require(p, 1, sizeof(wchar_t));
    size_t offset = reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(base_);
    std::wstring out;
    while (offset <= size_ - sizeof(wchar_t)) {
        wchar_t c{};
        memcpy(&c, base_ + offset, sizeof(c));
        if (!c)
            return out;
        out.push_back(c);
        offset += sizeof(c);
    }
    throw ApiError("Unterminated SCM string", ERROR_INVALID_DATA);
}
std::string start_name(DWORD v) {
    switch (v) {
    case SERVICE_BOOT_START:
        return "Boot";
    case SERVICE_SYSTEM_START:
        return "System";
    case SERVICE_AUTO_START:
        return "Automatic";
    case SERVICE_DEMAND_START:
        return "Manual (demand)";
    case SERVICE_DISABLED:
        return "Disabled";
    default:
        return "Unknown";
    }
}
std::string state_name(DWORD v) {
    switch (v) {
    case SERVICE_STOPPED:
        return "Stopped";
    case SERVICE_START_PENDING:
        return "Start pending";
    case SERVICE_STOP_PENDING:
        return "Stop pending";
    case SERVICE_RUNNING:
        return "Running";
    case SERVICE_CONTINUE_PENDING:
        return "Continue pending";
    case SERVICE_PAUSE_PENDING:
        return "Pause pending";
    case SERVICE_PAUSED:
        return "Paused";
    default:
        return "Unavailable/unknown";
    }
}
} // namespace ste
