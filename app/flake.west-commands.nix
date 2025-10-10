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
  (pkgs.writeShellScriptBin "west-build-for-board" ''
    set -ueo pipefail
    board="$1"
    shift
    ${mkWestCommand "build-for-board" [ "build" ]}/bin/west-build-for-board \
      --build-dir "builds/$(echo "$board" | sed 's%/%_%g')" \
      -b "$board" "$@"
  '')
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
  (mkWestCommand "wio_terminal-build" [
    "build"
    "--build-dir"
    "builds/wio_terminal"
    "-b"
    "wio_terminal"
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
