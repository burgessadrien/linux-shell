/*
 * =====================================================================================
 *
 *       Filename:  flash.c
 *
 *    Description:  Speed ahead with this shell written in C
 *
 *        Version:  1.0
 *        Created:  2019-01-29 10:12:11 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Adrien Burgess,
 *   Organization:  Self
 *
 * =====================================================================================
 */
#include "flash.h"
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


#define PATH_MAX 4096
#define MAX_LENGTH 1024


typedef struct saved saved;

struct saved {
    char *line;
    saved *prev;
    saved *newer;
};

saved *head;
saved *last;
struct termios term_orig;

void die(char *err) {
    printf("%s\n", err);
    exit(EXIT_FAILURE);
}

void disable_raw_mode() {
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_orig) == -1){
        die("Failed to disable raw mode.");
    }
}

void term_raw_mode() {
    struct termios raw;
    if(tcgetattr(STDIN_FILENO, &term_orig) < 0) {
        die("Unable to get tty settings.");
    }
    raw = term_orig;
    atexit(disable_raw_mode);
    raw.c_iflag &= ~(IXON);
    raw.c_lflag &= ~(ECHO | ECHOE | ICANON | ISIG | IEXTEN);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }
}

void shell_end() {
    saved *toFree;
    last = head;
    while(last) {
        toFree = last;
        last = last->newer;
        // freeing memory
        free(toFree);
    }
    disable_raw_mode();
}

void sig_int_handler(int sig) {
    if(sig == SIGINT) {
        signal(SIGINT, sig_int_handler);
    }
}

void sig_stop_handler(int sig) {
     if(sig == SIGTSTP) {
        signal(SIGTSTP, sig_stop_handler);
    }
}

void shell_init() {
    //start of history
    head = (struct saved*)malloc(sizeof(saved));
    head->line = "";
    head->newer = NULL;
    head->prev = NULL;
    last = head;
    // signals
    signal(SIGINT, sig_int_handler);
    signal(SIGTSTP, sig_stop_handler);
}

size_t get_size_without_white_space(char *value) {
    char *curr = value;
    int length = strlen(value);
    size_t numWhiteSpaces = 0;
    for(int i = 0; i < length - 1;i++) {
        if(isspace(value[i])) {
            numWhiteSpaces++;
        }
    }
    return length - numWhiteSpaces;
}

char* read_input() {
    char c;
    char *input = malloc(MAX_LENGTH * sizeof(char*));
    char *next = malloc(MAX_LENGTH * sizeof(char *));
    int pos = 0;
    int temp;
    saved *hist_check = last;
    term_raw_mode();
    while(read(STDIN_FILENO, &c, 1) ==1 && c != '\n') {
        if(!iscntrl(c)) {
            strcpy(&next[0], &input[pos]);
            input[pos] = c;
            pos++;
            strcpy(&input[pos], next);
            printf("%c\033[s%c[K%s\033[u", c,27, &input[pos]);
        }else {
            switch(c) {
                case 3 : // control c
                    disable_raw_mode();
                    fflush(stdin);
                    fflush(stdout);
                    input = "";
                    printf("\n");
                    return input;
                case 27:
                    read(STDIN_FILENO, &c, 1);
                    read(STDIN_FILENO, &c, 1);
                    switch(c) {
                        case 51:
                            read(STDIN_FILENO, &c, 1);
                            switch(c) {
                                case 126: // delete
                                    temp = pos+1;
                                    strcpy(&input[pos], &input[temp]);
                                    printf(" \b\033[s%c[K%s\033[u", 27, &input[pos]);
                                    break;
                            }
                            break;
                        case 65: // up arrow
                            if(hist_check->prev) {
                                hist_check = hist_check->prev;
                            }else if(hist_check == head){
                                hist_check = last;
                            }else {
                                hist_check = head;
                            }
                            strcpy(input, hist_check->line);
                            temp = strlen(input);
                            if(pos > 0)
                                printf("\033[%dD", pos);
                            printf("%s%c[K",input,27);
                            pos = temp;
                            break;
                        case 66: // down arrow
                            if(hist_check->newer) {
                                hist_check = hist_check->newer;
                                strcpy(input, hist_check->line);
                            }else {
                                hist_check = head;
                            }
                            temp = strlen(input);
                            if(pos > 0)
                                printf("\033[%dD", pos);
                            printf("%s%c[K",input,27);
                            pos = temp;
                            break;
                        case 68: // left arrow
                            if(pos > 0) {
                                printf("\033[1D");// move cursor left one column
                                pos--;
                            }
                            break;
                        case 67: // right arrow
                            if(pos < strlen(input)) {
                                printf("\033[1C");// move cursor right one column
                                pos++;
                            }
                            break;
                    }
                    break;
                case 72: //home
                    printf("\033[D");
                    break;
                case 70: //end
                    printf("\033[C");
                    break;
                case 127: // backspace
                    if(pos > 0) {
                        temp = pos;
                        pos--;
                        strcpy(&input[pos], &input[temp]);
                        printf("\b \b\033[s%c[K%s\033[u", 27, &input[pos]);
                    }
                    break;
            }
        }
        fflush(stdout);
    }
    free(next);
    disable_raw_mode();
    printf("\n");
    fflush(stdin);
    fflush(stdout);
    return input;
}

