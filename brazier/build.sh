# SPDX-License-Identifier: LGPL-3.0-or-later
set -e

PROJECT_NAME="brazier"
BUILD_DIR="build"
CONFIG="Release"
SOURCE_DIR="$(pwd)"
VCPKG_TOOLCHAIN="${VCPKG_TOOLCHAIN:-$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake}"
if [ ! -f "$VCPKG_TOOLCHAIN" ]; then
    echo "vcpkg toolset not found: $VCPKG_TOOLCHAIN"
    exit 1
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ -f "$PROJECT_NAME" ]; then
  mv "./$PROJECT_NAME" "./${PROJECT_NAME}_OLD"
fi

cmake -DCMAKE_TOOLCHAIN_FILE="$VCPKG_TOOLCHAIN" -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=$CONFIG "$SOURCE_DIR"
cmake --build . --config $CONFIG
if [ $? -ne 0 ]; then
  echo "Build failed"
  exit
fi

if [ -d "$SOURCE_DIR" ]; then
  mkdir -p "$SOURCE_DIR/build/$CONFIG"
  cp "$SOURCE_DIR/.env" "$SOURCE_DIR/build/$CONFIG/"
  if [ $? -eq 0 ]; then
    echo "Copied .env to $CONFIG."
  fi
fi

if [ -f "$PROJECT_NAME" ]; then
    echo "Launch $PROJECT_NAME..."
    ./"$PROJECT_NAME"
else
    echo "Binary $PROJECT_NAME not found"
fi

cd "$SOURCE_DIR"
