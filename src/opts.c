#include <libconfig.h>
#include <opts.h>
#include <log.h>

config_t cfg;

int opts_init() {
	config_init(&cfg);

	if (!config_read_file(&cfg, "client.cfg")) {
		ELOG("Can't open configuration file for this program: %s[%d] - %s.",
			config_error_file(&cfg),
			config_error_line(&cfg),
			config_error_text(&cfg)
		);

		LOG(YELLOW "[Warning] Config file was not found.");

		//~ config_destroy(&cfg);

		return 1;
	}

	return 1;
}

int opts_final() {
	config_destroy(&cfg);
	return 1;
}

int opts_connection_timeout() {
	int timeout = 12;
	config_lookup_int(&cfg, "client.timeout", &timeout);
	return timeout;
}

int opts_mx_port() {
	int port = 25;
	config_lookup_int(&cfg, "client.port", &port);
	return port;
}

const char *opts_my_domain() {
	const char *my_domain = "quint.com";
	config_lookup_string(&cfg, "client.domain", &my_domain);
	return my_domain;
}

const char *opts_maildir_root() {
	const char *root = "../maildir";
	config_lookup_string(&cfg, "client.maildir", &root);
	return root;
}
