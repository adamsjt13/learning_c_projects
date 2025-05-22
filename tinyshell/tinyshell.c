#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

typedef int (*CommandFunc)(char **args);

int cmd_echo(char **args);
int cmd_exit(char **args);
int cmd_type(char **args);
int find_in_path(const char *cmd);

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
  for (int i = 1; args[i] != NULL; i++) {
    printf("%s", args[i]);
    if (args[i + 1] != NULL) {
      printf(" ");
    }
  }
  printf("\n");
  return 0;
}

int cmd_exit(char **args) {
  if (args[1] != NULL && strcmp(args[1], "0") == 0) {
    return 1;
  }
  printf("%s: command not found\n", args[0]);
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
    printf("%s is a shell builtin\n", args[1]);
  } else if (find_in_path(args[1]) == 1) {
    // do nothing
  } else {
    printf("%s: not found\n", args[1]);
  }
  return 0;
}

int find_in_path(const char *cmd) {
  char *path_env = getenv("PATH");
  if(!path_env) return -1;

  char path_copy[PATH_MAX];
  strncpy(path_copy, path_env, PATH_MAX);
  // path_copy[PATH_MAX - 1] = "\0";

  char *dir =  strtok(path_copy, ":");
  while(dir != NULL) {
    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);

    if(access(full_path, X_OK) == 0) {
      printf("%s is %s\n", cmd, full_path);
      return 1;
    }

    dir = strtok(NULL, ":");
  }

  return -1;
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
    while(token != NULL && i < MAX_ARGS - 1) {
      args[i++] = token;
      token = strtok(NULL, " ");
    }

    args[i + 1] = NULL;

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
