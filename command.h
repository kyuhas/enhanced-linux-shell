#ifndef COMMAND_H
#define COMMAND_H

typedef struct {
	char *name;
    char **argv;
    int argc;
    char *input;
    char *output;
} command;

#endif