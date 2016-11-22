/*
---------------------------------------------------------------------------------------
ppmrw - from CS599 Project 1
R Mitchell Ralston (rmr5)
10/6/16
---------------------------------------------------------------------------------------
*/
// libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// typdefs
typedef struct RGBPixel {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} RGBPixel ;

typedef struct RGBAPixel {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
} RGBAPixel ;

typedef struct PPM_file_struct {
  char magic_number;
  int lines;
  int width;
  int height;
  int alpha;
  int depth;
  char *tupltype;
  //RGBPixel *pixel_map; // didn't get around to nesting pixel_maps into the file_struct
  FILE* fh_in;
} PPM_file_struct ;

// global variables
int CURRENT_CHAR        = 'a';
int PREV_CHAR           = '\n';
int OUTPUT_MAGIC_NUMBER = 0;
int VERBOSE             = 0; // controls logfile message level

// global data structures
PPM_file_struct     INPUT_FILE_DATA;
RGBPixel           *RGB_PIXEL_MAP;
RGBAPixel          *RGBA_PIXEL_MAP;
PPM_file_struct     OUTPUT_FILE_DATA;


// functions
int   readPPM         (char *infile,          PPM_file_struct *input);
void  writePPM        (char *outfile,         PPM_file_struct *input);
void  message         (char message_code[],   char message[]        );
int   getNumber       (int max_value,         PPM_file_struct *input);
char* getWord         (PPM_file_struct *input);
void  reportPPMStruct (PPM_file_struct *input);
void  reportPixelMap  (RGBPixel *pm          );
void  skipWhitespace  (PPM_file_struct *input);
void  skipLine        (PPM_file_struct *input);
void  writePPMHeader  (FILE* fh              );

void  help            ();
int   computeDepth();
char* computeTuplType();
void  freeGlobalMemory ();
void  closeAndExit ();

/*
  ------------------------
  FUNCTION IMPLEMENTATIONS
  ------------------------
*/

/*
  --- message ---
  - 9/10/16
  - rmr5
  ---------------
  print a message to stdout consisting of a message code and a message to a given channel
  current valid channels to write to (stdout, stderr, etc) - will add fh later
  //void message (char channel[], char message_code[], char message[]) {
*/
void message (char message_code[], char message[]) {
  if(message_code == "Error") {
    fprintf(stderr,"%s: %s\n",message_code,message);
    closeAndExit();
    exit(-1);
  } else {
    printf("%s: %s\n",message_code,message);
  }
}

/*
  --- help ---
  - rmr5
  ---------------
  print usage to user when arguments invalid
*/
void help () {
  message("Error","Invalid arguments!");
  message("Usage","ppmrw 3 input.ppm output.ppm");
}

/*
  --- freeGlobalMemory ---
  - 9/15/16
  - rmr5
  ---------------
  free up any globally malloc memory
*/
void freeGlobalMemory () {
  free(RGB_PIXEL_MAP);
  free(RGBA_PIXEL_MAP);
}

/*
  --- closeAndExit ---
  - 9/15/16
  - rmr5
  ---------------
  prepare to gracefully exit the program by freeing memory and closing any open filehandles
  TODO: need to finish
*/
void closeAndExit () {
  freeGlobalMemory();
  //fclose(INPUT_FILE_DATA->fh_in);
  exit(-1);
}

