# Repository Guidelines

## Project Overview

This is firmware for an automated macro stacking rail system controlled via a Progressive Web App (PWA) over Bluetooth. The system uses Zephyr RTOS and supports camera triggering (Sony Bluetooth) while moving a stepper motor across a HiWin rail. Key features include:
- State machine-based rail control for precise image stacking sequences
- Bluetooth connectivity: PWA WebBluetooth for control + GATT client for Sony camera remote triggering  
- Bundled PWA web interface in `app/web_interface/`
- Zephyr SMF (State Machine Framework) and zbus for messaging
- Enabled boards: `raytac_an7002q_db_nrf5340_cpuapp`, `xiao_nrf54l15_nrf54l15_cpuapp`, `xiao_esp32s3_esp32s3_procpu`, `xiao_esp32c6_esp32c6_hpcore`, `xiao_ble`, `nrf54l15dk_nrf54l15_cpuapp`, `native_sim`

## Getting Started

1. Run `nix run .#init` after cloning to bootstrap west and sync modules
2. Enter `nix develop` for a dev shell with west, clang, and pre-commit hooks
3. cd into `app`
4. Build with board-specific command (see Board-Specific Build Commands below)
5. Flash with `-and-flash` variant of the build command
6. Connect PWA via Bluetooth at `https://maxhbr.github.io/zephyr-rail/`

## Project Structure & Module Organization

The Zephyr firmware lives in `app/` with build configs in `CMakeLists.txt`, `Kconfig`, and `prj.conf`. Runtime logic is grouped in `app/src/` (state machine `StateMachine.h`, stack control `Stack.h`, Bluetooth services `pwa_service.h`, shell interface `shell.h`) and the bundled PWA resides in `app/web_interface/`. Board overlays and the manifest live in `app/boards/` and `app/west.yml`. Upstream modules and the pinned Zephyr tree sit in `lib/`, `modules/`, and `zephyr/`, while hardware collateral is stored in `3d-print.scad/`, `electronics/`, and `docs/`. Use `scripts/init-and-chores.sh` to keep the manifest consistent with `flake.nix`.

## Build, Test, and Development Commands

- `nix run .#init` bootstraps west, fetches modules, and applies repo chores after cloning or pin bumps.
- `nix develop` opens a shell with west, clang, and pre-commit hooks configured.
- `nix run .#west -- build -b <board> app` configures and compiles into `app/build/` (use board-specific commands below).
- `nix run .#west -- flash -d app/build` loads the image onto the connected target.
- `nix run .#west -- twister -T app/tests` runs Zephyr tests; append `--device-testing` for hardware loops.

### Board-Specific Build Commands

Prefer these commands over generic west calls - they automatically apply board-specific shields and configurations.

**In `nix develop` shell** (cd into `app` first, then use):
- `west-xiao_nrf54l15_nrf54l15_cpuapp-build` / `west-xiao_nrf54l15_nrf54l15_cpuapp-build-and-flash`
- `west-xiao_ble-build` / `west-xiao_ble-build-and-flash`
- `west-xiao_esp32s3_esp32s3_procpu-build` / `west-xiao_esp32s3_esp32s3_procpu-build-and-flash`
- `west-xiao_esp32c6_esp32c6_hpcore-build` / `west-xiao_esp32c6_esp32c6_hpcore-build-and-flash`
- `west-nrf54l15dk_nrf54l15_cpuapp-build` / `west-nrf54l15dk_nrf54l15_cpuapp-build-and-flash`
- `west-raytac_an7002q_db_nrf5340_cpuapp-build` / `west-raytac_an7002q_db_nrf5340_cpuapp-build-and-flash`
- `west-native_sim-build` / `west-native_sim-build-and-run` (for unit tests)

**Outside dev shell** (wrap with nix run):
- `nix develop --command west-xiao_ble-build` (run from `app/` directory)

## Supported Boards

All board overlays are in `app/boards/`. Shield definitions are in `app/boards/shields/`.

- `raytac_an7002q_db_nrf5340_cpuapp` - Raytac nRF5340
- `xiao_nrf54l15_nrf54l15_cpuapp` - Seeed XIAO nRF54L15 (requires `gpio_stepper_rail` shield)
- `xiao_esp32s3_esp32s3_procpu` - Seeed XIAO ESP32S3 (requires `gpio_stepper_rail` shield)
- `xiao_esp32c6_esp32c6_hpcore` - Seeed XIAO ESP32C6 (requires `gpio_stepper_rail` shield)
- `xiao_ble` - Seeed XIAO nRF52840 (requires `gpio_stepper_rail` shield)
- `nrf54l15dk_nrf54l15_cpuapp` - nRF54L15 DK
- `native_sim` - For unit tests without hardware

