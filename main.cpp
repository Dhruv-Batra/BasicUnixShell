#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <climits>

void runCommand(const char *args[]);
void cdCommand(const char* args[]);
char** parseTokens(char input[]);

int main() {
  // time user input and convert to tokens
  // handle command
  while(1){
    // print $ on STD err as prompt and read in user input
    char input[1000];
    std::cerr<<"$ ";
    std::cin.getline(input, 1000);

    //exit if EOF
    if(std::cin.eof()){
      exit(0);
    }
    
    //error if input > 1000 characters
    if(std::cin.fail()){
      std::cerr << "Error: input exceeds 1000 characters" << std::endl;
      std::cin.clear();
      std::cin.ignore(INT_MAX, '\n');
      continue;
    }

    //clean user input and convert to tokens
    char** tokens = parseTokens(input);

    runCommand(const_cast<const char**>(tokens));
    
  }

  return 0;

}

// clean up input and output pointer to token array
char** parseTokens(char input[]){
  //trim leading and trailing spaces
  char* start = input;
  char* end = input + strlen(input);

  //advance leading edge to remove spaces
  while(isspace(*start)){
    start++;
  }

  //retract trailing edge to remove spaces
  while(isspace(*end)){
    end--;
  }
  *(end+1)='\0'; //replace null terminating char if the end was moved

  //split input delimited by one or more spaces into tokens
  char* runner = start;
  char* start_token = runner;
  bool s_quote = false;
  int s_count = 0;
  bool d_quote = false;
  int d_count = 0;
  bool norm = false;
  char** args = new char*[100];
  int arg_count=0;
  while(*runner!='\0'){
    //single/double quotes count as one token
    if(*runner=='\''){
      if(!s_quote&&!d_quote&&!norm){ //start token capture if no one else has
        start_token=runner;
        s_quote=true;
        s_count++;
      }else if(d_quote){ //if double quote going on and single is encountered
        if(d_count>0){
          d_count--;
        }else{
          d_count++;
        }
        runner++;
        continue;
      }else if(norm&&!s_quote){ //case where space is encountered before opening quote
        start_token=runner;
        s_quote=true;
        s_count++;
        norm=false;
      }else{ 
        s_count--;
        s_quote=false;
        int t_len = strlen(start_token)-strlen(runner)+1;
        char* token = new char[t_len+1];//+1 for null terminating char
        strncpy(token,start_token,t_len);
        token[t_len] = '\0'; 
        if(arg_count<=100){ //error if more than 100 arguments
          args[arg_count] = token;
          arg_count++;
        }else{
          std::cerr << "Error: too many arguments" << std::endl;
          return NULL;
        }
      }
    }else if(*runner=='"'){ 
      if(!s_quote&&!d_quote&&!norm){ //start token capture if no one else has
        start_token=runner;
        d_count++;
        d_quote=true;
      }else if(s_quote){ //if single quote is going on and double is encountered
        if(s_count>0){
          s_count--;
        }else{
          s_count++;
        }
        runner++;
        continue;
      }else if(norm&&!d_quote){ //case where space is encountered before opening quote
        start_token=runner;
        d_quote=true;
        d_count++;
        norm=false;
      }else{//d_quote is true and end quote encountered
        d_count--;
        d_quote=false;
        int t_len = strlen(start_token)-strlen(runner)+1;
        char* token = new char[t_len+1];//+1 for null terminating char
        strncpy(token,start_token,t_len);
        token[t_len] = '\0'; 
        if(arg_count<=100){ //error if more than 100 arguments
          args[arg_count] = token;
          arg_count++;
        }else{
          std::cerr << "Error: too many arguments" << std::endl;
          return NULL;
        }
      }
    }else if(*runner==' '){ //normal case of space delimted word
      if(!s_quote&&!d_quote&&!norm){ //start token capture if no one else has
        start_token=runner+1;
        norm=true;
      }else if(s_quote||d_quote){ //ignore spaces if token involves quotes
        runner++;
        continue;
      }else{ //ending space encountered
        int t_len = strlen(start_token)-strlen(runner);
        char* token = new char[t_len+1];//+1 for null terminating char
        strncpy(token,start_token,t_len);
        token[t_len] = '\0'; 
        if(arg_count<=100){ //error if more than 100 arguments
          args[arg_count] = token;
          arg_count++;
        }else{
          std::cerr << "Error: too many arguments" << std::endl;
          return nullptr;
        }
        //another space has been encountered
        norm=true;
        start_token=runner+1;
      }
    }else{ //just a character
      if(!s_quote&&!d_quote&&!norm){ //start token capture if no one else has
        start_token=runner;
        norm=true;
      }
    }
    //increment runner
    runner++;
  }

  //EOF is the end of the token
  if(norm){ //start token capture if no one else has
    norm=false;
    int t_len = strlen(start_token)-strlen(runner);
    char* token = new char[t_len+1];//+1 for null terminating char
    strncpy(token,start_token,t_len);
    token[t_len] = '\0'; 
    if(arg_count<=100){ //error if more than 100 arguments
      args[arg_count] = token;
      arg_count++;
    }else{
      std::cerr << "Error: too many arguments" << std::endl;
      return nullptr;
    }
  }
  
  //error if quote mismatch
  if(s_count!=0||d_count!=0){
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
  char* const* typed_args = const_cast<char *const *>(args);

  //check for nullptr
  if(args==nullptr){
    std::cerr<<"Error: invalid input"<<std::endl;
    return;
  }

  //check if command is exit
  if (strcmp(arg0,"exit")==0) {
      exit(0);
  }

  //chek if command is cd
  if(strcmp(arg0,"cd")==0){
    cdCommand(args);
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
      char *newpath = new char[bufferSize + strlen(arg0) + 1]; //+1 for the '/', null termination is already included in arg0
      //copy characters into new path
      snprintf(newpath, bufferSize + 1, "%.*s", (int)bufferSize, envptr);
      sprintf(newpath + bufferSize, "/%s", arg0); //unix treats multiple backslash ex: // as one / so /ls is a valid relative path and would become //ls and still work
      // std::cout<<newpath<<std::endl;
      int c2 = execv(newpath, typed_args);
      if (c2 < 0) { // error
        perror("Invalid Input");
        std::cerr<<"error: command exited with code 1"<<std::endl;
        exit(1);
      }
      delete[] newpath;
    }
  } else if (pid > 0) { // parent
    if (waitpid(pid, &status, 0) < 0) { // Theres an error
      perror("Child Process Failed");
    }
  } else if (pid < 0) { // error
    perror("Process Creation Error");
  }
}


//do cd command with absolute or relative path
void cdCommand(const char* args[]){
  //chdir automatically handles both absolute and relative paths
  if (chdir(args[1]) == -1){
    perror("Invalid Path");
  }

  //TESTING
  char cwd[100];
  std::cout<<"cd complete: "<<getcwd(cwd, sizeof(cwd))<<std::endl;
  
}
