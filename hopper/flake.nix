{
  description = "A Nix-flake-based C/C++ development environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1";

  outputs =
    inputs:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forEachSupportedSystem =
        f:
        inputs.nixpkgs.lib.genAttrs supportedSystems (
          system:
          f {
            pkgs = import inputs.nixpkgs { inherit system; };
          }
        );
    in
    {
      devShells = forEachSupportedSystem (
        { pkgs }:
        {
          default =
            pkgs.mkShell.override
              {
                stdenv = pkgs.clangStdenv;
              }
              {
                packages =
                  with pkgs;
                  [
                    openssl
                    protobuf
                    spdlog
                  
                    clang-tools
                    llvmPackages_22.libcxxClang
                    emscripten
                    cmake
                    cppcheck
                    doxygen
                    doctest

                    meson
                    ninja
                    just

                    static-web-server
                  ]
                  ++ (if stdenv.hostPlatform.system == "aarch64-darwin" then [ ] else [ gdb ]);
                  
                  LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [pkgs.llvmPackages_22.libcxxClang.cc.lib];
              };
        }
      );
    };
}
