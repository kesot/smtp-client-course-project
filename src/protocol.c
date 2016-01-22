/**
 * \file protocol.c
 * \brief Файл содержащий структуры и функции для работы по протоколу SMTP
 */ 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <resolv.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include <key-listener.h>
#include <protocol.h>
#include <regexp.h>
#include <utils.h>
#include <opts.h>
#include <log.h>

#include <sys/poll.h>


struct mail_list *mails;
struct domain_set *domains;
struct mx_conn_list *connections;
static int connectionsCount;


/**
 * \fn int smtp_client_loop()
 * \brief Основная функция, которая мониторит директорию с почтой, чтобы ее отправить
 */
int smtp_client_loop() {
	while (1) {
		if (quit_key_pressed()) break;

		int mailcount = new_mail_exist();

		if (!mailcount) {
			LOG("No new mail. Sleeping. zzzzzz...");
			sleep(1);
		} else {
			LOG("New mail found! [%d]", mailcount);

			if (conn_init()) {
				conn_loop();
				conn_final();
			}
		}
	}

	return 0;
}

/**
 * \fn int conn_init()
 * \brief Initialize structures for connections with several SMTP servers
 */
// Initialize structures for connections with several SMTP servers
int conn_init() {
	mails		= calloc(1, sizeof(*mails));
	domains		= calloc(1, sizeof(*domains));
	connections = calloc(1, sizeof(*connections));
	
	connectionsCount = 0;
	
	TAILQ_INIT(mails);
	TAILQ_INIT(domains);
	TAILQ_INIT(connections);

	if (!read_all_mail(mails)) {
		ELOG("Can't read mail, aborting mail transfer.");
		return 0;
	}

	struct mail *m;
	struct rcpt *r;
	TAILQ_FOREACH(m, mails, entry) {
		TAILQ_FOREACH(r, &m->rcpts, entry) {
			domain_add(domains, r->domain);
		}
	}

	struct domain *d;
	struct mx_conn *conn;
	TAILQ_FOREACH(d, domains, entry) {
		if ((conn = create_connection(d))) {
			TAILQ_INSERT_TAIL(connections, conn, entry);
			++connectionsCount;
		}
	}

	return 1;
}


int free_connection(struct mx_conn *conn) {
	TAILQ_REMOVE(connections, conn, entry);
	close(conn->sock);
	free(conn);
	return 0;
}


int free_domain(struct domain *d) {
	TAILQ_REMOVE(domains, d, entry);
	free(d);
	return 0;
}

/**
 * \fn int conn_final()
 * \brief Frees all structures used in mail transfer
 */
// Frees all structures used in mail transfer
int conn_final() {
	struct mx_conn *conn, *conn_tmp;
	TAILQ_FOREACH_SAFE(conn, connections, entry, conn_tmp) {
		free_connection(conn);
	}

	struct domain *d, *d_tmp;
	TAILQ_FOREACH_SAFE(d, domains, entry, d_tmp) {
		free_domain(d);
	}

	free(domains);
	free(connections);
	free_mail_list(mails);

	return 0;
}

/**
 * \fn domain_add(struct domain_set *domains, char *new_domain_name)
 * \brief Adds another domain into domain set if it is not present there and if it is not local domain (MY_DOMAIN in maildir.h)
 * \param domains -- список доменов куда нужно добавить
 * \param new_domain_name -- имя домена для добавления
 */
// Adds another domain into domain set if it is not present there and
// if it is not local domain (MY_DOMAIN in maildir.h)
void domain_add(struct domain_set *domains, char *new_domain_name) {
	int exist = 0;

	if (strcmp(new_domain_name, opts_my_domain()) == 0) return;

	struct domain *d;
	TAILQ_FOREACH(d, domains, entry) {
		if (strcmp(d->name, new_domain_name) == 0) exist = 1;
	}

	if (!exist) {
		d = malloc(sizeof(*d));
		strcpy(d->name, new_domain_name);
		TAILQ_INSERT_TAIL(domains, d, entry);
	}
}


