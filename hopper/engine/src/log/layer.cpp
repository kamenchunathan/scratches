#include "log/layer.hpp"

#include "common.hpp"
#include "log.hpp"

#include <spdlog/spdlog.h>

namespace logging {

auto LogLayer::build(std::shared_ptr<core::Application>) -> void {
#ifdef NDEBUG
    logging::init(Config {.async = true});
#else
    logging::init();
#endif
}

} // namespace logging
