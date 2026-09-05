#!/usr/bin/env bash
#
# Download and install the TI ARM Code Generation Tools (armcl) unattended.
#
#   tools/ci/install-ti-cgt.sh [<prefix>]        default prefix: ~/ti/ti-cgt-arm_<version>
#
# Idempotent: if <prefix> already holds bin/armcl (e.g. restored from a CI cache) nothing
# is downloaded. Always ends by printing TI_CGT_ARM_ROOT=<root>, and appends the same
# line to $GITHUB_ENV when running under GitHub Actions, so later steps see it.
#
# Environment overrides:
#   TI_CGT_ARM_VERSION   default 20.2.7.LTS
#   TI_CGT_ARM_URL       installer URL (default: TI's download server for that version)
#   TI_CGT_ARM_SHA256    if set, the downloaded installer must match it
#   TI_CGT_ARM_RTS       run-time library variant to pre-build; must match the flags in
#                        cmake/toolchain-ti-armcl.cmake. Empty disables pre-building.
#
# The installer is a BitRock package; "--mode unattended" needs no display and no input.
set -euo pipefail

: "${TI_CGT_ARM_VERSION:=20.2.7.LTS}"
: "${TI_CGT_ARM_URL:=https://dr-download.ti.com/software-development/ide-configuration-compiler-or-debugger/MD-sDOoXkUcde/${TI_CGT_ARM_VERSION}/ti_cgt_tms470_${TI_CGT_ARM_VERSION}_linux-x64_installer.bin}"

prefix=${1:-$HOME/ti/ti-cgt-arm_${TI_CGT_ARM_VERSION}}

locate_root() {
    # The installer may or may not nest a versioned directory under the prefix.
    local armcl
    armcl=$(find "$prefix" -type f -name armcl -path '*/bin/*' 2>/dev/null | head -n 1 || true)
    [[ -n $armcl ]] || return 1
    cd "$(dirname "$armcl")/.." && pwd
}

if root=$(locate_root); then
    echo "TI ARM CGT already installed: $root"
else
    tmp=$(mktemp -d)
    trap 'rm -rf "$tmp"' EXIT

    echo "Downloading $TI_CGT_ARM_URL"
    curl -fsSL --retry 3 --retry-delay 5 -o "$tmp/installer.bin" "$TI_CGT_ARM_URL"

    if [[ -n ${TI_CGT_ARM_SHA256:-} ]]; then
        echo "$TI_CGT_ARM_SHA256  $tmp/installer.bin" | sha256sum -c -
    fi

    chmod +x "$tmp/installer.bin"
    mkdir -p "$prefix"
    echo "Installing to $prefix"
    "$tmp/installer.bin" --mode unattended --unattendedmodeui none --prefix "$prefix"

    root=$(locate_root) || { echo "install-ti-cgt.sh: no bin/armcl under $prefix after install" >&2; exit 1; }
fi

# TI ships the run-time libraries as source and the linker builds the variant a
# project needs on first use ("warning #10366-D: automatic library build"), which
# takes minutes. Build it here instead, while the install is still being populated,
# so it lands in the CI cache and no later build pays for it. Best-effort: if this
# does not work the linker still builds the library itself.
rts=${TI_CGT_ARM_RTS-rtsv7R4_A_be_v3D16_eabi.lib}
if [[ -n $rts && ! -f $root/lib/$rts && -x $root/lib/mklib ]]; then
    echo "Pre-building run-time library $rts"
    if (cd "$root/lib" && ./mklib --pattern="$rts" --index="$root/lib/libc.a"); then
        echo "Pre-built $rts"
    else
        echo "warning: could not pre-build $rts; the linker will build it on first use" >&2
    fi
fi

echo "armcl: $("$root/bin/armcl" --compiler_revision 2>/dev/null | head -n 1)"
echo "TI_CGT_ARM_ROOT=$root"
if [[ -n ${GITHUB_ENV:-} ]]; then
    echo "TI_CGT_ARM_ROOT=$root" >> "$GITHUB_ENV"
fi
