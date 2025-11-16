{
  description = "A Nix-flake-based C/C++ development environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1";
  inputs.clang-p2996.url = "path:./nix/clang-p2996";
  inputs.clang-p2996.flake = false;

  outputs =
    { self, clang-p2996, ... }@inputs:

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
        let
            clang-p2996-pkg = pkgs.callPackage inputs.clang-p2996 { };
        in
        {
          default =
            pkgs.mkShell
              {
                packages =
                  with pkgs;
                  [
                    meson
                    ninja
                    clang-p2996-pkg
                  ];
              };
        }
      );
    };
}
