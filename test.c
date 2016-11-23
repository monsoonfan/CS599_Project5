#include <stdio.h>
#include <stdlib.h>

void testFunc() {
  printf("TestFunc works...\n");
}

void message (char message_code[], char message[]) {
  if(message_code == "Error") {
    fprintf(stderr,"%s: %s\n",message_code,message);
    closeAndExit();
    exit(-1);
  } else {
    printf("%s: %s\n",message_code,message);
  }
}

void skipWhitespace (PPM_file_struct* input) {
  if (VERBOSE) printf("sS: skipping space...\n");
  while(CURRENT_CHAR == ' ' || CURRENT_CHAR == '\n') {
    CURRENT_CHAR = fgetc(input->fh_in);
  }
  PREV_CHAR = CURRENT_CHAR;
}
