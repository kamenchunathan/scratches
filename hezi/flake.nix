{
  description = "";

  inputs = {
    nixpkgs.url = "github:cachix/devenv-nixpkgs/rolling";
    devenv.url = "github:cachix/devenv";
  };

  nixConfig = {
    extra-trusted-public-keys = "devenv.cachix.org-1:w1cLUi8dv3hnoSPGAuibQv+f9TZLr6cv/Hm9XgU50cw=";
    extra-substituters = "https://devenv.cachix.org";
  };

  outputs =
    {
      self,
      nixpkgs,
      devenv,
      ...
    }@inputs:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forEachSupportedSystem =
        f:
        nixpkgs.lib.genAttrs supportedSystems (
          system:
          f {
            pkgs = import nixpkgs { inherit system; };
          }
        );
    in
    {

      packages = forEachSupportedSystem (
        { pkgs }:
        {
          devenv-up = self.devShells.${pkgs.system}.default.config.procfileScript;
          devenv-test = self.devShells.${pkgs.system}.default.config.test;
        }
      );

      devShells = forEachSupportedSystem (
        { pkgs }:
        {
          default = devenv.lib.mkShell {
            inherit inputs pkgs;
            modules = [
              (
                { pkgs, config, ... }:
                {
                  packages =
                    with pkgs;
                    [
                      nodejs_20
                      tailwindcss_4
                      watchman
                      folly
                    ]
                    ++
                      # Linux only
                      pkgs.lib.optionals pkgs.stdenv.isLinux (
                        with pkgs;
                        [
                          gigalixir
                          inotify-tools
                          libnotify
                        ]
                      )
                    ++
                      # macOS only
                      pkgs.lib.optionals pkgs.stdenv.isDarwin (
                        with pkgs;
                        [
                          terminal-notifier
                          darwin.apple_sdk.frameworks.CoreFoundation
                          darwin.apple_sdk.frameworks.CoreServices
                        ]
                      );

                  languages = {
                    elixir.enable = true;
                  };

                  services.postgres = {
                    enable = true;
                    package = pkgs.postgresql_15;
                    listen_addresses = "localhost";
                    initialDatabases = [
                      {
                        name = "hezi_dev";
                        user = "postgres";
                        pass = "postgres";
                      }
                    ];
                  };
                }
              )
            ];
          };
        }
      );
    };
}
