#include "web/layer.hpp"

#include <emscripten.h>

#include "application.hpp"

EM_JS(void, my_log, (const char* msg), { console.log('BrowserLayer: ' + UTF8ToString(msg)); });

void BrowserLayer::build(core::Application& app) {
    my_log("build method called.");
}
