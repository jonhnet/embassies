#include <stdio.h>				// NULL
#include <stdbool.h>
#include <linux/unistd.h>		// syscall __NR_* definitions
#include <linux/net.h>			// socketcall SYS_* definitions
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <poll.h>				// struct pollfd, POLLIN

#include "LiteLib.h"
#include "strace.h"
#include "xax_util.h"
#include "xax_extensions.h"
#include "debug_util.h"
#include "xax_endian.h"
#include "cheesy_snprintf.h"

char *strace_fmt_call_inner(Strace *strace, SyscallNameEntry *entries, char *buf, uint32_t syscall_number, uint32_t *args);
char *strace_fmt_call(Strace *strace, char *buf, UserRegs *ur);
char *strace_fmt_return(Strace *strace, char *buf, uint32_t return_val);

// Note we can't use a static initializer, because it would contain
// addresses of the strings, which would not get relocated. (I don't
// know why not; maybe there are data relocs that we're not applying?
// That doesn't sound like a -pie to me! Weird. Some mechanism is
// getting skipped over, or else the compiler owes me an error message.)

#define MAKE_ENTRY(n, at)\
	if (entries!=NULL) \
	{ \
		entries[i].number=n; \
		entries[i].name=#n; \
		entries[i].argtypes=at; \
	} \
	i++;

