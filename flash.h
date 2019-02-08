/*
 * =====================================================================================
 *
 *       Filename:  flash.h
 *
 *    Description:  header
 *
 *        Version:  1.0
 *        Created:  2019-01-29 10:16:26 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Adrien Burgess,
 *   Organization:  self
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>

__BEGIN_DECLS

/*
 * print out fun statement on starting shell
 */
void fun_welcome();
/*
 * Disable raw mode and enter canonical
 * */
void disable_raw_mode();

/*
 * Leave canonical mode and enter raw mode
 * */
void term_raw_mode();

/*
 * signal handler for control-c
 * */
void sig_int_handler(int sig);

/*
 * signal handler for control-z
 * */
void sig_stop_handler(int sig);

/*
 * Initialize values for shell
 * */
void shell_init();

/*
 * Return length of string excluding empty space
 * */
size_t get_size_without_white_space();

/*
 * Read input from console
 * */
char* read_input();

/*
 * Separate commands from console into array of strings
 * */
char** parse_input(char* input);

/*
 * Change working directory
 * */
void change_dir(char **args);
/*
 * Display history of commands enteredd during session
 * */
void shell_history();

/*
 * Built in command to exit shell
 * */
void shell_exit();

/*
 * Execute commands built into the shell
 * */
void* execute_built_in(char **args);

/*
 * Attempt to execute commands in system path
 * */
int execute_external(char **args);

/*
 * Take arguments from console and execute command
 * */
int execute_request(char **args);

/*
 * Save entered command to session history
 * */
void save_history(char *line);

/*
 * Run flash shell
 * */
void run_shell();

__END_DECLS
