#!/bin/bash
#
# scripts/build_sun7i_android.sh
# (c) Copyright 2013
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# James Deng <csjamesdeng@allwinnertech.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

set -e

function build_buildroot()
{
    local tooldir="${LICHEE_BR_OUT}/external-toolchain"
    mkdir -p ${tooldir}
    if [ -f ${tooldir}/.installed ] ; then
        printf "external toolchain has been installed\n"
    else
        printf "installing external toolchain\n"
        printf "please wait for a few minutes ...\n"
        tar --strip-components=1 \
            -jxf ${LICHEE_BR_DIR}/dl/gcc-linaro.tar.bz2 \
            -C ${tooldir}
        [ $? -eq 0 ] && touch ${tooldir}/.installed
    fi

    export PATH=${tooldir}/bin:${PATH}
}

case "$1" in
    clean)
        rm -rf ${LICHEE_BR_OUT}
        ;;
    *)
        build_buildroot
        ;;
esac

