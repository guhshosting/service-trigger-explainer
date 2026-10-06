#include "ste.hpp"
namespace ste {
std::vector<Trigger> decode_triggers(const void *buffer, size_t size) {
    BufferView view(buffer, size);
    const auto info = view.read(static_cast<const SERVICE_TRIGGER_INFO *>(buffer));
    view.require(info.pTriggers, info.cTriggers, sizeof(SERVICE_TRIGGER));
    std::vector<Trigger> out;
    for (DWORD i = 0; i < info.cTriggers; ++i) {
        const auto raw = view.read(info.pTriggers + i);
        Trigger t;
        t.type = raw.dwTriggerType;
        t.action = raw.dwAction;
        t.type_name = type_name(t.type);
        t.action_name = t.action == SERVICE_TRIGGER_ACTION_SERVICE_START  ? "Start"
                        : t.action == SERVICE_TRIGGER_ACTION_SERVICE_STOP ? "Stop"
                                                                          : "Unknown";
        Symbol symbol;
        if (raw.pTriggerSubtype) {
            const auto g = view.read(raw.pTriggerSubtype);
            t.guid = guid_text(g);
            symbol = subtype(g);
            t.subtype_name = symbol.name;
        } else
            t.warnings.push_back("Subtype GUID is absent.");
        std::string condition;
        if (!symbol.name.empty() && symbol.type == t.type)
            condition = symbol.meaning;
        else {
            if (!symbol.name.empty())
                t.warnings.push_back("Known subtype does not match the trigger category.");
            switch (t.type) {
            case SERVICE_TRIGGER_TYPE_DEVICE_INTERFACE_ARRIVAL:
                condition = "a device of the configured interface class arrives or is present at "
                            "boot; optional hardware/compatible IDs filter matches";
                break;
            case SERVICE_TRIGGER_TYPE_CUSTOM:
                condition = "the configured ETW provider emits a matching custom event; "
                            "provider-specific meaning is not exposed by SCM";
                break;
            default:
                condition = "the configured Windows trigger condition occurs; this subtype or "
                            "category has no documented interpretation in this tool";
                break;
            }
        }
        t.explanation = t.action_name + " when " + condition + ".";
        if (t.type_name == "Unknown")
            t.warnings.push_back("Unknown trigger type; raw configuration retained.");
        if (t.subtype_name.empty() && !t.guid.empty())
            t.warnings.push_back("Subtype GUID has no known symbolic name in this tool.");
        if (t.action_name == "Unknown")
            t.warnings.push_back("Unknown trigger action.");
        if (t.type == SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT &&
            t.action != SERVICE_TRIGGER_ACTION_SERVICE_START)
            t.warnings.push_back("Documented network endpoint triggers require a Start action.");
        view.require(raw.pDataItems, raw.cDataItems, sizeof(SERVICE_TRIGGER_SPECIFIC_DATA_ITEM));
        for (DWORD j = 0; j < raw.cDataItems; ++j) {
            const auto item = view.read(raw.pDataItems + j);
            view.require(item.pData, item.cbData, 1);
            std::vector<BYTE> bytes(item.cbData);
            if (item.cbData)
                memcpy(bytes.data(), item.pData, item.cbData);
            t.data.push_back(decode_data(item.dwDataType, bytes));
        }
        out.push_back(std::move(t));
    }
    return out;
}
} // namespace ste
