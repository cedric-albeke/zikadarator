#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build/verify-macos}"
CONFIGURATION="${CONFIGURATION:-Release}"
PLUGINVAL_VERSION="${PLUGINVAL_VERSION:-v1.0.4}"
PLUGINVAL_STRICTNESS="${PLUGINVAL_STRICTNESS:-5}"
PLUGINVAL_TIMEOUT_MS="${PLUGINVAL_TIMEOUT_MS:-60000}"
VST3_VALIDATOR_PATH="${VST3_VALIDATOR_PATH:-}"
OSX_ARCHITECTURES="${OSX_ARCHITECTURES:-x86_64;arm64}"
OSX_DEPLOYMENT_TARGET="${OSX_DEPLOYMENT_TARGET:-11.0}"
AUVAL_TYPE="${AUVAL_TYPE:-aufx}"
AUVAL_SUBTYPE="${AUVAL_SUBTYPE:-Zkfx}"
AUVAL_MANUFACTURER="${AUVAL_MANUFACTURER:-Zika}"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

step() {
  printf '\n==> %s\n' "$1"
}

step "Validate release metadata"
postinstall_path="packaging/macos/scripts/postinstall"
postinstall_stage="$(git ls-files --stage -- "$postinstall_path")"
if [[ ! "$postinstall_stage" =~ ^100755[[:space:]] ]]; then
  echo "$postinstall_path must be tracked as executable mode 100755. Found: $postinstall_stage" >&2
  exit 1
fi

if [[ ! -x "$postinstall_path" ]]; then
  echo "$postinstall_path must be executable in the macOS checkout." >&2
  exit 1
fi

step "Configure CMake"
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE="$CONFIGURATION" \
  -DCMAKE_OSX_ARCHITECTURES="$OSX_ARCHITECTURES" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$OSX_DEPLOYMENT_TARGET"

step "Run source smoke tests"
node scripts/source-smoke-tests.mjs

step "Build regression test targets"
cmake --build "$BUILD_DIR" --config "$CONFIGURATION" --target ZikadaEngineTests
cmake --build "$BUILD_DIR" --config "$CONFIGURATION" --target ZikadaProcessorTests

step "Run CTest"
ctest --test-dir "$BUILD_DIR" -C "$CONFIGURATION" --output-on-failure

step "Build plugin artifacts"
cmake --build "$BUILD_DIR" --config "$CONFIGURATION" --target ZikadaFX_All

vst3_path="$BUILD_DIR/ZikadaFX_artefacts/$CONFIGURATION/VST3/ZIKADARATOR.vst3"
au_path="$BUILD_DIR/ZikadaFX_artefacts/$CONFIGURATION/AU/ZIKADARATOR.component"
standalone_path="$BUILD_DIR/ZikadaFX_artefacts/$CONFIGURATION/Standalone/ZIKADARATOR.app"

for artifact in "$vst3_path" "$au_path" "$standalone_path"; do
  if [[ ! -e "$artifact" ]]; then
    echo "Expected artifact was not found: $artifact" >&2
    exit 1
  fi
done

au_install_dir="$HOME/Library/Audio/Plug-Ins/Components"
au_installed_path="$au_install_dir/ZIKADARATOR.component"
au_backup_path=""
au_installed_by_script=0

cleanup_au_validation_install() {
  if [[ "$au_installed_by_script" == "1" ]]; then
    rm -rf "$au_installed_path"
  fi

  if [[ -n "$au_backup_path" && -e "$au_backup_path" ]]; then
    mv "$au_backup_path" "$au_installed_path"
  fi
}
trap cleanup_au_validation_install EXIT

step "Install AU for validation"
mkdir -p "$au_install_dir"
if [[ -e "$au_installed_path" ]]; then
  au_backup_path="$au_install_dir/ZIKADARATOR.component.zikada-validate-backup-$$"
  mv "$au_installed_path" "$au_backup_path"
fi
cp -R "$au_path" "$au_installed_path"
au_installed_by_script=1
killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true

step "Prepare pluginval $PLUGINVAL_VERSION"
pluginval_dir="build/pluginval-$PLUGINVAL_VERSION-macos"
pluginval_zip="build/pluginval-$PLUGINVAL_VERSION-macos.zip"
pluginval_exe="$pluginval_dir/pluginval.app/Contents/MacOS/pluginval"

if command -v pluginval >/dev/null 2>&1; then
  pluginval_exe="$(command -v pluginval)"
else
  if [[ ! -x "$pluginval_exe" ]]; then
    mkdir -p build
    curl -L "https://github.com/Tracktion/pluginval/releases/download/$PLUGINVAL_VERSION/pluginval_macOS.zip" -o "$pluginval_zip"
    rm -rf "$pluginval_dir"
    unzip -q "$pluginval_zip" -d "$pluginval_dir"
    chmod +x "$pluginval_exe"
  fi
fi

if [[ ! -x "$pluginval_exe" ]]; then
  echo "pluginval executable was not found after setup: $pluginval_exe" >&2
  exit 1
fi

step "Run pluginval level $PLUGINVAL_STRICTNESS"
pluginval_results="build/pluginval-results"
mkdir -p "$pluginval_results"

pluginval_base_args=(
  --strictness-level "$PLUGINVAL_STRICTNESS"
  --timeout-ms "$PLUGINVAL_TIMEOUT_MS"
  --output-dir "$pluginval_results"
)

pluginval_vst3_args=("${pluginval_base_args[@]}"
  --output-filename "ZIKADARATOR-macos-vst3-pluginval-level${PLUGINVAL_STRICTNESS}.log")

if [[ -n "$VST3_VALIDATOR_PATH" ]]; then
  if [[ ! -x "$VST3_VALIDATOR_PATH" ]]; then
    echo "VST3 validator path does not exist or is not executable: $VST3_VALIDATOR_PATH" >&2
    exit 1
  fi

  pluginval_vst3_args+=(--vst3validator "$VST3_VALIDATOR_PATH")
else
  echo "No VST3 validator path supplied; pluginval will skip Steinberg's VST3 validator subtest."
fi

pluginval_vst3_args+=(--validate "$vst3_path")
"$pluginval_exe" "${pluginval_vst3_args[@]}"

step "Run pluginval AU level $PLUGINVAL_STRICTNESS"
pluginval_au_args=("${pluginval_base_args[@]}"
  --output-filename "ZIKADARATOR-macos-au-pluginval-level${PLUGINVAL_STRICTNESS}.log"
  --validate "$au_installed_path")
"$pluginval_exe" "${pluginval_au_args[@]}"

step "Run auval"
if ! command -v auval >/dev/null 2>&1; then
  echo "auval was not found on this macOS runner." >&2
  exit 1
fi
auval -v "$AUVAL_TYPE" "$AUVAL_SUBTYPE" "$AUVAL_MANUFACTURER"

printf '\nmacOS validation completed successfully.\n'
printf 'VST3: %s\n' "$vst3_path"
printf 'AU: %s\n' "$au_path"
printf 'Standalone: %s\n' "$standalone_path"
printf 'pluginval VST3 log: %s/%s\n' "$pluginval_results" "ZIKADARATOR-macos-vst3-pluginval-level${PLUGINVAL_STRICTNESS}.log"
printf 'pluginval AU log: %s/%s\n' "$pluginval_results" "ZIKADARATOR-macos-au-pluginval-level${PLUGINVAL_STRICTNESS}.log"
