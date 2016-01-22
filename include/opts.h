#ifndef OPTS_H
#define OPTS_H

int opts_init();
int opts_final();

int opts_mx_port();
int opts_connection_timeout();
const char *opts_maildir_root();
const char *opts_my_domain();

#endif
