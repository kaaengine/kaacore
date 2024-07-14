#pragma once

#include <exception>
#include <stdexcept>
#include <string>

#include "kaacore/log.h"

#define KAACORE_STRINGIFY_DETAIL(x) #x
#define KAACORE_STRINGIFY(x) KAACORE_STRINGIFY_DETAIL(x)

#define KAACORE_TRACE_STRING(s)                                                \
    __FILE__ ":" KAACORE_STRINGIFY(__LINE__) " !(" #s ")"

#define KAACORE_THROW_IF_NOT_PASSED(condition, ...)                            \
    do {                                                                       \
        if (not(condition)) {                                                  \
            std::string msg = fmt::format(__VA_ARGS__);                        \
            KAACORE_LOG_ERROR(                                                 \
                "{} - {}", KAACORE_TRACE_STRING(condition), msg                \
            );                                                                 \
            throw kaacore::exception(                                          \
                fmt::format("{} - {}", KAACORE_TRACE_STRING(condition), msg)   \
            );                                                                 \
        }                                                                      \
    } while (0)

#define KAACORE_TERMINATE_IF_NOT_PASSED(condition, ...)                        \
    do {                                                                       \
        if (not(condition)) {                                                  \
            std::string msg = fmt::format(__VA_ARGS__);                        \
            KAACORE_LOG_CRITICAL(                                              \
                "{} - {}", KAACORE_TRACE_STRING(condition), msg                \
            );                                                                 \
            std::terminate();                                                  \
        }                                                                      \
    } while (0)

#define KAACORE_IGNORE_IF_NOT_PASSED(condition, ...)                           \
    do {                                                                       \
    } while (0)

#if (KAACORE_PROTECT_ASSERTS)
#define KAACORE_ASSERT(condition, ...)                                         \
    KAACORE_THROW_IF_NOT_PASSED(condition, __VA_ARGS__)
#define KAACORE_ASSERT_TERMINATE(condition, ...)                               \
    KAACORE_TERMINATE_IF_NOT_PASSED(condition, __VA_ARGS__)
#else
#define KAACORE_ASSERT(condition, ...)                                         \
    KAACORE_IGNORE_IF_NOT_PASSED(condition, __VA_ARGS__)
#define KAACORE_ASSERT_TERMINATE(condition, ...)                               \
    KAACORE_IGNORE_IF_NOT_PASSED(condition, __VA_ARGS__)
#endif

#if (KAACORE_PROTECT_CHECKS)
#define KAACORE_CHECK(condition, ...)                                          \
    KAACORE_THROW_IF_NOT_PASSED(condition, __VA_ARGS__)
#define KAACORE_CHECK_TERMINATE(condition, ...)                                \
    KAACORE_TERMINATE_IF_NOT_PASSED(condition, __VA_ARGS__)
#else
#define KAACORE_CHECK(condition, ...)                                          \
    KAACORE_IGNORE_IF_NOT_PASSED(condition, __VA_ARGS__)
#define KAACORE_CHECK_TERMINATE(condition, ...)                                \
    KAACORE_IGNORE_IF_NOT_PASSED(condition, __VA_ARGS__)
#endif

namespace kaacore {

struct exception : std::logic_error {
    using std::logic_error::logic_error;
};

} // namespace kaacore
