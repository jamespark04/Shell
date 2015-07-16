#include<unistd.h>

int findRedirectIndices(int* indices, char* tokens[]) {
  int i=0;
  int seenRead = 0;
  int seenWrite = 0;
  while(tokens[i] != NULL) {
    if (*tokens[i] == '<') {
      indices[0] = i;
      if (seenRead == 0) {
	seenRead = 1;
      }
      else {
	return -1;
      }
    }
    else if (*tokens[i] == '>') {
      indices[1] = i;
      if (seenWrite == 0) {
	seenWrite = 1;
      }
      else {
	return -1;
      }
    }
    i++;
  }
  return 0;
}

int computeTokenLimit(int* k, char* tokens[]) {
  int indices[2] = {-1, -1};
  int limit;
  int l = findRedirectIndices(indices, tokens);
  if (l == -1) { 
    return -1;
  }
  if (indices[0] > 0 && indices[1] > 0) {
    if ((indices[0] != indices[1] + 2) && (indices[1] != indices[0] + 2)) {
      return -1;
    }
    limit = indices[0];
    if (limit > indices[1]) {
      limit = indices[1];
      *k = limit;
      return 3;
    }
    *k = limit;
    return 4;
  }
  else if (indices[0] > 0) {
    limit = indices[0];
    *k = limit;
    return 2;
  }
  else if (indices[1] > 0) {
    limit = indices[1];
    *k = limit;
    return 1;
  }
  else if (indices[0] == 0 || indices[1] == 0) {
    return -1;
  }
  else {
    return 0;
  }
}
