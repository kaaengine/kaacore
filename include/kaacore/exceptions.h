#pragma once

#include <exception>
#include <stdexcept>
#include <string>

#include "kaacore/log.h"

#define KAACORE_STRINGIFY_DETAIL(x) #x
#define KAACORE_STRINGIFY(x) KAACORE_STRINGIFY_DETAIL(x)

#define KAACORE_TRACE_STRING(s)                                                \
    __FILE__ ":" KAACORE_STRINGIFY(__LINE__) " !(" #s ")"

#define KAACORE_THROW_IF_NOT_PASSED(condition, message)                        \
    do {                                                                       \
        if (not(condition)) {                                                  \
            kaacore::log<kaacore::LogLevel::error>(                            \
                KAACORE_TRACE_STRING(condition));                              \
            kaacore::log<kaacore::LogLevel::error>(message);                   \
            throw kaacore::exception(                                          \
                std::string(KAACORE_TRACE_STRING(condition)) + " - " +         \
                message);                                                      \
        }                                                                      \
    } while (0)

#define KAACORE_TERMINATE_IF_NOT_PASSED(condition, message)                    \
    do {                                                                       \
        if (not(condition)) {                                                  \
            kaacore::log<kaacore::LogLevel::critical>(                         \
                KAACORE_TRACE_STRING(condition));                              \
            kaacore::log<kaacore::LogLevel::critical>(message);                \
            std::terminate();                                                  \
        }                                                                      \
    } while (0)

#define KAACORE_IGNORE_IF_NOT_PASSED(condition, message)                       \
    do {                                                                       \
    } while (0)

#if (KAACORE_PROTECT_ASSERTS)
#define KAACORE_ASSERT(condition, message)                                     \
    KAACORE_THROW_IF_NOT_PASSED(condition, message)
#define KAACORE_ASSERT_TERMINATE(condition, message)                           \
    KAACORE_TERMINATE_IF_NOT_PASSED(condition, message)
#else
#define KAACORE_ASSERT(condition)                                              \
    KAACORE_IGNORE_IF_NOT_PASSED(condition, message)
#define KAACORE_ASSERT_TERMINATE(condition, message)                           \
    KAACORE_IGNORE_IF_NOT_PASSED(condition, message)
#endif

#if (KAACORE_PROTECT_CHECKS)
#define KAACORE_CHECK(condition, message)                                      \
    KAACORE_THROW_IF_NOT_PASSED(condition, message)
#define KAACORE_CHECK_TERMINATE(condition, message)                            \
    KAACORE_TERMINATE_IF_NOT_PASSED(condition, message)
#else
#define KAACORE_CHECK(condition, message)                                      \
    KAACORE_IGNORE_IF_NOT_PASSED(condition, message)
#define KAACORE_CHECK_TERMINATE(condition, message)                            \
    KAACORE_IGNORE_IF_NOT_PASSED(condition, message)
#endif

namespace kaacore {

struct exception : std::logic_error {
    using std::logic_error::logic_error;
};

} // namespace kaacore
