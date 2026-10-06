#include "ste.hpp"
namespace ste {
std::string win_error(DWORD code) {
    wchar_t text[2048]{};
    const DWORD n = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                   nullptr, code, 0, text, 2048, nullptr);
    if (!n)
        return "Win32 error " + std::to_string(code);
    std::wstring message(text, n);
    while (!message.empty() &&
           (message.back() == L'\r' || message.back() == L'\n' || message.back() == L' '))
        message.pop_back();
    return utf8(message);
}
ApiError::ApiError(const std::string &operation, DWORD error)
    : std::runtime_error(operation + ": " + win_error(error) + " (" + std::to_string(error) + ")"),
      code(error) {}
int exit_code(DWORD c) {
    if (c == ERROR_SERVICE_DOES_NOT_EXIST)
        return 3;
    if (c == ERROR_ACCESS_DENIED)
        return 4;
    return 5;
}
} // namespace ste