/*
  --- readPPM ---
  - 9/10/16
  - rmr5
  ----------------
  read the file to initialize the data structure associated with it, finding its width/height
  and use that to malloc efficiently, do any error checking that its a valid PPM file

  Here is the spec used (from sourceforge.net/doc/ppm.html:
  1) A "magic number" for identifying the file type. A ppm image's magic number is the two characters "P6".
  2) Whitespace (blanks, TABs, CRs, LFs).
  *  Also have to deal with comment lines! This spec says nothing of them.
  3) A width, formatted as ASCII characters in decimal.
  4) Whitespace.
  5) A height, again in ASCII decimal.
  6) Whitespace.
  7) The maximum color value (Maxval), again in ASCII decimal. Must be less than 65536 and more than zero.
  8) A single whitespace character (usually a newline).
  9) A raster of Height rows, in order from top to bottom. Each row consists of Width pixels, 
     in order from left to right. Each pixel is a triplet of red, green, and blue samples, in that order. 
     Each sample is represented in pure binary by either 1 or 2 bytes. If the Maxval is less than 256, it
     is 1 byte. Otherwise, it is 2 bytes. The most significant byte is first.

     P7 = header is more free-form and has an end-of-header directive
     custom tuple types, but we just need to be able to do RGB and RBGA, anything else, just 
     return Error unsupported type
     we can assume just one byte for this project, but throw an error if it's more than one
     but you better check for anything that could go wrong, each line of code, throw an error to stderr

     fread error checking (don't exceed max val and don't hit EOF)
  ----------------
*/
int readPPM (char *infile, PPM_file_struct *input) {
  // opening message
  printf("Info: Scanning %s for info ...\n",infile);
  input->fh_in = fopen(infile,"r");

  // variables for this func with some initial settings
  int first_char = fgetc(input->fh_in);
  CURRENT_CHAR = fgetc(input->fh_in);
  int linecount = 0;
  int charcount = 0;
  
  // ------------------------------- HEADER --------------------------------
  // First char should always be a P (else exit), if so next char is number and advance from there
  if (first_char != 'P') {
    message("Error","File format not recognized!");
  } else {
    input->magic_number = getNumber(7,input);
    linecount++;
    CURRENT_CHAR = fgetc(input->fh_in);
  } // Done with first line, advanced to 1st char of 2nd line
  
  // P3/P6 cases - Now traverse any comment lines in one loop
  if (input->magic_number == 3 ||input->magic_number == 6) {
    int done_with_comments = 0;
    while(CURRENT_CHAR != EOF) {
      charcount++;
      // Deal with the comment lines by advancing until we hit a new line where first char is not #
      if(!done_with_comments) { // do this if only once
	if(CURRENT_CHAR == '#') {
	  while(!(PREV_CHAR == '\n' && CURRENT_CHAR != '#')) {
	    PREV_CHAR = CURRENT_CHAR;
	    CURRENT_CHAR = fgetc(input->fh_in);
	    if(CURRENT_CHAR == '\n') {linecount++;}
	  }
	} else {
	  message("Warning","No comment lines detected, please double-check file");
	}
	done_with_comments = 1;
	// always on the 1st char of the next line after comments
      }
      
      // Now get the width/height, format checking as part of this
      input->width = getNumber(8224,input);
      
      // Now height
      skipWhitespace(input);
      input->height = getNumber(8224,input);
      
      // Need the alpha channel next
      // If the Maxval is less than 256, it is 1 byte. Otherwise, it is 2 bytes. The most significant byte is first.
      skipWhitespace(input);
      input->alpha = getNumber(256,input);

      // Also need P7 values in case they get used
      input->depth = 3;
      input->tupltype = "RGB";
      
      message("Info","Completed processing header information.");
      
      break;
    }
    // P7 header case
  } else {
    // P7 headers have tokens to process
    int got_width    = 0;
    int got_height   = 0;
    int got_depth    = 0;
    int got_maxval   = 0;
    int got_tupltype = 0;
    int comment_lines= 0;
    
    // This while is to process the header only, will get the next "word" and process
  PH: while(CURRENT_CHAR != EOF) {
      char *token_name;
      //    char *token_name;
      token_name = getWord(input); // this is a hack, only getting first char of token but sould work

      // Break out of the loop if ENDHDR
      if (strcmp(token_name, "ENDHDR") == 0) {
	message("Info","ENDHDR line reached");
	break; 
      }

      // skip comment lines, else, advance the filehandle to next "word"
      if (token_name[0] == '#') {
	if (VERBOSE) printf("PH: tn[0] # skipping coments\n");
	skipLine(input);
	goto PH;
      } else {
	if (VERBOSE) printf("PH: token_name = %s     ...skipping space\n",token_name);
	skipWhitespace(input); // position at the first char of the token, switch will grab token and advance line
      }

      // Series of if/else if to process each token appropriately
      if (strcmp(token_name, "WIDTH") == 0) {
	if(got_width) {message("Error","More than one WIDTH token!");}
	message("Info","Processing WIDTH token");
	input->width = getNumber(8224,input);
	skipWhitespace(input);
	got_width = 1;
      } else if (strcmp(token_name, "HEIGHT") == 0) {
	if(got_height) {message("Error","More than one HEIGHT token!");}
	message("Info","Processing HEIGHT token");
	input->height = getNumber(8224,input);
	skipWhitespace(input);
	got_height = 1;
      } else if (strcmp(token_name, "DEPTH") == 0) {
	if(got_depth) {message("Error","More than one DEPTH token!");}
	message("Info","Processing DEPTH token");
	input->depth = getNumber(4,input);
	skipWhitespace(input);
	got_depth = 1;
      } else if (strcmp(token_name, "MAXVAL") == 0) {
	if(got_maxval) {message("Error","More than one MAXVAL token!");}
	message("Info","Processing MAXVAL token");
	input->alpha = getNumber(256,input);
	skipWhitespace(input);
	got_maxval = 1;
      } else if (strcmp(token_name, "TUPLTYPE") == 0) {
	if(got_tupltype) {message("Error","More than one TUPLTYPE token!");}
	message("Info","Processing TUPLTYPE token");
	input->tupltype = getWord(input);
	if((strcmp(input->tupltype,"RGB")) != 0 && (strcmp(input->tupltype,"RGB_ALPHA")) != 0) {message("Error","Unsupported TUPLTYPE!");}
	skipWhitespace(input);
	got_tupltype = 1;
      } else {
	printf("Info: Processing %s token\n",token_name);	
	message("Error","Unsupported token");
      }
    }
    if (got_width && got_height && got_depth && got_maxval && got_tupltype) {
      message("Info","Done processing header");
    } else {
      message("Error","Missing token(s), invalid header!");
    }
  }
  
  // ------------------------------- END HEADER ----------------------------

  // ------------------------------- BEGIN IMAGE ----------------------------
  // To read the raster/buffer info:
  // need a case statement to deal with 3/6/7 formats separately
  message("Info","Process image information...");
  RGB_PIXEL_MAP  = malloc(sizeof(RGBPixel)  * input->width * input->height );
  RGBA_PIXEL_MAP = malloc(sizeof(RGBAPixel) * input->width * input->height );
  int number_count = 0;
  int rgb_index = 0;
  int pm_index = 0;
  int total_pixels = (input->width) * (input->height) * 3;

  // This switch handles parsing the various input file formats for the image data
  switch(input->magic_number) {
  case(3):
    message("Info","  format version: 3");
    while(PREV_CHAR != EOF && number_count < total_pixels) {
      rgb_index = number_count % 3;
      skipWhitespace(input);
      int value = getNumber(255,input);
      switch(rgb_index) {
      case(0):
	RGB_PIXEL_MAP[pm_index].r = value;
	if(VERBOSE) printf("  stored[%d] %d to RGB_PIXEL_MAP red\n",pm_index,RGB_PIXEL_MAP[pm_index].r);
	break;
      case(1):
	RGB_PIXEL_MAP[pm_index].g = value;
	if(VERBOSE) printf("  stored[%d] %d to RGB_PIXEL_MAP green\n",pm_index,RGB_PIXEL_MAP[pm_index].g);
	break;
      case(2):
	RGB_PIXEL_MAP[pm_index].b = value;
	if(VERBOSE) printf("  stored[%d] %d to RGB_PIXEL_MAP blue\n",pm_index,RGB_PIXEL_MAP[pm_index].b);
	pm_index++;
	break;
      }
      number_count++;
    }
    printf("read %d numbers\n",number_count);
    message("Info","Done reading PPM 3");
    //reportPixelMap(RGB_PIXEL_MAP);
    break; // magic number case(3)
  case(6):
    message("Info","  format version: 6");
    while(number_count < total_pixels) {
      //      int value[4];
      unsigned char value;
      rgb_index = number_count % 3;
      // TODO: fread error checking (don't exceed max val and don't hit EOF)
      //      if (!fread(&value,sizeof(RGBPixel)/3,1,input->fh_in)) {message("Error","Binary data read error");}
      if (!fread(&value,sizeof(unsigned char),1,input->fh_in)) {message("Error","Binary data read error");}
      switch(rgb_index) {
      case(0):
	RGB_PIXEL_MAP[pm_index].r = value;
	if(VERBOSE) printf("  stored[%d](%d) %d to RGB_PIXEL_MAP red\n",pm_index,rgb_index,RGB_PIXEL_MAP[pm_index].r);
	break;
      case(1):
	RGB_PIXEL_MAP[pm_index].g = value;
	if(VERBOSE) printf("  stored[%d](%d) %d to RGB_PIXEL_MAP green\n",pm_index,rgb_index,RGB_PIXEL_MAP[pm_index].g);
	break;
      case(2):
	RGB_PIXEL_MAP[pm_index].b = value;
	if(VERBOSE) printf("  stored[%d](%d) %d to RGB_PIXEL_MAP blue\n",pm_index,rgb_index,RGB_PIXEL_MAP[pm_index].b);
	pm_index++;
	break;
      }
      number_count++;
    }
    printf("Info: read %d bytes\n",number_count);
    break; // magic number case(6)
  case(7):
    message("Info","  format version: 7");
    unsigned char value;
    // TODO: fread error checking (don't exceed max val and don't hit EOF)
    //      if (!fread(&value,sizeof(RGBPixel)/3,1,input->fh_in)) {message("Error","Binary data read error");}
    if (strcmp(input->tupltype,"RGB") == 0) {
      message("Info","  reading RGB information only, no alpha channel...");
      while(number_count < total_pixels) {
	if (!fread(&value,sizeof(unsigned char),1,input->fh_in)) {message("Error","Binary data read error");}
	rgb_index = number_count % 3;
	switch(rgb_index) {
	case(0):
	  RGB_PIXEL_MAP[pm_index].r = value;
	  if(VERBOSE) printf(" stored[%d](%d) %d to pixel_map red\n",pm_index,rgb_index,RGB_PIXEL_MAP[pm_index].r);
	  break;
	case(1):
	  RGB_PIXEL_MAP[pm_index].g = value;
	  if(VERBOSE) printf(" stored[%d](%d) %d to pixel_map green\n",pm_index,rgb_index,RGB_PIXEL_MAP[pm_index].g);
	  break;
	case(2):
	  RGB_PIXEL_MAP[pm_index].b = value;
	  if(VERBOSE) printf(" stored[%d](%d) %d to pixel_map blue\n",pm_index,rgb_index,RGB_PIXEL_MAP[pm_index].b);
	  pm_index++;
	  break;
	}
	number_count++;
      }
    } else if (strcmp(input->tupltype,"RGB_ALPHA") == 0) {
      message("Info","  reading RGB and ALPHA information...");
      total_pixels = (input->width) * (input->height) * 4;
      while(number_count < total_pixels) {
	if (!fread(&value,sizeof(unsigned char),1,input->fh_in)) {message("Error","Binary data read error");}
	rgb_index = number_count % 4;
	//	printf("DBG after read: rgb_index: %d number_count: %d\n",rgb_index,number_count);
	switch(rgb_index) {
	case(0):
	  RGB_PIXEL_MAP[pm_index].r = value;
	  RGBA_PIXEL_MAP[pm_index].r = value;
	  if(VERBOSE) printf(" stored[%d](%d) %d to pixel_map red\n",pm_index,rgb_index,RGBA_PIXEL_MAP[pm_index].r);
	  break;
	case(1):
	  RGB_PIXEL_MAP[pm_index].g = value;
	  RGBA_PIXEL_MAP[pm_index].g = value;
	  if(VERBOSE) printf(" stored[%d](%d) %d to pixel_map green\n",pm_index,rgb_index,RGBA_PIXEL_MAP[pm_index].g);
	  break;
	case(2):
	  RGB_PIXEL_MAP[pm_index].b = value;
	  RGBA_PIXEL_MAP[pm_index].b = value;
	  if(VERBOSE) printf(" stored[%d](%d) %d to pixel_map blue\n",pm_index,rgb_index,RGBA_PIXEL_MAP[pm_index].b);
	  break;
	case(3):
	  RGBA_PIXEL_MAP[pm_index].a = value;
	  if(VERBOSE) printf(" stored[%d](%d) %d to pixel_map alpha\n",pm_index,rgb_index,RGBA_PIXEL_MAP[pm_index].a);
	  pm_index++;
	  break;
	}
	number_count++;
      }
    } else {
      message("Error","Detected invalid TUPLTYPE");
    }
    printf("Info: read %d bytes\n",number_count);
    break; // magic number case(7)
  }
  
  // ------------------------------- END IMAGE ------------------------------

  // finishing
  fclose(input->fh_in);
  message("Info","Done");
  reportPPMStruct(input);
  return input->magic_number;
}

