#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <utils.h>

// Converts string into another string, substituting '\n' and '\r' by "\n" and "\r"
char *str_without_new_line(char *str, int length) {
	int i = 0, j = 0;
	static char buf[UTILS_BUF_SIZE];

	while (i < length && j < UTILS_BUF_SIZE - 2) {
		if (str[i] == '\n') {
			buf[j++] = '\\';
			buf[j++] = 'n';
		} else if (str[i] == '\r') {
			buf[j++] = '\\';
			buf[j++] = 'r';
		} else {
			buf[j++] = str[i];
		}
		i++;
	}

	buf[j] = '\0';

	return buf;
}

// Waits till one char will be available on stdio, and returns it
char getch() {
	char buf = 0;
	struct termios old = {0};

	if (tcgetattr(0, &old) < 0) return 0;

	old.c_lflag &=~ ICANON;
	old.c_lflag &=~ ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;

	if (tcsetattr(0, TCSANOW, &old) < 0) return 0;

	if (1) { //timeout
		fd_set set;
		struct timeval timeout;

		FD_ZERO(&set);
		FD_SET(0, &set);

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		if (select(1, &set, 0, 0, &timeout) <= 0) {
			return 0;
		}
	}

	if (read(0, &buf, 1) < 0) return 0;

	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;

	if (tcsetattr(0, TCSADRAIN, &old) < 0) return 0;

	return buf;
}
