#include<unistd.h>

int findPipeIndex(char* tokens[]) {
  int i = 0;
  int index = -1;
  int seenPipe = 0;
  while(tokens[i] != NULL) {
    if (*tokens[i] == '|') {
      if (seenPipe == 0) {
	index = i;
	seenPipe = 1;
      }
      else {
	return -1;
      }
    }
    i++;
  }
  return index;
}
