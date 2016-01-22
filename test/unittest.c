#include <CUnit/Basic.h>
#include <stdlib.h>

#include <client-fsm.h>
#include <protocol.h>
#include <maildir.h>
#include <regexp.h>
#include <utils.h>
#include <opts.h>
#include <log.h>


struct test {
	void (*func)(void);
	char *name;
};

void maildir_01_test() {
	CU_ASSERT(read_mail_file("testmail1") != 0);
}

void maildir_02_test() {
	CU_ASSERT(read_mail_file("testmail2") == 0);
}

void maildir_03_test() {
	CU_ASSERT(read_mail_file("testmail3") != 0);
}

void maildir_04_test() {
	CU_ASSERT(read_mail_file("testmail4") == 0);
}

void maildir_05_test() {
	CU_ASSERT(read_mail_file("testmail5") == 0);
}

void maildir_06_test() {
	CU_ASSERT(read_mail_file("testmail6") == 0);
}


void regexp_01_test() {
	char *msg = "220 hello!\r\n";
	CU_ASSERT(re_match(r220, msg, strlen(msg)));
}

void regexp_02_test() {
	char *msg = "354- nope\r\n";
	CU_ASSERT(!re_match(r354, msg, strlen(msg)));
}

void regexp_03_test() {
	char *msg = "MAIL FROM:  <somemail@mail.com>\r\n";
	CU_ASSERT(re_match(RE_mail_from, msg, strlen(msg)));
}

void regexp_04_test() {
	char *msg = "RCPT TO:<mmm%mail.com>\r\n";
	CU_ASSERT(!re_match(RE_rcpt_to, msg, strlen(msg)));
}

void regexp_05_test() {
	char *msg = "RCPT TO:<mmm@mail.com\r\n";
	CU_ASSERT(!re_match(RE_rcpt_to, msg, strlen(msg)));
}

void regexp_06_test() {
	char *msg = "250 OK\r\n";
	CU_ASSERT(re_match_any(msg, strlen(msg)) == r250);
}

void regexp_07_test() {
	char *msg = "DATA\r\n";
	CU_ASSERT(re_match_any(msg, strlen(msg)) == RE_data);
}

void regexp_08_test() {
	char *msg = "RCPT TO: <mmm@mao.mao>\r\n";
	CU_ASSERT(re_match_any(msg, strlen(msg)) == RE_rcpt_to);
}

void regexp_09_test() {
	char *msg = "MAIL FROM: <mail@mail.com>\r\n";
	CU_ASSERT(!re_match(RE_rcpt_to, msg, strlen(msg)));
}

void fsm_01_test() {
	struct mail *m = read_mail_file("testmailfsm1");
	CU_ASSERT(m != NULL);
	if (m == NULL) return;

	struct domain dom;
	strcpy(dom.name, "mail.com");

	struct mail_list ml;
	TAILQ_INIT(&ml);
	TAILQ_INSERT_TAIL(&ml, m, entry);

	struct mx_conn *conn = malloc(sizeof(*conn));
	conn->state = SMTP_CLIENT_FSM_ST_INIT;
	conn->m = m;
	conn->r = TAILQ_FIRST(&m->rcpts);
	conn->dom = &dom;

	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R220, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R354, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R221, conn);

	CU_ASSERT(conn->state == SMTP_CLIENT_FSM_ST_DONE);
}

void fsm_02_test() {
	struct mail *m1 = read_mail_file("testmailfsm2");
	CU_ASSERT(m1 != NULL);
	if (m1 == NULL) return;

	struct mail *m2 = read_mail_file("testmailfsm3");
	CU_ASSERT(m2 != NULL);
	if (m2 == NULL) return;

	struct domain dom;
	strcpy(dom.name, "gmail.com");

	struct mail_list ml;
	TAILQ_INIT(&ml);
	TAILQ_INSERT_TAIL(&ml, m1, entry);
	TAILQ_INSERT_TAIL(&ml, m2, entry);

	struct mx_conn *conn = malloc(sizeof(*conn));
	conn->state = SMTP_CLIENT_FSM_ST_INIT;
	conn->m = m1;
	conn->r = TAILQ_FIRST(&m1->rcpts);
	conn->dom = &dom;

	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R220, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R354, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R354, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R221, conn);
	CU_ASSERT(conn->state == SMTP_CLIENT_FSM_ST_DONE);
}