int strace_load_syscalls(SyscallNameEntry *entries)
{
	int i=0;
	{
		MAKE_ENTRY(__NR_restart_syscall, NULL)
		MAKE_ENTRY(__NR_exit, "X")
		MAKE_ENTRY(__NR_fork, NULL)
		MAKE_ENTRY(__NR_read, "XXX")
		MAKE_ENTRY(__NR_write, "XbX")
		MAKE_ENTRY(__NR_open, "CXX")
		MAKE_ENTRY(__NR_close, "X")
		MAKE_ENTRY(__NR_waitpid, NULL)
		MAKE_ENTRY(__NR_creat, "CX")
		MAKE_ENTRY(__NR_link, "CC")
		MAKE_ENTRY(__NR_unlink, "C")
		MAKE_ENTRY(__NR_execve, NULL)
		MAKE_ENTRY(__NR_chdir, NULL)
		MAKE_ENTRY(__NR_time, "")
		MAKE_ENTRY(__NR_mknod, NULL)
		MAKE_ENTRY(__NR_chmod, NULL)
		MAKE_ENTRY(__NR_lchown, NULL)
		MAKE_ENTRY(__NR_break, NULL)
		MAKE_ENTRY(__NR_oldstat, NULL)
		MAKE_ENTRY(__NR_lseek, "XXX")
		MAKE_ENTRY(__NR_getpid, NULL)
		MAKE_ENTRY(__NR_mount, NULL)
		MAKE_ENTRY(__NR_umount, NULL)
		MAKE_ENTRY(__NR_setuid, NULL)
		MAKE_ENTRY(__NR_getuid, NULL)
		MAKE_ENTRY(__NR_stime, NULL)
		MAKE_ENTRY(__NR_ptrace, NULL)
		MAKE_ENTRY(__NR_alarm, NULL)
		MAKE_ENTRY(__NR_oldfstat, NULL)
		MAKE_ENTRY(__NR_pause, NULL)
		MAKE_ENTRY(__NR_utime, "CX")
		MAKE_ENTRY(__NR_stty, NULL)
		MAKE_ENTRY(__NR_gtty, NULL)
		MAKE_ENTRY(__NR_access, "CX")
		MAKE_ENTRY(__NR_nice, NULL)
		MAKE_ENTRY(__NR_ftime, NULL)
		MAKE_ENTRY(__NR_sync, NULL)
		MAKE_ENTRY(__NR_kill, NULL)
		MAKE_ENTRY(__NR_rename, "CC")
		MAKE_ENTRY(__NR_mkdir, "CX")
		MAKE_ENTRY(__NR_rmdir, "C")
		MAKE_ENTRY(__NR_dup, "X")
		MAKE_ENTRY(__NR_pipe, NULL)
		MAKE_ENTRY(__NR_times, NULL)
		MAKE_ENTRY(__NR_prof, NULL)
		MAKE_ENTRY(__NR_brk, "X")
		MAKE_ENTRY(__NR_setgid, NULL)
		MAKE_ENTRY(__NR_getgid, NULL)
		MAKE_ENTRY(__NR_signal, NULL)
		MAKE_ENTRY(__NR_geteuid, NULL)
		MAKE_ENTRY(__NR_getegid, NULL)
		MAKE_ENTRY(__NR_acct, NULL)
		MAKE_ENTRY(__NR_umount2, NULL)
		MAKE_ENTRY(__NR_lock, NULL)
		MAKE_ENTRY(__NR_ioctl, NULL)
		MAKE_ENTRY(__NR_fcntl, "XX")
		MAKE_ENTRY(__NR_mpx, NULL)
		MAKE_ENTRY(__NR_setpgid, NULL)
		MAKE_ENTRY(__NR_ulimit, NULL)
		MAKE_ENTRY(__NR_oldolduname, NULL)
		MAKE_ENTRY(__NR_umask, NULL)
		MAKE_ENTRY(__NR_chroot, NULL)
		MAKE_ENTRY(__NR_ustat, NULL)
		MAKE_ENTRY(__NR_dup2, NULL)
		MAKE_ENTRY(__NR_getppid, NULL)
		MAKE_ENTRY(__NR_getpgrp, NULL)
		MAKE_ENTRY(__NR_setsid, NULL)
		MAKE_ENTRY(__NR_sigaction, NULL)
		MAKE_ENTRY(__NR_sgetmask, NULL)
		MAKE_ENTRY(__NR_ssetmask, NULL)
		MAKE_ENTRY(__NR_setreuid, NULL)
		MAKE_ENTRY(__NR_setregid, NULL)
		MAKE_ENTRY(__NR_sigsuspend, NULL)
		MAKE_ENTRY(__NR_sigpending, NULL)
		MAKE_ENTRY(__NR_sethostname, NULL)
		MAKE_ENTRY(__NR_setrlimit, NULL)
		MAKE_ENTRY(__NR_getrlimit, NULL)
		MAKE_ENTRY(__NR_getrusage, "XX")
		MAKE_ENTRY(__NR_gettimeofday, "tX")
		MAKE_ENTRY(__NR_settimeofday, NULL)
		MAKE_ENTRY(__NR_getgroups, NULL)
		MAKE_ENTRY(__NR_setgroups, NULL)
		MAKE_ENTRY(__NR_select, NULL)
		MAKE_ENTRY(__NR_symlink, NULL)
		MAKE_ENTRY(__NR_oldlstat, NULL)
		MAKE_ENTRY(__NR_readlink, "CXX")
		MAKE_ENTRY(__NR_uselib, NULL)
		MAKE_ENTRY(__NR_swapon, NULL)
		MAKE_ENTRY(__NR_reboot, NULL)
		MAKE_ENTRY(__NR_readdir, NULL)
		MAKE_ENTRY(__NR_mmap, "XXXXXX")
		MAKE_ENTRY(__NR_munmap, "XX")
		MAKE_ENTRY(__NR_truncate, "CX")
		MAKE_ENTRY(__NR_ftruncate, "XX")
		MAKE_ENTRY(__NR_fchmod, NULL)
		MAKE_ENTRY(__NR_fchown, NULL)
		MAKE_ENTRY(__NR_getpriority, NULL)
		MAKE_ENTRY(__NR_setpriority, NULL)
		MAKE_ENTRY(__NR_profil, NULL)
		MAKE_ENTRY(__NR_statfs, NULL)
		MAKE_ENTRY(__NR_fstatfs, NULL)
		MAKE_ENTRY(__NR_ioperm, NULL)
		MAKE_ENTRY(__NR_socketcall, "s")
		MAKE_ENTRY(__NR_syslog, NULL)
		MAKE_ENTRY(__NR_setitimer, NULL)
		MAKE_ENTRY(__NR_getitimer, NULL)
		MAKE_ENTRY(__NR_stat, NULL)
		MAKE_ENTRY(__NR_lstat, NULL)
		MAKE_ENTRY(__NR_fstat, NULL)
		MAKE_ENTRY(__NR_olduname, NULL)
		MAKE_ENTRY(__NR_iopl, NULL)
		MAKE_ENTRY(__NR_vhangup, NULL)
		MAKE_ENTRY(__NR_idle, NULL)
		MAKE_ENTRY(__NR_vm86old, NULL)
		MAKE_ENTRY(__NR_wait4, NULL)
		MAKE_ENTRY(__NR_swapoff, NULL)
		MAKE_ENTRY(__NR_sysinfo, NULL)
		MAKE_ENTRY(__NR_ipc, NULL)
		MAKE_ENTRY(__NR_fsync, NULL)
		MAKE_ENTRY(__NR_sigreturn, NULL)
		MAKE_ENTRY(__NR_clone, NULL)
		MAKE_ENTRY(__NR_setdomainname, NULL)
		MAKE_ENTRY(__NR_uname, "X")
		MAKE_ENTRY(__NR_modify_ldt, NULL)
		MAKE_ENTRY(__NR_adjtimex, NULL)
		MAKE_ENTRY(__NR_mprotect, "XXX")
		MAKE_ENTRY(__NR_sigprocmask, NULL)
		MAKE_ENTRY(__NR_create_module, NULL)
		MAKE_ENTRY(__NR_init_module, NULL)
		MAKE_ENTRY(__NR_delete_module, NULL)
		MAKE_ENTRY(__NR_get_kernel_syms, NULL)
		MAKE_ENTRY(__NR_quotactl, NULL)
		MAKE_ENTRY(__NR_getpgid, NULL)
		MAKE_ENTRY(__NR_fchdir, NULL)
		MAKE_ENTRY(__NR_bdflush, NULL)
		MAKE_ENTRY(__NR_sysfs, "X")
		MAKE_ENTRY(__NR_personality, NULL)
		MAKE_ENTRY(__NR_afs_syscall, NULL)
		MAKE_ENTRY(__NR_setfsuid, NULL)
		MAKE_ENTRY(__NR_setfsgid, NULL)
		MAKE_ENTRY(__NR__llseek, "XXXXX")
		MAKE_ENTRY(__NR_getdents, NULL)
		MAKE_ENTRY(__NR__newselect, NULL)
		MAKE_ENTRY(__NR_flock, NULL)
		MAKE_ENTRY(__NR_msync, NULL)
		MAKE_ENTRY(__NR_readv, NULL)
		MAKE_ENTRY(__NR_writev, NULL)
		MAKE_ENTRY(__NR_getsid, NULL)
		MAKE_ENTRY(__NR_fdatasync, NULL)
		MAKE_ENTRY(__NR__sysctl, NULL)
		MAKE_ENTRY(__NR_mlock, NULL)
		MAKE_ENTRY(__NR_munlock, NULL)
		MAKE_ENTRY(__NR_mlockall, NULL)
		MAKE_ENTRY(__NR_munlockall, NULL)
		MAKE_ENTRY(__NR_sched_setparam, NULL)
		MAKE_ENTRY(__NR_sched_getparam, NULL)
		MAKE_ENTRY(__NR_sched_setscheduler, NULL)
		MAKE_ENTRY(__NR_sched_getscheduler, NULL)
		MAKE_ENTRY(__NR_sched_yield, NULL)
		MAKE_ENTRY(__NR_sched_get_priority_max, NULL)
		MAKE_ENTRY(__NR_sched_get_priority_min, NULL)
		MAKE_ENTRY(__NR_sched_rr_get_interval, NULL)
		MAKE_ENTRY(__NR_nanosleep, NULL)
		MAKE_ENTRY(__NR_mremap, NULL)
		MAKE_ENTRY(__NR_setresuid, NULL)
		MAKE_ENTRY(__NR_getresuid, NULL)
		MAKE_ENTRY(__NR_vm86, NULL)
		MAKE_ENTRY(__NR_query_module, NULL)
		MAKE_ENTRY(__NR_poll, "oXX")
		MAKE_ENTRY(__NR_nfsservctl, NULL)
		MAKE_ENTRY(__NR_setresgid, NULL)
		MAKE_ENTRY(__NR_getresgid, NULL)
		MAKE_ENTRY(__NR_prctl, NULL)
		MAKE_ENTRY(__NR_rt_sigreturn, NULL)
		MAKE_ENTRY(__NR_rt_sigaction, NULL)
		MAKE_ENTRY(__NR_rt_sigprocmask, NULL)
		MAKE_ENTRY(__NR_rt_sigpending, NULL)
		MAKE_ENTRY(__NR_rt_sigtimedwait, NULL)
		MAKE_ENTRY(__NR_rt_sigqueueinfo, NULL)
		MAKE_ENTRY(__NR_rt_sigsuspend, NULL)
		MAKE_ENTRY(__NR_pread64, NULL)
		MAKE_ENTRY(__NR_pwrite64, NULL)
		MAKE_ENTRY(__NR_chown, NULL)
		MAKE_ENTRY(__NR_getcwd, "CX")
		MAKE_ENTRY(__NR_capget, NULL)
		MAKE_ENTRY(__NR_capset, NULL)
		MAKE_ENTRY(__NR_sigaltstack, NULL)
		MAKE_ENTRY(__NR_sendfile, NULL)
		MAKE_ENTRY(__NR_getpmsg, NULL)
		MAKE_ENTRY(__NR_putpmsg, NULL)
		MAKE_ENTRY(__NR_vfork, NULL)
		MAKE_ENTRY(__NR_ugetrlimit, NULL)
		MAKE_ENTRY(__NR_mmap2, "XXXXXX")
		MAKE_ENTRY(__NR_truncate64, NULL)
		MAKE_ENTRY(__NR_ftruncate64, NULL)
		MAKE_ENTRY(__NR_stat64, "CX")
		MAKE_ENTRY(__NR_lstat64, "CX")
		MAKE_ENTRY(__NR_fstat64, "XX")
		MAKE_ENTRY(__NR_lchown32, NULL)
		MAKE_ENTRY(__NR_getuid32, NULL)
		MAKE_ENTRY(__NR_getgid32, NULL)
		MAKE_ENTRY(__NR_geteuid32, NULL)
		MAKE_ENTRY(__NR_getegid32, NULL)
		MAKE_ENTRY(__NR_setreuid32, NULL)
		MAKE_ENTRY(__NR_setregid32, NULL)
		MAKE_ENTRY(__NR_getgroups32, NULL)
		MAKE_ENTRY(__NR_setgroups32, NULL)
		MAKE_ENTRY(__NR_fchown32, NULL)
		MAKE_ENTRY(__NR_setresuid32, NULL)
		MAKE_ENTRY(__NR_getresuid32, NULL)
		MAKE_ENTRY(__NR_setresgid32, NULL)
		MAKE_ENTRY(__NR_getresgid32, NULL)
		MAKE_ENTRY(__NR_chown32, "CXX")
		MAKE_ENTRY(__NR_setuid32, NULL)
		MAKE_ENTRY(__NR_setgid32, NULL)
		MAKE_ENTRY(__NR_setfsuid32, NULL)
		MAKE_ENTRY(__NR_setfsgid32, NULL)
		MAKE_ENTRY(__NR_pivot_root, NULL)
		MAKE_ENTRY(__NR_mincore, NULL)
		MAKE_ENTRY(__NR_madvise, NULL)
		MAKE_ENTRY(__NR_madvise1, NULL)
		MAKE_ENTRY(__NR_getdents64, NULL)
		MAKE_ENTRY(__NR_fcntl64, "XX")
		MAKE_ENTRY(__NR_gettid, NULL)
		MAKE_ENTRY(__NR_readahead, NULL)
		MAKE_ENTRY(__NR_setxattr, NULL)
		MAKE_ENTRY(__NR_lsetxattr, NULL)
		MAKE_ENTRY(__NR_fsetxattr, NULL)
		MAKE_ENTRY(__NR_getxattr, NULL)
		MAKE_ENTRY(__NR_lgetxattr, NULL)
		MAKE_ENTRY(__NR_fgetxattr, NULL)
		MAKE_ENTRY(__NR_listxattr, NULL)
		MAKE_ENTRY(__NR_llistxattr, NULL)
		MAKE_ENTRY(__NR_flistxattr, NULL)
		MAKE_ENTRY(__NR_removexattr, NULL)
		MAKE_ENTRY(__NR_lremovexattr, NULL)
		MAKE_ENTRY(__NR_fremovexattr, NULL)
		MAKE_ENTRY(__NR_tkill, NULL)
		MAKE_ENTRY(__NR_sendfile64, NULL)
		MAKE_ENTRY(__NR_futex, NULL)
		MAKE_ENTRY(__NR_sched_setaffinity, NULL)
		MAKE_ENTRY(__NR_sched_getaffinity, NULL)
		MAKE_ENTRY(__NR_set_thread_area, NULL)
		MAKE_ENTRY(__NR_get_thread_area, NULL)
		MAKE_ENTRY(__NR_io_setup, NULL)
		MAKE_ENTRY(__NR_io_destroy, NULL)
		MAKE_ENTRY(__NR_io_getevents, NULL)
		MAKE_ENTRY(__NR_io_submit, NULL)
		MAKE_ENTRY(__NR_io_cancel, NULL)
		MAKE_ENTRY(__NR_fadvise64, NULL)
		MAKE_ENTRY(__NR_exit_group, NULL)
		MAKE_ENTRY(__NR_lookup_dcookie, NULL)
		MAKE_ENTRY(__NR_epoll_create, NULL)
		MAKE_ENTRY(__NR_epoll_ctl, NULL)
		MAKE_ENTRY(__NR_epoll_wait, NULL)
		MAKE_ENTRY(__NR_remap_file_pages, NULL)
		MAKE_ENTRY(__NR_set_tid_address, NULL)
		MAKE_ENTRY(__NR_timer_create, NULL)
		MAKE_ENTRY(__NR_timer_settime, NULL)
		MAKE_ENTRY(__NR_timer_gettime, NULL)
		MAKE_ENTRY(__NR_timer_getoverrun, NULL)
		MAKE_ENTRY(__NR_timer_delete, NULL)
		MAKE_ENTRY(__NR_clock_settime, NULL)
		MAKE_ENTRY(__NR_clock_gettime, "XX")
		MAKE_ENTRY(__NR_clock_getres, "XX")
		MAKE_ENTRY(__NR_clock_nanosleep, NULL)
		MAKE_ENTRY(__NR_statfs64, NULL)
		MAKE_ENTRY(__NR_fstatfs64, NULL)
		MAKE_ENTRY(__NR_tgkill, NULL)
		MAKE_ENTRY(__NR_utimes, "CX")
		MAKE_ENTRY(__NR_fadvise64_64, NULL)
		MAKE_ENTRY(__NR_vserver, NULL)
		MAKE_ENTRY(__NR_mbind, NULL)
		MAKE_ENTRY(__NR_get_mempolicy, NULL)
		MAKE_ENTRY(__NR_set_mempolicy, NULL)
		MAKE_ENTRY(__NR_mq_open, NULL)
		MAKE_ENTRY(__NR_mq_unlink, "C")
		MAKE_ENTRY(__NR_mq_timedsend, NULL)
		MAKE_ENTRY(__NR_mq_timedreceive, NULL)
		MAKE_ENTRY(__NR_mq_notify, NULL)
		MAKE_ENTRY(__NR_mq_getsetattr, NULL)
		MAKE_ENTRY(__NR_kexec_load, NULL)
		MAKE_ENTRY(__NR_waitid, NULL)
		MAKE_ENTRY(__NR_add_key, NULL)
		MAKE_ENTRY(__NR_request_key, NULL)
		MAKE_ENTRY(__NR_keyctl, NULL)
		MAKE_ENTRY(__NR_ioprio_set, NULL)
		MAKE_ENTRY(__NR_ioprio_get, NULL)
		MAKE_ENTRY(__NR_inotify_init, NULL)
		MAKE_ENTRY(__NR_inotify_add_watch, NULL)
		MAKE_ENTRY(__NR_inotify_rm_watch, NULL)
		MAKE_ENTRY(__NR_migrate_pages, NULL)
		MAKE_ENTRY(__NR_openat, NULL)
		MAKE_ENTRY(__NR_mkdirat, NULL)
		MAKE_ENTRY(__NR_mknodat, NULL)
		MAKE_ENTRY(__NR_fchownat, NULL)
		MAKE_ENTRY(__NR_futimesat, NULL)
		MAKE_ENTRY(__NR_fstatat64, NULL)
		MAKE_ENTRY(__NR_unlinkat, NULL)
		MAKE_ENTRY(__NR_renameat, NULL)
		MAKE_ENTRY(__NR_linkat, "XCXCX")
		MAKE_ENTRY(__NR_symlinkat, "CXC")
		MAKE_ENTRY(__NR_readlinkat, "XCCX")
		MAKE_ENTRY(__NR_fchmodat, NULL)
		MAKE_ENTRY(__NR_faccessat, NULL)
		MAKE_ENTRY(__NR_pselect6, NULL)
		MAKE_ENTRY(__NR_ppoll, NULL)
		MAKE_ENTRY(__NR_unshare, NULL)
		MAKE_ENTRY(__NR_set_robust_list, NULL)
		MAKE_ENTRY(__NR_get_robust_list, NULL)
		MAKE_ENTRY(__NR_splice, NULL)
		MAKE_ENTRY(__NR_sync_file_range, NULL)
		MAKE_ENTRY(__NR_tee, NULL)
		MAKE_ENTRY(__NR_vmsplice, NULL)
		MAKE_ENTRY(__NR_move_pages, NULL)
		MAKE_ENTRY(__NR_getcpu, NULL)
		MAKE_ENTRY(__NR_epoll_pwait, NULL)
		MAKE_ENTRY(__NR_utimensat, NULL)
		MAKE_ENTRY(__NR_signalfd, NULL)
		MAKE_ENTRY(__NR_timerfd_create, NULL)
		MAKE_ENTRY(__NR_eventfd, NULL)
		MAKE_ENTRY(__NR_fallocate, NULL)
		MAKE_ENTRY(__NR_timerfd_settime, NULL)
		MAKE_ENTRY(__NR_timerfd_gettime, NULL)
#ifdef __NR_signalfd4
		MAKE_ENTRY(__NR_signalfd4, NULL)
		MAKE_ENTRY(__NR_eventfd2, NULL)
		MAKE_ENTRY(__NR_epoll_create1, NULL)
		MAKE_ENTRY(__NR_dup3, NULL)
		MAKE_ENTRY(__NR_pipe2, "pX")
		MAKE_ENTRY(__NR_inotify_init1, NULL)
		MAKE_ENTRY(__NR_preadv, NULL)
		MAKE_ENTRY(__NR_pwritev, NULL)
		MAKE_ENTRY(__NR_rt_tgsigqueueinfo, NULL)
		MAKE_ENTRY(__NR_perf_event_open, NULL)
#endif
		MAKE_ENTRY(__NR_xe_get_system_clock, "")
		MAKE_ENTRY(__NR_xe_network_add_default_handler, "")
		MAKE_ENTRY(__NR_xe_xpe_mount_vfs, "CX")
		MAKE_ENTRY(__NR_xe_allow_new_brk, "XC")
		MAKE_ENTRY(__NR_xe_wait_until_all_brks_claimed, "")
		MAKE_ENTRY(__NR_xe_mount_fuse_client, "CXXX")
	}

	if (entries!=NULL)
	{
		entries[i].number=-1;
		entries[i].name="<unknown>";
		entries[i].argtypes=NULL;
	}
	i++;
	return i;
}

