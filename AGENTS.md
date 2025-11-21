# Repository Guidelines

## Project Structure & Module Organization
The Zephyr firmware lives in `app/` with build configs in `CMakeLists.txt`, `Kconfig`, and `prj.conf`. Runtime logic is grouped in `app/src/` (state machine, stack control, Bluetooth services) and the bundled PWA resides in `app/web_interface/`. Board overlays and the manifest live in `app/boards/` and `app/west.yml`. Upstream modules and the pinned Zephyr tree sit in `lib/`, `modules/`, and `zephyr/`, while hardware collateral is stored in `3d-print.scad/`, `electronics/`, and `docs/`. Use `scripts/init-and-chores.sh` to keep the manifest consistent with `flake.nix`.

## Build, Test, and Development Commands
- `nix run .#init` bootstraps west, fetches modules, and applies repo chores after cloning or pin bumps.
- `nix develop` opens a shell with west, clang, and pre-commit hooks configured.
- `nix run .#west -- build -b stm32h7b3i_dk app` (swap the board name as needed) configures and compiles into `app/build/`.
- `nix run .#west -- flash -d app/build` loads the image onto the connected target.
- `nix run .#west -- twister -T app/tests` runs Zephyr tests; append `--device-testing` for hardware loops.

## Coding Style & Naming Conventions
C++ sources follow clang-format via pre-commit; keep 2-space indents, PascalCase classes (`StateMachine`), and snake_case helpers. Mirror header/source names inside `app/src`, and prefer Zephyr-style prefixes (`k`, `CONFIG_`, `BT_`). Shell scripts must satisfy `shellcheck`, `.nix` files run through `nixfmt`, and typo checks rely on `_typos.toml`. Run `pre-commit run --all-files` before pushing.

## Testing Guidelines
Place new suites next to the relevant feature or under `app/tests/<feature>/` with `sample.yaml` metadata describing required hardware. Name cases after scenarios (`stack_limits_wraparound`) and keep timing deterministic so `twister` can execute them headlessly. For manual coverage, log the flashing commit, board revision, and any scope captures in the PR.

## Commit & Pull Request Guidelines
Follow the concise, present-tense style seen in `git log` (e.g., `cleanup`, `max paired to two`). Group work by topic and keep commits buildable. PRs should summarize intent, list validation commands (`west build`, `twister`, hardware checks), reference issues, and include screenshots or logs for UI or motion changes. Flag manifest bumps or wiring edits so reviewers know to rerun `nix run .#init`.

## Security & Configuration Tips
Store Bluetooth credentials and board IDs in Zephyr settings rather than code. Modify `app/west.yml` only through the helper script to preserve pins, and vet new modules for license compatibility. Document new sensors or regulators in `docs/` to keep firmware assumptions aligned with the electronics.
