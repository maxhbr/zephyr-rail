{
  self,
  nixpkgs,
  flake-utils,
  ...
}@inputs:
system:
let
  pkgs = nixpkgs.legacyPackages.${system};
  inherit (pkgs) lib;
  mkWestCommand =
    cmdName: args:
    pkgs.writeShellApplication {
      name = "west-${cmdName}";
      runtimeInputs = [
        self.packages.${system}.zephyr-env
      ];
      text = ''
        set -x
        west ${lib.concatStringsSep " " args} "$@"
      '';
    };
in
[
  (mkWestCommand "build" [
    "build"
    "--build-dir"
    "builds/stm32h7b3i_dk"
    "-b"
    "stm32h7b3i_dk"
    "--shield"
    "x_nucleo_idb05a1"
  ])
  # (mkWestCommand "flash" ["flash"])
  (mkWestCommand "build-and-flash" [
    "build"
    "--build-dir"
    "builds/stm32h7b3i_dk"
    "-b"
    "stm32h7b3i_dk"
    "--shield"
    "x_nucleo_idb05a1"
    "-t"
    "flash"
  ])
  (mkWestCommand "STM32H757I-DISCO-build" [
    "build"
    "--build-dir"
    "builds/STM32H757I-DISCO"
    "-b"
    "stm32h747i_disco/stm32h747xx/m7"
    "--shield"
    "st_b_lcd40_dsi1_mb1166_a09"
  ])
  (mkWestCommand "STM32H757I-DISCO-build-and-flash" [
    "build"
    "--build-dir"
    "builds/STM32H757I-DISCO"
    "-b"
    "stm32h747i_disco/stm32h747xx/m7"
    "--shield"
    "st_b_lcd40_dsi1_mb1166_a09"
    "-t"
    "flash"
  ])
  (mkWestCommand "native_sim-build" [
    "build"
    "--build-dir"
    "builds/native_sim"
    "-b"
    "native_sim"
  ])
  (pkgs.writeShellScriptBin "west-native_sim-build-and-run" ''
    set -x
    west-native_sim-build "$@"
    ./builds/native_sim/zephyr/zephyr.exe
  '')
]
