#include <string>
namespace renderer {

class RenderCommand {
public:
    virtual const std::string target_pass() const = 0;
    virtual void execute(RendererBackend&) = 0;
};

} // namespace renderer