//  small helper to assign proper depth to a P7 file
int computeDepth() {
  if ((strcmp(INPUT_FILE_DATA.tupltype,"RGB_ALPHA")) == 0) {
    return 4; 
  } else {
    return 3;
  }
}

// helper to assign preper tupltype in P7
char* computeTuplType() {
  if ((strcmp(INPUT_FILE_DATA.tupltype,"RGB_ALPHA")) == 0) {
    if (VERBOSE) printf("cD: returning tupltype RGB_ALPHA because input was %s\n",INPUT_FILE_DATA.tupltype);
    return "RGB_ALPHA"; 
  } else {
    if (VERBOSE) printf("cD: returning tupltype RGB because input was %s\n",INPUT_FILE_DATA.tupltype);
    return "RGB"; 
  }
}

// helper function to write the header to a file handle
void writePPMHeader (FILE* fh) {
  int magic_number = OUTPUT_MAGIC_NUMBER;

  // These values/header elements are the same regardless format
  printf("Info: Converting to format %d ...\n",magic_number);
  fprintf(fh,"P%d\n",magic_number);
  fprintf(fh,"# PPM file format %d\n",magic_number);
  fprintf(fh,"# written by ppmrw(rmr5)\n");
  // make some variable assignments from input -> output
  OUTPUT_FILE_DATA.magic_number = magic_number;
  OUTPUT_FILE_DATA.width        = INPUT_FILE_DATA.width;
  OUTPUT_FILE_DATA.height       = INPUT_FILE_DATA.height;
  OUTPUT_FILE_DATA.alpha        = INPUT_FILE_DATA.alpha;
  
  if (magic_number == 3 || magic_number == 6) {
    fprintf(fh,"%d %d\n",       OUTPUT_FILE_DATA.width,OUTPUT_FILE_DATA.height);
    fprintf(fh,"%d\n",          OUTPUT_FILE_DATA.alpha);
  } else if (magic_number == 7) {
    OUTPUT_FILE_DATA.depth      = computeDepth();
    OUTPUT_FILE_DATA.tupltype   = computeTuplType();
    
    fprintf(fh,"WIDTH %d\n",    OUTPUT_FILE_DATA.width);
    fprintf(fh,"HEIGHT %d\n",   OUTPUT_FILE_DATA.height);
    fprintf(fh,"DEPTH %d\n",    OUTPUT_FILE_DATA.depth);
    fprintf(fh,"MAXVAL %d\n",   OUTPUT_FILE_DATA.alpha);
    fprintf(fh,"TUPLTYPE %s\n", OUTPUT_FILE_DATA.tupltype);
    fprintf(fh,"ENDHDR\n");
  } else {
    message("Error","Trying to output unsupported format!\n");
  }
  message("Info","Done writing header");
}