int strace_load_socketcalls(SyscallNameEntry *entries)
{
	int i=0;

	{
		MAKE_ENTRY(__NR_restart_syscall, NULL)
		MAKE_ENTRY(SYS_SOCKET, "XXX")
		MAKE_ENTRY(SYS_BIND, "XAX")
		MAKE_ENTRY(SYS_CONNECT, "XAX")
		MAKE_ENTRY(SYS_LISTEN, "XX")
		MAKE_ENTRY(SYS_ACCEPT, "XXX")
		MAKE_ENTRY(SYS_GETSOCKNAME, "XAX")
		MAKE_ENTRY(SYS_GETPEERNAME, "XAX")
		MAKE_ENTRY(SYS_SOCKETPAIR, "XXXX")
		MAKE_ENTRY(SYS_SEND, "XBXX")
		MAKE_ENTRY(SYS_RECV, "XBXX")
		MAKE_ENTRY(SYS_SENDTO, "XBXXAX")
		MAKE_ENTRY(SYS_RECVFROM, "XBXXXAX")
		MAKE_ENTRY(SYS_SHUTDOWN, "XX")
		MAKE_ENTRY(SYS_SETSOCKOPT, "XXXXX")
		MAKE_ENTRY(SYS_GETSOCKOPT, "XXXXX")
		MAKE_ENTRY(SYS_SENDMSG, "XXX")
		MAKE_ENTRY(SYS_RECVMSG, "XXX")
#ifdef SYS_ACCEPT4
		MAKE_ENTRY(SYS_ACCEPT4, NULL)
#endif // SYS_ACCEPT4
	}

	if (entries!=NULL)
	{
		entries[i].number=-1;
		entries[i].name="<unknown>";
		entries[i].argtypes=NULL;
	}
	i++;
	return i;
}

