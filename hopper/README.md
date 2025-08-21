# Hopper

Hopper is a multiplayer terminal-based platformer game.


## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

This project uses [Nix](https://nixos.org/) to manage dependencies. You will need to have Nix installed on your system.

Once you have Nix installed, you can enter the development environment by running the following command in the project root:

```bash
nix develop
```

If you are not using Nix, you will need to install the dependencies using your package manager. You will need a C++23 compatible compiler, Meson, and Ninja to build this project.

**Debian/Ubuntu:**
```bash
sudo apt update
sudo apt install g++ meson ninja-build
```

**Fedora:**
```bash
sudo dnf install gcc-c++ meson ninja-build
```

**Arch Linux:**
```bash
sudo pacman -S gcc meson ninja
```

**macOS (using Homebrew):**
```bash
brew install llvm meson ninja
```

The other dependencies (FTXUI and Asio) are managed by Meson and will be downloaded automatically.

### Building

1.  Clone the repository:
    ```bash
    git clone <repository-url>
    cd hopper
    ```
2.  Configure the project with Meson:
    ```bash
    meson setup build
    ```
3.  Compile the project with Ninja:
    ```bash
    meson compile -C build
    ```

## Usage

The project is split into a client and a server.

### Server

To run the server:
```bash
./build/hopper-server
```

### Client

To run the client:
```bash
./build/hopper-client
```



## Project Structure

The game consists of three projects all in the src directory, engine which contains core and shared components, the server and the client. The labs directory contain small self contained examples to explore c++ topics