/*
  --- writePPM ---
  - 9/13/16
  - rmr5
  ---------------
  Major function to write the actual output ppm file
  takes a output filename and an input PPM struct
  uses global data

  This function has case statements to support all supported formats 
*/
void writePPM (char *outfile, PPM_file_struct *input) {
  printf("Info: Writing file %s...\n",outfile);
  FILE* fh_out = fopen(outfile,"wb");

  // -------------------------- write header ---------------------------------
  writePPMHeader(fh_out);
  // ---------------------- done write header --------------------------------

  // -------------------------- write image ----------------------------------
  int pixel_index = 0;
  int modulo;
  switch(OUTPUT_FILE_DATA.magic_number) {
    // P3 format
    // Iterate over each pixel in the pixel map and write them byte by byte
  case(3):
    message("Info","Outputting format 3");
    while(pixel_index < (OUTPUT_FILE_DATA.width) * (OUTPUT_FILE_DATA.height)) {      
      fprintf(fh_out,"%3d %3d %3d",RGB_PIXEL_MAP[pixel_index].r,RGB_PIXEL_MAP[pixel_index].g,RGB_PIXEL_MAP[pixel_index].b);
      modulo = (pixel_index + 1) % (OUTPUT_FILE_DATA.width);
      if ( modulo == 0 ) {
	fprintf(fh_out,"\n");
      } else {
	fprintf(fh_out," ");
      }
      pixel_index++;
    }
    break;
    // P6 format
    // write the entire pixel_map in one command
  case(6):
    message("Info","Outputting format 6");
    fwrite(RGB_PIXEL_MAP, sizeof(RGBPixel), OUTPUT_FILE_DATA.width * OUTPUT_FILE_DATA.height, fh_out);
    break;
    // P7 format
  case(7):
    // write the entire pixel_map in one command, RGB writes from RGB pixel_map and RGBA writes from RGBA pixel_map
    message("Info","Outputting format 7");
    if (strcmp(OUTPUT_FILE_DATA.tupltype,"RGB_ALPHA") == 0) {
      message("Info","   output file will have alpha data");
      fwrite(RGBA_PIXEL_MAP, sizeof(RGBAPixel), OUTPUT_FILE_DATA.width * OUTPUT_FILE_DATA.height, fh_out);
    } else {
      message("Info","   output file is RGB only");
      fwrite(RGB_PIXEL_MAP, sizeof(RGBPixel), OUTPUT_FILE_DATA.width * OUTPUT_FILE_DATA.height, fh_out);
    }
    break;
  default:
    message("Error","Unrecognized output format");
  }
  // ---------------------- done write image ---------------------------------

  fclose(fh_out);
  reportPPMStruct(&OUTPUT_FILE_DATA);
  message("Info","Done ");
}

