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
  mkWestBuildCommand =
    cmdName: args:
    mkWestCommand cmdName (
      [
        "build"
        "--pristine=auto"
      ]
      ++ args
    );
  mkWestBuildBoardCommand =
    board: cmdAppendix: args:
    let
      boardWithoutSlash = lib.replaceStrings [ "/" ] [ "_" ] board;
    in
    mkWestBuildCommand "${boardWithoutSlash}-build${cmdAppendix}" (
      [
        "--build-dir"
        "builds/${boardWithoutSlash}"
        "-b"
        board
      ]
      ++ args
    );
in
[
  (pkgs.writeShellScriptBin "west-build-for-board" ''
    set -ueo pipefail
    board="$1"
    shift
    ${mkWestBuildCommand "build-for-board" [ ]}/bin/west-build-for-board \
      --build-dir "builds/$(echo "$board" | sed 's%/%_%g')" \
      -b "$board" "$@"
  '')
  (mkWestBuildBoardCommand "stm32h7b3i_dk" "" [
    "--shield"
    "x_nucleo_idb05a1"
  ])
  (mkWestBuildBoardCommand "stm32h7b3i_dk" "-and-flash" [
    "--shield"
    "x_nucleo_idb05a1"
    "-t"
    "flash"
  ])
  (mkWestBuildBoardCommand "stm32h747i_disco/stm32h747xx/m7" "" [
    "--shield"
    "st_b_lcd40_dsi1_mb1166_a09"
    "--shield"
    "x_nucleo_idb05a1"
  ])
  (mkWestBuildBoardCommand "stm32h747i_disco/stm32h747xx/m7" "-and-flash" [
    "--shield"
    "st_b_lcd40_dsi1_mb1166_a09"
    "--shield"
    "x_nucleo_idb05a1"
    "-t"
    "flash"
  ])
  (mkWestBuildBoardCommand "wio_terminal" "" [ ])
  (mkWestBuildBoardCommand "native_sim" "" [ ])
  (pkgs.writeShellScriptBin "west-native_sim-build-and-run" ''
    set -x
    west-native_sim-build "$@"
    ./builds/native_sim/zephyr/zephyr.exe
  '')
  (mkWestBuildBoardCommand "xiao_esp32s3/esp32s3/procpu" "" [ ])
  (mkWestBuildBoardCommand "xiao_esp32s3/esp32s3/procpu" "-and-flash" [
    "-t"
    "flash"
  ])
  (mkWestCommand "espressif-monitor" [
    "espressif"
    "monitor"
  ])
]