char** parse_input(char* input) {
    char **args = malloc(sizeof(char) * MAX_LENGTH);
    if(!args) {
        perror("flash");
        exit(errno);
    }
    int i = 0;
    char *arg;
    while(1) {
        arg = strsep(&input, " ");
        if(!arg) {
            args[i] = NULL;
            break;
        }
        if(arg != "\0") {
            args[i] = arg;
        }
        i++;
    }
    return args;
}

void change_dir(char **args) {
    if(chdir(args[1]) == -1) {
        perror("flash");
    }
}
void shell_history() {
    if(!head) {
        printf("\n");
    }else if(!head->newer) {
        printf("\n");
    }else {
        saved *check = head;
        while((check = check->newer)) {
            printf("%s\n", check->line);
        }
    }
}

void shell_exit() {
    shell_end();
    exit(EXIT_SUCCESS);
}

// Check for commands built into the shell
void* execute_built_in(char **args) {
    if(strcmp(*args,"exit") == 0) {
        return &shell_exit;
    }else if(strcmp(*args, "history") == 0) {
        return &shell_history;
    }else if(strcmp(*args, "cd") == 0){
        return &change_dir;
    }else {
        return NULL;
    }
}

int execute_external(char **args) {
    pid_t child;
    int status;
    child = fork();
    if(child < 0) {
        //error creating child process
        perror("flash");
        return 0;
    }else if(child == 0) { // is the child process
        signal(SIGTSTP, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        if(execvp(*args, args) == -1) {
            perror("flash");
        }
        exit(errno);
        return 0;
    }else { // is the parent process
        do{
            waitpid(child, &status, WUNTRACED); // waiting for child process to finish
        }while(!WIFSIGNALED(status) && !WIFEXITED(status) && !WIFSTOPPED(status)); //check that child process was exited normally or by signal, or stopped
    }
    return 1;
}

int execute_request(char **input) {
    void (*builtIn)(char **args);
    builtIn = execute_built_in(input);
    if(!builtIn) {
        return execute_external(input);
    }else {
        builtIn(input);
        return 1; //change in future for response from built in commands
    }
}

void save_history(char *line) {
    saved *next = (struct saved*)malloc(sizeof(saved));
    next->line = malloc(strlen(line) * sizeof(char));
    strcpy(next->line, line);
    next->prev = last;
    next->newer = NULL;
    last->newer = next;
    last = next;
}

void run_shell() {
    //read standard input
    char *input;
    char **parsedInput;
    int status = 1;
    char curDir[PATH_MAX];
    while(1) {
        getcwd(curDir, sizeof(curDir));
        printf("\n%s\n", curDir); // Printing out current directory
        printf("🏃💨 "); // Beginning of line for typing
        fflush(stdout);
        fflush(stdin);
        input = read_input();
        if(get_size_without_white_space(input) > 0) {
            save_history(input);
            //parse input
            parsedInput = parse_input(input);
            //execute
            status = execute_request(parsedInput);
            free(parsedInput);
        }
    }
}

void fun_welcome() {
    printf("---------------------------------------------------------\n");
    printf("|             __________                                |\n");
    printf("|            /    /  /  \\                               |\n");
    printf("|           /    /  / __ \\                              |\n");
    printf("|          /    /  /_/ /  \\                             |\n");
    printf("|         |    /  _   /    |   Welcome to FlaSH!        |\n");
    printf("|          \\  /_/ /  /    /                             |\n");
    printf("|           \\    /  /    /  Enjoy your quick stay!      |\n");
    printf("|            \\__/__/____/                               |\n");
    printf("|                                                       |\n");
    printf("---------------------------------------------------------\n");
}

int main(int arc, char **argv) {
    // fun welcome to shell
    fun_welcome();
    // Initialize shell
    shell_init();
    // Run command loop
    run_shell();
    // clean up
    shell_end();
    return EXIT_SUCCESS;
}