// helper function to visualize what's in a given PPM struct
void reportPPMStruct (PPM_file_struct *input) {
  message("Info","Contents of PPM struct:");
  printf("     magic_number: %d\n",input->magic_number);
  printf("     width:        %d\n",input->width);
  printf("     height:       %d\n",input->height);
  if (input->magic_number == 7) {
  printf("     max_value:    %d\n",input->alpha);
  printf("     depth:        %d\n",input->depth);
  printf("     tupltype:     %s\n",input->tupltype);
  } else {
    printf("     alpha:        %d\n",input->alpha);
  }
}

// small utility function to print the contents of a pixelMap
void reportPixelMap (RGBPixel *pm) {
  int index = 0;
  int fail_safe = 0;
  while(index < sizeof(pm) && fail_safe < 1000) {
    printf("rPM: [%d] = [%d,%d,%d]\n",index,pm[index].r,pm[index].g,pm[index].b);
    index++;
    fail_safe++;
  }
}

// meant to be used with skipWhitespace function that basically skips whitespace
int getNumber (int max_value, PPM_file_struct *input) {
  int tc_index = 0;
  int max_chars = 16;
  char tmp[max_chars];
  tmp[tc_index] = CURRENT_CHAR;
  
  do {
    if (VERBOSE) printf("gN: CURRENT_CHAR (%d)%c\n",tc_index,CURRENT_CHAR);
    CURRENT_CHAR = fgetc(input->fh_in);
    tmp[++tc_index] = CURRENT_CHAR;
    // fail safe
    if(tc_index > max_chars) {
      message("Error","File format error, more chars than expected without whitespace");
    }
  } while(CURRENT_CHAR != ' ' && CURRENT_CHAR != '\n' && CURRENT_CHAR != EOF) ;
  //  CURRENT_CHAR = fgetc(input->fh_in);
  
  // finish up and return converted value
  tmp[tc_index] = 0;
  PREV_CHAR = CURRENT_CHAR;
  int value = atoi(tmp);
  if (VERBOSE) printf("gN: returning %d\n",value);

  // error checking
  if(value > max_value || value < 0) {
    //    message("Error","Unsupported byte sizes in image data, 0 to 256 is supported");
    message("Error","Value out of range, possible causes:\n Only 1 byte per channel is supported, does your image have 2 bytes per channel?\n Maximum depth value is 4\n Maximum height/width are 8224 pixels\n Only 255 bit color is supported");
  }
  return value;
}

