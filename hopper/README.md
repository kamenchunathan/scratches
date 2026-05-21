# Hopper

Hopper is a C++ game engine library for building terminal and web apps.


## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

**We strongly prefer and recommend using [Nix](https://nixos.org/) for development.** The Nix environment guarantees that all necessary tools and libraries are present and correctly versioned. Building without Nix or on package managers directly is currently **untested and unsupported**.

If you have Nix installed, simply enter the development environment by running:

```bash
nix develop
```

This shell automatically fetches and provides all required tools (C++23 compiler, Meson, Ninja) and dependencies used by the engine, including: `openssl`, `protobuf`, `spdlog`, `gtest`, `doctest`, `zlib`, `asio`, `eigen`, and `libpng`.

### Building

This project uses Nix flakes to handle cross-platform builds and toolchains seamlessly.

1.  Clone the repository:
    ```bash
    git clone <repository-url>
    cd hopper
    ```

2. Build for your desired platform:

    **Native (Linux/macOS):**
    ```bash
    nix build
    ```

    **Windows (MinGW-w64):**
    ```bash
    nix build .#windows
    ```

    **WebAssembly (Emscripten):**
    ```bash
    nix build .#wasm
    ```

The compiled outputs will be available in the generated `result` symlink.

### Build Options

When configuring the project manually with Meson, you can pass several options to customize the build. For example, to enable Tracy profiling and disable logging:

```bash
meson setup build -Dhopper_tracy=true -Dhopper_logging=false
```

Available engine options include:
- `hopper_logging` (default: `true`): Enable engine logging via spdlog.
- `hopper_tracy` (default: `false`): Enable Tracy profiling.
- `hopper_metrics` (default: `false`): Enable runtime metrics reporting.
- `hopper_crash_reporter` (default: `true`): Enable crash handler and stack trace capture.

## Usage

Hopper is structured as a library. You can include it in your own projects by linking against the engine.

### Hello World Example

Here is a basic example of how to structure your code when using the Hopper engine. It demonstrates setting up the core application, adding platform-specific layers, and scheduling basic systems.

```cpp
#include <memory>

#include "application.hpp"
#include "ecs/system.hpp"
#include "ecs/world.hpp"
#include "input.hpp"
#include "log.hpp"
#include "log/layer.hpp"

// Include platform-specific rendering layers
#if PLATFORM_WASM
    #include "web/layer.hpp"
#else
    #include "term/layer.hpp"
#endif

// A startup system runs exactly once when the application starts
void startup_system(ecs::World& world) {
    HOPPER_INFO("app", "Application starting up!");
}

// A pre-update system runs every frame before the main logic
void handle_input(ecs::World& world) {
    // Retrieve the input state resource from the ECS world
    if (auto inp_state = world.get_resource<core::input::InputState>(); inp_state.has_value()) {
        auto inp = inp_state->get();
        // Quit the application if 'Q' is pressed
        if (inp.just_pressed(core::input::KeyCode::Q)) {
            if (auto app = world.get_resource<core::Application*>(); app.has_value()) {
                HOPPER_INFO("app", "Quit requested, exiting...");
                app->get()->set_should_exit(true);
            }
        }
    }
}

int main() {
    // 1. Create the core application
    // NOTE: We use a static pointer so the application instance persists past main(). 
    // This is required for WebAssembly builds where the run function exits early 
    // and execution continues in an asynchronous browser loop. It is not needed for 
    // native applications
    static auto app = std::make_shared<core::Application>();
    app->world.insert_resource<core::Application*>(app.get());
    
    // 2. Add essential layers (like logging and inputs)
    app->add_layer(logging::LogLayer {});
    app->add_layer(core::input::InputLayer {});

    // 3. Add platform-specific rendering layer (Terminal vs Web)
#if PLATFORM_WASM
    web::BrowserLayer browser_layer {};
    app->add_layer(browser_layer);
#else
    TerminalLayer term_layer(std::make_unique<Terminal>(), 60);
    app->add_layer(term_layer);
#endif

    // 4. Schedule systems
    // ecs::Stage::Startup executes once at application launch
    app->scheduler.add_system(ecs::Stage::Startup, startup_system);
    // ecs::Stage::PreUpdate executes every frame to process input before game logic
    app->scheduler.add_system(ecs::Stage::PreUpdate, handle_input);

    // 5. Run the main game loop
    app->run();
    return 0;
}
```

See the `examples/` directory for more standalone examples demonstrating how to use the engine's capabilities.

## Project Structure

- `engine/` - Static library containing the core engine (ECS, renderer, physics, terminal/web layers)
- `examples/` - Collection of examples demonstrating how to use different engine features (rendering, physics, input, etc.)
- `labs/` - Experimental area and sandbox for testing new features during development
- `subprojects/` - Third-party dependencies managed by Meson
