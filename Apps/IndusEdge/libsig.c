#include "sig.h"

int signal_register(int signum, void (*fun)(int), struct sigaction *oldact, 
                        sigset_t *sa_mask) 
{
    // TODO 1: Register the signal handler and save the current behaviour in oldact
	struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    if (fun != NULL)
    {
        sa.sa_handler = fun;
        if (sa_mask != NULL)
            sa.sa_mask = *sa_mask;
        sigaction(signum, &sa, oldact);
        return 0;
    }
    return -1;
}

int signal_deregister(int signum) 
{
    // TODO 2: Reset the behaviour to SIG_DFL
	struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigaction(signum, &sa, NULL);

    return 0;
}

int signal_restore(int signum, struct sigaction *sa)
{
    // TODO 3: Restore the behaviour as per sa
	if (sa != NULL)
    {
        sigaction(signum, sa, NULL);
        return 0;
    }
    return -1;
}

int signal_ignore(int signum) 
{
    // TODO 4: Ignore the signal
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigaction(signum, &sa, NULL);
    return 0;
}

int signal_send(pid_t pid, int signum)
{
    // TODO 5: Send the signal to the process 
    kill(pid, signum);
    return 0;
}