// Returns 1 if recipient is from specified domain; otherwise 0
int rcpt_is_from_domain(struct rcpt *r, struct domain *d) {
	return strcmp(r->domain, d->name) == 0;
}


// Returns 1 if mail has recipient from specified domain; 0 otherwise
int mail_has_rcpts_from_domain(struct mail *m, struct domain *d) {
	int ret = 0;

	struct rcpt *r;
	TAILQ_FOREACH(r, &m->rcpts, entry) {
		if (rcpt_is_from_domain(r, d)) ret = 1;
	}

	return ret;
}


// Tries to connect to remote host with timeout; returns 0 on failure,
// or returns socket on success.
int connect_with_timeout(int sock, struct addrinfo *addr, int ms) {
	struct pollfd fd[1];
	fd[0].fd = sock;
	fd[0].events = POLLOUT;
	
	fcntl(sock, F_SETFL, O_NONBLOCK);

	if (connect(sock, addr->ai_addr, addr->ai_addrlen) < 0) {
		if (errno != EINPROGRESS || poll(fd, 1, ms) <= 0) {
			return 0;
		}
	}

	fcntl(sock, F_SETFL, !O_NONBLOCK);

	return sock;
}


// Tries to connect to any possible IPs of specified host; returns 0 on
// failure, or socket on success
int connect_to_any_with_timeout(struct addrinfo *info, int ms) {
	int sock;
	struct addrinfo *addr;

	for (addr = info; addr != NULL; addr = addr->ai_next) {
		sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

		if (sock < 0) {
			ELOG("Can't create a socket.");
			continue;
		}

		if (!connect_with_timeout(sock, addr, ms)) {
			close(sock);
			DLOG("Couldn't connect to one of the possible addresses: '%s'; trying next one...", addr->ai_canonname);
			continue;
		}

		break;
	}

	if (addr == NULL) {
		return 0;
	}

	DLOG("Sucessfully connected to one of the possible addresses: '%s'.", addr->ai_canonname);

	return sock;
}


// Tries to establish connection with MX from specified domain; returns
// 0 on failure, or a pointer to mx_conn structure on success
struct mx_conn* create_connection(struct domain *dom) {
	LOG(BLUE "Connecting to MX on domain '%s'.", dom->name);

	char mx_address[200];
	if (!check_dns(dom->name, mx_address)) {
		return 0;
	}

	struct addrinfo hints, *servinfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	char mx_port[5];
	sprintf(mx_port, "%d", opts_mx_port());

	if (getaddrinfo(mx_address, "25", &hints, &servinfo) != 0) {
		ELOG("Can't get address info about MX '%s'.", mx_address);
		return 0;
	}

	int sock = connect_to_any_with_timeout(servinfo, 500);

	freeaddrinfo(servinfo);

	if (!sock) {
		ELOG("Can't connect to MX '%s'.", mx_address);
		return 0;
	} else {
		LOG(GREEN "Sucessfully connected to MX '%s'.", mx_address);
	}

	struct mx_conn *conn = malloc(sizeof(*conn));
	conn->state = SMTP_CLIENT_FSM_ST_INIT;
	conn->time_of_last_response = time(0);
	conn->sock = sock;
	conn->dom = dom;

	conn->m = TAILQ_FIRST(mails);
	while (!mail_has_rcpts_from_domain(conn->m, dom)) {
		conn->m = TAILQ_NEXT(conn->m, entry);
	}

	conn->r = TAILQ_FIRST(&conn->m->rcpts);
	while (!rcpt_is_from_domain(conn->r, dom)) {
		conn->r = TAILQ_NEXT(conn->r, entry);
	}

	return conn;
}


