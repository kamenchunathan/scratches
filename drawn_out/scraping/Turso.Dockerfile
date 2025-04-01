FROM nixos/nix

RUN echo "experimental-features = nix-command flakes" >> /etc/nix/nix.conf

RUN nix profile install "nixpkgs#sqld"

RUN nix profile install "nixpkgs#turso-cli"

WORKDIR /app

COPY local.db .

CMD ["turso", "dev", "--db-file", "local.db"]
