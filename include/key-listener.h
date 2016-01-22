#ifndef KEY_LISTENER_H
#define KEY_LISTENER_H


/** \file key-listener.h
 *  \brief Ожидание сигнала для завершения программы
 *
 *   Ожидание символа 'Q' ведется в отдельном потоке. При нажатии на эту
 * клавишу основному процессу посылается сингла о том, что выполнение
 * программы пора прекратить. Проверка на завершение следует производить
 * через вызов функции quit_key_pressed().
 */

int keyboard_listener_fork();
int keyboard_listener_final();
int quit_key_pressed();


#endif
