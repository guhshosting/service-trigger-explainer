#include "ste.hpp"
#include <cstdio>
#include <iostream>
namespace {
void write(const std::string &s, bool error = false) {
    const HANDLE h = GetStdHandle(error ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && h != nullptr && GetConsoleMode(h, &mode)) {
        const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(),
                                          static_cast<int>(s.size()), nullptr, 0);
        if (!n)
            throw std::runtime_error("Output conversion failed");
        std::wstring wide(static_cast<size_t>(n), L'\0');
        if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(),
                                 static_cast<int>(s.size()), wide.data(), n))
            throw std::runtime_error("Output conversion failed");
        size_t pos = 0;
        while (pos < wide.size()) {
            DWORD written = 0;
            const DWORD count =
                static_cast<DWORD>((std::min)(wide.size() - pos, static_cast<size_t>(16384)));
            if (!WriteConsoleW(h, wide.data() + pos, count, &written, nullptr) || !written)
                throw std::runtime_error("Console write failed");
            pos += written;
        }
    } else {
        FILE *stream = error ? stderr : stdout;
        if (fwrite(s.data(), 1, s.size(), stream) != s.size() || fflush(stream) != 0)
            throw std::runtime_error("Output write failed");
    }
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    bool json_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"--json") == 0)
            json_mode = true;
    }
    try {
        std::vector<std::wstring> args;
        for (int i = 1; i < argc; ++i)
            args.emplace_back(argv[i]);
        const auto options = ste::parse(args);
        if (options.help) {
            write("service-trigger-explainer 1.0.0\nUsage: service-trigger-explainer [--json] "
                  "<service-name>\n       service-trigger-explainer --help | --version\nUse the "
                  "canonical local service name, not the display name. Read-only inspection.\n");
            return 0;
        }
        if (options.version) {
            write("service-trigger-explainer 1.0.0\n");
            return 0;
        }
        const auto result = ste::inspect(options.name);
        write(options.json ? ste::json(result) : ste::human(result));
        return 0;
    } catch (const ste::ApiError &e) {
        const int c = ste::exit_code(e.code);
        try {
            write(json_mode ? ste::json_error(c, e.code, e.what()) : std::string(e.what()) + '\n',
                  !json_mode);
        } catch (...) {
            return 6; // Reporting failed; preserve a deterministic failure exit.
        }
        return c;
    } catch (const std::invalid_argument &e) {
        try {
            write(json_mode ? ste::json_error(2, 0, e.what())
                            : std::string(e.what()) + "\nUse --help for usage.\n",
                  !json_mode);
        } catch (...) {
            return 6;
        }
        return 2;
    } catch (const std::exception &e) {
        try {
            write(json_mode ? ste::json_error(6, 0, e.what()) : std::string(e.what()) + '\n',
                  !json_mode);
        } catch (...) {
            return 6;
        }
        return 6;
    }
}