// Returns address of MX server for given domain in 'output address'
int check_dns(char *d, char *output_address) {
	u_char nsbuf[4096];
	char dispbuf[4096];
	ns_msg msg;
	ns_rr rr;

	LOG(BLUE "Making DNS request for MX entries for domain '%s'.", d);

	int l = res_query(d, ns_c_any, ns_t_mx, nsbuf, sizeof(nsbuf));

	if (l < 0) {
		ELOG("Couldn't make DNS request. Code: %d.", l);
		return 0;
	}

	int prio, prio_best = 999999;
	char prio_str[10];
	char dns_addr[200], dns_addr_best[200];

	ns_initparse(nsbuf, l, &msg);
	l = ns_msg_count(msg, ns_s_an);

	for (int i = 0; i < l; ++i) {
		int matches = 0;
		ns_parserr(&msg, ns_s_an, i, &rr);
		ns_sprintrr(&msg, &rr, NULL, NULL, dispbuf, sizeof(dispbuf));

		matches |= re_match_and_fill_substring(RE_mx_dns, dispbuf, strlen(dispbuf), dns_addr);
		matches |= re_match_and_fill_substring(RE_mx_dns_prio, dispbuf, strlen(dispbuf), prio_str);

		if (matches) {
			prio = (int)strtol(prio_str, 0, 10);

			if (prio < prio_best) {
				prio_best = prio;
				strcpy(dns_addr_best, dns_addr);
			}

			DLOG("DNS MX entry '%s' matches, prio=%d, addr='%s'.", dispbuf, prio, dns_addr);
		}
	}

	if (prio_best < 999999) {
		LOG(BLUE "Best DNS entry for domain '%s': '%s'[%d].", d, dns_addr_best, prio_best);
		strcpy(output_address, dns_addr_best);
		return 1;
	} else {
		ELOG("No MX entries in DNS response for domain '%s'.", d);
		output_address[0] = '\0';
		return 0;
	}
}


// Returns mx_conn struct for given socket; 0 if it is not present
struct mx_conn* get_conn_by_socket(struct mx_conn_list *cl, int sock) {
	struct mx_conn *conn, *rc = 0;

	TAILQ_FOREACH(conn, cl, entry) {
		if (conn->sock == sock) rc = conn;
	}

	if (!rc) DLOG("Warning: connection by socket not found.");

	return rc;
}


// Loop for connections state machine
void conn_loop() {
	while (!TAILQ_EMPTY(connections)) {
		wait_for_response();

		struct mx_conn *conn, *conn_tmp;
		TAILQ_FOREACH_SAFE(conn, connections, entry, conn_tmp) {
			int remove = 0;

			if (difftime(time(0), conn->time_of_last_response) > opts_connection_timeout()) {
				ELOG("Timeout for connection with domain '%s'.", conn->dom->name);
				invalidate_connection(conn);
			}

			if (conn->state == SMTP_CLIENT_FSM_ST_DONE) {
				LOG(GREEN "All mail for domain '%s' was successfully sent!", conn->dom->name);
				remove = 1;
			}

			if (conn->state == SMTP_CLIENT_FSM_ST_INVALID) {
				ELOG("Connection with domain '%s' was marked as invalid. Aborting mail transfer.", conn->dom->name);
				remove = 1;
			}

			if (remove) {
				free_connection(conn);
			}
		}
	}

	struct mail *m;
	TAILQ_FOREACH(m, mails, entry) {
		if (m->was_sent) {
			LOG("Mail '%s' was successfully sent. Deleting file from NEW directory.", m->filename);
			delete_mail(m->filename, DIR_NEW);
		} else {
			ELOG("Mail '%s' was not sent. Moving it to NOT_SENT directory.", m->filename);
			move_mail(m->filename, DIR_NEW, DIR_NOTSENT);
		}
	}

	LOG(GREEN "All connections were finished. Waiting for another mail...");
}


