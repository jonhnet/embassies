#include <assert.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void trace_me(const char *id)
{
	int rc;
	rc = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
	fprintf(stderr, "%s ptrace(PTRACE_TRACEME)==%d\n", id, rc);
	assert(rc==0);

	int mypid = getpid();
	fprintf(stderr, "%s killin'\n", id);
	rc = kill(mypid, SIGHUP);
	fprintf(stderr, "%s kill==%d\n", id, rc);
}

void trace_child(const char *id, pid_t child_pid)
{
	int status = 0;
	int waitval = waitpid(child_pid, &status, 0);
	assert(waitval==child_pid);
	assert(WIFSTOPPED(status));
	assert(WSTOPSIG(status)==SIGHUP);
	fprintf(stderr, "%s successfully traces child\n", id);
}

void guest(const char *id)
{
	trace_me(id);

	fprintf(stderr, "%s I'm a happy fellow.\n", id);
	int i;
	for (i=0; i<100; i++)
	{
		sleep(1);
		fprintf(stderr, "%s countdown to awesome %d *******************\n", id, i);
	}
	exit(-1);
}

void display_wait_status(const char *id, int status)
{
	fprintf(stderr, "%s  WIFEXITED %d\n", id, WIFEXITED(status));
	fprintf(stderr, "%s  WIFSIGNALED %d\n", id, WIFSIGNALED(status));
	fprintf(stderr, "%s  WTERMSIG %d\n", id, WTERMSIG(status));
	fprintf(stderr, "%s  WCOREDUMP %d\n", id, WCOREDUMP(status));
	fprintf(stderr, "%s  WIFSTOPPED %d\n", id, WIFSTOPPED(status));
	fprintf(stderr, "%s  WSTOPSIG %d\n", id, WSTOPSIG(status));
	fprintf(stderr, "%s  WIFCONTINUED %d\n", id, WIFCONTINUED(status));
}

void debug_wait_child(const char *id, pid_t child_pid)
{
	int status=69;
	int waitval = waitpid(child_pid, &status, 0);
	fprintf(stderr, "%s waitval = %x status=%d\n", id, waitval, status);
	display_wait_status(id, status);
}

void host(const char *id, char **argv)
{
	trace_me(id);

	fprintf(stderr, "%s I'm a happy fellow.\n", id);

	pid_t guest_pid = fork();
	assert(guest_pid>=0);
	if (guest_pid==0)
	{
		strcpy(argv[0], "guest");
		guest("guest");
		return;
	}

	trace_child(id, guest_pid);

	while (1)
	{
		int rc;
		fprintf(stderr, "%s host ptrace_syscall\n", id);
		rc = ptrace(PTRACE_SYSCALL, guest_pid, NULL, NULL);
		if (rc!=0) { perror("ptrace"); }
		assert(rc==0);
		fprintf(stderr, "%s wait\n", id);

		int status=-1;
		int waitval = waitpid(guest_pid, &status, 0);
		assert(waitval==guest_pid);
		if (WIFEXITED(status))
		{
			fprintf(stderr, "%s child dead; so go I!\n", id);
			break;
		}
		fprintf(stderr, "%s waitval = %x status=%d\n", id, waitval, status);
	}
}

void watchdog(const char *id, pid_t host_pid)
{
	trace_child(id, host_pid);
	int rc;
#if 1
	int options = PTRACE_O_TRACEEXIT;
		// cast int to void*? Man, that's bugly.
	rc = ptrace(PTRACE_SETOPTIONS, host_pid, NULL, (void*) options);
	if (rc!=0) { perror("ptrace"); }
	assert(rc==0);
#endif

	while (1)
	{
//		fprintf(stderr, "%s cont host\n", id);
		rc = ptrace(PTRACE_CONT, host_pid, NULL, NULL);
		if (rc!=0) { perror("ptrace"); }
		assert(rc==0);

//		fprintf(stderr, "%s wait %d\n", id, host_pid);
		int status=0;
		int waitval = waitpid(host_pid, &status, 0);
		assert(waitval==host_pid);
		if (WIFEXITED(status))
		{
			display_wait_status(id, status);
			kill(host_pid, SIGKILL);
			break;
		}
		if (!(WIFSTOPPED(status) && WSTOPSIG(status)==SIGTRAP))
		{
			display_wait_status(id, status);
		}
	}
}

int main(int argc, char **argv)
{
	int pid = fork();
	assert(pid>=0);
	if (pid==0)
	{
		host("host_", argv);
	}
	else
	{
			strcpy(argv[0], "watch");
		watchdog("watch", pid);
	}
	return 0;
}
