#ifndef REGEXP_H
#define REGEXP_H
/**
 * \file regexp.h
 * \brief Набор функций для работы с регулярными выражениями.
	smtp_re_name перечисляет все представленные регулярные выражения.
 *
 * 1) Для инициализации работы необходим вызов функции re_init(), а для
 * кооректного завершения - re_final().\r\n
 *
 * 2) re_match() возвращает 1, если строка подходит под регулярку, и 0
 * во всех других случаях.
 *
 * 3) re_match_any() возвращает номер регулярки, если строка подходит
 * под любую из всех доступных (номер равен номеру первой подошедшей).
 * Если ни одна регулярка не подошла, возвращает -1.
 *
 * 4) re_match_and_fill_substring() и re_match_and_alloc_substring()
 * заполняют или выделяют память под строку, соответствующей первой
 * группе скобок в регулярке соответственно. Если нет соответствия
 * реулярке, то возвращается 0.
*/
#define RE_CMD_220 "^220(?:\\s+.+)?\\r\n"
#define RE_CMD_221 "^221(?:\\s+.+)?\\r\n"
#define RE_CMD_250 "^250(?:\\s+.+)?\\r\n"
#define RE_CMD_354 "^354(?:\\s+.+)?\\r\n"
#define RE_CMD_DATA "^DATA\\r\n"
#define RE_CMD_MAIL_FROM "^MAIL FROM:\\s*<.+>\\r\n"
#define RE_CMD_RCPT_TO "^RCPT TO:\\s*<(.+@.+)>\\r\n"
#define RE_CMD_RCPT_TO_DOMAIN "^RCPT TO:\\s*<.+@(.+)>\\r\n"
#define RE_CMD_MX_DNS "MX\\s+\\d+\\s*(.+)\.$"
#define RE_CMD_MX_DNS_PRIO "MX\\s+(\\d+)\\s*.+\.$"

typedef enum {
	r220,
	r221,
	r250,
	r354,
	RE_data,
	RE_mail_from,
	RE_rcpt_to,
	RE_rcpt_domain,
	RE_mx_dns,
	RE_mx_dns_prio,
	smtp_re_count
} smtp_re_name;


int re_init();
int re_final();

int				re_match(smtp_re_name re_name, char *str, int length);
smtp_re_name	re_match_any(char *str, int length);
int				re_match_and_fill_substring(smtp_re_name re_name, char *str, int length, char *substr);
char*			re_match_and_alloc_substring(smtp_re_name re_name, char *str, int length);

#endif
