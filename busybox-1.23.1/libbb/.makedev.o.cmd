cmd_libbb/makedev.o := arm-linux-gnueabi-gcc -Wp,-MD,libbb/.makedev.o.d   -std=gnu99 -Iinclude -Ilibbb  -include include/autoconf.h -D_GNU_SOURCE -DNDEBUG -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D"BB_VER=KBUILD_STR(1.23.1)" -DBB_BT=AUTOCONF_TIMESTAMP  -Wall -Wshadow -Wwrite-strings -Wundef -Wstrict-prototypes -Wunused -Wunused-parameter -Wunused-function -Wunused-value -Wmissing-prototypes -Wmissing-declarations -Wno-format-security -Wdeclaration-after-statement -Wold-style-definition -fno-builtin-strlen -finline-limit=0 -fomit-frame-pointer -ffunction-sections -fdata-sections -fno-guess-branch-probability -funsigned-char -static-libgcc -falign-functions=1 -falign-jumps=1 -falign-labels=1 -falign-loops=1 -fno-unwind-tables -fno-asynchronous-unwind-tables -Os     -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(makedev)"  -D"KBUILD_MODNAME=KBUILD_STR(makedev)" -c -o libbb/makedev.o libbb/makedev.c

deps_libbb/makedev.o := \
  libbb/makedev.c \
  include/platform.h \
    $(wildcard include/config/werror.h) \
    $(wildcard include/config/big/endian.h) \
    $(wildcard include/config/little/endian.h) \
    $(wildcard include/config/nommu.h) \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../lib/gcc/arm-linux-gnueabi/4.6.3/include-fixed/limits.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../lib/gcc/arm-linux-gnueabi/4.6.3/include-fixed/syslimits.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/limits.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/features.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/predefs.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/cdefs.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/wordsize.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/gnu/stubs.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/posix1_lim.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/local_lim.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/linux/limits.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/posix2_lim.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/xopen_lim.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/stdio_lim.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/byteswap.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/byteswap.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/endian.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/endian.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../lib/gcc/arm-linux-gnueabi/4.6.3/include/stdint.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/stdint.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/wchar.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../lib/gcc/arm-linux-gnueabi/4.6.3/include/stdbool.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/unistd.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/posix_opt.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/environments.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/types.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/typesizes.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../lib/gcc/arm-linux-gnueabi/4.6.3/include/stddef.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/confname.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/getopt.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/sysmacros.h \

libbb/makedev.o: $(deps_libbb/makedev.o)

$(deps_libbb/makedev.o):
