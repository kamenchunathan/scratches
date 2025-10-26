#include "application.hpp"
#include <cstdint>

class BrowserLayer {
public:
    struct State {
        core::Application* app;
        double last_time;
    };

private:
    State m_state;

public:
    std::uint32_t fps = 0;

    void build(core::Application&);
    void run(core::Application&);
};
