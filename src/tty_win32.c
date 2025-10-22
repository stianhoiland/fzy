#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <signal.h>
#include <errno.h>

#include "tty.h"

#include "../config.h"

struct conmode {
	DWORD in, out;
};

struct tty {
	HANDLE hin;
	HANDLE hout;
	struct conmode original_conmode;
	int fgcolor;
	size_t maxwidth;
	size_t maxheight;
};

void tty_reset(tty_t *tty) {
	SetConsoleMode(tty->hin, tty->original_conmode.in);
	SetConsoleMode(tty->hout, tty->original_conmode.out);
}

void tty_close(tty_t *tty) {
	tty_reset(tty);
	CloseHandle(tty->hin);
	CloseHandle(tty->hout);
	free(tty);
}

static void handle_sigwinch(int sig){
	(void)sig;
}

tty_t *tty_init(const char *tty_filename) {
	tty_t *tty = malloc(sizeof(*tty));

	tty->hin = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (tty->hin == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Failed to open CONIN$: error %ld\n", GetLastError());
		exit(EXIT_FAILURE);
	}

	tty->hout = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (tty->hout == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Failed to open CONOUT$: error %ld\n", GetLastError());
		exit(EXIT_FAILURE);
	}

	if (GetConsoleMode(tty->hin, &tty->original_conmode.in) == 0) {
		fprintf(stderr, "Failed to get console mode for CONIN$: error %ld\n", GetLastError());
		exit(EXIT_FAILURE);
	}

	if (GetConsoleMode(tty->hout, &tty->original_conmode.out) == 0) {
		fprintf(stderr, "Failed to get console mode for CONOUT$: error %ld\n", GetLastError());
		exit(EXIT_FAILURE);
	}

	DWORD raw_in = tty->original_conmode.in;
	DWORD raw_out = tty->original_conmode.out;

    // Raw modes
    raw_in &= ~ENABLE_PROCESSED_INPUT;
    raw_in &= ~ENABLE_LINE_INPUT;
    raw_in &= ~ENABLE_ECHO_INPUT;
    raw_out |= ENABLE_PROCESSED_OUTPUT;

    // VT modes
	raw_in  |= ENABLE_VIRTUAL_TERMINAL_INPUT;
	raw_out |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	raw_out |= DISABLE_NEWLINE_AUTO_RETURN;

	if (SetConsoleMode(tty->hin, raw_in) == 0) {
		fprintf(stderr, "Failed to set raw console mode for CONIN$: error %ld\n", GetLastError());
	}
	if (SetConsoleMode(tty->hout, raw_out) == 0) {
		fprintf(stderr, "Failed to set raw console mode for CONOUT$: error %ld\n", GetLastError());
	}

	tty_getwinsz(tty);

	tty_setnormal(tty);

	//signal(SIGWINCH, handle_sigwinch);

	return tty;
}

void tty_getwinsz(tty_t *tty) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(tty->hout, &csbi) == 0) {
		tty->maxwidth = 80;
		tty->maxheight = 25;
	} else {
		tty->maxwidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		tty->maxheight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	}
}

char tty_getchar(tty_t *tty) {
	char ch;
	unsigned long size = 0;
	if (ReadConsoleA(tty->hin, &ch, 1, &size, NULL) == 0) {
		fprintf(stderr, "ReadConsoleA: error %ld\n", GetLastError());
		exit(EXIT_FAILURE);
	} else if (size == 0) {
		/* EOF */
		exit(EXIT_FAILURE);
	} else {
		return ch;
	}
}

int tty_input_ready(tty_t *tty, long int timeout, int return_on_signal) {
	return WaitForSingleObject(tty->hin, timeout) == 0;
}

static void tty_sgr(tty_t *tty, int code) {
	tty_printf(tty, "%c%c%im", 0x1b, '[', code);
}

void tty_setfg(tty_t *tty, int fg) {
	if (tty->fgcolor != fg) {
		tty_sgr(tty, 30 + fg);
		tty->fgcolor = fg;
	}
}

void tty_setinvert(tty_t *tty) {
	tty_sgr(tty, 7);
}

void tty_setunderline(tty_t *tty) {
	tty_sgr(tty, 4);
}

void tty_setnormal(tty_t *tty) {
	tty_sgr(tty, 0);
	tty->fgcolor = 9;
}

void tty_setnowrap(tty_t *tty) {
	tty_printf(tty, "%c%c?7l", 0x1b, '[');
}

void tty_setwrap(tty_t *tty) {
	tty_printf(tty, "%c%c?7h", 0x1b, '[');
}

void tty_newline(tty_t *tty) {
	tty_printf(tty, "\r\n");
}

void tty_clearline(tty_t *tty) {
	tty_printf(tty, "%c%cK", 0x1b, '[');
}

void tty_setcol(tty_t *tty, int col) {
	tty_printf(tty, "%c%c%iG", 0x1b, '[', col + 1);
}

void tty_moveup(tty_t *tty, int i) {
	tty_printf(tty, "%c%c%iA", 0x1b, '[', i);
}

void tty_printf(tty_t *tty, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char buf[1024];
	WriteConsoleA(tty->hout, buf, vsnprintf(buf, sizeof(buf), fmt, args), 0, 0);
	va_end(args);
}

void tty_puts(tty_t *tty, char *s) {
	WriteConsoleA(tty->hout, s, strlen(s), 0, 0);
}

void tty_putc(tty_t *tty, char c) {
	WriteConsoleA(tty->hout, &c, 1, 0, 0);
}

void tty_flush(tty_t *tty) {
	(void)tty;
}

size_t tty_getwidth(tty_t *tty) {
	return tty->maxwidth;
}

size_t tty_getheight(tty_t *tty) {
	return tty->maxheight;
}
