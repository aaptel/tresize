#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define strsz(x) x,sizeof(x)-1
#define E(fmt, ...) fprintf(stderr, fmt "\n",##__VA_ARGS__)

void tty_raw(int fd, struct termios *init)
{
	struct termios raw;

	raw = *init;  /* copy original and then modify below */

	/* input modes - clear indicated ones giving: no break, no CR to NL,
	   no parity check, no strip char, no start/stop output (sic) control */
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

	/* output modes - clear giving: no post processing such as NL to CR+NL */
	raw.c_oflag &= ~(OPOST);

	/* control modes - set 8 bit chars */
	raw.c_cflag |= (CS8);

	/* local modes - clear giving: echoing off, canonical off (no erase with
	   backspace, ^U,...),  no extended functions, no signal chars (^Z,^C) */
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	/* control chars - set return condition: min number of bytes and timer */
	raw.c_cc[VMIN] = 5; raw.c_cc[VTIME] = 8; /* after 5 bytes or .8 seconds
						    after first byte seen      */
	raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0; /* immediate - anything       */
	raw.c_cc[VMIN] = 2; raw.c_cc[VTIME] = 0; /* after two bytes, no timer  */
	raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 8; /* after a byte or .8 seconds */

	tcsetattr(fd, TCSAFLUSH, &raw);
}

int parse_sizes(char *buf, ssize_t size, int *w, int *h)
{
	errno = 0;
	if (sscanf(buf, "\033[%d;%dR", w, h) != 2 || errno)
		return -1;
	return 0;
}

int main(void)
{
	int fd;
	char buf[64] = {0};
	ssize_t ret;
	int rc = 0;
	struct termios init;
	int w, h;

	fd = open("/dev/tty", O_RDWR);
	if (fd < 0) {
		E("open");
		return 1;
	}

	rc = tcgetattr(fd, &init);
	if (rc < 0) {
		E("can't get tty settings");
		return 1;
	}

	tty_raw(fd, &init);
	write(fd, strsz("\0337\33[r\33[999;999H\33[6n"));
	ret = read(fd, buf, sizeof(buf)-1);
	if (ret <= 0) {
		E("read: %s", strerror(errno));
		rc = 1;
		goto out;
	}

	rc = parse_sizes(buf, ret, &w, &h);
	if (rc < 0) {
		E("couldn't parse terminal size");
		goto out;
	}



out:
	tcsetattr(fd, TCSAFLUSH, &init);
	if (rc == 0)
		printf("\nCOLUMNS=%d\nLINES=%d\nexport COLUMNS LINES\n", w, h);

	return 0;
}
