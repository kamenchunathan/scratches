#include "log/layer.hpp"

#include "common.hpp"
#include "log.hpp"

namespace logging {

auto LogLayer::build(std::shared_ptr<core::Application>) -> void {
#if HOPPER_LOGGING
    #ifdef NDEBUG
        logging::init(Config {.async = true});
    #else
        logging::init();
    #endif
#endif
}

} // namespace logging
