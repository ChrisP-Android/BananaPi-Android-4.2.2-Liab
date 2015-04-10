cmd_debianutils/run_parts.o := arm-linux-gnueabi-gcc -Wp,-MD,debianutils/.run_parts.o.d   -std=gnu99 -Iinclude -Ilibbb  -include include/autoconf.h -D_GNU_SOURCE -DNDEBUG -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D"BB_VER=KBUILD_STR(1.23.1)" -DBB_BT=AUTOCONF_TIMESTAMP  -Wall -Wshadow -Wwrite-strings -Wundef -Wstrict-prototypes -Wunused -Wunused-parameter -Wunused-function -Wunused-value -Wmissing-prototypes -Wmissing-declarations -Wno-format-security -Wdeclaration-after-statement -Wold-style-definition -fno-builtin-strlen -finline-limit=0 -fomit-frame-pointer -ffunction-sections -fdata-sections -fno-guess-branch-probability -funsigned-char -static-libgcc -falign-functions=1 -falign-jumps=1 -falign-labels=1 -falign-loops=1 -fno-unwind-tables -fno-asynchronous-unwind-tables -Os     -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(run_parts)"  -D"KBUILD_MODNAME=KBUILD_STR(run_parts)" -c -o debianutils/run_parts.o debianutils/run_parts.c

deps_debianutils/run_parts.o := \
  debianutils/run_parts.c \
    $(wildcard include/config/feature/run/parts/long/options.h) \
    $(wildcard include/config/feature/run/parts/fancy.h) \
  include/libbb.h \
    $(wildcard include/config/feature/shadowpasswds.h) \
    $(wildcard include/config/use/bb/shadow.h) \
    $(wildcard include/config/selinux.h) \
    $(wildcard include/config/feature/utmp.h) \
    $(wildcard include/config/locale/support.h) \
    $(wildcard include/config/use/bb/pwd/grp.h) \
    $(wildcard include/config/lfs.h) \
    $(wildcard include/config/feature/buffers/go/on/stack.h) \
    $(wildcard include/config/feature/buffers/go/in/bss.h) \
    $(wildcard include/config/feature/verbose.h) \
    $(wildcard include/config/feature/ipv6.h) \
    $(wildcard include/config/feature/seamless/xz.h) \
    $(wildcard include/config/feature/seamless/lzma.h) \
    $(wildcard include/config/feature/seamless/bz2.h) \
    $(wildcard include/config/feature/seamless/gz.h) \
    $(wildcard include/config/feature/seamless/z.h) \
    $(wildcard include/config/feature/check/names.h) \
    $(wildcard include/config/feature/prefer/applets.h) \
    $(wildcard include/config/long/opts.h) \
    $(wildcard include/config/feature/getopt/long.h) \
    $(wildcard include/config/feature/pidfile.h) \
    $(wildcard include/config/feature/syslog.h) \
    $(wildcard include/config/feature/individual.h) \
    $(wildcard include/config/echo.h) \
    $(wildcard include/config/printf.h) \
    $(wildcard include/config/test.h) \
    $(wildcard include/config/kill.h) \
    $(wildcard include/config/chown.h) \
    $(wildcard include/config/ls.h) \
    $(wildcard include/config/xxx.h) \
    $(wildcard include/config/route.h) \
    $(wildcard include/config/feature/hwib.h) \
    $(wildcard include/config/desktop.h) \
    $(wildcard include/config/feature/crond/d.h) \
    $(wildcard include/config/use/bb/crypt.h) \
    $(wildcard include/config/feature/adduser/to/group.h) \
    $(wildcard include/config/feature/del/user/from/group.h) \
    $(wildcard include/config/ioctl/hex2str/error.h) \
    $(wildcard include/config/feature/editing.h) \
    $(wildcard include/config/feature/editing/history.h) \
    $(wildcard include/config/feature/editing/savehistory.h) \
    $(wildcard include/config/feature/tab/completion.h) \
    $(wildcard include/config/feature/username/completion.h) \
    $(wildcard include/config/feature/editing/vi.h) \
    $(wildcard include/config/feature/editing/save/on/exit.h) \
    $(wildcard include/config/pmap.h) \
    $(wildcard include/config/feature/show/threads.h) \
    $(wildcard include/config/feature/ps/additional/columns.h) \
    $(wildcard include/config/feature/topmem.h) \
    $(wildcard include/config/feature/top/smp/process.h) \
    $(wildcard include/config/killall.h) \
    $(wildcard include/config/pgrep.h) \
    $(wildcard include/config/pkill.h) \
    $(wildcard include/config/pidof.h) \
    $(wildcard include/config/sestatus.h) \
    $(wildcard include/config/unicode/support.h) \
    $(wildcard include/config/feature/mtab/support.h) \
    $(wildcard include/config/feature/clean/up.h) \
    $(wildcard include/config/feature/devfs.h) \
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
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/ctype.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/xlocale.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/dirent.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/dirent.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/errno.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/errno.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/linux/errno.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/asm/errno.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/asm-generic/errno.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/asm-generic/errno-base.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/fcntl.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/fcntl.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/types.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/time.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/select.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/select.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/sigset.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/time.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/sysmacros.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/pthreadtypes.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/uio.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/stat.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/inttypes.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/netdb.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/netinet/in.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/socket.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/uio.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/socket.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/sockaddr.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/asm/socket.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/asm/sockios.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/in.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/rpc/netdb.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/siginfo.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/netdb.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/setjmp.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/setjmp.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/signal.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/signum.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/sigaction.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/sigcontext.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/asm/sigcontext.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/sigstack.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/ucontext.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/procfs.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/time.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/user.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/sigthread.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/stdio.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/libio.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/_G_config.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/wchar.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../lib/gcc/arm-linux-gnueabi/4.6.3/include/stdarg.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/sys_errlist.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/stdlib.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/waitflags.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/waitstatus.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/alloca.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/string.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/libgen.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/poll.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/poll.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/poll.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/ioctl.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/ioctls.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/asm/ioctls.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/asm-generic/ioctls.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/linux/ioctl.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/asm/ioctl.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/asm-generic/ioctl.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/ioctl-types.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/ttydefaults.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/mman.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/mman.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/stat.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/wait.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/resource.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/resource.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/termios.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/termios.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/param.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/linux/param.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/asm/param.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/pwd.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/grp.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/mntent.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/paths.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/sys/statfs.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/statfs.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/utmp.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arm-linux-gnueabi/bits/utmp.h \
  /home/wbail/bananapi/ANDROID/lichee/out/android/common/buildroot/external-toolchain/bin/../arm-linux-gnueabi/libc/usr/include/arpa/inet.h \
  include/pwd_.h \
  include/grp_.h \
  include/shadow_.h \
  include/xatonum.h \

debianutils/run_parts.o: $(deps_debianutils/run_parts.o)

$(deps_debianutils/run_parts.o):
