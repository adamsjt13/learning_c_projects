#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef int (*CommandFunc)(char **args);

int cmd_echo(char **args);
int cmd_exit(char **args);
int cmd_type(char **args);
int cmd_pwd(char **args);
int cmd_cd(char **args);
int find_in_path(const char *cmd, const int print_path);

struct Command {
  const char *name;
  CommandFunc func;
};

struct Command builtins[] = {
  {"echo", cmd_echo},
  {"exit", cmd_exit},
  {"type", cmd_type},
  {"pwd", cmd_pwd},
  {"cd", cmd_cd},
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
  } else if (find_in_path(args[1], 1) == 1) {
    // do nothing
  } else {
    printf("%s: not found\n", args[1]);
  }
  return 0;
}

int cmd_pwd(char **args) {
  char cwd[PATH_MAX];
  if(getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
  }
  return 0;
}

int cmd_cd(char **args) {
  char *target_dir = args[1];

  if(args[1] == NULL) {
    fprintf(stderr, "cd: missing operand\n");
    return 0;
  } else if (strcmp(target_dir, "~") == 0) {
    target_dir = getenv("HOME");
  }
  if (chdir(target_dir) != 0) {
    fprintf(stderr, "cd: %s: ", args[1]);
    perror("");
    return 0;
  }
}

int find_in_path(const char *cmd, const int print_path) {
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
      if(print_path == 1) {
        printf("%s is %s\n", cmd, full_path);
      }
      return 1;
    }

    dir = strtok(NULL, ":");
  }

  return -1;
}

int is_builtin(char *cmd) {
  for(int i = 0; builtins[i].name != NULL; i++) {
    if (strcmp(cmd, builtins[i].name) == 0) {
      return i;
    }
  }
  return -1;
}

int is_external_command(char *cmd) {
  if (find_in_path(cmd, 0) == 1) {
    return 1;
  } else {
    return -1;
  }
}

int run_external_command(char **args) {
  pid_t pid = fork();

  if(pid < 0){
    perror("Fork failed\n");
    return -1;
  } else if(pid == 0) {
    execvp(args[0], args);
    perror("execvp failed\n");
  } else {
    int status;
    waitpid(pid, &status, 0);
    return status;
  }

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
    int builtin_func = is_builtin(args[0]);
    if(builtin_func >= 0) {
      exit_shell = builtins[builtin_func].func(args);
    } else if(is_external_command(args[0]) == 1) {
      run_external_command(args);
    } else {
      printf("%s: command not found\n", args[0]);
    }
  }
  
  return 0;
}
