#include "common.hpp"

#ifdef PLATFORM_WASM

    #include <cstdint>
    #include <memory>

    #include "application.hpp"

class BrowserLayer {
public:
    struct State {
        std::shared_ptr<core::Application> app;
        double last_time;
    };

private:
    State state_;

public:
    std::uint32_t fps = 0;

    void build(std::shared_ptr<core::Application>);
    void run(std::shared_ptr<core::Application>);
};

#endif
