#pragma once

#include <exception>
#include <stdexcept>

#include "kaacore/log.h"

#define KAACORE_STRINGIFY_DETAIL(x) #x
#define KAACORE_STRINGIFY(x) KAACORE_STRINGIFY_DETAIL(x)

#define KAACORE_TRACE_STRING(s)                                                \
    __FILE__ ":" KAACORE_STRINGIFY(__LINE__) " !(" #s ")"

#define KAACORE_THROW_IF_NOT_PASSED(condition)                                 \
    do {                                                                       \
        if (not(condition)) {                                                  \
            kaacore::log<kaacore::LogLevel::error>(                            \
                KAACORE_TRACE_STRING(condition));                              \
            throw kaacore::exception(KAACORE_TRACE_STRING(condition));         \
        }                                                                      \
    } while (0)

#define KAACORE_TERMINATE_IF_NOT_PASSED(condition)                             \
    do {                                                                       \
        if (not(condition)) {                                                  \
            kaacore::log<kaacore::LogLevel::critical>(                         \
                KAACORE_TRACE_STRING(condition));                              \
            std::terminate();                                                  \
        }                                                                      \
    } while (0)

#define KAACORE_IGNORE_IF_NOT_PASSED(condition)                                \
    do {                                                                       \
    } while (0)

#if (KAACORE_PROTECT_ASSERTS)
#define KAACORE_ASSERT(condition) KAACORE_THROW_IF_NOT_PASSED(condition)
#define KAACORE_ASSERT_TERMINATE(condition) KAACORE_TERMINATE_IF_NOT_PASSED(condition)
#else
#define KAACORE_ASSERT(condition) KAACORE_IGNORE_IF_NOT_PASSED(condition)
#define KAACORE_ASSERT_TERMINATE(condition) KAACORE_IGNORE_IF_NOT_PASSED(condition)
#endif

#if (KAACORE_PROTECT_CHECKS)
#define KAACORE_CHECK(condition) KAACORE_THROW_IF_NOT_PASSED(condition)
#define KAACORE_CHECK_TERMINATE(condition) KAACORE_TERMINATE_IF_NOT_PASSED(condition)
#else
#define KAACORE_CHECK(condition) KAACORE_IGNORE_IF_NOT_PASSED(condition)
#define KAACORE_CHECK_TERMINATE(condition) KAACORE_IGNORE_IF_NOT_PASSED(condition)
#endif

namespace kaacore {

struct exception : std::logic_error {
    using std::logic_error::logic_error;
};

} // namespace kaacore
