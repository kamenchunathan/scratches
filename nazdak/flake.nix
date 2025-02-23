{
  description = "Kadi | Card and Board Game Site ";
  inputs = {
    nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";
    purescript-overlay = {
      url = "github:thomashoneyman/purescript-overlay";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, ... }@inputs:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];

      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;

      nixpkgsFor = forAllSystems (system: import nixpkgs {
        inherit system;
        config = { };
        overlays = builtins.attrValues self.overlays;
      });

    in
    rec {
      overlays = {
        purescript = inputs.purescript-overlay.overlays.default;
      };

      devShells = forAllSystems (system:
        let
          pkgs = nixpkgsFor.${system};
        in
        {
          default = pkgs.mkShell {
            name = "nazdak";
            buildInputs = with pkgs; [
              nodejs_23
              nodePackages.pnpm

              purs
              spago-unstable
              purescript-language-server
              purs-tidy
              purs-backend-es
            ];
          };
        });
    };
}
