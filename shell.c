#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/wait.h>
#include"tokenizer.h"
#include"tokenizer.c"
#include"redirect.c"
#include"linked_list.c"
#include"pipe.c"
#include<errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
char string[1024] = "";
char storedString[1024];
char bgString[1024];
int pid;
int readSuccess;
int writeSuccess;
TOKENIZER *tokenizer;
char *token;
char *tokens[1024];
char *copyTokens[1024];
int i = 0;
int j = 0;
int k = 0;
int l;
int readFileNo;
int writeFileNo;
int success;
int dupSuccess1;
int dupSuccess2;
int dupSuccess;
int isAmp;
int currentStop;
int currentBg = 0;
int currentFg = 0;
int canPrompt=0;
int pidList[1024];
int mostRecentProcess = 0;
int n = 0;
int hasStop = 0;
int testError;
int test;
struct struct_of_ints_struct finishedJobs;
struct_of_ints *head_node = NULL;

// finds the index of the ampersand in the tokens
int findAmp(char *tokens[]) {
  int i = 0;
  int index = -1;
  while(tokens[i] != NULL) {
    if (*tokens[i] == '&') {
      if (tokens[i+1] == NULL) {
	index = i;
      }
      else {
	return -1;
      }
    }
    i++;
  }
  return index;
}

// checks for the bg command (no spaces allowed after)
int isBg(char *string) {
  if (string[2] != '\n') {
    return 0;
  }
  if (string[0] == 'b' && string[1] == 'g') {
    return 1;
  }
  else {
    return 0;
  }
}

// checks for the fg command (no spaces allowed after)
int isFg(char *string) {
  if (string[2] != '\n') {
    return 0;
  }
  if (string[0] == 'f' && string[1] == 'g') {
    return 1;
  }
  else {
    return 0;
  }
}