void strace_init(
	Strace *strace,
	MallocFactory *mf,
	ZoogDispatchTable_v1 *xdt,
	strace_time_ifc_f *time_ifc,
	void *time_obj)
{
	int count;
	count = strace_load_syscalls(NULL);
	strace->syscall_entries = (mf->c_malloc)(mf, count*sizeof(SyscallNameEntry));
	strace_load_syscalls(strace->syscall_entries);

	count = strace_load_socketcalls(NULL);
	strace->socketcall_entries = (mf->c_malloc)(mf, count*sizeof(SyscallNameEntry));
	strace_load_socketcalls(strace->socketcall_entries);

	strace->xdt = xdt;
	strace->time_initted = false;
	strace->time_ifc = time_ifc;
	strace->time_obj = time_obj;
}


SyscallNameEntry *strace_lookup(SyscallNameEntry *entries, int number)
{
	int i;
	for (i=0; entries[i].number!=-1; i++)
	{
		if (entries[i].number==number)
		{
			break;
		}
	}
	return &entries[i];
}

char *strace_fmt_call(Strace *strace, char *buf, UserRegs *ur)
{
	buf[0] = '\0';

	{
		int real_time_ms, user_time_ms;
		(strace->time_ifc)(strace->time_obj, &real_time_ms, &user_time_ms);
		if (!strace->time_initted)
		{
			strace->real_time_base = real_time_ms;
			strace->user_time_base = user_time_ms;
			strace->time_initted = true;
		}

		char timebuf[32];
		cheesy_snprintf(timebuf, sizeof(timebuf), "%10d %10d ",
			real_time_ms - strace->real_time_base,
			user_time_ms - strace->user_time_base);
		lite_strcat(buf, timebuf);
	}

#define THREAD_IDS 1
#if THREAD_IDS
	{
		char hexbuf[16];
		lite_strcat(buf, "THRx");
		lite_strcat(buf, hexstr(hexbuf, get_gs_base()));
		lite_strcat(buf, ": ");
	}
#endif //THREAD_IDS

	return strace_fmt_call_inner(strace, strace->syscall_entries, buf+lite_strlen(buf), ur->syscall_number, &ur->arg1);
}

