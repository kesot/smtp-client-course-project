/**
 * \file protocol.h
 * \brief Файл содержащий структуры и функции для работы по протоколу SMTP
 */ 
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <queue.h>
#include <time.h>

#include <client-fsm.h>
#include <maildir.h>


//~ #define MX_PORT "25"

//~ #define CONN_TIMEOUT (12)

struct mx_conn {
	int sock;
	te_smtp_client_fsm_state state;
	struct mail *m;
	struct rcpt *r;
	struct domain *dom;
	time_t time_of_last_response;
	TAILQ_ENTRY(mx_conn) entry;
};
TAILQ_HEAD(mx_conn_list, mx_conn);

struct domain {
	char name[100];
	TAILQ_ENTRY(domain) entry;
};
TAILQ_HEAD(domain_set, domain);


// Main functions
int		smtp_client_loop();
int		conn_init();
void	conn_loop();
int		conn_final();

// Domain related functions
void	domain_add(struct domain_set *domains, char *new_domain_name);
int		rcpt_is_from_domain(struct rcpt *r, struct domain *d);
int		mail_has_rcpts_from_domain(struct mail *m, struct domain *d);

// Connection related stuff
int				check_dns(char *d, char *output_address);
struct mx_conn*	create_connection(struct domain *dom);
struct mx_conn*	get_conn_by_socket(struct mx_conn_list *cl, int sock);
int				wait_for_response();
int				parse_response(struct mx_conn *conn, char *str, int length);
void			invalidate_connection(struct mx_conn *conn);

// Protocol realted stuff
int send_hello(struct mx_conn *conn);
int send_mailfrom(struct mx_conn *conn);
int send_rcptto(struct mx_conn *conn);
int send_data(struct mx_conn *conn);
int send_datastr(struct mx_conn *conn);
int send_quit(struct mx_conn *conn);

#endif
