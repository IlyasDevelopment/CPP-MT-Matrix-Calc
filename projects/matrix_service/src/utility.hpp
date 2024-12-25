#pragma once

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sstream>

namespace matrix_service {

    inline void RaiseLinuxCallError(std::size_t line, const char* file, const char* call_str, const char* comment) {
        std::stringstream ss;
        ss << "System call '" << call_str << "' on line " << line << " in file " << file
           << " failed (== -1). Errno = " << errno << " (" << strerror(errno) << "). Comment: " << comment;
        throw std::runtime_error(ss.str());
    }

    inline void RaiseOnLinuxCallError(std::size_t line, const char* file, int call_result, const char* call_str, const char* comment) {
        if (call_result == -1) [[unlikely]]
            RaiseLinuxCallError(line, file, call_str, comment);
    }

    #define VALIDATE_LINUX_CALL(X) RaiseOnLinuxCallError(__LINE__, __FILE__, (X), #X, "<nothing>")
    #define VALIDATE_LINUX_CALL_COMMENT(X, comment) RaiseOnLinuxCallError(__LINE__, __FILE__, (X), #X, comment)

} // namespace matrix_service