void encode_sockaddr(char *str, const struct sockaddr *sa)
{
	if (sa->sa_family==AF_UNIX)
	{
		(*str++)='\"';
		(*str)='\0';
		const char *path = ((const struct sockaddr_un*) sa)->sun_path;

		// man AF_UNIX:
		// "an abstract socket address is distinguished  by  the  fact
		// that  sun_path[0] is a null byte ('\0')"
		if (path[0]=='\0')
		{
			lite_strcat(str, "\\0");
			path += 1;
		}

		lite_strcpy(str, path);
		str+=lite_strlen(str);
		(*str++)='\"';
	}
	else if (sa->sa_family==AF_INET)
	{
		char hexbuf[16];
		const struct sockaddr_in *si = (const struct sockaddr_in *) sa;
		int i;
		uint32_t addr = si->sin_addr.s_addr;
		for (i=0; i<4; i++)
		{
			lite_strcat(str, "x");
			lite_strcat(str, hexstr(hexbuf, (addr>>24) & 0xff));
			addr=(addr<<8);
			lite_strcat(str, ".");
		}
		lite_strcat(str, "x");
		lite_strcat(str, hexstr(hexbuf, xax_ntohs(si->sin_port)));
		str+=lite_strlen(str);
	}
	else
	{
		(*str++)='?';
	}

	(*str++)='\0';
}

