#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 64
#define MAX_TOKEN_LENGTH 256

typedef struct {
  char *cmd_args[MAX_TOKENS];
  char *infile;
  char *outfile;
  char *errfile;
  int append;
} Command;

typedef int (*CommandFunc)(Command *cmd);

int cmd_echo(Command *cmd);
int cmd_exit(Command *cmd);
int cmd_type(Command *cmd);
int cmd_pwd(Command *cmd);
int cmd_cd(Command *cmd);
int find_in_path(const char *cmd, const int print_path);
void tokenize(char *input, char **argv, char tokens[][MAX_TOKEN_LENGTH]);

struct Builtin {
  const char *name;
  CommandFunc func;
};

typedef enum {
  STATE_DEFAULT,
  STATE_IN_WORD,
  STATE_IN_SINGLE_QUOTE,
  STATE_IN_DOUBLE_QUOTE
} TokenizerState;

struct Builtin builtins[] = {
  {"echo", cmd_echo},
  {"exit", cmd_exit},
  {"type", cmd_type},
  {"pwd", cmd_pwd},
  {"cd", cmd_cd},
  {NULL, NULL}
};

int cmd_echo(Command *cmd) {
  int saved_stdout = -1;
  int saved_stderr = -1;

  if(cmd->outfile){
    saved_stdout = dup(STDOUT_FILENO);
    int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
    int fd = open(cmd->outfile, flags, 0644);
    if (fd < 0) { perror("open outfile"); exit(1); }
    dup2(fd, STDOUT_FILENO);
    close(fd);
  }
  if(cmd->errfile){
    saved_stderr = dup(STDERR_FILENO);
    int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
    int fd = open(cmd->errfile, flags, 0644);
    if (fd < 0) { perror("open errfile"); exit(1); }
    dup2(fd, STDERR_FILENO);
    close(fd);
  }

  for (int i = 1; cmd->cmd_args[i] != NULL; i++) {
    printf("%s", cmd->cmd_args[i]);
    if (cmd->cmd_args[i + 1] != NULL) {
      printf(" ");
    }
  }
  printf("\n");

  if (saved_stdout != -1) {
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
  }
  if (saved_stderr != -1) {
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);
  }

  return 0;
}

int cmd_exit(Command *cmd) {
  if (cmd->cmd_args[1] != NULL && strcmp(cmd->cmd_args[1], "0") == 0) {
    return 1;
  }
  printf("%s: command not found\n", cmd->cmd_args[0]);
  return 0;
}

int cmd_type(Command *cmd) {
  int found = 0;
  for(int i = 0; builtins[i].name != NULL; i++) {
    if (strcmp(cmd->cmd_args[1], builtins[i].name) == 0) {
      found = 1;
    }
  }

  if (found) {
    printf("%s is a shell builtin\n", cmd->cmd_args[1]);
  } else if (find_in_path(cmd->cmd_args[1], 1) == 1) {
    // do nothing
  } else {
    printf("%s: not found\n", cmd->cmd_args[1]);
  }
  return 0;
}

int cmd_pwd(Command *cmd) {
  char cwd[PATH_MAX];
  if(getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
  }
  return 0;
}

int cmd_cd(Command *cmd) {
  char *target_dir = cmd->cmd_args[1];

  if(cmd->cmd_args[1] == NULL) {
    fprintf(stderr, "cd: missing operand\n");
    return 0;
  } else if (strcmp(target_dir, "~") == 0) {
    target_dir = getenv("HOME");
  }
  if (chdir(target_dir) != 0) {
    fprintf(stderr, "cd: %s: ", cmd->cmd_args[1]);
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

int run_external_command(Command *cmd) {
  pid_t pid = fork();

  if(pid < 0){
    perror("Fork failed\n");
    return -1;
  } else if(pid == 0) {
    if(cmd->infile) {
      int fd = open(cmd->infile, O_RDONLY);
      if (fd < 0) { perror("open infile"); exit(1); }
      dup2(fd, STDIN_FILENO);
      close(fd);
    }
    if(cmd->outfile) {
      int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
      int fd = open(cmd->outfile, flags, 0644);
      if (fd < 0) { perror("open outfile"); exit(1); }
      dup2(fd, STDOUT_FILENO);
      close(fd);
    }
    if(cmd->errfile) {
      int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
      int fd = open(cmd->errfile, flags, 0644);
      if (fd < 0) { perror("open outfile"); exit(1); }
      dup2(fd, STDERR_FILENO);
      close(fd);
    }
    execvp(cmd->cmd_args[0], cmd->cmd_args);
    perror("execvp failed\n");
    return 1;
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

void parse(char **args, Command *cmd) {
  int arg_index = 0;

  for(int i = 0; args[i] != NULL; i++) {
    if(strcmp(args[i],"1>") == 0 || strcmp(args[i], ">") == 0) {
      cmd->outfile = args[i + 1];
      i++;
    } else if(strcmp(args[i],"1>>") == 0 || strcmp(args[i], ">>") == 0) {
      cmd->outfile = args[i + 1];
      cmd->append = 1;
      i++;
    } else if(strcmp(args[i],"2>") == 0) {
      cmd->errfile = args[i + 1];
      i++;
    } else if(strcmp(args[i],"2>>") == 0) {
      cmd->errfile = args[i + 1];
      cmd->append = 1;
      i++;
    } else {
      cmd->cmd_args[arg_index++] = args[i];
    }
  }
  cmd->cmd_args[arg_index] = NULL;
}

int main() {
  char input[MAX_INPUT_SIZE];
  char *args[MAX_TOKENS];
  char tokens[MAX_TOKENS][MAX_TOKEN_LENGTH];
  int exit_shell = 0;

  setbuf(stdout, NULL); // flush output

  while(!exit_shell) {

    printf("$ ");

    Command cmd = {
      .cmd_args = {NULL},
      .infile = NULL,
      .outfile = NULL,
      .append = 0
    };

    if(fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
      break;
    }

    // clear out buffers for new input
    memset(tokens, 0, sizeof(tokens));
    memset(args, 0, sizeof(args));

    input[strlen(input) - 1] = '\0';

    tokenize(input, args, tokens);

    parse(args, &cmd);

    // for(int i = 0; cmd.cmd_args[i] != NULL; i++) {
    //   printf("arg %d: %s\n", i, cmd.cmd_args[i]);
    // }

    // printf("outfile: %s\n", cmd.outfile);
    // printf("infile: %s\n", cmd.infile);

    int builtin_func = is_builtin(cmd.cmd_args[0]);
    if(builtin_func >= 0) {
      exit_shell = builtins[builtin_func].func(&cmd);
    } else if(is_external_command(cmd.cmd_args[0]) == 1) {
      run_external_command(&cmd);
    } else {
      printf("%s: command not found\n", cmd.cmd_args[0]);
    }
  }
  
  return 0;
}
