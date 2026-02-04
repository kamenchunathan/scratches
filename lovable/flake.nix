{
  description = "A Nix-flake-based Rust development environment";

  inputs = {
    nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1";
    nixgl.url = "github:nix-community/nixGL";
    rust-overlay = {
      url = "github:oxalica/rust-overlay";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = inputs:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSupportedSystem = f: inputs.nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import inputs.nixpkgs {
          inherit system;
          overlays = [
            inputs.rust-overlay.overlays.default
            inputs.self.overlays.default
            inputs.nixgl.overlay
          ];
        };
      });
    in
    {
      overlays.default = final: prev: {
        rustToolchain =
          let
            rust = prev.rust-bin;
          in
          if builtins.pathExists ./rust-toolchain.toml then
            rust.fromRustupToolchainFile ./rust-toolchain.toml
          else if builtins.pathExists ./rust-toolchain then
            rust.fromRustupToolchainFile ./rust-toolchain
          else
            rust.stable.latest.default.override {
              extensions = [ "rust-src" "rustfmt" ];
            };
      };

      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            rustToolchain
            openssl
            pkg-config
            cargo-deny
            cargo-edit
            cargo-watch
            rust-analyzer

            # Optimizations
            clang
            mold

            # Game dev deps
            alsa-lib
            systemd

            vulkan-loader
            vulkan-tools

            xorg.libX11
            xorg.libXcursor
            xorg.libXi
            xorg.libXrandr
            libxkbcommon

            nixgl.nixVulkanIntel
          ];


          shellHook = ''
            mkdir -p .cargo
            cat > .cargo/config.toml <<EOF
            [target.x86_64-unknown-linux-gnu]
            linker = "${pkgs.clang}/bin/clang"
            rustflags = ["-C", "link-arg=--ld-path=${pkgs.mold}/bin/mold"]
            EOF
          '';

          env = {
            # Required by rust-analyzer
            RUST_SRC_PATH = "${pkgs.rustToolchain}/lib/rustlib/src/rust/library";
            LD_LIBRARY_PATH = with pkgs; inputs.nixpkgs.lib.makeLibraryPath [
              vulkan-loader
              xorg.libX11
              xorg.libXi
              xorg.libXcursor
              libxkbcommon
            ];
          };
        };
      });
    };
}
