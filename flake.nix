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
    flake-utils.inputs.nixpkgs.follows = "nixpkgs";

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
        zephyr-packages = inputs.zephyr-nix.packages.${system};
        # west2nix = pkgs.callPackage inputs.west2nix.lib.mkWest2nix { };
        inherit (nixpkgs) lib;
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
                    zephyr-packages.pythonEnv
                  ]
                  ++ (with pkgs; [
                    git
                    jq
                    diffutils
                  ]);
                  text = builtins.readFile ./init.sh;
                };
              in
              "${init-script}/bin/init-script";
          };
        };
        formatter = nixpkgs.legacyPackages.${system}.nixfmt-tree;
        devShells.default = pkgs.mkShell {
          packages = [
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
      }
    ));
}
