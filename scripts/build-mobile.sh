#!/usr/bin/env bash
# Local development builds; never uploads or uses upstream signing credentials.
set -euo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
platform="${1:-}"
mobile_build="${2:-$repo_root/../work/sonocraft-ios-xcode}"
case "$platform" in
  android)
    export ANDROID_HOME="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
    export JAVA_HOME="${JAVA_HOME:-/Applications/Android Studio.app/Contents/jbr/Contents/Home}"
    export CMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL:-3}"
    exec "$repo_root/mobile/Builds/Android/gradlew" -p "$repo_root/mobile/Builds/Android" :app:assemblePreview "-PsonocraftAbis=${SONOCRAFT_ABIS:-arm64-v8a,x86_64}"
    ;;
  ios)
    cmake_bin="${CMAKE:-cmake}"
    "$cmake_bin" -S "$repo_root" -B "$mobile_build" -G Xcode \
      -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator \
      -DCMAKE_OSX_ARCHITECTURES="$(uname -m)" -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
      -DUniversalBinary=OFF
    "$cmake_bin" --build "$mobile_build" --config Debug --target SonoBus_Standalone --parallel 3 -- CODE_SIGNING_ALLOWED=NO
    ;;
  *) echo "Usage: $0 android | ios [build-directory]" >&2; exit 2 ;;
esac
