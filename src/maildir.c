/**
 * \file maildir.c
 * \brief Файл со структурами и функциями для работы с сообщениями 
 */ 
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <maildir.h>
#include <regexp.h>
#include <utils.h>
#include <opts.h>
#include <log.h>


// Array of all dir paths in MAILDIR directory
char *maildir_path[maildir_count];


// Allocates and itinializes all maildir path strings
int maildir_init() {
	const char *root = opts_maildir_root(); //"../maildir";
	const char *new = "/new";
	const char *cur = "/cur";
	const char *not_sent = "/not_sent";

	maildir_path[DIR_ROOT]		= malloc(strlen(root) + 1);
	maildir_path[DIR_NEW]		= malloc(strlen(root) + 1 + strlen(new));
	maildir_path[DIR_CUR]		= malloc(strlen(root) + 1 + strlen(cur));
	maildir_path[DIR_NOTSENT]	= malloc(strlen(root) + 1 + strlen(not_sent));

	sprintf(maildir_path[DIR_ROOT],		"%s",   root);
	sprintf(maildir_path[DIR_NEW],		"%s%s", root, new);
	sprintf(maildir_path[DIR_CUR],		"%s%s", root, cur);
	sprintf(maildir_path[DIR_NOTSENT],	"%s%s", root, not_sent);

	return 1;
}


// Frees all maildir path strings
int maildir_final() {
	for (int i = 0; i < maildir_count; ++i) {
		free(maildir_path[i]);
	}

	return 1;
}


// Returns count of mail files in MAILDIR/NEW/ directory
int new_mail_exist() {
	int ret = 0;

	struct dirent *dir;
	DIR *root = opendir(maildir_path[DIR_NEW]);

	if (root) {
		while ((dir = readdir(root)) != 0) {
			if (dir->d_type == DT_REG) {
				ret += 1;
			}
		}

		closedir(root);
	}

	return ret;
}


// Allocates structures for and reads all mail in MAILDIR/NEW directory;
// returns count of mail files successfully read, or 0 on failure
int read_all_mail(struct mail_list *ml) {
	int total = 0, success = 0;

	struct dirent *dir;
	DIR *root = opendir(maildir_path[DIR_NEW]);

	if (root) {
		while ((dir = readdir(root)) != 0) {
			if (dir->d_type == DT_REG) {
				LOG(YELLOW "Found mail file: '%s'.", dir->d_name);

				struct mail *m = read_mail_file(dir->d_name);
				if (m) {
					TAILQ_INSERT_TAIL(ml, m, entry);
					success++;
				}

				total++;
			}
		}

		if (!success) {
			ELOG("None of %d mail files were read.", total);
			return 0;
		}

		LOG(YELLOW "Successfully read %d/%d mail files.", success, total);
		filter_my_mail(ml);
		closedir(root);

		return 1;
	} else {
		ELOG("Can't open MAILDIR directory.");
		return 0;
	}
}


// Returns pointer to allocated mail structure, or 0 on failure
struct mail* read_mail_file(const char *filename) {
	char file[500];
	sprintf(file, "%s/%s", maildir_path[DIR_NEW], filename);
	FILE *f = fopen(file, "r");

	if (!f) {
		ELOG("Can't open mail file.");
		return 0;
	}

	struct mail *m = calloc(1, sizeof(*m));
	TAILQ_INIT(&m->rcpts);
	m->msg = 0;
	m->was_sent = 0;
	m->filename = malloc(strlen(filename)+1);
	strcpy(m->filename, filename);

	int from = read_mail_from(f, m);
	int to   = read_mail_to  (f, m);
	int data = read_mail_data(f, m);

	fclose(f);

	if (!(from && to && data)) {
		ELOG("Incorrect syntax of mail in file '%s'.", filename);
		move_mail(filename, DIR_NEW, DIR_NOTSENT);
		free_mail(m);
		return 0;
	}

	return m;
}


// Fills in information about MAIL FROM section of mail
int read_mail_from(FILE *f, struct mail *m) {
	char buf[500];

	if (!fgets(buf, sizeof(buf), f)) {
		ELOG("Can't read from mail file '%s'.", m->filename);
		return 0;
	}

	if (!re_match(RE_mail_from, buf, strlen(buf))) {
		ELOG("Incorrect mail format (can't read MAIL FROM) of file '%s'.", m->filename);
		return 0;
	}

	strcpy(m->from, buf);

	return 1;
}


