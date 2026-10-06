#include "acquire.hpp"
#include "ste.hpp"
#include <memory>
namespace ste {
namespace {
struct Close {
    void operator()(SC_HANDLE h) const noexcept {
        if (h)
            CloseServiceHandle(h);
    }
};
using Handle = std::unique_ptr<std::remove_pointer_t<SC_HANDLE>, Close>;
} // namespace
Result inspect(const std::wstring &name) {
    Handle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!manager)
        throw ApiError("OpenSCManagerW", GetLastError());
    Handle service(OpenServiceW(manager.get(), name.c_str(), SERVICE_QUERY_CONFIG));
    if (!service)
        throw ApiError("OpenServiceW", GetLastError());
    const auto config = acquire(
        [&](BYTE *b, DWORD n, DWORD *needed) {
            return QueryServiceConfigW(service.get(), reinterpret_cast<QUERY_SERVICE_CONFIGW *>(b),
                                       n, needed);
        },
        "QueryServiceConfigW");
    BufferView view(config.data(), config.size());
    const auto c = view.read(reinterpret_cast<const QUERY_SERVICE_CONFIGW *>(config.data()));
    Result r;
    r.name = utf8(name);
    r.display = utf8(view.string(c.lpDisplayName));
    r.account = utf8(view.string(c.lpServiceStartName));
    r.start = c.dwStartType;
    r.service_type = c.dwServiceType;
    const auto triggers = acquire(
        [&](BYTE *b, DWORD n, DWORD *needed) {
            return QueryServiceConfig2W(service.get(), SERVICE_CONFIG_TRIGGER_INFO, b, n, needed);
        },
        "QueryServiceConfig2W(trigger info)");
    r.triggers = decode_triggers(triggers.data(), triggers.size());
    SERVICE_DELAYED_AUTO_START_INFO delayed{};
    DWORD needed = 0;
    if (QueryServiceConfig2W(service.get(), SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
                             reinterpret_cast<BYTE *>(&delayed), sizeof(delayed), &needed)) {
        r.delayed = delayed.fDelayedAutostart != FALSE;
        r.delayed_known = true;
    } else
        r.warnings.push_back(ApiError("Delayed start query unavailable", GetLastError()).what());
    Handle status_service(OpenServiceW(manager.get(), name.c_str(), SERVICE_QUERY_STATUS));
    if (status_service) {
        SERVICE_STATUS_PROCESS status{};
        if (QueryServiceStatusEx(status_service.get(), SC_STATUS_PROCESS_INFO,
                                 reinterpret_cast<BYTE *>(&status), sizeof(status), &needed))
            r.state = status.dwCurrentState;
        else
            r.warnings.push_back(
                ApiError("Current state query unavailable", GetLastError()).what());
    } else
        r.warnings.push_back(ApiError("Current state access unavailable", GetLastError()).what());
    r.warnings.push_back(
        "Configuration and status are separate snapshots; SCM does not expose trigger history or "
        "whether a condition currently matches. Trigger execution is not guaranteed (for example "
        "under low memory); running dependents can prevent a trigger stop.");
    if (r.start == SERVICE_DISABLED && !r.triggers.empty())
        r.warnings.push_back("The service is disabled; configured Start triggers do not override "
                             "disabled configuration.");
    return r;
}
} // namespace ste
