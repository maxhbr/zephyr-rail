{
  description = "My Rail to take macro photos";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    # Customize the version of Zephyr used by the flake here
    zephyr.url = "github:zephyrproject-rtos/zephyr/v4.2.0";
    zephyr.flake = false;

    zephyr-nix.url = "github:nix-community/zephyr-nix";
    zephyr-nix.inputs.nixpkgs.follows = "nixpkgs";
    zephyr-nix.inputs.zephyr.follows = "zephyr";

    flake-utils.url = "github:numtide/flake-utils";

    git-hooks.url = "github:cachix/git-hooks.nix";

    # west2nix.url = "github:adisbladis/west2nix";
    # west2nix.inputs.nixpkgs.follows = "nixpkgs";
    # west2nix.inputs.zephyr-nix.follows = "zephyr-nix";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      ...
    }@inputs:
    (flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        inherit (nixpkgs) lib;
        zephyr-packages = inputs.zephyr-nix.packages.${system};
        zephyr-env = pkgs.symlinkJoin {
          name = "zephyr-env";
          paths = [
            (zephyr-packages.sdk.override {
              targets = [
                "arm-zephyr-eabi"
              ];
            })
            zephyr-packages.pythonEnv
            zephyr-packages.hosttools-nix
          ]
          ++ (with pkgs; [
            cmake
            ninja
          ]);
        };
        # west2nix = pkgs.callPackage inputs.west2nix.lib.mkWest2nix { };
      in
      {
        apps = {
          init = {
            type = "app";
            program =
              let
                init-script = pkgs.writeShellApplication {
                  name = "init-script";
                  runtimeInputs = [
                    zephyr-env
                  ]
                  ++ (with pkgs; [
                    git
                    jq
                    diffutils
                    mermaid-cli
                  ]);
                  text = builtins.readFile ./scripts/init-and-chores.sh;
                };
              in
              "${init-script}/bin/init-script";
          };
          west = {
            type = "app";
            program = "${zephyr-env}/bin/west";
          };
        };
        formatter = nixpkgs.legacyPackages.${system}.nixfmt-tree;
        checks = {
          pre-commit-check = inputs.git-hooks.lib.${system}.run {
            src = ./.;
            hooks = {
              nixfmt-rfc-style.enable = true;
              # shfmt.enable = true;
              # shfmt.settings.simplify = true;
              shellcheck.enable = true;
              # typos.enable = true;
            };
          };
          shell-fmt-check =
            let
              pkgs = inputs.nixpkgs.legacyPackages."${system}";
              files = pkgs.lib.concatStringsSep " " [
                "scripts/init-and-chores.sh"
              ];
            in
            pkgs.stdenv.mkDerivation {
              name = "shell-fmt-check";
              src = ./.;
              doCheck = true;
              nativeBuildInputs = with pkgs; [
                shellcheck
                shfmt
              ];
              checkPhase = ''
                shfmt -d -s -i 4 -ci ${files}
                shellcheck -x ${files}
              '';
              installPhase = ''
                mkdir "$out"
              '';
            };
        };
        devShells.default = pkgs.mkShell {
          packages = [zephyr-env];
        };
      }
    ));
}