// Fills in information about RCPT TO section of mail
int read_mail_to(FILE *f, struct mail *m) {
	char buf[500], rcpt_buf[500];

	int count = 0;
	int data = 0;

	while (!data) {
		if (!fgets(buf, sizeof(buf), f)) {
			ELOG("Can't read from mail file '%s'.", m->filename);
			return 0;
		}

		if (re_match(RE_data, buf, strlen(buf))) {
			data = 1;
		} else {
			if (!re_match(RE_rcpt_to, buf, strlen(buf))) {
				ELOG("Incorrect mail format (can't read RCPT TO) of file '%s'.", m->filename);
				return 0;
			}

			struct rcpt *r = malloc(sizeof(struct rcpt));

			re_match_and_fill_substring(RE_rcpt_to, buf, strlen(buf), rcpt_buf);
			strcpy(r->name, rcpt_buf);

			re_match_and_fill_substring(RE_rcpt_domain, buf, strlen(buf), rcpt_buf);
			strcpy(r->domain, rcpt_buf);

			TAILQ_INSERT_TAIL(&m->rcpts, r, entry);
			count++;
		}
	}

	return count;
}


// Fills in information about DATA section of mail
int read_mail_data(FILE *f, struct mail *m) {
	char buf[1000];

	int curr_size = 0;
	int max_size = 1000;
	char *msg = malloc(max_size);

	while (fgets(buf, sizeof(buf), f)) {
		int bytes_read = strlen(buf) > sizeof(buf) ? sizeof(buf) : strlen(buf);

		if ((curr_size + bytes_read) > max_size) {
			max_size *= 2;
			msg = realloc(msg, max_size);
		}

		strcpy(msg + curr_size, buf);
		curr_size += bytes_read;
	}

	if (curr_size == 0 || strcmp(buf, ".\r\n") != 0) {
		ELOG("Empty message in file '%s'.", m->filename);
		free(msg);
		m->msg = 0;
		return 0;
	} else {
		msg = realloc(msg, curr_size + 1);
		m->msg = msg;
		return 1;
	}
}


// Removes recipients from local domain
int filter_my_mail(struct mail_list *ml) {
	struct rcpt *r, *r_tmp;
	struct mail *m, *m_tmp;

	LOG(YELLOW "Filtering mail list...");

	TAILQ_FOREACH_SAFE(m, ml, entry, m_tmp) {
		int has_local_rcpt = 0;
		int other_rcpts = 0;

		TAILQ_FOREACH_SAFE(r, &m->rcpts, entry, r_tmp) {
			if (strcmp(r->domain, opts_my_domain()) != 0) {
				other_rcpts = 1;
			} else {
				DLOG(YELLOW "Found local recipient '%s'.", r->name);
				has_local_rcpt = 1;
				TAILQ_REMOVE(&m->rcpts, r, entry);
				free(r);
			}
		}

		if (!other_rcpts) {
			LOG(GREEN "Mail '%s' is local only, moving to 'cur' dir.", m->filename);
			move_mail(m->filename, DIR_NEW, DIR_CUR);
			TAILQ_REMOVE(ml, m, entry);
			free_mail(m);
		} else if (has_local_rcpt) {
			LOG(GREEN "Mail '%s' has local recipient, copying to 'cur' dir.", m->filename);
			copy_mail(m->filename, DIR_NEW, DIR_CUR);
		}
	}

	return 0;
}





// Frees all allocated memory for mail structure
void free_mail(struct mail *m) {
	struct rcpt *r, *r_tmp;

	TAILQ_FOREACH_SAFE(r, &m->rcpts, entry, r_tmp) {
		TAILQ_REMOVE(&m->rcpts, r, entry);
		free(r);
	}

	free(m->filename);
	free(m->msg);
	free(m);
}


// Frees all allocated memory for mail_list structure
void free_mail_list(struct mail_list *ml) {
	while (!TAILQ_EMPTY(ml)) {
		struct mail *m = TAILQ_FIRST(ml);
		TAILQ_REMOVE(ml, m, entry);
		free_mail(m);
	}

	free(ml);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *		Below are functions used to move/copy/delete files
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// Moves file from 'from_dir' to 'to_dir'
void move_mail(const char *filename, maildir_dir from_dir, maildir_dir to_dir) {
	char from[500], to[500];
	sprintf(from, "%s/%s", maildir_path[from_dir], filename);
	sprintf(to,   "%s/%s", maildir_path[to_dir],   filename);

	if (rename(from, to) != 0) {
		ELOG("Can't move mail '%s' from '%s' to '%s'.",
				filename,
				maildir_path[from_dir],
				maildir_path[to_dir]
			);
	}
}


// Copies file from 'from_dir' to 'to_dir'
void copy_mail(const char *filename, maildir_dir from_dir, maildir_dir to_dir) {
	char from[500], to[500];
	sprintf(from, "%s/%s", maildir_path[from_dir], filename);
	sprintf(to,   "%s/%s", maildir_path[to_dir],   filename);

	if (link(from, to) != 0) {
		ELOG("Can't copy mail '%s' from '%s' to '%s'.",
				filename,
				maildir_path[from_dir],
				maildir_path[to_dir]
			);
	}
}


// Deletes file from directory
void delete_mail(const char *filename, maildir_dir dir) {
	char file[500];
	sprintf(file, "%s/%s", maildir_path[dir], filename);

	if (unlink(file) != 0) {
		ELOG("Can't delete mail '%s' from '%s' dir.", filename, maildir_path[dir]);
	}
}
