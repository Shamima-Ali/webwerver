
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  printf("Hello world\n\n");
  printf("This is the response from the web werver.\n\n");

  char* data = getenv("QUERY_STRING");

  char *parsedQ = strtok(data, ":");
  char * parsed = strtok(data, "&");
  int parsedArgs[8]; // hardcoded val = 8
  int ind = 0;

  while(parsed != NULL) {
    size_t lenArg = strlen(parsed) - 2;
    char val[lenArg+1];
    strncpy(val, parsed+2, lenArg);
    parsedArgs[ind] = atoi(val);
    // printf("parsedArgs = %d\n", parsedArgs[ind]);
    ind++;
    parsed = strtok(NULL, "&");
  }
  
  int x = parsedArgs[0];
  int y = parsedArgs[1];

  int res = x*y; 

  printf("%d\n", res);
  return 0;
}


/*
export QUERY_STRING="x=6&y=5" 
*/