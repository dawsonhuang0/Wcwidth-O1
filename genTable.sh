#!/bin/bash
set -e

VERSION=1

echo "This script only requires sudo to install glibc's building dependencies."
SUDO=""
[ "$EUID" -ne 0 ] && SUDO="sudo"

echoo() {
  echo -e "\033[1m${1}\033[0m"
}

has() {
  command -v "$1" >/dev/null 2>&1
}

install() {
  if ! has "$1"; then
    $SUDO apt install "$1"
  fi
}

# ansi texts
ERROR="\033[48;5;9m\033[38;5;16m ERROR \033[49;39m"
WARNING="\033[48;5;11m\033[38;5;16m WARNING \033[49;39m"
SUCCESS="\033[48;5;10m\033[38;5;16m SUCCESS \033[49;39m"

if ! ldd --version 2>&1 | grep -qiE 'glibc|gnu libc'; then
  echoo "$WARNING\033[0m Please compile on a glibc-based Linux distro (e.g. Debian)."
  exit 1
fi

GLIBC_DIR="glibc-$VERSION"
BUILD_DIR="$GLIBC_DIR/build"
INSTALL_DIR="$HOME/glibc"

IS_INSTALLED=0

if [ -f "$INSTALL_DIR/.installed-$VERSION" ]; then
  IS_INSTALLED=1
fi

if [ "$IS_INSTALLED" -eq 0 ]; then
  if [ ! -f "$GLIBC_DIR/.cloned-$VERSION" ]; then
    # clone glibc
    echoo "Attempting to clone glibc from source..."

    if ! has git; then
      # install git
      echoo "Attempting to install git..."

      $SUDO apt update
      $SUDO apt install -y git

      # verify
      if ! has git; then
        echoo "$ERROR Failed to install git, please run:"
        echo "sudo && apt update && apt install -y git"
        exit 1
      fi

      echoo "Successfully installed git."
    fi

    if [ -d "$GLIBC_DIR" ]; then
      rm -rf "$GLIBC_DIR"
    fi

    git clone --depth=1 https://sourceware.org/git/glibc.git "$GLIBC_DIR"

    # verify
    if [ ! -d "$GLIBC_DIR" ]; then
      echoo "$ERROR Failed to clone glibc, please run:"
      echo "git clone --depth=1 https://sourceware.org/git/glibc.git"
      exit 1
    fi

    touch "$GLIBC_DIR/.cloned-$VERSION"
    echoo "Successfully cloned glibc."
  fi

  if [ -f "$BUILD_DIR/.built-$VERSION" ]; then
    cd "$BUILD_DIR"
  else
    # preparation
    echoo "Installing dependencies..."

    $SUDO apt update

    install git
    install gawk
    install bison
    install python3
    install gettext
    install texinfo
    if ! has gcc || ! has "g++"; then
      $SUDO apt install -y build-essential
    fi

    echoo "Dependencies are installed, prepare to build glibc..."

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    ../configure \
        --prefix="$INSTALL_DIR" \
        --disable-werror

    # build
    echoo "Building glibc..."
    make -j"$(nproc)"

    touch ".built-$VERSION"
    echoo "Build complete."
  fi

  # install
  echoo "Installing glibc..."

  if [ -d "$INSTALL_DIR" ]; then
    echoo "$WARNING $INSTALL_DIR already exists."
    read -p "Overwrite? [y/N]: " -n 1 -r
    echo  # Move to new line

    REPLY_LOWER=$(echo "$REPLY" | tr '[:upper:]' '[:lower:]')
    if [ ! "$REPLY_LOWER" = "y" ]; then
      echo "Aborted."
      exit 1
    fi

    rm -rf "$INSTALL_DIR"
  fi

  make install

  mkdir -p "$INSTALL_DIR/lib/locale"
  "$INSTALL_DIR/bin/localedef" -i C -f UTF-8 "$INSTALL_DIR/lib/locale/C.UTF-8"

  touch "$INSTALL_DIR/.installed-$VERSION"
  echoo "Successfully installed glibc."

  cd ../..
fi

# generate table
LOADER=$(find "$INSTALL_DIR/lib" -name "ld-*.so*" | head -n 1)

gcc extract.c -o extract \
  -I"$INSTALL_DIR/include" \
  -L"$INSTALL_DIR/lib" \
  -Wl,--dynamic-linker="$LOADER" \
  -lc

export LOCPATH="$INSTALL_DIR/lib/locale"
export LANG=C.UTF-8
export LC_ALL=C.UTF-8

"$LOADER" --library-path "$INSTALL_DIR/lib" ./extract

rm extract
