#include <memory>

#include "renderer/present.hpp"

namespace renderer {

class TerminalPresenter: public Presenter {
public:
    TerminalPresenter();
    ~TerminalPresenter();

    void present(const OutputBuffers& front_buffer, const OutputBuffers& back_buffer);
};

} // namespace renderer
