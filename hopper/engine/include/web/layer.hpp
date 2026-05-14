#include "common.hpp"

#ifdef PLATFORM_WASM

    #include <cstdint>
    #include <memory>

    #include "application.hpp"

namespace web {

class BrowserLayer {
public:
    struct State {
        std::shared_ptr<core::Application> app;
        double last_time;
    };

public:
    std::uint32_t fps = 0;

    void build(std::shared_ptr<core::Application>);
};

} // namespace web

#endif