void fsm_03_test() {
	struct mail *m = read_mail_file("testmailfsm1");
	CU_ASSERT(m != NULL);
	if (m == NULL) return;

	struct domain dom;
	strcpy(dom.name, "mail.com");

	struct mail_list ml;
	TAILQ_INIT(&ml);
	TAILQ_INSERT_TAIL(&ml, m, entry);

	struct mx_conn *conn = malloc(sizeof(*conn));
	conn->state = SMTP_CLIENT_FSM_ST_INIT;
	conn->m = m;
	conn->r = TAILQ_FIRST(&m->rcpts);
	conn->dom = &dom;

	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R220, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R250, conn);
	conn->state = smtp_client_fsm_step(conn->state, SMTP_CLIENT_FSM_EV_R354, conn);

	CU_ASSERT(conn->state == SMTP_CLIENT_FSM_ST_INVALID);
}


int init_maildir_suite() {
	maildir_init();
	re_init();
	return 0;
}

int clean_maildir_suite() {
	maildir_final();
	re_final();
	return 0;
}

int init_regexp_suite() {
	re_init();
	return 0;
}

int clean_regexp_suite() {
	re_final();
	return 0;
}

int init_fsm_suite() {
	maildir_init();
	re_init();
	return 0;
}

int clean_fsm_suite() {
	maildir_final();
	re_final();
	return 0;
}

struct test maildir_tests[] = {
	{maildir_01_test, "Correct mail file."},
	{maildir_03_test, "Mail file with multiple recipients."},
	{maildir_05_test, "Mail file with multiple senders."},
	{maildir_02_test, "Mail file without recipients."},
	{maildir_06_test, "Mail file without DATA."},
	{maildir_04_test, "Mail file without dot."}
};

struct test regexp_tests[] = {
	{regexp_01_test, "Correct R220."},
	{regexp_03_test, "Correct MAIL FROM."},
	{regexp_02_test, "Incorrect R354."},
	{regexp_04_test, "Incorrect RCPT TO."},
	{regexp_05_test, "Incorrect RCPT TO."},
	{regexp_09_test, "Correct regexp, but incorrect param."},
	{regexp_06_test, "Match any, should be r250."},
	{regexp_07_test, "Match any, should be DATA."},
	{regexp_08_test, "Match any, should be RCPT TO."},
};

struct test fsm_tests[] = {
	{fsm_01_test, "Correct minimal session."},
	{fsm_02_test, "Correct session with 2 mails with multiple recipients."},
	{fsm_03_test, "Incorrect session."},
};

int main(int argc, char **argv) {
	opts_init();

	CU_pSuite maildir_suite = NULL;
	CU_pSuite regexp_suite = NULL;
	CU_pSuite fsm_suite = NULL;

	if (CU_initialize_registry() != CUE_SUCCESS) goto exit;

	if (!(maildir_suite = CU_add_suite("Test maildir.", init_maildir_suite, clean_maildir_suite))) goto clean;
	for (int i = 0; i < sizeof(maildir_tests) / sizeof(struct test); ++i) {
		if (!CU_add_test(maildir_suite, maildir_tests[i].name, maildir_tests[i].func)) goto clean;
	}

	if (!(regexp_suite = CU_add_suite("Test regexp.", init_regexp_suite, clean_regexp_suite))) goto clean;
	for (int i = 0; i < sizeof(regexp_tests) / sizeof(struct test); ++i) {
		if (!CU_add_test(regexp_suite, regexp_tests[i].name, regexp_tests[i].func)) goto clean;
	}

	if (!(fsm_suite = CU_add_suite("Test FSM.", init_fsm_suite, clean_fsm_suite))) goto clean;
	for (int i = 0; i < sizeof(fsm_tests) / sizeof(struct test); ++i) {
		if (!CU_add_test(fsm_suite, fsm_tests[i].name, fsm_tests[i].func)) goto clean;
	}

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();

clean:
	CU_cleanup_registry();
exit:
	opts_final();
	return CU_get_error();
}
