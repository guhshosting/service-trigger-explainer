#pragma once
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <windows.h>
#include <winsvc.h>
namespace ste {
inline constexpr char version[] = "1.0.0";
struct ApiError : std::runtime_error {
    DWORD code;
    ApiError(const std::string &operation, DWORD error);
};
std::string win_error(DWORD code);
int exit_code(DWORD code);
std::string utf8(const std::wstring &value);
std::string guid_text(const GUID &value);
struct Symbol {
    std::string name, meaning;
    DWORD type{};
};
Symbol subtype(const GUID &value);
std::string type_name(DWORD type);
struct Data {
    DWORD type{};
    std::string name, hex;
    std::vector<std::string> strings;
    std::string number;
    std::vector<std::string> warnings;
};
struct Trigger {
    DWORD type{}, action{};
    std::string type_name, action_name, guid, subtype_name, explanation;
    std::vector<Data> data;
    std::vector<std::string> warnings;
};
struct Result {
    std::string name, display, account;
    DWORD start{}, service_type{}, state{};
    bool delayed{}, delayed_known{};
    std::vector<Trigger> triggers;
    std::vector<std::string> warnings;
};
class BufferView {
    const BYTE *base_;
    size_t size_;

  public:
    BufferView(const void *base, size_t size)
        : base_(static_cast<const BYTE *>(base)), size_(size) {}
    void require(const void *p, size_t count, size_t width) const;
    template <class T> T read(const T *p) const {
        require(p, 1, sizeof(T));
        T v{};
        memcpy(&v, p, sizeof(T));
        return v;
    }
    std::wstring string(const wchar_t *p) const;
};
Data decode_data(DWORD type, const std::vector<BYTE> &bytes);
std::vector<Trigger> decode_triggers(const void *buffer, size_t size);
Result inspect(const std::wstring &name);
std::string human(const Result &result);
std::string json(const Result &result);
std::string json_error(int code, DWORD win32, const std::string &message);
std::string quote(const std::string &value);
std::string start_name(DWORD value);
std::string state_name(DWORD value);
struct Options {
    bool json{}, help{}, version{};
    std::wstring name;
};
Options parse(const std::vector<std::wstring> &args);
} // namespace ste