// helper function to get a string of chars before next whitespace and return them
// TODO: need to work out the "maxval" error check here like getNumber
char* getWord (PPM_file_struct *input) {
  int index = 0;
  int max_chars = 32; // large enough to deal with TUPLTYPE tokens
  //TODO: need to free this
  //  char tmp[max_chars];
  char *tmp = malloc(sizeof(char)*max_chars);
  //static char tmp[64]; <- works, but seems to be corruption, DEPTHT
  //char tmp[64]; <- core dumps
  //char *tmp;
  
  while(CURRENT_CHAR != ' ' && CURRENT_CHAR != '\n' && CURRENT_CHAR != EOF) {
    if (VERBOSE) printf("gW: CURRENT_CHAR (%d):%c\n",index,CURRENT_CHAR);
    PREV_CHAR = CURRENT_CHAR;
    CURRENT_CHAR = fgetc(input->fh_in);
    tmp[index++] = PREV_CHAR;
    // error check
    if(index > max_chars) {
      message("Error","File format error, more chars than expected without whitespace");
    }
  } 

  // finish up and return converted value
  tmp[++index] = 0; // NULL terminator

  if (VERBOSE) printf("gW: returning (%s), index:(%d), CC:c()\n",tmp,index,CURRENT_CHAR);

  return tmp;

}

// helper to advance the file handle to next non-whitespace char
void skipWhitespace (PPM_file_struct *input) {
  if (VERBOSE) printf("sS: skipping space...\n");
  while(CURRENT_CHAR == ' ' || CURRENT_CHAR == '\n') {
    CURRENT_CHAR = fgetc(input->fh_in);
  }
  PREV_CHAR = CURRENT_CHAR;
}

// helper function to skip the rest of the current line
void skipLine (PPM_file_struct *input) {
  if (VERBOSE) printf("sL: skipping line...\n");
  while(CURRENT_CHAR != '\n') {
    if (VERBOSE) printf("   skipping %c\n",CURRENT_CHAR);
    CURRENT_CHAR = fgetc(input->fh_in);
  }
  CURRENT_CHAR = fgetc(input->fh_in); // advance past the \n as getWord/Number are designed to work this way
  PREV_CHAR = CURRENT_CHAR;
}