// process handling for signal
void process_handler(int signum) {
  if (pid != 0) {
    killpg(pid, signum);
  }
}

 
int main(int argc, char *argv[]) {
  while(1) {
    // set up terminal control for the shell
    int control1 = tcsetpgrp(STDIN_FILENO, getpgrp());
    int control2 = tcsetpgrp(STDOUT_FILENO, getpgrp());
    if (control1 != 0 || control2 != 0) {
      perror("Failed to set terminal control");
    }
    signal(SIGTERM, process_handler);
    signal(SIGINT, process_handler);
    signal(SIGTSTP, process_handler);
    k = 0;
    i = 0;
    for (j = 0; j < 1024; j++) {
      tokens[j] = NULL;
      string[j] = '\0';
    }

    // if background process finishes, notify user
    if ((test = waitpid(-1, &n, WNOHANG)) > 0) {
      isAmp = 0;
      struct_of_ints* temp = search_list(head_node, test);
      if (temp != NULL) {
	printf("Finished: %s\n", temp->string);
	head_node = delete_from_list(head_node, test);
      }
    }

    writeSuccess = write(1, "$ ", 3);
    if (writeSuccess < 0) {
      perror("Write has failed");
      exit(0);
    }
    readSuccess = read(0, string, 1023);
    if (readSuccess < 1) {
      if (readSuccess < 0) {
	perror("Read has failed");
	exit(0);
      }
      break;
    }
    currentBg = isBg(string);
    currentFg = isFg(string);
    // if bg or fg
    if (currentBg == 1 || currentFg == 1) {
      if(hasStop) {
	if (currentBg == 1) {
	  testError = killpg(mostRecentProcess, SIGCONT);
	  if (testError < 0) {
	    perror("Error relaying Cont signal");
	  }
	  int bgStatus = waitpid(-1, &n, WCONTINUED | WNOHANG);
	  if (bgStatus < 0) {
	    perror("Wait has failed");
	  }
	  else if (WIFCONTINUED(n)) {
	    printf("Running: %s\n", storedString);
	  }
	  int bgStatusStop = waitpid(-1, &n, WUNTRACED | WNOHANG);
	  if (bgStatusStop < 0) {
	    perror("Wait has failed");
	  }    
	  if (WIFSTOPPED(n)) {
	    printf("Stopped: %s\n", storedString);
	  }
	  else if (bgStatusStop > 0) {
	    struct_of_ints* temp = search_list(head_node, test);
	    if (temp != NULL) {
	      printf("Finished: %s\n", temp->string);
	      head_node = delete_from_list(head_node, test);
	    }
	  }
	  else {
	    hasStop = 0;
	  }
	}
	// fg
	else {
	  signal(SIGTTOU, SIG_IGN);
	  signal(SIGTTIN, SIG_IGN);
	  int t1 = tcsetpgrp(STDIN_FILENO, mostRecentProcess);
	  int t2 = tcsetpgrp(STDOUT_FILENO, mostRecentProcess);
	  if (t1 < 0 || t2 < 0) {
	    perror("Failed to set terminal control");
	  }
	  testError = killpg(mostRecentProcess, SIGCONT);
          if (testError < 0) {
            perror("Failed to send Cont signal");
          }
	  signal(SIGTERM, process_handler);
	  signal(SIGINT, process_handler);
	  signal(SIGTSTP, process_handler);
	  testError = waitpid(-1 * mostRecentProcess, &n, WCONTINUED);
	  if (testError < 0) {
	    perror("Wait for continue has failed");
	  }

	  testError = waitpid(-1 * mostRecentProcess, &n, WUNTRACED);
	  if (testError < 0) {
	    perror("Wait has failed");
	  }
	  if (WIFSTOPPED(n)) {
	    printf("Stopped: %s\n", storedString);
	  }
	  else {
	    hasStop = 0;
	  }

	  t1 = tcsetpgrp(STDIN_FILENO, getpgrp());
	  t2 = tcsetpgrp(STDOUT_FILENO, getpgrp());
	  if (t1 < 0 || t2 < 0) {
	    perror("Failed to set terminal control");
	  }
	  signal(SIGTTOU, SIG_DFL);
	  signal(SIGTTIN, SIG_DFL); 
	}
      }
      else {
	errno = ECHILD;
	perror("No recently stopped jobs to fg or bg");
      }
    }
    else {
      tokenizer = init_tokenizer(string);
      if (tokenizer == NULL) {
	perror("Tokenizer initialization has failed");
	exit(0);
      }
      while((token = get_next_token(tokenizer)) != NULL) {
	tokens[i] = token;
	i++;
      }      
      isAmp = findAmp(tokens);
      if (isAmp > 0) {
	i--;
	free(tokens[i]);
	tokens[i] = NULL;
	i--;
      }
      int isRedirect = computeTokenLimit(&k, tokens);
      int isPipe = findPipeIndex(tokens);
      int pipefd[2];
      int cpid;
      if (isPipe > 0) {
	if (pipe(pipefd) < 0) {
	  perror("Failed to create pipe");
	}
      }
      pid = fork();
      char* tempTokens[1024];
      char* tempTokens1[1024];
      char* tempTokens2[1024];
      if (pid == 0) {
	// piping (left child)
	if (isPipe > 0) {
	  for (l = 0; l < 1024; l++) {
	    tempTokens1[l] = NULL;
	  }
	  for (l = 0; l < isPipe; l++) {
	    tempTokens1[l] = tokens[l];
	  }
	  close(pipefd[0]);
	  int dupPipeSuccess1 = dup2(pipefd[1],STDOUT_FILENO);
	  if (dupPipeSuccess1 < 0) {
	    perror("Failed to redirect pipe");
	    exit(0);
	  }
	  k = 0;
	  // piping with redirection
	  int isRedirectWithPipe = computeTokenLimit(&k, tempTokens1);
	  if (isRedirectWithPipe == 2) {
	    for (l = 0; l < 1024; l++) {
	      tempTokens[l] = NULL;
	    }
	    for (l = 0; l < k; l++) {
	      tempTokens[l] = tempTokens1[l];
	    }
	    readFileNo = open(tokens[k+1], O_RDONLY);
	    if (readFileNo < 0) {
	      perror("Failed to open file");
	      exit(0);
	    }
	    dupSuccess = dup2(readFileNo, STDIN_FILENO);
	    if (dupSuccess < 0) {
	      perror("Failed to redirect");
	      exit(0);
	    }
	    success = execvp(tokens[0],tempTokens);
	  }
	  else if (isRedirectWithPipe == 0) {
	    success = execvp(tokens[0],tempTokens1);
	  }
	  else {
	    errno = EINVAL;
	    perror("Invalid redirect syntax");
	    exit(0);
	  }
	  if (success < 0) {
	    perror("Exec has failed");
	    exit(0);
	  }
	}      
	// not piping
	else {
	  for (l = 0; l < 1024; l++) {
	    tempTokens[l] = NULL;
	  }
	  for (l = 0; l < k; l++) {
	    tempTokens[l] = tokens[l];
	  }
	  // potential redirection, case 0 is no redirection/no piping
	  // case 1 is >, case 2 is <, case 3 is >...<, case 4 is <....>
	  switch(isRedirect) {
	  case 0:
	    success = execvp(tokens[0], tokens);
	    break;
	  case 1:
	    if (tokens[k+1] == NULL) {
	      errno = EINVAL;
	      perror("Invalid Redirection Syntax");
	      exit(0);
	    }
	    writeFileNo = open(tokens[k+1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
	    if (writeFileNo < 0) {
	      perror("Failed to open given file");
	      exit(0);
	    }
	    dupSuccess = dup2(writeFileNo,STDOUT_FILENO);
	    if (dupSuccess < 0) {
	      perror("Failed to redirect file pointer");
	      exit(0);
	    }
	    success = execvp(tokens[0], tempTokens);
	    break;
	  case 2:
	    if (writeFileNo < 0) {
	      perror("Failed to open given file");
	      exit(0);
	    }
	    if (tokens[k+1] == NULL) {
	      errno = EINVAL;
	      perror("Invalid Redirection Syntax");
	      exit(0);
	    }
	    readFileNo = open(tokens[k+1], O_RDONLY);
	    if (readFileNo < 0) {
	      perror("Failed to open given file");
	      exit(0);
	    }
	    dupSuccess = dup2(readFileNo,STDIN_FILENO);
	    if (dupSuccess < 0) {
	      perror("Failed to redirect file pointer");
	      exit(0);
	    }
	    success = execvp(tokens[0], tempTokens);
	    break; 
	  case 3:
	    if (tokens[k+1] == NULL || tokens[k+3] == NULL) {
	      errno = EINVAL;
	      perror("Invalid Redirection Syntax");
	      exit(0);
	    }
	    writeFileNo = open(tokens[k+1], O_WRONLY|O_CREAT|O_TRUNC,0644);
	    if (writeFileNo < 0) {
	      perror("Failed to open given file");
	      exit(0);
	    }
	    dupSuccess1 = dup2(writeFileNo,STDOUT_FILENO);
	    if (dupSuccess1 < 0) {
	      perror("Failed to redirect file pointer");
	      exit(0);
	    }
	    readFileNo = open(tokens[k+3], O_RDONLY);
	    if (readFileNo < 0) {
	      perror("Failed to open given file");
	      exit(0);
	    }
	    dupSuccess2 = dup2(readFileNo,STDIN_FILENO);
	    if (dupSuccess2 < 0) {
	      perror("Failed to redirect file pointer");
	      exit(0);
	    }
	    success = execvp(tokens[0], tempTokens);
	    break;
	  case 4:
	    if (tokens[k+1] == NULL || tokens[k+3] == NULL) {
	      errno = EINVAL;
	      perror("Invalid Redirection Syntax");
	      exit(0);
	    }
	    readFileNo = open(tokens[k+1], O_RDONLY);
	    if (readFileNo < 0) {
	      perror("Failed to open given file");
	      exit(0);
	    }
	    dupSuccess2 = dup2(readFileNo,STDIN_FILENO);
	    if (dupSuccess2 < 0) {
	      perror("Failed to redirect file pointer");
	      exit(0);
	    }
	    writeFileNo = open(tokens[k+3], O_WRONLY|O_CREAT|O_TRUNC,0644);
	    if (writeFileNo < 0) {
	      perror("Failed to open given file");
	      exit(0);
	    }
	    dupSuccess1 = dup2(writeFileNo,STDOUT_FILENO);
	    if (dupSuccess1 < 0) {
	      perror("Failed to redirect file pointer");
	      exit(0);
	    }
	    success= execvp(tokens[0], tempTokens);
	    break;
	  default:
	    errno = EINVAL;
	    perror("Invalid Redirection Syntax");
	    exit(0);
	  }
	  if (success == -1) {
	    perror("Exec has failed");
	    exit(0);
	  }
	}
      }
      // parent shell after first fork
      else if (pid > 0) {
	if (setpgid(pid, 0) != 0) {
	  perror("Setting process group has failed");
	  exit(0);
	}
	// if piping, need to fork parent again
	if (isPipe > 0) {
	  cpid = fork();
	  if (cpid < 0) {
	    perror("Failed to fork");
	    exit(0);
	  }
	  // right child of pipe
	  if (cpid == 0) {
	    for (l = 0; l < 1024; l++) {
	      tempTokens2[l] = NULL;
	    }
	    for (l = isPipe+1; l < 1024; l++) {
	      tempTokens2[l - (isPipe + 1)] = tokens[l];
	    }
	    close(pipefd[1]);
	    int dupPipeSuccess1 = dup2(pipefd[0],STDIN_FILENO);
	    if (dupPipeSuccess1 < 0) {
	      perror("Failed to redirect for pipe");
	      exit(0);
	    }
	    k = 0;
	    // piping with redirection
	    int isRedirectWithPipe = computeTokenLimit(&k, tempTokens2);
	    if (isRedirectWithPipe == 1) {
	      for (l = 0; l < 1024; l++) {
		tempTokens[l] = NULL;
	      }
	      for (l = 0; l < k; l++) {
		tempTokens[l] = tempTokens2[l];
	      }
	      writeFileNo = open(tempTokens2[k+1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
	      if (writeFileNo < 0) {
		perror("Failed to open file");
		exit(0);
	      }
	      dupSuccess = dup2(writeFileNo, STDOUT_FILENO);
	      if (dupSuccess < 0) {
		perror("Failed to redirect file pointer");
		exit(0);
	      }
	      success = execvp(tokens[isPipe + 1],tempTokens);
	      
	    }
	    else if (isRedirectWithPipe == 0) {
	      success = execvp(tokens[isPipe + 1],tempTokens2);
	    }
	    else {
	      errno = EINVAL;
	      perror("Invalid Redirection Syntax");
	      exit(0);
	    }
	    if (success < 0) {
	      perror("Exec has failed");
	      exit(0);
	    }
	  }
	  // parent of piped command
	  else if (cpid > 0) {
	    if (setpgid(cpid, pid) != 0) {
	      perror("Setting process group has failed");
	    }
	    close(pipefd[0]);
	    close(pipefd[1]);
	    if (isAmp <= 0) {
	      signal(SIGTTOU, SIG_IGN);
	      signal(SIGTTIN, SIG_IGN);
	      int t1 = tcsetpgrp(STDIN_FILENO, pid);
	      int t2 = tcsetpgrp(STDOUT_FILENO, pid);
	      signal(SIGTTOU, SIG_DFL);
	      signal(SIGTTIN, SIG_DFL);
	      signal(SIGINT, process_handler);
	      signal(SIGTSTP, process_handler);
	      signal(SIGTERM, process_handler);
	      if (t1 != 0 || t2 != 0) {
		perror("Failed to give child terminal control");
	      }
	    
	      test = waitpid(pid, &n, WUNTRACED);
	    }
	    else {
	      test = waitpid(pid, &n, WUNTRACED | WNOHANG);
	    }
	    if (test < 0) {
	      perror("Wait has failed");
	    }
	    signal(SIGTTOU, SIG_IGN);
	    signal(SIGTTIN, SIG_IGN);
	    int t3 = tcsetpgrp(STDIN_FILENO, getpgrp());
	    int t4 = tcsetpgrp(STDOUT_FILENO, getpgrp());
	    if (t3 != 0 || t4 != 0) {
	      perror("Failed to give child terminal control");
	    }
	    signal(SIGTTOU, SIG_DFL);
	    signal(SIGTTIN, SIG_DFL);
	  }
	  else {
	    perror("Fork has failed");
	  }
	}
	// still shell parent, if not a background command
	if (isAmp <= 0) {
	  signal(SIGTTOU, SIG_IGN);
	  signal(SIGTTIN, SIG_IGN);
	  int t1 = tcsetpgrp(STDIN_FILENO, pid);
	  int t2 = tcsetpgrp(STDOUT_FILENO, pid);
	  signal(SIGTTOU, SIG_DFL);
	  signal(SIGTTIN, SIG_DFL);
          signal(SIGINT, process_handler);
          signal(SIGTSTP, process_handler);
          signal(SIGTERM, process_handler);
	  if (t1 != 0 || t2 != 0) {
	    perror("Failed to give child terminal control");
	  }
	  int statusChild = waitpid(-1 * pid, &n, WUNTRACED);
	  if (statusChild > 0) {
	    if (WIFSTOPPED(n)) {
	      mostRecentProcess = pid;
	      hasStop = 1;
              for (l = 0; l < 1024; l++) {
                storedString[l] = '\0';
              }
              l = 0;
              while (l < 1024 && string[l] != '&') {
                storedString[l] = string[l];
                l++;
              }
              printf("Stopped: %s\n", storedString);
	    }
	  }
	  if (statusChild < 0) {
	    perror("Wait has failed");
	    exit(0);
	  }
	  signal(SIGTTOU, SIG_IGN);
	  signal(SIGTTIN, SIG_IGN);
	  t1 = tcsetpgrp(STDIN_FILENO, getpgrp());
	  t2 = tcsetpgrp(STDOUT_FILENO, getpgrp());
	  signal(SIGTTOU, SIG_DFL);
	  signal(SIGTTIN, SIG_DFL);
	  if (t1 != 0 || t2 != 0) {
	    perror("Failed to give shell terminal control");
	  }
	  if ((test = waitpid(-1, &n, WNOHANG)) > 0) {
	    struct_of_ints* temp = search_list(head_node, test);
	    if (temp != NULL) {
	      printf("Finished: %s\n", temp->string);
	      head_node = delete_from_list(head_node, test);
	    }
	  }
	}
	// is background command
	else {
	  int statusBg = waitpid(-1, &n, WNOHANG);
	  if (statusBg == 0 && isAmp > 0) {
	    for (l = 0; l < 1024; l++) {
	      bgString[l] = '\0';
	    }
	    l = 0;
	    while (l < 1024 && string[l] != '&') {
	      bgString[l] = string[l];
	      l++;
	    }
	    printf("Running: %s\n", bgString);
	    head_node = add_to_list(head_node, pid, bgString);
	  }
	  int statusBgStop = waitpid(-1, &n, WUNTRACED | WNOHANG);
	  if (statusBgStop > 0 && isAmp > 0) {
	    if (WIFSTOPPED(n)) {
	      mostRecentProcess = pid;
	      hasStop = 1;
	      for (l = 0; l < 1024; l++) {
		storedString[l] = '\0';
	      }
	      l = 0;
	      while (l < 1024 && string[l] != '&') {
		storedString[l] = string[l];
		l++;
	      }
	      printf("Stopped: %s\n", storedString);
	    }
	    else {
	      if ((test = waitpid(-1, &n, WNOHANG)) > 0) {
		struct_of_ints* temp = search_list(head_node, test);
		if (temp != NULL) {
		  printf("Finished: %s\n", temp->string);
		  head_node = delete_from_list(head_node, test);
		}
	      }
	    }
	  }
	}
	while (i >= 0) {
	  free(tokens[i]);
	  i--;
	}
	free_tokenizer(tokenizer);
      }
      else {
	perror("Fork has failed");
      }
    }
  }
  delete_list(head_node);
  return 0;
}
