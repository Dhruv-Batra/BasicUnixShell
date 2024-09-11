#include <climits>
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void runCommand(const char *args[]);
void cdCommand(const char *args[]);
char **parseTokens(char input[]);
void addToken(char *start_token, char *runner, char **args, int &arg_count);

int main() {
  while (1) {
    // print $ on STD err as prompt and read in user input
    char input[1000];
    std::cerr << "$ ";
    std::cin.getline(input, 1000);

    // exit if EOF
    if (std::cin.eof()) {
      exit(0);
    }

    // error if input > 1000 characters
    if (std::cin.fail() || input[0] == '\n') {
      std::cerr << "Error: input exceeds 1000 characters" << std::endl;
      std::cin.clear();
      std::cin.ignore(INT_MAX, '\n');
      continue;
    }

    // clean user input and convert to tokens
    char **tokens = parseTokens(input);

    // check for nullptr
    if (tokens == nullptr) {
        continue;
    }


    if(*tokens[0]!='\n'){
         runCommand(const_cast<const char **>(tokens));
    }

    // TESTING
     // int b = 0;
     // while (tokens[b] != nullptr) {
     //     std::cout << tokens[b] << std::endl;
     //     ++b;
     // }

    

    // free allocated memory
    int i = 0;
    while (tokens[i] != NULL) {
      delete[] tokens[i];
      i++;
    }
    delete[] tokens;
  }

  return 0;
}

// clean up input and output pointer to token array
char **parseTokens(char input[]) {
  // trim leading and trailing spaces
  char *start = input;
  char *end = input + strlen(input);
  // advance leading edge to remove spaces
  while (isspace(*start)) {
    start++;
  }
  // retract trailing edge to remove spaces
  while (isspace(*end)) {
    end--;
  }
  *(end + 1) = '\0'; // replace null terminating char if the end was moved

  if(*start=='\0'){ //check if just enter was pressed which results in just null r
    return nullptr;
  }

  // split input delimited by one or more spaces into tokens
  char *runner = start;       // iterates through input string
  char *start_token = runner; // start of a token
  // keep track of which state for what type of token is being captured
  bool s_quote = false;
  bool d_quote = false;
  bool norm = false;
  bool prev_space = false;
  // count the ' and " characters
  int s_count = 0;
  int d_count = 0;
  // output
  char **args = new char *[100];
  int arg_count = 0;

  while (*runner != '\0') {
    // single/double quotes count as one token
    if (*runner == '\'') {
      prev_space = false;
      if (!s_quote && !d_quote &&
          !norm) { // start token capture if no one else has
        start_token = runner+1;
        s_quote = true;
        s_count++;
      } else if (d_quote) { // if double quote going on and single is
                            // encountered record and proceed
        runner++;
        continue;
      } else if (norm && !s_quote) { // case where space is encountered before opening quote then change state to quote
        start_token = runner+1;
        s_quote = true;
        s_count++;
        norm = false;
      } else { // encounter closing quote then add token to args
        s_count--;
        s_quote = false;
        addToken(start_token, runner, args, arg_count);
      }
    } else if (*runner == '"') {
      prev_space = false;
      if (!s_quote && !d_quote &&
          !norm) { // start token capture if no one else has
        start_token = runner+1;
        d_count++;
        d_quote = true;
      } else if (s_quote) { // if single quote is going on and double is encountered record and proceed
        runner++;
        continue;
      } else if (norm && !d_quote) { // case where space is encountered before opening quote then change state to quote
        start_token = runner+1;
        d_quote = true;
        d_count++;
        norm = false;
      } else { // end quote encountered then add token to args
        d_count--;
        d_quote = false;
        addToken(start_token, runner, args, arg_count);
      }
    } else if (*runner == ' ') { // normal case of space delimted word
      if (!s_quote && !d_quote &&
          !norm) { // start token capture if no one else has
        start_token = runner + 1;
        prev_space = true;
        norm = true;
      } else if (s_quote || d_quote) { // ignore spaces if token involves quotes
        runner++;
        continue;
      } else { // ending space encountered
        if (prev_space) {
          start_token = runner + 1;
          runner++;
          continue;
        }
        addToken(start_token, runner, args, arg_count);
        // another space has been encountered
        norm = true;
        start_token = runner + 1;
      }
      // just finished handling a space
      prev_space = true;
    } else { // just a character
      prev_space = false;
      if (!s_quote && !d_quote &&
          !norm) { // start token capture if no one else has
        start_token = runner;
        norm = true;
      }
    }
    // increment runner
    runner++;
  }

  // for case when EOF is the end of the token
  if (norm) { // if a space delimited word has been started
    addToken(start_token, runner, args, arg_count);
  }

  // error if quote mismatch
  if (s_count != 0 || d_count != 0) {
    std::cerr << "Error: quote mismatch" << std::endl;
    return nullptr;
  }

  return args;
}

// handle commands
/*
   1. Check if command is exit
   2. Check if command is cd/runs if applicable
   3. Runs absolute path and if that errors tries relative path
   4. Provide Error Invalid input if reaches this point
*/
void runCommand(const char *args[]) {

  int status;
  pid_t pid;
  char *arg0 = const_cast<char *>(args[0]);
  char *const *typed_args = const_cast<char *const *>(args);

  // check if command is exit
  if (strcmp(arg0, "exit") == 0) {
    exit(0);
  }

  // chek if command is cd
  if (strcmp(arg0, "cd") == 0) {
    cdCommand(args);
    return;
  }

  // run commands with absolute and relative paths
  if ((pid = fork()) == 0) { // child
    // first try handle the command through absolute path
    int c1 = execv(
        arg0, typed_args); // need to change from const char* to char *const
    if (c1 < 0) {          // error
      // second try handle the command through relative path
      char *envptr = getenv("PATH");
      char *delim = strchr(envptr, ':');  // first delimiter if multiple paths
      size_t bufferSize = strlen(envptr); // length of path
      if (delim != nullptr) {
        bufferSize = strlen(envptr) - strlen(delim);
      }
      char *newpath = new char[bufferSize + strlen(arg0) +
                               1]; //+1 for the '/', null termination is already
                                   //included in arg0
      // copy characters into new path
      snprintf(newpath, bufferSize + 1, "%.*s", (int)bufferSize, envptr);
      sprintf(
          newpath + bufferSize, "/%s",
          arg0); // unix treats multiple backslash ex: // as one / so /ls is a
                 // valid relative path and would become //ls and still work
      int c2 = execv(newpath, typed_args);
      if (c2 < 0) { // error
        perror("Invalid Input");
        std::cerr << "error: command exited with code 1" << std::endl;
        exit(1);
      }
      delete[] newpath;
    }
  } else if (pid > 0) { // parent
    if (waitpid(pid, &status, 0) < 0) {
      perror("Child Process Failed");
    }
  } else if (pid < 0) { // error
    perror("Process Creation Error");
  }
}

// do cd command with absolute or relative path
void cdCommand(const char *args[]) {
  // chdir automatically handles both absolute and relative paths
  if (chdir(args[1]) == -1) {
    perror("Invalid Path");
  }
}

// add token to args array
void addToken(char *start_token, char *runner, char **args, int &arg_count) {
  int t_len = 0;
  t_len = strlen(start_token) - strlen(runner);
  char *token = new char[t_len + 1]; //+1 for null terminating char
  strncpy(token, start_token, t_len);
  token[t_len] = '\0';
  if (arg_count <= 99) { // error if more than 100 arguments
    args[arg_count] = token;
    arg_count++;
  } else {
    std::cerr << "Error: too many arguments" << std::endl;
  }
}
