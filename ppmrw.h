/*
---------------------------------------------------------------------------------------
ppmrw - from CS599 Project 1
R Mitchell Ralston (rmr5)
10/6/16
---------------------------------------------------------------------------------------
*/
// libraries
// functions
int   readPPM         (char* infile,          PPM_file_struct* input);
void  writePPM        (char* outfile,         PPM_file_struct* input);
void  message         (char message_code[],   char message[]        );
int   getNumber       (int max_value,         PPM_file_struct* input);
char* getWord         (PPM_file_struct* input);
void  reportPPMStruct (PPM_file_struct* input);
void  reportPixelMap  (RGBPixel* pm          );
void  skipWhitespace  (PPM_file_struct* input);
void  skipLine        (PPM_file_struct* input);
void  writePPMHeader  (FILE* fh              );

void  help            ();
int   computeDepth();
char* computeTuplType();
void  freeGlobalMemory ();
void  closeAndExit ();
