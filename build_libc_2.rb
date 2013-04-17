# Define the common source files for all the libc instances
# =========================================================


syscall_src = []
syscall_src << "arch-x86/syscalls/_exit.S"
syscall_src << "arch-x86/syscalls/_exit_thread.S"
syscall_src << "arch-x86/syscalls/__fork.S"
syscall_src << "arch-x86/syscalls/_waitpid.S"
syscall_src << "arch-x86/syscalls/__waitid.S"
syscall_src << "arch-x86/syscalls/__sys_clone.S"
syscall_src << "arch-x86/syscalls/execve.S"
syscall_src << "arch-x86/syscalls/__setuid.S"
syscall_src << "arch-x86/syscalls/getuid.S"
syscall_src << "arch-x86/syscalls/getgid.S"
syscall_src << "arch-x86/syscalls/geteuid.S"
syscall_src << "arch-x86/syscalls/getegid.S"
syscall_src << "arch-x86/syscalls/getresuid.S"
syscall_src << "arch-x86/syscalls/getresgid.S"
syscall_src << "arch-x86/syscalls/gettid.S"
syscall_src << "arch-x86/syscalls/readahead.S"
syscall_src << "arch-x86/syscalls/getgroups.S"
syscall_src << "arch-x86/syscalls/getpgid.S"
syscall_src << "arch-x86/syscalls/getppid.S"
syscall_src << "arch-x86/syscalls/setsid.S"
syscall_src << "arch-x86/syscalls/setgid.S"
syscall_src << "arch-x86/syscalls/__setreuid.S"
syscall_src << "arch-x86/syscalls/__setresuid.S"
syscall_src << "arch-x86/syscalls/setresgid.S"
syscall_src << "arch-x86/syscalls/__brk.S"
syscall_src << "arch-x86/syscalls/kill.S"
syscall_src << "arch-x86/syscalls/tkill.S"
syscall_src << "arch-x86/syscalls/tgkill.S"
syscall_src << "arch-x86/syscalls/__ptrace.S"
syscall_src << "arch-x86/syscalls/__set_thread_area.S"
syscall_src << "arch-x86/syscalls/__getpriority.S"
syscall_src << "arch-x86/syscalls/setpriority.S"
syscall_src << "arch-x86/syscalls/setrlimit.S"
syscall_src << "arch-x86/syscalls/getrlimit.S"
syscall_src << "arch-x86/syscalls/getrusage.S"
syscall_src << "arch-x86/syscalls/setgroups.S"
syscall_src << "arch-x86/syscalls/setpgid.S"
syscall_src << "arch-x86/syscalls/setregid.S"
syscall_src << "arch-x86/syscalls/chroot.S"
syscall_src << "arch-x86/syscalls/prctl.S"
syscall_src << "arch-x86/syscalls/capget.S"
syscall_src << "arch-x86/syscalls/capset.S"
syscall_src << "arch-x86/syscalls/sigaltstack.S"
syscall_src << "arch-x86/syscalls/acct.S"
syscall_src << "arch-x86/syscalls/read.S"
syscall_src << "arch-x86/syscalls/write.S"
syscall_src << "arch-x86/syscalls/pread64.S"
syscall_src << "arch-x86/syscalls/pwrite64.S"
syscall_src << "arch-x86/syscalls/__open.S"
syscall_src << "arch-x86/syscalls/__openat.S"
syscall_src << "arch-x86/syscalls/close.S"
syscall_src << "arch-x86/syscalls/lseek.S"
syscall_src << "arch-x86/syscalls/__llseek.S"
syscall_src << "arch-x86/syscalls/getpid.S"
syscall_src << "arch-x86/syscalls/__mmap2.S"
syscall_src << "arch-x86/syscalls/munmap.S"
syscall_src << "arch-x86/syscalls/mremap.S"
syscall_src << "arch-x86/syscalls/msync.S"
syscall_src << "arch-x86/syscalls/mprotect.S"
syscall_src << "arch-x86/syscalls/madvise.S"
syscall_src << "arch-x86/syscalls/mlock.S"
syscall_src << "arch-x86/syscalls/munlock.S"
syscall_src << "arch-x86/syscalls/mincore.S"
syscall_src << "arch-x86/syscalls/__ioctl.S"
syscall_src << "arch-x86/syscalls/readv.S"
syscall_src << "arch-x86/syscalls/writev.S"
syscall_src << "arch-x86/syscalls/__fcntl.S"
syscall_src << "arch-x86/syscalls/flock.S"
syscall_src << "arch-x86/syscalls/fchmod.S"
syscall_src << "arch-x86/syscalls/dup.S"
syscall_src << "arch-x86/syscalls/pipe.S"
syscall_src << "arch-x86/syscalls/pipe2.S"
syscall_src << "arch-x86/syscalls/dup2.S"
syscall_src << "arch-x86/syscalls/select.S"
syscall_src << "arch-x86/syscalls/ftruncate.S"
syscall_src << "arch-x86/syscalls/ftruncate64.S"
syscall_src << "arch-x86/syscalls/getdents.S"
syscall_src << "arch-x86/syscalls/fsync.S"
syscall_src << "arch-x86/syscalls/fdatasync.S"
syscall_src << "arch-x86/syscalls/fchown.S"
syscall_src << "arch-x86/syscalls/sync.S"
syscall_src << "arch-x86/syscalls/__fcntl64.S"
syscall_src << "arch-x86/syscalls/__fstatfs64.S"
syscall_src << "arch-x86/syscalls/sendfile.S"
syscall_src << "arch-x86/syscalls/fstatat.S"
syscall_src << "arch-x86/syscalls/mkdirat.S"
syscall_src << "arch-x86/syscalls/fchownat.S"
syscall_src << "arch-x86/syscalls/fchmodat.S"
syscall_src << "arch-x86/syscalls/renameat.S"
syscall_src << "arch-x86/syscalls/fsetxattr.S"
syscall_src << "arch-x86/syscalls/fgetxattr.S"
syscall_src << "arch-x86/syscalls/flistxattr.S"
syscall_src << "arch-x86/syscalls/fremovexattr.S"
syscall_src << "arch-x86/syscalls/link.S"
syscall_src << "arch-x86/syscalls/unlink.S"
syscall_src << "arch-x86/syscalls/unlinkat.S"
syscall_src << "arch-x86/syscalls/chdir.S"
syscall_src << "arch-x86/syscalls/mknod.S"
syscall_src << "arch-x86/syscalls/chmod.S"
syscall_src << "arch-x86/syscalls/chown.S"
syscall_src << "arch-x86/syscalls/lchown.S"
syscall_src << "arch-x86/syscalls/mount.S"
syscall_src << "arch-x86/syscalls/umount2.S"
syscall_src << "arch-x86/syscalls/fstat.S"
syscall_src << "arch-x86/syscalls/stat.S"
syscall_src << "arch-x86/syscalls/lstat.S"
syscall_src << "arch-x86/syscalls/mkdir.S"
syscall_src << "arch-x86/syscalls/readlink.S"
syscall_src << "arch-x86/syscalls/rmdir.S"
syscall_src << "arch-x86/syscalls/rename.S"
syscall_src << "arch-x86/syscalls/__getcwd.S"
syscall_src << "arch-x86/syscalls/access.S"
syscall_src << "arch-x86/syscalls/faccessat.S"
syscall_src << "arch-x86/syscalls/symlink.S"
syscall_src << "arch-x86/syscalls/fchdir.S"
syscall_src << "arch-x86/syscalls/truncate.S"
syscall_src << "arch-x86/syscalls/setxattr.S"
syscall_src << "arch-x86/syscalls/lsetxattr.S"
syscall_src << "arch-x86/syscalls/getxattr.S"
syscall_src << "arch-x86/syscalls/lgetxattr.S"
syscall_src << "arch-x86/syscalls/listxattr.S"
syscall_src << "arch-x86/syscalls/llistxattr.S"
syscall_src << "arch-x86/syscalls/removexattr.S"
syscall_src << "arch-x86/syscalls/lremovexattr.S"
syscall_src << "arch-x86/syscalls/__statfs64.S"
syscall_src << "arch-x86/syscalls/pause.S"
syscall_src << "arch-x86/syscalls/gettimeofday.S"
syscall_src << "arch-x86/syscalls/settimeofday.S"
syscall_src << "arch-x86/syscalls/times.S"
syscall_src << "arch-x86/syscalls/nanosleep.S"
syscall_src << "arch-x86/syscalls/clock_gettime.S"
syscall_src << "arch-x86/syscalls/clock_settime.S"
syscall_src << "arch-x86/syscalls/clock_getres.S"
syscall_src << "arch-x86/syscalls/clock_nanosleep.S"
syscall_src << "arch-x86/syscalls/getitimer.S"
syscall_src << "arch-x86/syscalls/setitimer.S"
syscall_src << "arch-x86/syscalls/__timer_create.S"
syscall_src << "arch-x86/syscalls/__timer_settime.S"
syscall_src << "arch-x86/syscalls/__timer_gettime.S"
syscall_src << "arch-x86/syscalls/__timer_getoverrun.S"
syscall_src << "arch-x86/syscalls/__timer_delete.S"
syscall_src << "arch-x86/syscalls/utimes.S"
syscall_src << "arch-x86/syscalls/utimensat.S"
syscall_src << "arch-x86/syscalls/sigaction.S"
syscall_src << "arch-x86/syscalls/sigprocmask.S"
syscall_src << "arch-x86/syscalls/__sigsuspend.S"
syscall_src << "arch-x86/syscalls/__rt_sigaction.S"
syscall_src << "arch-x86/syscalls/__rt_sigprocmask.S"
syscall_src << "arch-x86/syscalls/__rt_sigtimedwait.S"
syscall_src << "arch-x86/syscalls/sigpending.S"
syscall_src << "arch-x86/syscalls/socket.S"
syscall_src << "arch-x86/syscalls/bind.S"
syscall_src << "arch-x86/syscalls/connect.S"
syscall_src << "arch-x86/syscalls/listen.S"
syscall_src << "arch-x86/syscalls/accept.S"
syscall_src << "arch-x86/syscalls/getsockname.S"
syscall_src << "arch-x86/syscalls/getpeername.S"
syscall_src << "arch-x86/syscalls/socketpair.S"
syscall_src << "arch-x86/syscalls/sendto.S"
syscall_src << "arch-x86/syscalls/recvfrom.S"
syscall_src << "arch-x86/syscalls/shutdown.S"
syscall_src << "arch-x86/syscalls/setsockopt.S"
syscall_src << "arch-x86/syscalls/getsockopt.S"
syscall_src << "arch-x86/syscalls/sendmsg.S"
syscall_src << "arch-x86/syscalls/recvmsg.S"
syscall_src << "arch-x86/syscalls/sched_setscheduler.S"
syscall_src << "arch-x86/syscalls/sched_getscheduler.S"
syscall_src << "arch-x86/syscalls/sched_yield.S"
syscall_src << "arch-x86/syscalls/sched_setparam.S"
syscall_src << "arch-x86/syscalls/sched_getparam.S"
syscall_src << "arch-x86/syscalls/sched_get_priority_max.S"
syscall_src << "arch-x86/syscalls/sched_get_priority_min.S"
syscall_src << "arch-x86/syscalls/sched_rr_get_interval.S"
syscall_src << "arch-x86/syscalls/sched_setaffinity.S"
syscall_src << "arch-x86/syscalls/__sched_getaffinity.S"
syscall_src << "arch-x86/syscalls/__getcpu.S"
syscall_src << "arch-x86/syscalls/ioprio_set.S"
syscall_src << "arch-x86/syscalls/ioprio_get.S"
syscall_src << "arch-x86/syscalls/uname.S"
syscall_src << "arch-x86/syscalls/__wait4.S"
syscall_src << "arch-x86/syscalls/umask.S"
syscall_src << "arch-x86/syscalls/__reboot.S"
syscall_src << "arch-x86/syscalls/__syslog.S"
syscall_src << "arch-x86/syscalls/init_module.S"
syscall_src << "arch-x86/syscalls/delete_module.S"
syscall_src << "arch-x86/syscalls/klogctl.S"
syscall_src << "arch-x86/syscalls/sysinfo.S"
syscall_src << "arch-x86/syscalls/personality.S"
syscall_src << "arch-x86/syscalls/perf_event_open.S"
syscall_src << "arch-x86/syscalls/futex.S"
syscall_src << "arch-x86/syscalls/epoll_create.S"
syscall_src << "arch-x86/syscalls/epoll_ctl.S"
syscall_src << "arch-x86/syscalls/epoll_wait.S"
syscall_src << "arch-x86/syscalls/inotify_init.S"
syscall_src << "arch-x86/syscalls/inotify_add_watch.S"
syscall_src << "arch-x86/syscalls/inotify_rm_watch.S"
syscall_src << "arch-x86/syscalls/poll.S"
syscall_src << "arch-x86/syscalls/eventfd.S"

