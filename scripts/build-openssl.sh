#!/usr/bin/env bash
# please run this in msys2 shell
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OPENSSL_DIR="$ROOT/third_party/openssl"
PREBUILT_DIR="$ROOT/third_party/prebuilt"

export QNX_HOST="${QNX_HOST:-/c/bbndk/10.3.1.995/host_10_3_1_12/win32/x86}"
export QNX_TARGET="${QNX_TARGET:-/c/bbndk/10.3.1.995/target_10_3_1_995/qnx6}"
export PATH="$QNX_HOST/usr/bin:$PATH"

JOBS="${JOBS:-8}"

patch_bss_log() {
  cat > "$OPENSSL_DIR/crypto/bio/bss_log.c" <<'EOF'
#include <openssl/bio.h>

const BIO_METHOD *BIO_s_log(void)
{
    return NULL;
}
EOF
}

build_one() {
  local name="$1"
  local cc="$2"
  local ar="$3"
  local ranlib="$4"
  local prefix="$PREBUILT_DIR/$name/openssl"

  echo "==> Building OpenSSL for $name"

  cd "$OPENSSL_DIR"

  make clean >/dev/null 2>&1 || true
  rm -f Makefile configdata.pm configdata.pm.bak

  patch_bss_log

  export CC="$cc"
  export AR="$ar"
  export RANLIB="$ranlib"

  ./Configure gcc no-shared no-tests no-asm no-ui-console \
    --prefix="$prefix"

  patch_bss_log

  make -j"$JOBS" build_libs

  mkdir -p "$prefix/include"
  mkdir -p "$prefix/lib"

  rm -rf "$prefix/include/openssl"
  cp -r include/openssl "$prefix/include/"
  cp libssl.a libcrypto.a "$prefix/lib/"

  echo "==> Done: $prefix"
  ls -lh "$prefix/lib/libssl.a" "$prefix/lib/libcrypto.a"
}

build_one simulator ntox86-gcc ntox86-ar ntox86-ranlib
build_one device ntoarmv7-gcc ntoarmv7-ar ntoarmv7-ranlib

echo "==> All done"