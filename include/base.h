#ifndef BASE_H
#define BASE_H

#include "core/logger.h"
#include "memory/smart_ptr.h"
#include "io/json.h"
#include "io/xml.h"
#include "io/filesystem.h"
#include "net/tcp.h"
#include "net/udp.h"
#include "util/config.h"
#include "util/error.h"
#include "util/thread_pool.h"
#include "util/time.h"

namespace base {

inline const char* version() {
    return "1.0.0";
}

inline int majorVersion() {
    return 1;
}

inline int minorVersion() {
    return 0;
}

inline int patchVersion() {
    return 0;
}

}

#endif // BASE_H