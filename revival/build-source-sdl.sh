#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
ROOT=$(pwd)
SDL_REV=a1f2392fd46137815ada13ff9fdbae21219f49e5
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
PREFIX=/tmp/psxprv-sdl
git init -q "$WORK/sdl"
cd "$WORK/sdl"
git remote add origin https://github.com/pspdev/SDL-legacy.git
git fetch --depth=1 origin "$SDL_REV"
git checkout --detach FETCH_HEAD
test "$(git rev-parse HEAD)" = "$SDL_REV"
printf 'SDL source revision: %s\n' "$SDL_REV"
git apply --check --unidiff-zero "$ROOT/revival/patches/sdl-pspsdk-types.patch"
git apply --unidiff-zero "$ROOT/revival/patches/sdl-pspsdk-types.patch"
git diff --check
git diff
# Regenerate configuration with the installed host tools.
sh autogen.sh
CC=psp-gcc AR=psp-gcc-ar RANLIB=psp-gcc-ranlib \
CFLAGS="-O2 -std=gnu99 -fcommon -G0 -DICONV_INBUF_NONCONST" \
LDFLAGS="-L$(psp-config --pspsdk-path)/lib" LIBS="-lc -lpspuser" \
./configure --host=psp --prefix="$PREFIX" --disable-shared --enable-static --disable-video-opengl
make -k -j2
make install
test -s "$PREFIX/lib/libSDL.a"
test -f "$PREFIX/include/SDL/SDL.h"
sha256sum "$PREFIX/lib/libSDL.a"
