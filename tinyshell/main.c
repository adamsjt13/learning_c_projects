#include <stdio.h>
#include <string.h>

typedef int (*CommandFunc)(char **args);

int cmd_echo(char **args);
int cmd_exit(char **args);
int cmd_type(char **args);

struct Command {
  const char *name;
  CommandFunc func;
};

struct Command builtins[] = {
  {"echo", cmd_echo},
  {"exit", cmd_exit},
  {"type", cmd_type},
  {NULL, NULL}
};

int cmd_echo(char **args) {
  for (int i = 1; args[i]; i++) {
    printf("%s ", args[i]);
  }
  printf("\n");
  return 0;
}

int cmd_exit(char **args) {
  if (strcmp(args[1], "0") == 0) {
    return 1;
  }
  return 0;
}

int cmd_type(char **args) {
  int found = 0;
  for(int i = 0; builtins[i].name != NULL; i++) {
    if (strcmp(args[1], builtins[i].name) == 0) {
      found = 1;
    }
  }
  if (found) {
    printf("%s is a shell builtin\n", args[0]);
  } else {
    printf("%s: not found\n", args[1]);
  }
  return 0;
}

#define INPUT_SIZE 100
#define MAX_ARGS 10

int main() {
  char input[INPUT_SIZE];
  char output[INPUT_SIZE];
  char *args[MAX_ARGS];
  char *token;
  int exit_shell = 0;
  setbuf(stdout, NULL); // flush output

  while(!exit_shell) {

    printf("$ ");
    if(fgets(input, INPUT_SIZE, stdin) == NULL) {
      break;
    }

    input[strlen(input) - 1] = '\0';
    
    int i = 0;
    token = strtok(input, " ");
    while(token != NULL && i < MAX_ARGS) {
      args[i++] = token;
      token = strtok(NULL, " ");
    }

    int found = 0;
    for(int i = 0; builtins[i].name != NULL; i++) {
      if(strcmp(args[0], builtins[i].name) == 0) {
        exit_shell = builtins[i].func(args);
        found = 1;
      }
    }

    if (!found) {
      printf("%s: command not found\n", args[0]);
    }
  
  }

  return 0;
}