libc_common_src_files = [
	"unistd/abort.c", 
	"unistd/alarm.c", 
	"unistd/brk.c", 
	"unistd/creat.c", 
	"unistd/daemon.c", 
	"unistd/eventfd.c", 
	"unistd/exec.c", 
	"unistd/fcntl.c", 
	"unistd/fnmatch.c", 
	"unistd/fstatfs.c", 
	"unistd/ftime.c", 
	"unistd/ftok.c", 
	"unistd/getcwd.c", 
	"unistd/getdtablesize.c", 
	"unistd/gethostname.c", 
	"unistd/getopt_long.c", 
	"unistd/getpgrp.c", 
	"unistd/getpriority.c", 
	"unistd/getpt.c", 
	"unistd/initgroups.c", 
	"unistd/isatty.c", 
	"unistd/issetugid.c", 
	"unistd/killpg.c", 
	"unistd/lseek64.c", 
	"unistd/mmap.c", 
	"unistd/nice.c", 
	"unistd/open.c", 
	"unistd/openat.c", 
	"unistd/opendir.c", 
	"unistd/pathconf.c", 
	"unistd/perror.c", 
	"unistd/popen.c", 
	"unistd/pread.c", 
	"unistd/pselect.c", 
	"unistd/ptsname.c", 
	"unistd/ptsname_r.c", 
	"unistd/pwrite.c", 
	"unistd/raise.c", 
	"unistd/reboot.c", 
	"unistd/recv.c", 
	"unistd/sbrk.c", 
	"unistd/send.c", 
	"unistd/setegid.c", 
	"unistd/setuid.c", 
	"unistd/seteuid.c", 
	"unistd/setreuid.c", 
	"unistd/setresuid.c", 
	"unistd/setpgrp.c", 
	"unistd/sigblock.c", 
	"unistd/siginterrupt.c", 
	"unistd/siglist.c", 
	"unistd/signal.c", 
	"unistd/signame.c", 
	"unistd/sigsetmask.c", 
	"unistd/sigsuspend.c", 
	"unistd/sigwait.c", 
	"unistd/sleep.c", 
	"unistd/statfs.c", 
	"unistd/strsignal.c", 
	"unistd/syslog.c", 
	"unistd/system.c", 
	"unistd/tcgetpgrp.c", 
	"unistd/tcsetpgrp.c", 
	"unistd/time.c", 
	"unistd/umount.c", 
	"unistd/unlockpt.c", 
	"unistd/usleep.c", 
	"unistd/wait.c", 
	"stdio/asprintf.c", 
	"stdio/clrerr.c", 
	"stdio/fclose.c", 
	"stdio/fdopen.c", 
	"stdio/feof.c", 
	"stdio/ferror.c", 
	"stdio/fflush.c", 
	"stdio/fgetc.c", 
	"stdio/fgetln.c", 
	"stdio/fgetpos.c", 
	"stdio/fgets.c", 
	"stdio/fileno.c", 
	"stdio/findfp.c", 
	"stdio/flags.c", 
	"stdio/flockfile.c", 
	"stdio/fopen.c", 
	"stdio/fprintf.c", 
	"stdio/fpurge.c", 
	"stdio/fputc.c", 
	"stdio/fputs.c", 
	"stdio/fread.c", 
	"stdio/freopen.c", 
	"stdio/fscanf.c", 
	"stdio/fseek.c", 
	"stdio/fsetpos.c", 
	"stdio/ftell.c", 
	"stdio/funopen.c", 
	"stdio/fvwrite.c", 
	"stdio/fwalk.c", 
	"stdio/fwrite.c", 
	"stdio/getc.c", 
	"stdio/getchar.c", 
	"stdio/gets.c", 
	"stdio/makebuf.c", 
	"stdio/mktemp.c", 
	"stdio/printf.c", 
	"stdio/putc.c", 
	"stdio/putchar.c", 
	"stdio/puts.c", 
	"stdio/putw.c", 
	"stdio/refill.c", 
	"stdio/remove.c", 
	"stdio/rewind.c", 
	"stdio/rget.c", 
	"stdio/scanf.c", 
	"stdio/setbuf.c", 
	"stdio/setbuffer.c", 
	"stdio/setvbuf.c", 
	"stdio/snprintf.c", 
	"stdio/sprintf.c", 
	"stdio/sscanf.c", 
	"stdio/stdio.c", 
	"stdio/tempnam.c", 
	"stdio/tmpfile.c", 
	"stdio/tmpnam.c", 
	"stdio/ungetc.c", 
	"stdio/vasprintf.c", 
	"stdio/vfprintf.c", 
	"stdio/vfscanf.c", 
	"stdio/vprintf.c", 
	"stdio/vsnprintf.c", 
	"stdio/vsprintf.c", 
	"stdio/vscanf.c", 
	"stdio/vsscanf.c", 
	"stdio/wbuf.c", 
	"stdio/wsetup.c", 
	"stdlib/_rand48.c", 
	"stdlib/assert.c", 
	"stdlib/atexit.c", 
	"stdlib/atoi.c", 
	"stdlib/atol.c", 
	"stdlib/atoll.c", 
	"stdlib/bsearch.c", 
	"stdlib/ctype_.c", 
	"stdlib/div.c", 
	"stdlib/exit.c", 
	"stdlib/getenv.c", 
	"stdlib/jrand48.c", 
	"stdlib/ldiv.c", 
	"stdlib/lldiv.c", 
	"stdlib/locale.c", 
	"stdlib/lrand48.c", 
	"stdlib/mrand48.c", 
	"stdlib/nrand48.c", 
	"stdlib/putenv.c", 
	"stdlib/qsort.c", 
	"stdlib/seed48.c", 
	"stdlib/setenv.c", 
	"stdlib/setjmperr.c", 
	"stdlib/srand48.c", 
	"stdlib/strntoimax.c", 
	"stdlib/strntoumax.c", 
	"stdlib/strtod.c", 
	"stdlib/strtoimax.c", 
	"stdlib/strtol.c", 
	"stdlib/strtoll.c", 
	"stdlib/strtoul.c", 
	"stdlib/strtoull.c", 
	"stdlib/strtoumax.c", 
	"stdlib/tolower_.c", 
	"stdlib/toupper_.c", 
	"stdlib/wchar.c", 
	"string/index.c", 
	"string/memccpy.c", 
	"string/memchr.c", 
	"string/memmem.c", 
	"string/memrchr.c", 
	"string/memswap.c", 
	"string/strcasecmp.c", 
	"string/strcasestr.c", 
	"string/strcat.c", 
	"string/strchr.c", 
	"string/strcoll.c", 
	"string/strcspn.c", 
	"string/strdup.c", 
	"string/strerror.c", 
	"string/strerror_r.c", 
	"string/strlcat.c", 
	"string/strlcpy.c", 
	"string/strncat.c", 
	"string/strncpy.c", 
	"string/strndup.c", 
	"string/strnlen.c", 
	"string/strpbrk.c", 
	"string/strrchr.c", 
	"string/strsep.c", 
	"string/strspn.c", 
	"string/strstr.c", 
	"string/strtok.c", 
	"string/strtotimeval.c", 
	"string/strxfrm.c", 
	"wchar/wcpcpy.c", 
	"wchar/wcpncpy.c", 
	"wchar/wcscasecmp.c", 
	"wchar/wcscat.c", 
	"wchar/wcschr.c", 
	"wchar/wcscmp.c", 
	"wchar/wcscoll.c", 
	"wchar/wcscpy.c", 
	"wchar/wcscspn.c", 
	"wchar/wcsdup.c", 
	"wchar/wcslcat.c", 
	"wchar/wcslcpy.c", 
	"wchar/wcslen.c", 
	"wchar/wcsncasecmp.c", 
	"wchar/wcsncat.c", 
	"wchar/wcsncmp.c", 
	"wchar/wcsncpy.c", 
	"wchar/wcsnlen.c", 
	"wchar/wcspbrk.c", 
	"wchar/wcsrchr.c", 
	"wchar/wcsspn.c", 
	"wchar/wcsstr.c", 
	"wchar/wcstok.c", 
	"wchar/wcswidth.c", 
	"wchar/wcsxfrm.c", 
	"wchar/wmemchr.c", 
	"wchar/wmemcmp.c", 
	"wchar/wmemcpy.c", 
	"wchar/wmemmove.c", 
	"wchar/wmemset.c", 
	"inet/bindresvport.c", 
	"inet/inet_addr.c", 
	"inet/inet_aton.c", 
	"inet/inet_ntoa.c", 
	"inet/inet_ntop.c", 
	"inet/inet_pton.c", 
	"inet/ether_aton.c", 
	"inet/ether_ntoa.c", 
	"tzcode/asctime.c", 
	"tzcode/difftime.c", 
	"tzcode/localtime.c", 
	"tzcode/strftime.c", 
	"tzcode/strptime.c", 
	"bionic/__set_errno.c", 
	"bionic/cpuacct.c", 
	"bionic/arc4random.c", 
	"bionic/basename.c", 
	"bionic/basename_r.c", 
	"bionic/clearenv.c", 
	"bionic/dirname.c", 
	"bionic/dirname_r.c", 
	"bionic/drand48.c", 
	"bionic/erand48.c", 
	"bionic/err.c", 
	"bionic/fdprintf.c", 
	"bionic/fork.c", 
	"bionic/fts.c", 
	"bionic/if_nametoindex.c", 
	"bionic/if_indextoname.c", 
	"bionic/ioctl.c", 
	"bionic/ldexp.c", 
	"bionic/libc_init_common.c", 
	"bionic/logd_write.c", 
	"bionic/md5.c", 
	"bionic/memmove_words.c", 
	"bionic/pututline.c", 
	"bionic/realpath.c", 
	"bionic/sched_getaffinity.c", 
	"bionic/sched_getcpu.c", 
	"bionic/sched_cpualloc.c", 
	"bionic/sched_cpucount.c", 
	"bionic/semaphore.c", 
	"bionic/sha1.c", 
	"bionic/ssp.c", 
	"bionic/stubs.c", 
	"bionic/system_properties.c", 
	"bionic/tdelete.c", 
	"bionic/tdestroy.c", 
	"bionic/time64.c", 
	"bionic/tfind.c", 
	"bionic/thread_atexit.c", 
	"bionic/tsearch.c", 
	"bionic/utime.c", 
	"bionic/utmp.c", 
	"netbsd/gethnamaddr.c", 
	"netbsd/isc/ev_timers.c", 
	"netbsd/isc/ev_streams.c", 
	"netbsd/inet/nsap_addr.c", 
	"netbsd/resolv/__dn_comp.c", 
	"netbsd/resolv/__res_close.c", 
	"netbsd/resolv/__res_send.c", 
	"netbsd/resolv/herror.c", 
	"netbsd/resolv/res_comp.c", 
	"netbsd/resolv/res_data.c", 
	"netbsd/resolv/res_debug.c", 
	"netbsd/resolv/res_init.c", 
	"netbsd/resolv/res_mkquery.c", 
	"netbsd/resolv/res_query.c", 
	"netbsd/resolv/res_send.c", 
	"netbsd/resolv/res_state.c", 
	"netbsd/resolv/res_cache.c", 
	"netbsd/net/nsdispatch.c", 
	"netbsd/net/getaddrinfo.c", 
	"netbsd/net/getnameinfo.c", 
	"netbsd/net/getservbyname.c", 
	"netbsd/net/getservent.c", 
	"netbsd/net/base64.c", 
	"netbsd/net/getservbyport.c", 
	"netbsd/nameser/ns_name.c", 
	"netbsd/nameser/ns_parse.c", 
	"netbsd/nameser/ns_ttl.c", 
	"netbsd/nameser/ns_netint.c", 
	"netbsd/nameser/ns_print.c", 
	"netbsd/nameser/ns_samedomain.c", 
	"regex/regcomp.c", 
	"regex/regerror.c", 
	"regex/regexec.c", 
	"regex/regfree.c",
	# "arch-x86/bionic/__get_sp.S", 
	"arch-x86/bionic/__get_tls.c", 
	"arch-x86/bionic/__set_tls.c", 
	# "arch-x86/bionic/clone.S", 
	# "arch-x86/bionic/_exit_with_stack_teardown.S", 
	# "arch-x86/bionic/futex_x86.S", 
	# "arch-x86/bionic/setjmp.S", 
	# "arch-x86/bionic/_setjmp.S", 
	# "arch-x86/bionic/sigsetjmp.S", 
	# "arch-x86/bionic/vfork.S", 
	# "arch-x86/bionic/syscall.S", 
	# "arch-x86/string/bcopy_wrapper.S", 
	# "arch-x86/string/memcpy_wrapper.S", 
	# "arch-x86/string/memmove_wrapper.S", 
	# "arch-x86/string/bzero_wrapper.S", 
	# "arch-x86/string/memcmp_wrapper.S", 
	# "arch-x86/string/memset_wrapper.S", 
	# "arch-x86/string/strcmp_wrapper.S", 
	# "arch-x86/string/strncmp_wrapper.S", 
	# "arch-x86/string/strlen_wrapper.S", 
	"string/strcpy.c", 
	"bionic/pthread-atfork.c", 
	"bionic/pthread-rwlocks.c", 
	"bionic/pthread-timers.c", 
	"bionic/ptrace.c" ] 

