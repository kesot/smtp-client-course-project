#ifndef LOG_H
#define LOG_H

/** \file log.h
 * 	\brief Логирование сообщений в отдельном процессе.
 * 
 * Передача сообщений ведется через пару сокетов. Возможна раскраска
 * сообщений. Три вида сообщений: отладка, ошибки, обычные.
 *
 * Есть возможность включения/выключения режима отладки (пропадают все
 * сообщения типа DLOG), работы лога в отдельном процессе (в таком
 * случае все сообщения выводятся сразу же) и раскраски (все цвета
 * не выводятся).
 *
 * Ограничения:
 * 1) первый аргумент любого макроса - строка вида "..."; использование
 * переменных в качестве первого параметра недопустимо;
 * 2) максимальная длина сообщения установлена как MAX_LOG_MESSAGE_SIZE;
 * 3) для запуска лога следует использовать функцию fork_log(), а для
 * завершения - close_log(); при этом основной процесс будет ожидать,
 * пока не будут выведены все накопленные сообщения.
 * 4) для логирования следует использовать макросы ELOG, DLOG и LOG;
 * использование функции send_log() недопустимо.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>

// if not defined DLOG messages will not be displayed
//~ #define DEBUG
// if not defined logger will not be forked
#define FORKED_LOG
// if not defined color MACROs will not be displayed
#define COLORED_LOG
#define DEBUG

#define EXIT_SYMBOL   ((char)0)
#define STDOUT_SYMBOL  ((char)1)
#define STDERR_SYMBOL ((char)2)
#define DEBUG_SYMBOL  ((char)3)


/** Max log message; larger messages will cause unexpected behavior
 */
#define MAX_LOG_MESSAGE_SIZE 1000
// Line that will be drawn ar the start and the end of log job
#define SPLIT_LINE "[============================================================]"

// Three main macros: LOG, DLOG and ELOG; they work like printf function
#define LOG_IMP(SYMBOL, FORMAT, ...) { \
	char _SOME_BUFF_DONT_MIND_IT_[MAX_LOG_MESSAGE_SIZE]; \
	sprintf(_SOME_BUFF_DONT_MIND_IT_, "%c" FORMAT "%s", SYMBOL, __VA_ARGS__); \
	send_log(_SOME_BUFF_DONT_MIND_IT_); \
}
#define LOG(...) LOG_IMP(STDOUT_SYMBOL, __VA_ARGS__, "")
#define ELOG(...) LOG_IMP(STDERR_SYMBOL, __VA_ARGS__, "")

#ifdef DEBUG
	#define DLOG(...) LOG_IMP(DEBUG_SYMBOL, __VA_ARGS__, "")
#else
	#define DLOG(...) /* nope */
#endif



#ifdef COLORED_LOG
	#define RED			"\x1B[31;1m"
	#define GREEN		"\x1B[32;1m"
	#define YELLOW		"\x1B[33;1m"
	#define BLUE		"\x1B[34;1m"
	#define MAGENTA		"\x1B[35;1m"
	#define CYAN		"\x1B[36;1m"
	#define WHITE		"\x1B[37;1m"
	#define COLOR_RESET	"\x1B[0m"
#else
	#define RED			""
	#define GREEN		""
	#define YELLOW		""
	#define BLUE		""
	#define MAGENTA		""
	#define CYAN		""
	#define WHITE		""
	#define COLOR_RESET	""
#endif

int fork_log();
int close_log();
int send_log(char *msg);

#endif
