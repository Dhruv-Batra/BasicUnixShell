#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void runCommand(const char *args[]);
void cd_command(const char* args[]);

int main() {
  // print $ on STD err as prompt and read in user input
  // handle EOF
  // time user input and convert to tokens
  // handle command
  const char *args[] = {"cd", "/home/runner", NULL};
  runCommand(args);

  return 0;
}

// trim user input for spaces and split into tokens in vector, index 0 is
// commands, and following indices are arguments

// remove leading and trailing spaces
// delimit based on spaces into a token array, unless surrounded by single or
// double quotes in which case everything within them is considered one token
// error if user input more than 100 arguments

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
  char* const* typed_args = const_cast<char *const *>(args);

  //check if command is exit
  if (strcmp(arg0,"exit")==0) {
      exit(0);
  }

  //chek if command is cd
  if(strcmp(arg0,"cd")==0){
    cd_command(args);
    return;
  }

  // run commands with absolute and relative paths
  if ((pid = fork()) == 0) { // child
    // first try handle the command through absolute path
    int c1 = execv(arg0, typed_args); // need to change from const char* to char *const
    if (c1 < 0) {          // error
      // second try handle the command through relative path
      char *envptr = getenv("PATH");
      char *delim = strchr(envptr, ':');  // first delimiter if multiple paths
      size_t bufferSize = strlen(envptr); // length of path
      if (delim != nullptr) {
        bufferSize = strlen(envptr) - strlen(delim);
      }
      char *newpath = (char *)malloc(bufferSize + strlen(arg0) + 1); //+1 for the '/', null termination is already included in arg0
      //copy characters into new path
      snprintf(newpath, bufferSize + 1, "%.*s", (int)bufferSize, envptr);
      sprintf(newpath + bufferSize, "/%s", arg0); //unix treats multiple backslash ex: // as one / so /ls is a valid relative path and would become //ls and still work
      // std::cout<<newpath<<std::endl;
      int c2 = execv(newpath, typed_args);
      if (c2 < 0) { // error
        perror("Invalid Input");
        exit(255);
      }
      free(newpath);
    }
  } else if (pid > 0) {                 // parent
    if (waitpid(pid, &status, 0) < 0) { // Theres an error
      perror("Child Process Failed");
    }
  } else if (pid < 0) { // error
    perror("Process Creation Error");
  }
}


//do cd command with absolute or relative path
void cd_command(const char* args[]){
  //chdir automatically handles both absolute and relative paths
  if (chdir(args[1]) == -1){
    perror("Invalid Path");
  }
  // if (chdir(args[1]) == -1) {
  //   std::cout<<"relative"<<std::endl;
  //   //if that fails try relative path
  //   size_t sizePath = 64; //aritrary reasonable starting value
  //   char *path;
  //   path = (char *)malloc(sizePath);
  //   path = getcwd(path,sizePath);
  //   //make sure has path has space for the path
  //   while(1){
  //     if(path==nullptr){
  //       sizePath*=2;
  //       path = (char*) realloc(path, sizePath);
  //       path = getcwd(path,sizePath);
  //     }else{
  //       break;
  //     }
  //   }
  //   //make sure path has space for the target path being appended then copy over target path
  //   path = (char*) realloc(path, sizePath+strlen(args[1]));
  //   sprintf(path + strlen(path), "/%s", args[1]);
  //   //attempt relative cd
  //   if (chdir(path) == -1) {
  //     perror("Invalid Path");
  //   }
    
  //   free(path);
  // }
  
  char cwd[100];
  std::cout<<"yay "<<getcwd(cwd, sizeof(cwd));
  
}
