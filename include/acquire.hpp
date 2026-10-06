#pragma once
#include "ste.hpp"
namespace ste {
// Bound retries and memory use when SCM resizes a result between calls.
template <class Query> std::vector<BYTE> acquire(Query query, const char *operation) {
    DWORD needed = 0;
    for (int attempt = 0; attempt < 8; ++attempt) {
        std::vector<BYTE> bytes(needed);
        if (query(bytes.empty() ? nullptr : bytes.data(), static_cast<DWORD>(bytes.size()),
                  &needed))
            return bytes;
        const DWORD error = GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER)
            throw ApiError(operation, error);
        if (!needed || needed > 16 * 1024 * 1024)
            throw ApiError("Unreasonable SCM buffer size", ERROR_INVALID_DATA);
    }
    throw ApiError("SCM configuration changed repeatedly during sizing", ERROR_RETRY);
}
} // namespace ste
