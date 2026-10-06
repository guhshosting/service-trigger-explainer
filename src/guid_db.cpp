#include "ste.hpp"
#include <algorithm>
#include <initguid.h>
#include <objbase.h>
namespace ste {
std::string guid_text(const GUID &value) {
    wchar_t text[39]{};
    if (!StringFromGUID2(value, text, 39))
        throw std::runtime_error("GUID formatting failed");
    return utf8(text);
}
Symbol subtype(const GUID &value) {
    struct Entry {
        const GUID *guid{};
        const char *name{};
        const char *meaning{};
        DWORD type{};
    };
    const Entry entries[] = {
        {&NETWORK_MANAGER_FIRST_IP_ADDRESS_ARRIVAL_GUID,
         "NETWORK_MANAGER_FIRST_IP_ADDRESS_ARRIVAL_GUID",
         "the first IP address on the TCP/IP stack becomes available",
         SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY},
        {&NETWORK_MANAGER_LAST_IP_ADDRESS_REMOVAL_GUID,
         "NETWORK_MANAGER_LAST_IP_ADDRESS_REMOVAL_GUID",
         "the last IP address on the TCP/IP stack becomes unavailable",
         SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY},
        {&DOMAIN_JOIN_GUID, "DOMAIN_JOIN_GUID", "the computer joins a domain",
         SERVICE_TRIGGER_TYPE_DOMAIN_JOIN},
        {&DOMAIN_LEAVE_GUID, "DOMAIN_LEAVE_GUID", "the computer leaves a domain",
         SERVICE_TRIGGER_TYPE_DOMAIN_JOIN},
        {&FIREWALL_PORT_OPEN_GUID, "FIREWALL_PORT_OPEN_GUID",
         "the specified firewall port opens (all configured strings must match)",
         SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT},
        {&FIREWALL_PORT_CLOSE_GUID, "FIREWALL_PORT_CLOSE_GUID",
         "approximately 60 seconds have passed after the specified firewall port closes",
         SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT},
        {&MACHINE_POLICY_PRESENT_GUID, "MACHINE_POLICY_PRESENT_GUID", "machine policy changes",
         SERVICE_TRIGGER_TYPE_GROUP_POLICY},
        {&USER_POLICY_PRESENT_GUID, "USER_POLICY_PRESENT_GUID", "user policy changes",
         SERVICE_TRIGGER_TYPE_GROUP_POLICY},
        {&RPC_INTERFACE_EVENT_GUID, "RPC_INTERFACE_EVENT_GUID",
         "an endpoint resolution request arrives for the configured RPC interface GUID",
         SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT},
        {&NAMED_PIPE_EVENT_GUID, "NAMED_PIPE_EVENT_GUID",
         "a request opens the configured named pipe", SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT},
        {&CUSTOM_SYSTEM_STATE_CHANGE_EVENT_GUID, "CUSTOM_SYSTEM_STATE_CHANGE_EVENT_GUID",
         "the configured custom system state changes; the SDK does not describe the condition "
         "payload",
         SERVICE_TRIGGER_TYPE_CUSTOM_SYSTEM_STATE_CHANGE}};
    const auto found = std::find_if(std::begin(entries), std::end(entries),
                                    [&](const Entry &e) { return IsEqualGUID(value, *e.guid); });
    if (found != std::end(entries))
        return {found->name, found->meaning, found->type};
    return {};
}
std::string type_name(DWORD t) {
    switch (t) {
    case SERVICE_TRIGGER_TYPE_DEVICE_INTERFACE_ARRIVAL:
        return "Device-interface arrival";
    case SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY:
        return "IP-address availability";
    case SERVICE_TRIGGER_TYPE_DOMAIN_JOIN:
        return "Domain join/leave";
    case SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT:
        return "Firewall-port event";
    case SERVICE_TRIGGER_TYPE_GROUP_POLICY:
        return "Group-policy event";
    case SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT:
        return "Network endpoint";
    case SERVICE_TRIGGER_TYPE_CUSTOM_SYSTEM_STATE_CHANGE:
        return "Custom system-state change";
    case SERVICE_TRIGGER_TYPE_CUSTOM:
        return "Custom ETW event";
    case SERVICE_TRIGGER_TYPE_AGGREGATE:
        return "Aggregate (SDK-defined; semantics unavailable)";
    default:
        return "Unknown";
    }
}
} // namespace ste
