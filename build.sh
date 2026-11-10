#!/usr/bin/env bash
set -euo pipefail

case "$(uname -s)" in
  Linux*)
    echo "Detected Linux"
    exec ./install_local.sh
    ;;
  Darwin*)
    echo "Detected macOS"
    exec ./install_local_macos.sh
    ;;
  MINGW*|MSYS*|CYGWIN*)
    echo "Detected Windows-like shell"
    echo "Run PowerShell installer instead:"
    echo "  powershell -ExecutionPolicy Bypass -File .\\install_local.ps1"
    exit 1
    ;;
  *)
    echo "Unsupported platform: $(uname -s)"
    exit 1
    ;;
esac
