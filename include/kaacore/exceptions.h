#pragma once

#include <stdexcept>

#include "kaacore/log.h"


#define KAACORE_STRINGIFY_DETAIL(x) #x
#define KAACORE_STRINGIFY(x) KAACORE_STRINGIFY_DETAIL(x)

#define KAACORE_TRACE_STRING(s) \
    __FILE__ ":" KAACORE_STRINGIFY(__LINE__) " !(" #s ")"

#define KAACORE_CHECK(condition) \
    do { \
        if (not (condition)) { \
            kaacore::log<kaacore::LogLevel::error>(KAACORE_TRACE_STRING(condition)); \
            throw kaacore::exception(KAACORE_TRACE_STRING(condition)); \
        } \
    } while(0)

#ifdef NDEBUG
    #define KAACORE_ASSERT(condition) do {} while(0)
#else
    #define KAACORE_ASSERT(condition) KAACORE_CHECK(condition)
#endif


namespace kaacore {

struct exception : std::logic_error {
    using std::logic_error::logic_error;
};

} // namespace kaacore
