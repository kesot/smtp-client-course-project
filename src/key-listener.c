/** \file key-listener.c
 *  \brief Функции и переменные для разбора команд.
 *
 *  В этом файле описаны функции необходимые для работы key listener
 */
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <key-listener.h>
#include <utils.h>
#include <log.h>

/**
 * \var int key_sock
 * \brief Socket to pass exit signal
 */ 
int key_sock;


// Loop while waiting for 'Q' key
int keyboard_loop() {
	while (1) {
		char c = getch();

		if (c == 'q' || c == 'Q') {
			LOG("Key listener got 'Q' from keyboard, sending quit signal...");
			send(key_sock, "\0", 1, 0);
			break;
		}

		if (recv(key_sock, 0, 0, 0) > 0) {
			ELOG("Seems like connection between main process and key listener broke. Exiting...");
		}
	}

	return 0;
}


// Forkes process, which will wait for 'Q' key
int keyboard_listener_fork() {
	int fd[2];

	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fd)) {
		ELOG("Can't start keyboard reader: can't create sockets");
		return 0;
	}

	int pid = fork();

	if (pid == -1) {
		ELOG("Can't start keyboard reader: can't create child");
		return 0;
	}

	if (pid == 0) {
		close(fd[1]);
		key_sock = fd[0];
		fcntl(key_sock, F_SETFL, O_NONBLOCK);

		LOG(GREEN "Keyboard reader started.");
		keyboard_loop();
		LOG(GREEN "Keyboard reader stopped.");

		exit(0);
	} else {
		close(fd[0]);
		key_sock = fd[1];
		fcntl(key_sock, F_SETFL, O_NONBLOCK);
		return 1;
	}
}


// Returns 1 if 'Q' was pressed; 0 otherwise
int quit_key_pressed() {
	return recv(key_sock, 0, 0, 0) >= 0;
}


// Waits till listener will stop its work
int keyboard_listener_final() {
	fcntl(key_sock, F_SETFL, !O_NONBLOCK);
	recv(key_sock, 0, 0, 0);
	return 0;
}
