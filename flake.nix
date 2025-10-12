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
        # allow unfree in nixpkgs
        pkgs = import nixpkgs {
          inherit system;
          config = {
            allowUnfreePredicate =
              pkg:
              builtins.elem (lib.getName pkg) [
                "segger-jlink"
                "STM32CubeProg"
              ];
            segger-jlink.acceptLicense = true;
          };
        };
        inherit (pkgs) lib;
        inherit (pkgs.pkgsi686Linux) SDL2; # for 32-bit libs needed by native_sim
        STM32CubeProg = pkgs.callPackage ./nix/STM32CubeProg.nix { };
        zephyr-packages = inputs.zephyr-nix.packages.${system};
        zephyr-sdk = zephyr-packages.sdk.override {
          targets = [
            "arm-zephyr-eabi"
            "xtensa-espressif_esp32s3_zephyr-elf"
          ];
        };
        zephyr-python-env = zephyr-packages.pythonEnv.override {
          extraPackages =
            ps: with ps; [
              pykwalify
            ];
        };
        zephyr-env = pkgs.symlinkJoin {
          name = "zephyr-env";
          meta.mainProgram = "west";
          nativeBuildInputs = with pkgs; [
            makeWrapper
          ];
          paths = [
            zephyr-sdk
            zephyr-packages.pythonEnv
            zephyr-packages.hosttools-nix
            STM32CubeProg
            SDL2
          ]
          ++ (with pkgs; [
            cmake
            ninja
            binutils
            gdb
            pkg-config

            # openocd
            # segger-jlink-headless
            stlink # st-flash, st-info, st-util
            dfu-util
            pyocd
            bossa
          ]);
          postBuild = ''
            wrapProgram "$out/bin/west" \
              --prefix PATH : $out/bin \
              --prefix PKG_CONFIG_PATH : "${SDL2.dev}/lib/pkgconfig" \
              --prefix LD_LIBRARY_PATH : "${SDL2.out}/lib" \
              --prefix CMAKE_PREFIX_PATH : ${zephyr-sdk}/cmake \
              --set ZEPHYR_TOOLCHAIN_VARIANT zephyr
          '';
        };
        init-script = pkgs.writeShellApplication {
          name = "init-script";
          runtimeInputs = [
            zephyr-env
          ]
          ++ (with pkgs; [
            git
            jq
            diffutils
          ]);
          text = builtins.readFile ./scripts/init-and-chores.sh;
        };
        west-commands = import ./nix/flake.west-commands.nix inputs system;
      in
      {
        packages = {
          inherit STM32CubeProg zephyr-env;
        };
        apps = {
          init = {
            type = "app";
            program = "${init-script}/bin/init-script";
          };
          west = {
            type = "app";
            program = "${zephyr-env}/bin/west";
          };
          entr-mermaid = {
            type = "app";
            program =
              let
                entr-mermaid = pkgs.writeShellApplication {
                  name = "entr-mermaid";
                  runtimeInputs = with pkgs; [
                    entr
                    mermaid-cli
                  ];
                  text = builtins.readFile ./scripts/entr-mermaid.sh;
                };
              in
              "${entr-mermaid}/bin/entr-mermaid";
          };
          scad-build = {
            type = "app";
            program =
              let
                scad-build = pkgs.writeShellApplication {
                  name = "scad-build";
                  runtimeInputs = with pkgs; [
                    openscad
                  ];
                  text = builtins.readFile ./3d-print.scad/build.sh;
                };
              in
              "${scad-build}/bin/scad-build";
          };
        };
        formatter =
          let
            pkgs = nixpkgs.legacyPackages.${system};
            config = self.checks.${system}.pre-commit-check.config;
            inherit (config) package configFile;
            script = ''
              ${pkgs.lib.getExe package} run --all-files --config ${configFile}
            '';
          in
          pkgs.writeShellScriptBin "pre-commit-run" script;
        checks = {
          pre-commit-check = inputs.git-hooks.lib.${system}.run {
            src = ./.;
            hooks = {
              nixfmt-rfc-style.enable = true;
              shfmt.enable = false;
              shfmt.settings.simplify = true;
              shellcheck.enable = true;
              typos.enable = true;
              cmake-format.enable = false;
              clang-format.enable = true;
              clang-tidy.enable = false;
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
                shfmt
              ];
              checkPhase = ''
                shfmt -d -s -i 4 -ci ${files}
              '';
              installPhase = ''
                mkdir "$out"
              '';
            };
        };
        devShells.default =
          let
            inherit (self.checks.${system}.pre-commit-check) shellHook enabledPackages;
          in
          pkgs.mkShell {
            packages = [
              zephyr-env
              init-script
            ]
            ++ west-commands
            ++ enabledPackages;
            shellHook = ''
              ${shellHook}
              source <(west completion bash)
            '';
          };
      }
    ));
}