libc_static_common_src_files = [ "unistd/sysconf.c", "bionic/__errno.c", "bionic/pthread.c" ]

# this is needed for static versions of libc
libc_arch_static_src_files = [	"arch-x86/bionic/dl_iterate_phdr_static.c" ]


# Define some common cflags
# ========================================================
libc_common_cflags = [
		"-DWITH_ERRLIST", 
		"-DANDROID_CHANGES", 
		"-DUSE_LOCKS", 
		"-DREALLOC_ZERO_BYTES_FREES", 
		"-D_LIBC=1", 
		"-DSOFTFLOAT", 
		"-DFLOATING_POINT	", 
		"-DINET6", 
		"-DNO_MALLINFO", 
		"-I.", 
		"-Iprivate", 
		"-Iinclude", 
		"-I../libm/include", 
		"-I..", 
		"-I../../system/core/include", 
		"-Istdlib", 
    "-Iarch-x86/include", 
    "-Ikernel/arch-x86", 
    "-Ikernel/common", 
		"-DUSE_DL_PREFIX", 
		"-DPOSIX_MISTAKE", 
    "-DLOG_ON_HEAP_ERROR",
    "-D_BYTE_ORDER=1234",
    "-fPIC" ]

# these macro definitions are required to implement the
# 'timezone' and 'daylight' global variables, as well as
# properly update the 'tm_gmtoff' field in 'struct tm'.
#
libc_common_cflags << [ "-DTM_GMTOFF=tm_gmtoff", "-DUSG_COMPAT=1" ]

