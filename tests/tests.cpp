#include "acquire.hpp"
#include "ste.hpp"
#include <array>
#include <cstdio>
#include <iostream>
namespace {
void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, "Expected rejection");
}
struct Fixture {
    SERVICE_TRIGGER_INFO info{};
    std::array<SERVICE_TRIGGER, 9> triggers{};
    GUID guid{};
    SERVICE_TRIGGER_SPECIFIC_DATA_ITEM item{};
    std::array<BYTE, 16> bytes{};
    Fixture() {
        info.pTriggers = triggers.data();
        guid = GUID{0x1ce20aba, 0x9851, 0x4421, {0x94, 0x30, 0x1d, 0xde, 0xb7, 0x66, 0xe8, 0x09}};
    }
};
int integration() {
    SC_HANDLE h = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!h) {
        std::cerr << ste::win_error(GetLastError());
        return 77;
    }
    DWORD needed = 0, count = 0, resume = 0;
    std::vector<BYTE> b(size_t{256} * 1024);
    const BOOL ok =
        EnumServicesStatusExW(h, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, b.data(),
                              static_cast<DWORD>(b.size()), &needed, &count, &resume, nullptr);
    const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
    CloseServiceHandle(h);
    if (!ok && error != ERROR_MORE_DATA) {
        std::cerr << ste::win_error(error);
        return 77;
    }
    ste::BufferView view(b.data(), b.size());
    const auto p = reinterpret_cast<const ENUM_SERVICE_STATUS_PROCESSW *>(b.data());
    view.require(p, count, sizeof(*p));
    size_t inspected = 0, triggered = 0;
    for (DWORD i = 0; i < count; ++i) {
        const auto entry = view.read(p + i);
        const auto name = view.string(entry.lpServiceName);
        try {
            const auto r = ste::inspect(name);
            ++inspected;
            if (!r.triggers.empty())
                ++triggered;
        } catch (const ste::ApiError &e) {
            if (e.code != ERROR_ACCESS_DENIED && e.code != ERROR_SERVICE_DOES_NOT_EXIST)
                throw;
        }
    }
    std::cout << "Read " << inspected << " existing services; " << triggered << " have triggers.\n";
    return inspected ? 0 : 77;
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc > 1 && std::string(argv[1]) == "json") {
            ste::Result r;
            r.name = ste::utf8(L"服務");
            r.display = ste::utf8(L"サービス😀\n\"\\");
            Fixture f;
            f.info.cTriggers = 1;
            f.triggers[0].pTriggerSubtype = &f.guid;
            f.triggers[0].dwTriggerType = SERVICE_TRIGGER_TYPE_DOMAIN_JOIN;
            f.triggers[0].dwAction = SERVICE_TRIGGER_ACTION_SERVICE_START;
            r.triggers = ste::decode_triggers(&f, sizeof(f));
            r.triggers[0].data = {ste::decode_data(1, {0, 255}),
                                  ste::decode_data(2, {'a', 0, 0, 0}), ste::decode_data(3, {5}),
                                  ste::decode_data(4, {255, 255, 255, 255, 255, 255, 255, 255}),
                                  ste::decode_data(5, {1, 0, 0, 0, 0, 0, 0, 0})};
            std::cout << ste::json(r);
            return 0;
        }
        if (argc > 1)
            return integration();
        const wchar_t bounded[] = {L'a', L'b', L'c'};
        ste::BufferView strings(bounded, sizeof(bounded));
        rejects([&] { strings.string(bounded); });
        rejects([&] { strings.string(nullptr); });
        const wchar_t terminated[] = {L'服', L'務', L'\0'};
        check(ste::BufferView(terminated, sizeof(terminated)).string(terminated) == L"服務",
              "bounded Unicode configuration");
        int calls = 0;
        const auto acquired = ste::acquire(
            [&](BYTE *p, DWORD n, DWORD *needed) -> BOOL {
                ++calls;
                *needed = calls == 1 ? 4 : 8;
                if (n < *needed) {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return FALSE;
                }
                p[0] = 42;
                return TRUE;
            },
            "synthetic race");
        check(calls == 3 && acquired.size() == 8 && acquired[0] == 42, "sizing race");
        rejects([] {
            ste::acquire(
                [](BYTE *, DWORD, DWORD *n) -> BOOL {
                    *n = 1;
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return FALSE;
                },
                "persistent race");
        });
        rejects([] {
            ste::acquire(
                [](BYTE *, DWORD, DWORD *n) -> BOOL {
                    *n = MAXDWORD;
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return FALSE;
                },
                "size limit");
        });
        rejects([] {
            ste::acquire(
                [](BYTE *, DWORD, DWORD *) -> BOOL {
                    SetLastError(ERROR_SERVICE_MARKED_FOR_DELETE);
                    return FALSE;
                },
                "disappeared service");
        });
        check(ste::parse({L"--help"}).help, "help");
        check(ste::parse({L"--version"}).version, "version");
        check(ste::parse({L"--json", L"服務"}).json, "unicode CLI");
        const GUID *known_guids[] = {&NETWORK_MANAGER_FIRST_IP_ADDRESS_ARRIVAL_GUID,
                                     &NETWORK_MANAGER_LAST_IP_ADDRESS_REMOVAL_GUID,
                                     &DOMAIN_JOIN_GUID,
                                     &DOMAIN_LEAVE_GUID,
                                     &FIREWALL_PORT_OPEN_GUID,
                                     &FIREWALL_PORT_CLOSE_GUID,
                                     &MACHINE_POLICY_PRESENT_GUID,
                                     &USER_POLICY_PRESENT_GUID,
                                     &RPC_INTERFACE_EVENT_GUID,
                                     &NAMED_PIPE_EVENT_GUID,
                                     &CUSTOM_SYSTEM_STATE_CHANGE_EVENT_GUID};
        for (const auto *guid : known_guids)
            check(!ste::subtype(*guid).name.empty(), "known subtype database");
        for (const auto &args : std::vector<std::vector<std::wstring>>{{},
                                                                       {L"--json"},
                                                                       {L"--bad"},
                                                                       {L"a", L"b"},
                                                                       {L"--help", L"a"},
                                                                       {L"--json", L"--json", L"a"},
                                                                       {L"a/b"}})
            rejects([&] { ste::parse(args); });
        Fixture f;
        check(ste::decode_triggers(&f, sizeof(f)).empty(), "zero triggers");
        const DWORD types[] = {SERVICE_TRIGGER_TYPE_DEVICE_INTERFACE_ARRIVAL,
                               SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY,
                               SERVICE_TRIGGER_TYPE_DOMAIN_JOIN,
                               SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT,
                               SERVICE_TRIGGER_TYPE_GROUP_POLICY,
                               SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT,
                               SERVICE_TRIGGER_TYPE_CUSTOM_SYSTEM_STATE_CHANGE,
                               SERVICE_TRIGGER_TYPE_CUSTOM,
                               SERVICE_TRIGGER_TYPE_AGGREGATE};
        f.info.cTriggers = 9;
        for (size_t i = 0; i < 9; ++i) {
            f.triggers[i].dwTriggerType = types[i];
            f.triggers[i].dwAction = (i % 2) ? SERVICE_TRIGGER_ACTION_SERVICE_STOP
                                             : SERVICE_TRIGGER_ACTION_SERVICE_START;
            f.triggers[i].pTriggerSubtype = &f.guid;
        }
        auto triggers = ste::decode_triggers(&f, sizeof(f));
        check(triggers.size() == 9, "multiple triggers");
        for (const auto &t : triggers)
            check(t.type_name != "Unknown", "known category");
        check(triggers[2].subtype_name == "DOMAIN_JOIN_GUID", "known GUID");
        check(triggers[1].action_name == "Stop", "stop");
        f.guid.Data1 = 0xdeadbeef;
        f.info.cTriggers = 1;
        f.triggers[0].dwTriggerType = 999;
        f.triggers[0].dwAction = 999;
        triggers = ste::decode_triggers(&f, sizeof(f));
        check(triggers[0].subtype_name.empty() && triggers[0].type_name == "Unknown" &&
                  !triggers[0].warnings.empty(),
              "unknown values");
        f.triggers[0].cDataItems = 1;
        f.triggers[0].pDataItems = &f.item;
        f.item.pData = f.bytes.data();
        f.item.cbData = 4;
        f.item.dwDataType = SERVICE_TRIGGER_DATA_TYPE_BINARY;
        check(ste::decode_triggers(&f, sizeof(f))[0].data[0].hex == "00000000",
              "binary acquisition");
        f.item.pData = nullptr;
        rejects([&] { ste::decode_triggers(&f, sizeof(f)); });
        f.item.pData = f.bytes.data();
        f.item.cbData = 999;
        rejects([&] { ste::decode_triggers(&f, sizeof(f)); });
        f.info.cTriggers = MAXDWORD;
        rejects([&] { ste::decode_triggers(&f, sizeof(f)); });
        rejects([&] { ste::decode_triggers(&f, 1); });
        check(ste::decode_data(1, {0, 255}).hex == "00ff", "binary");
        check(ste::decode_data(2, {'a', 0, 0, 0, 'b', 0, 0, 0, 0, 0}).strings.size() == 2,
              "multistring");
        check(!ste::decode_data(2, {'a'}).warnings.empty(), "odd UTF16");
        check(!ste::decode_data(2, {'a', 0}).warnings.empty(), "unterminated");
        check(!ste::decode_data(2, {0, 0xd8, 0, 0}).warnings.empty(), "invalid surrogate");
        check(ste::decode_data(3, {255}).number == "255", "level");
        check(!ste::decode_data(3, {}).warnings.empty(), "bad level");
        for (DWORD type : {4UL, 5UL}) {
            check(ste::decode_data(type, {255, 255, 255, 255, 255, 255, 255, 255}).number ==
                      "18446744073709551615",
                  "keyword precision");
            check(!ste::decode_data(type, {1}).warnings.empty(), "bad keyword");
        }
        check(!ste::decode_data(99, {}).warnings.empty(), "unknown data");
        check(ste::quote("\"\\\n\t\x01") == "\"\\\"\\\\\\n\\t\\u0001\"", "JSON escaping");
        ste::Result r;
        r.name = ste::utf8(L"服務");
        r.display = ste::utf8(L"サービス😀");
        r.triggers = std::move(triggers);
        check(ste::json(r).find(r.display) != std::string::npos, "Unicode JSON");
        check(ste::json(r).find("\"schema_version\":1") != std::string::npos, "schema");
        r.triggers.clear();
        check(ste::human(r).find("No service triggers are configured.") != std::string::npos,
              "zero human");
        check(ste::exit_code(ERROR_SERVICE_DOES_NOT_EXIST) == 3 &&
                  ste::exit_code(ERROR_ACCESS_DENIED) == 4 &&
                  ste::exit_code(ERROR_SERVICE_MARKED_FOR_DELETE) == 5,
              "Win32 mapping");
        check(!ste::win_error(ERROR_ACCESS_DENIED).empty(), "Win32 message");
        std::cout << "All synthetic checks passed.\n";
        return 0;
    } catch (const std::exception &e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    } catch (...) {
        return 1;
    }
}
