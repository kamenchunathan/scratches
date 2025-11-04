{
  description = "A Nix-flake-based C/C++ development environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1";

  outputs =
    { self, ... }@inputs:

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
                buildInputs = with pkgs; [
                  llvm
                  clang-tools
                  gtest
                  llvmPackages.libclang
                ];

                packages =
                  with pkgs;
                  [
                    meson
                    ninja
                    just
                    cmake
                    pkg-config
                  ]
                  ++ (if system == "aarch64-darwin" then [ ] else [ gdb ]);

                CLANG_INCLUDE_PATH = "${pkgs.llvmPackages.libclang.dev}/include";
                LLVM_LIB_PATH = "${pkgs.llvm}/lib";
              };
        }
      );
    };
}