# Define ANDROID_SMP appropriately.
libc_common_cflags << [ "-DANDROID_SMP=1" ]


# Define some common includes
# ========================================================
libc_common_c_includes = [ "stdlib", "string", "stdio" ]


# Define the libc run-time (crt) support object files that must be built,
# which are needed to build all other objects (shared/static libs and
# executables)
# ==========================================================================

# ARM and x86 need crtbegin_so/crtend_so.
#
# For x86, the .init section must point to a function that calls all
# entries in the .ctors section. (on ARM this is done through the
# .init_array section instead).
#
# For both platforms, the .fini_array section must point to a function
# that will call __cxa_finalize(&__dso_handle) in order to ensure that
# static C++ destructors are properly called on dlclose().
#

# ========================================================
# libc.so
# ========================================================

# pthread deadlock prediction:
# set -DPTHREAD_DEBUG -DPTHREAD_DEBUG_ENABLED=1 to enable support for
# pthread deadlock prediction.
# Since this code is experimental it is disabled by default.
# see libc/bionic/pthread_debug.c for details

libc_common_cflags << [ "-DLIBC_STATIC", "-DPTHREAD_DEBUG",  "-DPTHREAD_DEBUG_ENABLED=0" ]

files = libc_common_src_files + libc_arch_static_src_files + libc_static_common_src_files +
  [ "bionic/dlmalloc.c", "bionic/libc_init_static.c" ]


@flags = (libc_common_cflags + libc_common_c_includes.map { |x| "-I#{x}/include"}).join(" ")

# WARNING: The only library libc.so should depend on is libdl.so!  If you add other libraries,
# make sure to add -Wl,--exclude-libs=libgcc.a to the LOCAL_LDFLAGS for those libraries.  This
# ensures that symbols that are pulled into those new libraries from libgcc.a are not declared
# external; if that were the case, then libc would not pull those symbols from libgcc.a as it
# should, instead relying on the external symbols from the dependent libraries.  That would
# create an "cloaked" dependency on libgcc.a in libc though the libraries, which is not what
# you wanted!

require "fileutils"

def compile(file)
  puts file

  FileUtils.mkdir_p "output/" + File.dirname(file)
  #puts "clang -emit-llvm #{@flags} -c #{file} -o output/#{file}.bc"
  `clang -O0 -w -nostdinc++ -nobuiltininc -emit-llvm #{@flags} -c #{file} -o output/#{file}.bc`
end

files.each { |f| compile(f) }

FileUtils.cd("output") do
  `llvm-link **/*.bc **/**/*.bc > libc.bc`
end
