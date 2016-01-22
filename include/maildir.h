/**
 * \file maildir.h
 * \brief Файл со структурами и функциями для работы с сообщениями 
 */ 
#ifndef MAILDIR_H
#define MAILDIR_H

#include <queue.h>
#include <stdio.h>


//~ #define MY_DOMAIN "quint.com"
/**
 * \brief Структура для хранения имени и домена получателя
 */
struct rcpt {
	char name[200];
	char domain[100];
	TAILQ_ENTRY(rcpt) entry;
};
TAILQ_HEAD(rcpt_list, rcpt);

/**
 * \brief Структура для хранения писем
 */
struct mail {
	char from[200];
	struct rcpt_list rcpts;
	char *msg;
	int was_sent;
	char *filename;
	TAILQ_ENTRY(mail) entry;
};
TAILQ_HEAD(mail_list, mail);

typedef enum {
	DIR_ROOT,
	DIR_NEW,
	DIR_CUR,
	DIR_NOTSENT,
	maildir_count
} maildir_dir;


// Main functions
int		maildir_init();
int		maildir_final();
void	free_mail(struct mail *m);
void	free_mail_list(struct mail_list *ml);

// Disk operations
int		new_mail_exist();
void	move_mail(const char *filename, maildir_dir from_dir, maildir_dir to_dir);
void	copy_mail(const char *filename, maildir_dir from_dir, maildir_dir to_dir);
void	delete_mail(const char *filename, maildir_dir dir);

// Allocations and operations with maildir structures
int				filter_my_mail(struct mail_list *ml);
int				read_all_mail(struct mail_list *ml);
struct mail*	read_mail_file(const char *filename);
int				read_mail_from(FILE *f, struct mail *m);
int				read_mail_to  (FILE *f, struct mail *m);
int				read_mail_data(FILE *f, struct mail *m);

#endif