void encode_buffer(char *str, uint8_t *buf, int len)
{
	if (len>50) { len = 50; }

	(*str++) = '"';
	int i;
	for (i=0; i<len; i++)
	{
		if (' '<=buf[i] && buf[i]<=127)
		{
			(*str++) = buf[i];
		}
		else
		{
			(*str++) = '\\';
			(*str++) = NYBBLE_TO_HEX((buf[i] >> 4) & 0x0f);
			(*str++) = NYBBLE_TO_HEX((buf[i] >> 0) & 0x0f);
		}
	}
	(*str++) = '"';
	(*str++) = 0;
}

char *strace_fmt_call_inner(Strace *strace, SyscallNameEntry *entries, char *buf, uint32_t syscall_number, uint32_t *args)
{
	buf[0] = '\0';

	SyscallNameEntry *sne = strace_lookup(entries, syscall_number);

	lite_strcat(buf, sne->name);
	if (sne->number==-1)
	{
		char hexbuf[16];
		lite_strcat(buf, "[0x");
		lite_strcat(buf, hexstr(hexbuf, syscall_number));
		lite_strcat(buf, "]");
	}
	lite_strcat(buf, "(");
	if (sne->argtypes==NULL)
	{
		lite_strcat(buf, "...");
	}
	else
	{
		int ai;
		for (ai=0; sne->argtypes[ai]!='\0'; ai++)
		{
			uint32_t arg = args[ai];
			switch (sne->argtypes[ai])
			{
			case 'C':
			case 'b':
			{
				bool ellipses = false;
				char *s = (char*) arg;
				int len =0;
				if (sne->argtypes[ai]=='C')
				{
					len=lite_strlen(s);
				}
				else if (sne->argtypes[ai]=='b')
				{
					// len in next arg
					lite_assert(sne->argtypes[ai+1]=='X');
					len=args[ai+1];
				}
				else
				{
					lite_assert(false);
				}
				if (len>160)
				{
					len = 160;
					ellipses = true;
				}
				char *dst = &buf[lite_strlen(buf)];
				*(dst++) = '"';
				int i;
				for (i=0; i<len && i<160; i++)
				{
					char c = s[i];
					if (c<=32 || c>=127)
					{
						c = '_';
					}
					*(dst++) = c;
				}
				//lite_memcpy(dst, s, len);
				//dst+=len;
				*(dst++) = '"';
				*(dst++) = '\0';
				if (ellipses)
				{
					lite_strcat(dst-1, "...");
				}
				break;
			}
			case 'A':
			{
				encode_sockaddr(buf+lite_strlen(buf), (const struct sockaddr *) arg);
				break;
			}
			case 'B':
			{
				// "B"uffer assumes that the next argument is present
				// and a length field.
				encode_buffer(buf+lite_strlen(buf), (uint8_t*) arg, args[ai+1]);
				break;
			}
			case 'X':
			{
				char hexbuf[16];
				lite_strcat(buf, "0x");
				lite_strcat(buf, hexstr(hexbuf, arg));
				break;
			}
			case 's':
			{
				strace_fmt_call_inner(strace, strace->socketcall_entries, &buf[lite_strlen(buf)], arg, (uint32_t*) args[ai+1]);
				break;
			}
			case 't':	// struct timeval*
			{
				char hexbuf[16];
				struct timeval *tv = (struct timeval *) arg;
				lite_strcat(buf, "{0x");
				lite_strcat(buf, hexstr(hexbuf, tv->tv_sec));
				lite_strcat(buf, ", 0x");
				lite_strcat(buf, hexstr(hexbuf, tv->tv_usec));
				lite_strcat(buf, "}");
				break;
			}
			case 'o':	// struct pollfd*
			{
				int num_fd = args[ai+1];
				bool truncated = false;
					// NB assuming this is a poll, so arg is followed by num_fd
				if (num_fd<0)
				{
					lite_strcat(buf, "?");
					break;
				}
				if (num_fd>10)
				{
					truncated = true;
					num_fd=10;
				}
			
				lite_strcat(buf, "[");
				struct pollfd* pfd = (struct pollfd*) arg;
				int i;
				for (i=0; i<num_fd; i++)
				{
					char hexbuf[16];
					lite_strcat(buf, "0x");
					lite_strcat(buf, hexstr(hexbuf, pfd[i].fd));
					if (pfd[i].events&POLLIN) { lite_strcat(buf, "i"); }
					if (pfd[i].events&POLLOUT) { lite_strcat(buf, "o"); }
					if (pfd[i].events&POLLERR) { lite_strcat(buf, "e"); }
					if (i<num_fd-1)
					{
					lite_strcat(buf, ", ");
					}
				}
				if (truncated)
				{
					lite_strcat(buf, "...");
				}
				lite_strcat(buf, "]");
				break;
			}
			case 'p':
			{
				int *pipes = (int *) arg;
				char hexbuf[16];
				lite_strcat(buf, "[0x");
				lite_strcat(buf, hexstr(hexbuf, pipes[0]));
				lite_strcat(buf, ", 0x");
				lite_strcat(buf, hexstr(hexbuf, pipes[1]));
				lite_strcat(buf, "]");
				break;
			}
			default:
				xax_assert(0);	// invalid argtypes char
			}

			if (sne->argtypes[ai+1]!='\0')
			{
				lite_strcat(buf, ", ");
			}
		}
	}
	lite_strcat(buf, ")");
	return buf;
}

