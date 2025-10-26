#include "application.hpp"
#include "web/layer.hpp"

int main() {
    core::Application app;

    BrowserLayer layer;
    app.add_layer(layer);

    app.run();
}
