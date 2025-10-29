
#include <emscripten.h>
#include <emscripten/html5.h>

#include "application.hpp"
#include "ecs/system.hpp"
#include "web/layer.hpp"

EM_JS(void, console_log, (const char* msg), { console.log('BrowserLayer: ' + UTF8ToString(msg)); });

int main() {
    // Application has to be static because it's methods and variables are accessed
    // in a callback from javascript
    static core::Application app;

    BrowserLayer layer;
    app.add_layer(layer);

    app.scheduler.add_system(ecs::SystemStage::Update, [](ecs::World&) { console_log("wow"); });

    app.run();
}