// Parses response from MX server and activates state machine
int parse_response(struct mx_conn *conn, char *str, int length) {
	te_smtp_client_fsm_event event;

	switch (re_match_any(str, length)) {
		case r220: 	event = SMTP_CLIENT_FSM_EV_R220;	break;
		case r221:	event = SMTP_CLIENT_FSM_EV_R221;	break;
		case r250:	event = SMTP_CLIENT_FSM_EV_R250;	break;
		case r354:	event = SMTP_CLIENT_FSM_EV_R354;	break;
		default:	event = SMTP_CLIENT_FSM_EV_INVALID;
					ELOG(BLUE "[%s] " RED "Received unexpected message: '%s'.",
							conn->dom->name,
							str_without_new_line(str, length)
					);
					break;
	}

	conn->time_of_last_response = time(0);

	conn->state = smtp_client_fsm_step(conn->state, event, conn);

	return 0;
}


// Marks connections as invalid
void invalidate_connection(struct mx_conn *conn) {
	conn->state = SMTP_CLIENT_FSM_ST_INVALID;
}


// Waiting for response from any SMTP server; if there was any, returns
// 1, or 0 otherwise
int wait_for_response() {
	char buf[500];

	fd_set readfds;
	FD_ZERO(&readfds);

	struct pollfd pfds[connectionsCount];
	struct mx_conn *conn;
	
	int i = 0;
	TAILQ_FOREACH(conn, connections, entry) {
		pfds[i].fd = conn->sock;
		pfds[i++].events = POLLIN;
	}

	int res = poll(pfds, connectionsCount, 5000);

	if (res == -1) {
		ELOG("Can't use 'poll()' on multiple connections.");
		return 0;
	} else if (res == 0) {
		DLOG(MAGENTA "Timeout, no responses from any of connections.");
		return 0;
	}

	for (int i = 0; i < res; ++i) {
		if (pfds[i].revents & POLLIN) {
			conn = get_conn_by_socket(connections, pfds[i].fd);

			res = recv(conn->sock, buf, sizeof(buf), 0);

			if (res == -1) {
				ELOG("Can't recieve any data from MX '%s'.", conn->dom->name);
				invalidate_connection(conn);
				return 0;
			} else if (res == 0) {
				ELOG("MX '%s' disconnected.", conn->dom->name);
				invalidate_connection(conn);
				return 0;
			} else {
				DLOG(BLUE "[%s] " MAGENTA "recv [%d]: >%s",
						((struct mx_conn*)conn)->dom->name,
						res,
						str_without_new_line(buf, res)
				);

				parse_response(conn, buf, res);
				return 1;
			}
		}
	}

	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Below are functions, used in state machine to communicate
 * with external SMTP servers;
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// Send HELLO message to SMTP server
int send_hello(struct mx_conn *conn) {
	const char *msg_helo = "HELO quint.nope\r\n";
	send(conn->sock, msg_helo, strlen(msg_helo), 0);

	return 0;
}


// Send MAIL FROM message to SMTP server
int send_mailfrom(struct mx_conn *conn) {
	char *msg = conn->m->from;
	send(conn->sock, msg, strlen(msg), 0);

	return 0;
}


// Send RCPT TO message to SMTP server
int send_rcptto(struct mx_conn *conn) {
	while (conn->r && !rcpt_is_from_domain(conn->r, conn->dom)) {
		conn->r = TAILQ_NEXT(conn->r, entry);
	}

	if (conn->r) {
		char msg[200];
		sprintf(msg, "RCPT TO: <%s>\r\n", conn->r->name);

		send(conn->sock, msg, strlen(msg), 0);
		conn->r = TAILQ_NEXT(conn->r, entry);

		return 1;
	}

	return 0;
}


// Send DATA message to SMTP server
int send_data(struct mx_conn *conn) {
	const char *msg_data = "DATA\r\n";
	send(conn->sock, msg_data, strlen(msg_data), 0);

	return 0;
}


// Send mail message to SMTP server
int send_datastr(struct mx_conn *conn) {
	send(conn->sock, conn->m->msg, strlen(conn->m->msg), 0);

	return 0;
}


// Send QUIT message to SMTP server
int send_quit(struct mx_conn *conn) {
	const char *msg_quit = "QUIT\r\n";
	send(conn->sock, msg_quit, strlen(msg_quit), 0);

	return 0;
}

