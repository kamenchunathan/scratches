{
  description = "A Nix-flake-based Haskell development environment";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/4e6868b1aa3766ab1de169922bb3826143941973";

  outputs = inputs:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSupportedSystem = f: inputs.nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import inputs.nixpkgs { inherit system; };
      });
    in
    {
      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell {
          packages = with pkgs; [ 
          cabal-install 
          ghc 
          haskell-language-server 

          zlib
          ];
        };
      });
    };
}
