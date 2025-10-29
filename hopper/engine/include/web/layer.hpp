#include <cstdint>
#include <memory>

namespace core {
class Application;
}

class BrowserLayer {
public:
    struct State {
        std::shared_ptr<core::Application> app;
        double last_time;
    };

private:
    State m_state;

public:
    std::uint32_t fps = 0;

    void build(std::shared_ptr<core::Application>);
    void run(std::shared_ptr<core::Application>);
};
