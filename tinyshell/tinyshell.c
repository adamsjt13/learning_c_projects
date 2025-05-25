#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 64
#define MAX_TOKEN_LENGTH 256

typedef int (*CommandFunc)(char **args);

int cmd_echo(char **args);
int cmd_exit(char **args);
int cmd_type(char **args);
int cmd_pwd(char **args);
int cmd_cd(char **args);
int find_in_path(const char *cmd, const int print_path);
void tokenize(char *input, char **argv, char tokens[][MAX_TOKEN_LENGTH]);

struct Command {
  const char *name;
  CommandFunc func;
};

typedef enum {
  STATE_DEFAULT,
  STATE_IN_WORD,
  STATE_IN_SINGLE_QUOTE,
  STATE_IN_DOUBLE_QUOTE
} TokenizerState;

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

void tokenize(char *input, char **argv, char tokens[][MAX_TOKEN_LENGTH]) {
  TokenizerState state = STATE_DEFAULT;
  int i = 0;
  int token_count = 0;
  int token_index = 0;
  int in_token = 0;
  int escape = 0;

  while(input[i] != '\0'){
    char c = input[i];

    switch(state) {
      case STATE_DEFAULT:
        if(c == '\\') {
          escape = 1;
          i++;
          break;
        }

        if(escape) {
          tokens[token_count][token_index++] = c;
          i++;
          escape = 0;
          break;
        }

        if(c == ' ') {
          if(in_token) {
            tokens[token_count][token_index++] = '\0';
            argv[token_count] = tokens[token_count];
            token_count++;
            token_index = 0;
            in_token = 0;
          }
          i++;
          break;
        } else if(c == '\'') {
          state = STATE_IN_SINGLE_QUOTE;
          i++;
          in_token = 1;
          break;
        } else if(c == '\"') {
          state = STATE_IN_DOUBLE_QUOTE;
          i++;
          in_token = 1;
          break;
        } else {
          state = STATE_IN_WORD;
          in_token = 1;
        }

      case STATE_IN_WORD:
        if(c == ' ' || c == '\\') {
            state = STATE_DEFAULT;
            break;
        } else if(c == '\'' || c == '\"') {
            i++;
            break;
        } else {
            tokens[token_count][token_index++] = c;
            i++;
            break;
        }        

      case STATE_IN_SINGLE_QUOTE:
        if(c == '\'') {
          i++;
          state = STATE_DEFAULT;
          break;
        } else {
          tokens[token_count][token_index++] = c;
          i++;
          break;
        }

      case STATE_IN_DOUBLE_QUOTE:
        if(escape) {
          if(c == '\\' || c == '$' || c == '\"' || c == '\n') {
            tokens[token_count][token_index++] = c;
            i++;
          } else {
            tokens[token_count][token_index++] = '\\';
          }
          escape = 0;
          break;
        }
      
        if(c == '\"') {
          i++;
          state = STATE_DEFAULT;
          break;
        } else if(c == '\\') {
          escape = 1;
          i++;
        } else {
          tokens[token_count][token_index++] = c;
          i++;
          break;
        }

        
    }
    argv[token_count] = tokens[token_count];
  }
  argv[token_count + 1] = NULL;
}

int main() {
  char input[MAX_INPUT_SIZE];
  char *args[MAX_TOKENS];
  char tokens[MAX_TOKENS][MAX_TOKEN_LENGTH];
  int exit_shell = 0;

  setbuf(stdout, NULL); // flush output

  while(!exit_shell) {

    printf("$ ");

    if(fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
      break;
    }

    // clear out buffers for new input
    memset(tokens, 0, sizeof(tokens));
    memset(args, 0, sizeof(args));

    input[strlen(input) - 1] = '\0';

    tokenize(input, args, tokens);

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
