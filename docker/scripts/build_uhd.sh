#!/bin/bash

# SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
# SPDX-License-Identifier: BSD-3-Clause-Open-MPI


set -e # stop executing after error

# Exit 0 when UHD should be patched for Fedora 43+ with GCC 15+ (fixed-width ints need <cstdint>).
# Runs in a subshell so sourcing /etc/os-release does not alter the caller's environment.
uhd_should_apply_fedora43_gcc15_stdint_patches() {
    (
        [[ -r /etc/os-release ]] || exit 1
        # shellcheck source=/dev/null
        . /etc/os-release
        [[ "${ID}" == "fedora" ]] || exit 1

        local vid="${VERSION_ID%%.*}"
        [[ "${vid}" =~ ^[0-9]+$ ]] || exit 1
        # Note: 10#$vid (with $) is required; 10#vid is parsed as an invalid literal, not variable vid.
        ((10#$vid >= 43)) || exit 1

        local gcc_major
        if command -v gcc >/dev/null 2>&1; then
            gcc_major=$(gcc -dumpversion 2>/dev/null | cut -d. -f1)
        elif command -v g++ >/dev/null 2>&1; then
            gcc_major=$(g++ -dumpversion 2>/dev/null | cut -d. -f1)
        else
            exit 1
        fi
        [[ "${gcc_major}" =~ ^[0-9]+$ ]] || exit 1
        ((10#$gcc_major >= 15)) || exit 1
        exit 0
    )
}

# Insert #include <cstdint> after a matching #include line in known UHD 4.7.x headers (GCC 15+ / Fedora).
# $1: extracted UHD tree root (e.g. /tmp/uhd-4.7.0.0)
uhd_apply_fedora43_gcc15_plus_stdint_patches() {
    local uhd_root="${1:?}"
    local -a fixes=(
        'host/include/uhd/features/ref_clk_calibration_iface.hpp|^#include <memory>$'
        'host/lib/include/uhdlib/usrp/dboard/fbx/fbx_constants.hpp|^#include <vector>$'
    )
    local fix rel pattern path
    for fix in "${fixes[@]}"; do
        rel="${fix%%|*}"
        pattern="${fix#*|}"
        path="${uhd_root}/${rel}"
        [[ -f "${path}" ]] || continue
        grep -qE '\<(uint8_t|uint16_t|uint32_t|uint64_t|int8_t|int16_t|int32_t|int64_t)\>' "${path}" || continue
        grep -qF '#include <cstdint>' "${path}" && continue
        sed -i "/${pattern}/a#include <cstdint>" "${path}"
    done
}

main() {
    # Check number of args
    if [ $# -lt 1 ] || [ $# -gt 3 ]; then
        echo >&2 "Illegal number of parameters"
        echo >&2 "Run like this: \"./build_uhd.sh <uhd_version> [<arch> [<ncores>]]\" where arch is any gcc/clang compatible -march and ncores could be any number or empty for all"
        exit 1
    fi

    local uhd_version=$1
    local arch="${2:--march=native}"
    local ncores="${3:-$(nproc)}"

    cd /tmp
    curl -L "https://github.com/EttusResearch/uhd/archive/refs/tags/v${uhd_version}.tar.gz" | tar xzf -

    local uhd_root="/tmp/uhd-${uhd_version}"
    if uhd_should_apply_fedora43_gcc15_stdint_patches; then
        local gcc_v
        gcc_v=$(gcc -dumpversion 2>/dev/null || g++ -dumpversion 2>/dev/null || echo unknown)
        echo "Applying UHD <cstdint> source patches (Fedora 43+, GCC ${gcc_v})"
        uhd_apply_fedora43_gcc15_plus_stdint_patches "${uhd_root}"
    fi

    cd uhd*"${uhd_version}"/host && mkdir -p build && cd build
    cmake \
        -DCMAKE_INSTALL_PREFIX=/opt/uhd/"${uhd_version}" \
        -DENABLE_LIBUHD=On \
        -DENABLE_PYTHON_API=Off \
        -DENABLE_EXAMPLES=Off \
        -DENABLE_TESTS=Off \
        -DCMAKE_CXX_FLAGS="${arch}" ..
    cmake --build . -- -j"${ncores}"
    cmake --install .

    rm -Rf /tmp/uhd*"${uhd_version}"

}

main "$@"
