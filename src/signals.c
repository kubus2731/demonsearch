#include "signals.h"

#include <signal.h>
#include <stddef.h>

/* Zmienne globalne przechowujące informacje o zakolejkowanych 
   żądaniach sygnałów i ostatnim odebranym sygnale */
static volatile sig_atomic_t *g_pending_requests = NULL;
static volatile sig_atomic_t g_last_signal = 0;

static void ds_signal_handler(int signo)
{
	volatile sig_atomic_t *pending = g_pending_requests;
	if (pending == NULL) {
		return;
	}

    /* Mapowanie odebranego sygnału na odpowiednie flagi 
	   żądań i zapisywanie numeru ostatniego sygnału. */
	if (signo == SIGUSR1) {
		*pending |= DS_REQ_RESCAN;
		g_last_signal = SIGUSR1;
		return;
	}
	if (signo == SIGUSR2) {
		*pending |= DS_REQ_ABORT_SCAN;
		g_last_signal = SIGUSR2;
		return;
	}
	if (signo == SIGINT || signo == SIGTERM) {
		*pending |= DS_REQ_TERMINATE;
		g_last_signal = signo;
	}
}

static int install_one_handler(int signo)
{
	struct sigaction sa;

	sa.sa_handler = ds_signal_handler;
	sigemptyset(&sa.sa_mask);

	/* Blokowanie pozostałych sygnałów w trakcie wykonywania handlera 
	   w celu uniknięcia kolizji i zapewnienia spójności stanu. */
	sigaddset(&sa.sa_mask, SIGUSR1);
	sigaddset(&sa.sa_mask, SIGUSR2);
	sigaddset(&sa.sa_mask, SIGINT);
	sigaddset(&sa.sa_mask, SIGTERM);
	sa.sa_flags = 0;

	return sigaction(signo, &sa, NULL);
}

int ds_signals_install(ds_runtime_state_t *state)
{
	if (state == NULL) {
		return -1;
	}

	state->pending_requests = DS_REQ_NONE;
	g_last_signal = 0;
	g_pending_requests = &state->pending_requests;

	if (install_one_handler(SIGUSR1) != 0) {
		return -1;
	}
	if (install_one_handler(SIGUSR2) != 0) {
		return -1;
	}
	if (install_one_handler(SIGINT) != 0) {
		return -1;
	}
	if (install_one_handler(SIGTERM) != 0) {
		return -1;
	}

	return 0;
}

void ds_signals_uninstall(void)
{
	g_pending_requests = NULL;

	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

int ds_signals_consume_requests(ds_runtime_state_t *state, int *out_last_signal)
{
	sigset_t set;
	sigset_t oldset;
	sig_atomic_t requests;
	sig_atomic_t last;

	if (state == NULL) {
		return DS_REQ_NONE;
	}

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGUSR2);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);

    /* Maskowanie sygnałów na czas odczytu i resetowania flag. */
	sigprocmask(SIG_BLOCK, &set, &oldset);

	requests = state->pending_requests;
	state->pending_requests = DS_REQ_NONE;

	last = g_last_signal;
	g_last_signal = 0;

    /* Przywrócenie oryginalnej maski sygnałów. */
	sigprocmask(SIG_SETMASK, &oldset, NULL);

	if (out_last_signal != NULL) {
		*out_last_signal = (int)last;
	}

	return (int)requests;
}
