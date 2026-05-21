{
  description = "Hopper game engine: A C++ game engine for building terminal apps";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1";

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      
      forEachSupportedSystem = f:
        nixpkgs.lib.genAttrs supportedSystems (system: f {
          pkgs = import nixpkgs { inherit system; };
        });
    in
    {
      # ---------------------------------------------------------
      packages = forEachSupportedSystem ({ pkgs }: 
        let
          # Shared build-time tools required to configure and compile the project
          nativeTools = with pkgs; [ pkg-config meson ninja ];
          
          # Shared runtime/linked libraries used by the engine across all targets
          libraries = with pkgs; [ openssl protobuf spdlog gtest doctest zlib asio eigen 
            libpng
          ];

          # Helper function to standardize the Hopper build across different platform toolchains
          buildHopper = { stdenv, extraNative ? [], extraLibs ? [] }: stdenv.mkDerivation {
            pname = "hopper";
            version = "0.2.0";
            
            src = ./.; 

            nativeBuildInputs = nativeTools ++ extraNative;
            buildInputs = libraries ++ extraLibs;
          };
        in
        {
          # Native target built using LLVM/Clang
          default = buildHopper {
            stdenv = pkgs.llvmPackages_22.stdenv;
          };

          # Windows cross-compilation target using MinGW-w64
          # Note: Requires explicit injection of the Windows pthreads library
          windows = buildHopper {
            stdenv = pkgs.pkgsCross.mingwW64.stdenv;
            extraLibs = [ pkgs.pkgsCross.mingwW64.windows.pthreads ];
          };

          # WebAssembly target using the Emscripten toolchain
          wasm = buildHopper {
            stdenv = pkgs.emscriptenStdenv;
          };
        }
      );

      devShells = forEachSupportedSystem ({ pkgs }:
        let
          devTools = with pkgs; [ 
            clang-tools just cppcheck doxygen 
          ] ++ (if pkgs.stdenv.hostPlatform.isDarwin then [ ] else [ gdb ]);
        in
        {
          default = pkgs.mkShell.override { stdenv = pkgs.llvmPackages_22.stdenv; } {
            inputsFrom = [ self.packages.${pkgs.stdenv.hostPlatform.system}.default ];
            nativeBuildInputs = devTools;
            LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [ pkgs.llvmPackages_22.libcxxClang.cc.lib ];
          };

          cross-windows = pkgs.pkgsCross.mingwW64.mkShell {
            inputsFrom = [ self.packages.${pkgs.stdenv.hostPlatform.system}.windows ];
            nativeBuildInputs = devTools ++ [ pkgs.wine64 ];
          };

          cross-wasm = pkgs.mkShell {
            inputsFrom = [ self.packages.${pkgs.stdenv.hostPlatform.system}.wasm ];
            nativeBuildInputs = devTools ++ [ pkgs.static-web-server ];
          };
        }
      );
    };
}