char *strace_fmt_return(Strace *strace, char *buf, uint32_t return_val)
{
	char hexbuf[16];
	lite_strcpy(buf, " = 0x");
	lite_strcat(buf, hexstr(hexbuf, return_val));
	lite_strcat(buf, "\n");
	return buf;
}

void ebp_walk(ZoogDispatchTable_v1 *xdt, uint32_t ebp_start)
{
	char buf[500];
	char hexbuf[16];
	uint32_t *ebp = (uint32_t*) ebp_start;
//	__asm__("movl %%ebp,%0" : "=m"(ebp) : /**/ : "%eax");
	while (ebp!=0)
	{
		lite_strcpy(buf, "ebp ");
		lite_strcat(buf, hexstr(hexbuf, ebp[0]));
		lite_strcat(buf, " ret ");
		lite_strcat(buf, hexstr(hexbuf, ebp[1]));
		lite_strcat(buf, "\n");
		debug_logfile_append(xdt, "stack", buf);
		ebp = (uint32_t*) ebp[0];
	}
	lite_strcpy(buf, "\n");
	debug_logfile_append(xdt, "stack", buf);
}

#if 0
int strace_emit_call(Strace *strace, UserRegs *ur)
{
	char buf[500], *ptr=buf;
	cheesy_lock_acquire(&strace->lock);
	int id = strace->next_id++;
	if (strace->open_id != 0)
	{
		// breaking someone else's open.
		lite_strcpy(buf, "...\n");
		ptr = buf+lite_strlen(buf);
	}
	strace_fmt_call(strace, ptr, ur);
	debug_logfile_append(strace->xdt, "strace", buf);
	strace->open_id = id;
	cheesy_lock_release(&strace->lock);
	return id;
}

void strace_emit_return(Strace *strace, int id, uint32_t return_val)
{
	char buf[500], *ptr = buf;
	cheesy_lock_acquire(&strace->lock);
	if (strace->open_id != id)
	{
		if (strace->open_id != 0)
		{
			// breaking someone else's open
			lite_strcpy(buf, "...\n");
			ptr = buf+lite_strlen(buf);
		}
		// reprint my context. Oh, wait, I don't have it any more.
		lite_strcpy(buf, "...");
		ptr = buf+lite_strlen(buf);
	}
	strace_fmt_return(strace, buf, return_val)
	debug_logfile_append(strace->xdt, "strace",);
	strace->open_id 
	cheesy_lock_release(&strace->lock);
}
#endif // 0

void strace_emit_trace(Strace *strace, UserRegs *ur, uint32_t return_val)
{
	char buf[500], *ptr = buf;
	strace_fmt_call(strace, ptr, ur);
	ptr = buf+lite_strlen(buf);
	strace_fmt_return(strace, ptr, return_val);
	debug_logfile_append(strace->xdt, "strace", buf);
}
