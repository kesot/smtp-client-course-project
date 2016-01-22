#include <key-listener.h>
#include <protocol.h>
#include <maildir.h>
#include <regexp.h>
#include <opts.h>
#include <log.h>

// Initializes all procesess and required structures
int init() {
	if (!fork_log()) {
		printf(RED "Can't start log. Exiting...\n" COLOR_RESET);
	} else {

		if (!keyboard_listener_fork()) {
			ELOG("Can't start keyboard reader. Exiting...");
		} else {

			if (!opts_init()) {
				ELOG("Can't read options. Exiting...");
			} else {

				if (!re_init()) {
					ELOG("Can't compile regular expressions. Exiting...");
				} else {
					maildir_init();
					return 1;
				}

				opts_final();
			}
		}
	}

	return 0;
}


// Stops all processes and frees allocated structures
void final() {
	maildir_final();
	keyboard_listener_final();
	re_final();
	opts_final();
	close_log();
}


// Main function
int main(int argc, char **argv) {
	if (init()) {
		smtp_client_loop();
		final();
	}

	return 0;
}
