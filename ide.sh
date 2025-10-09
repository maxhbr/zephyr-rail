#!/usr/bin/env bash

set -euo pipefail
root="$(cd "$(dirname "$0")" && pwd)"
nix develop -c code "$root" & disown