The `gpio_stepper_rail` shield defines the stepper motor interface using the XIAO gpio connector pins (D3, D4, D5).

## Bluetooth Workflow

The firmware supports dual Bluetooth connections: one for the PWA control interface, one for Sony camera remote triggering. The `PwaService` class (`app/src/pwa_service.h`) exposes a GATT service with:
- **Command characteristic (write)**: Receives commands from the PWA
- **Status characteristic (read + notify)**: Sends status updates to connected PWA clients

Pairing and bonds are persisted via Zephyr settings (`CONFIG_BT_SETTINGS=y`), so camera reconnection is automatic. Configure device name in `app/prj.conf` (`CONFIG_BT_DEVICE_NAME="ZephyrRail"`). Max 2 concurrent connections are supported.

## Web Interface Development

The PWA is bundled in `app/web_interface/` with static HTML/CSS/JS. For local testing during development:
1. Serve the directory: `python -m http.server 8000 --directory app/web_interface`
2. Open browser to `http://localhost:8000`
3. Use WebBluetooth to connect to the device (device advertises as `ZephyrRail`)

The hosted PWA at `https://maxhbr.github.io/zephyr-rail/` is for production. Update it by pushing changes to the `gh-pages` branch.

## Debugging & Logging

Logging is enabled via `CONFIG_LOG=y` in `app/prj.conf`. Use shell for runtime debugging (`CONFIG_SHELL=y`). Common debug steps:
- View logs via UART console during flash/debug session
- Access shell over UART for commands (see `shell.h` for available commands)
- Enable additional debug levels by modifying `CONFIG_*_LOG_LEVEL_*` configs
- Use `native_sim` board for unit tests (`app/tests/state_machine/`) without physical hardware

## Coding Style & Naming Conventions

C++ sources follow clang-format via pre-commit; keep 2-space indents, PascalCase classes (`StateMachine`), and snake_case helpers. Mirror header/source names inside `app/src`, and prefer Zephyr-style prefixes (`k`, `CONFIG_`, `BT_`). Shell scripts must satisfy `shellcheck`, `.nix` files run through `nixfmt`, and typo checks rely on `_typos.toml`. Run `pre-commit run --all-files` before pushing.

Pre-commit hooks are provided via Nix and symlinked from `/nix/store`. The config includes checks for C++ formatting, shell scripts, and Nix files.

## Testing Guidelines

Place new suites next to the relevant feature or under `app/tests/<feature>/` with `sample.yaml` metadata describing required hardware. Name cases after scenarios (`stack_limits_wraparound`) and keep timing deterministic so `twister` can execute them headlessly. Existing tests: `app/tests/state_machine/`. For manual coverage, log the flashing commit, board revision, and any scope captures in the PR.

## Common Gotchas

- **West vs Nix**: Always prefer `nix run .#west -- <cmd>` over direct `west` commands to ensure environment consistency
- **Module sync**: After `flake.nix` pin bumps, run `nix run .#init` to update `app/west.yml` and refetch modules
- **Bluetooth bonding**: First-time pair requires manual intervention; bonds persist automatically thereafter
- **Memory constraints**: Watch stack usage on resource-constrained ESP32 boards when adding features
- **Hardware deps**: 24V power required for stepper motor; JTAG/SWD adapter needed for debugging (board-specific)

## Troubleshooting

- **Flash failures**: Check power supply, JTAG connection, and verify correct board is selected
- **West manifest sync errors**: Run `nix run .#init` to resync `app/west.yml` with `flake.nix` pins
- **Twister failures**: Check `sample.yaml` hardware requirements and use `--device-testing` for hardware-dependent tests
- **Bluetooth connection drops**: Verify `CONFIG_BT_MAX_CONN=2` in `prj.conf`, check power management settings
- **Build errors**: Ensure all modules are fetched (`nix run .#init`), check toolchain in `nix develop`

## Commit & Pull Request Guidelines

Follow the concise, present-tense style seen in `git log` (e.g., `cleanup`, `max paired to two`). Group work by topic and keep commits buildable. PRs should summarize intent, list validation commands (`west build`, `twister`, hardware checks), reference issues, and include screenshots or logs for UI or motion changes. Flag manifest bumps or wiring edits so reviewers know to rerun `nix run .#init`.

## Security & Configuration Tips

Store Bluetooth credentials and board IDs in Zephyr settings rather than code. Modify `app/west.yml` only through the helper script to preserve pins, and vet new modules for license compatibility. Document new sensors or regulators in `docs/` to keep firmware assumptions aligned with the electronics.