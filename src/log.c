/**
 * \file log.c
 * \brief Логирование сообщений в отдельном процессе.
 * 
 * Передача сообщений ведется через пару сокетов. Возможна раскраска
 * сообщений. Три вида сообщений: отладка, ошибки, обычные.
 */
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <log.h>

/**
 * \var int msg_sock
 * Socket: 1) to get messages (from log); 2) to send messages (from main process)
 */
int msg_sock;

// Local functions: send one message to output and loop waiting messages
int log_message(const char *buf, char type);
void log_loop();


// Create subprocess to log messages
// Returns 1 on success, 0 on failure
int fork_log() {
#ifdef FORKED_LOG
	int fd[2];

	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fd)) {
		log_message("Can't start log: can't create sockets", STDERR_SYMBOL);
		return 0;
	}

	int pid = fork();

	if (pid == -1) {
		log_message("Can't start log: can't create child", STDERR_SYMBOL);
		return 0;
	}

	if (pid == 0) {
		close(fd[1]);
		msg_sock = fd[0];

		log_message(BLUE SPLIT_LINE, STDOUT_SYMBOL);
		log_message(GREEN "Log started.", STDOUT_SYMBOL);
		log_loop();
		log_message(GREEN "Log stopped.", STDOUT_SYMBOL);
		log_message(BLUE SPLIT_LINE, STDOUT_SYMBOL);

		exit(0);
	} else {
		close(fd[0]);
		msg_sock = fd[1];
		return 1;
	}
#else
	log_message(BLUE SPLIT_LINE, STDOUT_SYMBOL);
	log_message(GREEN "Log started.", STDOUT_SYMBOL);
	return 1;
#endif
}


// Loop forever waiting messages until '\0' is received
void log_loop() {
#ifdef FORKED_LOG
	int res;
	int running = 1;

	int i = 0;
	char buf[500];

	while (running) {
		res = recv(msg_sock, buf + i, 1, 0);

		if (res < 1) {
			log_message("Some error occured while logging...", STDERR_SYMBOL);
			running = 0;
		} else {
			if (buf[i] == EXIT_SYMBOL) {
				if (i == 0) {
					running = 0;
				} else {
					log_message(buf+1, buf[0]);
					i = 0;
				}
			} else {
				i++;
			}
		}
	}
#endif
}


// Write one message to either stdout/stderr
int log_message(const char *buf, char type) {
	char timestring[50];
	time_t now = time(0);

	strftime (timestring, 100, "%Y-%m-%d %H:%M:%S", localtime (&now));

	switch (type) {
		case STDOUT_SYMBOL:
			fprintf(stdout, WHITE "[%s] %s" COLOR_RESET "\n", timestring, buf);
			break;
		case STDERR_SYMBOL:
			fprintf(stderr, RED "[%s] [ERROR] %s" COLOR_RESET "\n", timestring, buf);
			break;
		case DEBUG_SYMBOL:
			fprintf(stdout,  "[%s] " BLUE "[DEBUG] " COLOR_RESET "%s" COLOR_RESET "\n", timestring, buf);
			break;
		default:
			fprintf(stderr, RED "[%s] [ERROR] Unrecognised message format." COLOR_RESET "\n", timestring);
			break;
	}

	return 0;
}


/**
 * \fn int close_log()
 * Send stop signal to log from main process and wait till log process
 * finishes his work; if log is not forked, function just returns
 */
int close_log() {
#ifdef FORKED_LOG
	char msg[2] = "!\0";
	msg[0] = EXIT_SYMBOL;
	send_log(msg);
	recv(msg_sock, 0, 0, 0); // waiting log to stop
	return 0;
#else
	log_message(GREEN "Log stopped.", STDOUT_SYMBOL);
	log_message(BLUE SPLIT_LINE, STDOUT_SYMBOL);
	return 0;
#endif
}


// Used in macros to send message to log subprocess
int send_log(char *msg) {
#ifdef FORKED_LOG
	send(msg_sock, msg, strlen(msg)+1, 0);
	return 0;
#else
	log_message(msg+1, msg[0]);
	return 0;
#endif
}
