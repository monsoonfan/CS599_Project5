/*
CS599 Project 5 - ezview.c
R Mitchell Ralston
rmr5@nau.edu

Usage: ezview <input.ppm>

Implementation:
Take demo.c from Dr. P's starter kit, add my PPM reader and comments from when we worked
on it in class, and adapt it to the project needs

Notes/answers to questions:
- do we need the whole starter kit in our repo? (no)
- approach for rendering something out of PPM? (texture map to a pair of triangles)

Issues:
- couldn't get ppmrw included as a lib, syntax error "expression evaluates to missing function"
#include "ppmrw.h"
#include <test.h>

Questions:
- where do the indeces fit in to the picture?
- can't seem to modify any values in the Vertex array. Once this is solved, need to write
  the math functions to do the proper matrix multiply, currently not correct.
  * supposed to do this in GLSL, pass in the Vertexes, and it will make a copy in GPU mem and mod
*/

// need these on windows
#define GLFW_DLL 1
#define GL_GLEXT_PROTOTYPES

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3
#define cos45 0.525
#define sin45 0.851
#define cos90 -0.448
#define sin90 0.894

/*
  INCLUDES
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include "linmath2.h"
#include "GLFW/linmath.h"

#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>

/*
  VARIABLES and TYPEDEFS
 */
GLFWwindow* window;

typedef struct {
  float position[3];
  float color[4];
  float texcoord[2];
} Vertex;

// in general, OpenGL likes big blobs of memory like this. What it doesn't like is information
// where things are defined separately. So, creating classes/objects for everything isn't a great idea
// because you spend time flattening all of that information into one place for OGL
// these are the points of a square
const Vertex Vertices_Orig_noTex[] = {
  {{1, -1, 0}, {1, 0, 0, 1}},
  {{1, 1, 0}, {0, 1, 0, 1}},
  {{-1, 1, 0}, {0, 0, 0, 1}},
  {{-1, -1, 0}, {0, 0, 1, 1}}
};

const Vertex Vertices_Orig[] = {
  {{1, -1, 0}, {1, 0, 0, 1}, {15,0}},
  {{1, 1, 0}, {0, 1, 0, 1}, {15,25}},
  {{-1, 1, 0}, {0, 0, 0, 1}, {0,25}},
  {{-1, -1, 0}, {0, 0, 1, 1}, {0,0}}
};

float Vertices_Test[28] = {
  1, -1, 0, 1, 0, 0, 1,
  1, 1, 0, 0, 1, 0, 1,
  -1, 1, 0, 0, 0, 0, 1,
  -1, -1, 0, 0, 0, 1, 1
};

float Rotation_Matrix_X[16] = {
  // Assume 45 degree rotations about the X axis
  1.0,     0,      0,     0,
  0,     cos45, -sin45, 0,
  0,     sin45,  cos45, 0,
  0,     0,      0,     1
};

float Rotation_Matrix_Y[16] = {
  // Assume 90 degree rotations about the Y axis
  cos90, 0,      sin90, 0,
  0,     1,      0,     0,
  -sin90,0,      cos90, 0,
  0,     0,      0,     1
};
  
//Vertex *Vertices_Current = Vertices_Orig;
float *Vertices_Current = Vertices_Test;

// this is basically an unsigned char. OpenGL defines these constructs and then maps them 
// for you based on the platform so the idea is that it's platform independent for you
//
// Need to order the vertices into some consistent "winding order"
//
// the downside to this approach is that we have no warranty that these have the same
// winding order, that they form as expected.
// "polygon soup"
const GLubyte Indices[] = {
  // these defines two different triangles
  0, 1, 2,
  2, 3, 0
};

// vertex shader code here
char* vertex_shader_src =
  "attribute vec4 Position;\n"
  "attribute vec4 SourceColor;\n"
  "attribute vec2 TexCoordIn;\n"
  "varying vec2 TexCoordOut;\n"
  "\n"
  "varying vec4 DestinationColor;\n"
  "\n"
  "void main(void) {\n"
  "    DestinationColor = SourceColor;\n"
  "    TexCoordOut = TexCoordIn;\n"
  "    gl_Position = Position;\n"
  "}\n";

// fragment shader code here
char* fragment_shader_src =
  "varying lowp vec4 DestinationColor;\n"
  "varying lowp vec2 TexCoordOut;\n"
  "uniform sampler2D Texture;\n"
  "\n"
  "void main(void) {\n"
  "    gl_FragColor = DestinationColor;\n"
  "    //gl_FragColor = texture2D(Texture, TexCoordOut);\n"
  "}\n";

//--------------------------------------------------------------------------
/*
  PPMRW
 */
//--------------------------------------------------------------------------
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
  FILE* fh_in;
} PPM_file_struct ;

// global variables
int CURRENT_CHAR        = 'a';
int PREV_CHAR           = '\n';
int VERBOSE             = 0; // controls logfile message level

// global data structures
PPM_file_struct     INPUT_FILE_DATA;
RGBPixel           *RGB_PIXEL_MAP;
RGBAPixel          *RGBA_PIXEL_MAP;

//--------------------------------------------------------------------------
/*
  FUNCTIONS
 */
//--------------------------------------------------------------------------
// PPM functions
int   readPPM         (char *infile,          PPM_file_struct *input);
void  message         (char message_code[],   char message[]        );
int   getNumber       (int max_value,         PPM_file_struct *input);
char* getWord         (PPM_file_struct *input);
void  reportPPMStruct (PPM_file_struct *input);
void  reportPixelMap  (RGBPixel *pm          );
void  skipWhitespace  (PPM_file_struct *input);
void  skipLine        (PPM_file_struct *input);
void  help            ();
int   computeDepth();
char* computeTuplType();
void  freeGlobalMemory ();
void  closeAndExit ();

// OpenGL functions
void keyHandler (GLFWwindow *window, int key, int code, int action, int mods);
GLint simpleShader(GLint shader_type, char* shader_src);
GLint simpleProgram();
static void error_callback(int error, const char* description);

// Image processing functions
void scaleImage(float scale_amount, Vertex *input_vertices);
void invertColors(Vertex *input_vertices);
void rotateImage(char axis);
void origImage();

// Misc inline functions
static inline int fileExist(char *filename) {
  struct stat st;
  int result = stat(filename, &st);
  return result == 0;
}

//--------------------------------------------------------------------------
/*
  MAIN
 */
//--------------------------------------------------------------------------
int main(int argc, char *argv[]) {

  // Process the input texture map file
  if (argc != 2) {
    help();
    return EXIT_FAILURE;
  }
  char *infile = argv[1];
  if (!fileExist(infile)) {
    fprintf(stderr,"Error: Input file \"%s\" does not exist!\n",infile);
    return EXIT_FAILURE;
   }
  
  printf("Loading texture map input: %s\n",infile);
  
  // Open the input file and traverse it, storing the image to buffer
  readPPM(infile,&INPUT_FILE_DATA);
  
  // Prepare the OGL environment
  GLint program_id, position_slot, color_slot, tex_coord_slot, texture_uniform;
  GLuint vertex_buffer;
  GLuint index_buffer;
  GLuint texture_name;

  glfwSetErrorCallback(error_callback);

  // Initialize GLFW library
  if (!glfwInit()) return -1; // has the effect of exiting if problem with GLFW

  // misnomer, these aren't hints, actually required
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);  // different per OS
  // leave this in only if downloading new API from DrP
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  // Now create the actual window
  window = glfwCreateWindow(640,
                            480,
                            "ezview",
                            NULL,
                            NULL);

  // after you create anything, make sure it really got created, will have value 0 if not
  // clean up if it didn't happen and bail
  if (!window) {
    glfwTerminate();
    printf("glfwCreateWindow Error\n");
    exit(1);
  }

  // Now need to map the GLFW context onto that window
  // this is now the current context and we are going to start working on it
  glfwMakeContextCurrent(window);

  // WARNING: Do NOT do openGL calls before the window is set up!! OGL expects that canvas
  // and if it's not there, it will cause problems
  program_id = simpleProgram();

  // tell it we want to use the program we just created (which has the shaders)
  // remember that OGL is essentially a state machine, so this will change the state
  glUseProgram(program_id);

  // get the attribute from the program and look for a variable called Position
  // this is not a memory position like C, this is OGL so it's an index
  position_slot = glGetAttribLocation(program_id, "Position");
  color_slot = glGetAttribLocation(program_id, "SourceColor");
  // next need to use a vertex array to pass information into slots, tell OGL to expect that
  glEnableVertexAttribArray(position_slot);
  glEnableVertexAttribArray(color_slot);

  // create a single buffer for the vertex/index buffers
  glGenBuffers(1, &vertex_buffer);
  //glfwSwapInterval	(	int 	interval	)	

  // Now bind the OGL buffer to the right kind of buffer: GL_ARRAY_BUFFER
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  // now send the buffer data to that bound buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices_Orig), Vertices_Orig, GL_STATIC_DRAW);

  // Now create the index buffer same as the vertices and bind it to the API
  glGenBuffers(1, &index_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

  // Texturing
  glGenTextures(1,&RGB_PIXEL_MAP);
  glBindTexture(GL_TEXTURE_2D, RGB_PIXEL_MAP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D,
	       0,
	       GL_RGBA,
	       INPUT_FILE_DATA.width,
	       INPUT_FILE_DATA.height, 
	       0,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       &RGB_PIXEL_MAP
	       );
  tex_coord_slot = glGetAttribLocation(program_id, "TexCoordIn");
  glEnableVertexAttribArray(tex_coord_slot);
  texture_uniform = glGetUniformLocation(program_id, "Texture");
  glVertexAttribPointer(tex_coord_slot, 2, GL_FLOAT, GL_FALSE,
			sizeof(Vertex),
			sizeof(float)*7
			);
  glActiveTexture(GL_TEXTURE);
  glBindTexture(GL_TEXTURE_2D, texture_name);
  glUniform1i(texture_uniform, 0);
  
  // Now need to create an Event Loop
  // ask for input, display output, back and forth, super popular in any kind of UI, game, etc...
  while (!glfwWindowShouldClose(window)) {
    // handle keyboard events
    glfwSetKeyCallback(window, keyHandler);

    // this has been std since like OGL 1.0
    glClearColor(0, 104.0/255.0, 55.0/255.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // next, set the viewport to match our window. This could be outside the while
    // if it's never changing
    glViewport(0, 0, 640, 480);

    // before we draw, have to tell it what elements to draw
    glVertexAttribPointer(position_slot,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex),
                          0);

    // do same thing to color slot
    glVertexAttribPointer(color_slot,
                          4,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex),
			  // this is the offset to get to color from Vertex, not posision
                          (GLvoid*) (sizeof(float) * 3));
    
    // finally we can draw the stuff!
    glDrawElements(GL_TRIANGLES,
                   sizeof(Indices) / sizeof(GLubyte),
                   GL_UNSIGNED_BYTE, 0);

    glfwSwapBuffers(window);
    glfwPollEvents(); // poll for events that might have happened while we were drawing
  }

  // finalization code after window is closed
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------
/*
  FUNCTION IMPLEMENTATIONS
 */
//--------------------------------------------------------------------------
// Keyboard input - need to handle:
// - translate (pan)
// - rotate
// - scale
// - shear
// - also things like exit
void keyHandler (GLFWwindow *window, int key, int code, int action, int mods) {
  switch(action) {
  // do nothing but notify on a press
  case GLFW_PRESS:
    printf("key press detected...",key);
    break;
  // Action is taken upon release of key
  case GLFW_RELEASE:

    switch(key) {
    case(265): // up arrow = pan up
      printf("(up arrow): panning up...\n");
      invertColors(Vertices_Orig);
      break;
    case(262): // right arrow = pan right
      printf("(right arrow): panning right...\n");
      break;
    case(263): // left arrow = pan left
      printf("(left arrow): panning left...\n");
      break;
    case(264): // down arrow = pan down
      printf("(down arrow): panning down...\n");
      break;
    case(82): // r = rotate
      printf("(r): rotating clockwise...\n");
      //rotateImage('X');
      rotateImage('Y');
      reportPPMStruct(&INPUT_FILE_DATA);
      break;
    case(83): // s = shear
      origImage();
      printf("(s): shear applied to top of image...\n");
      break;
    case(73): // i = scale up
      printf("(i): scaling up...\n");
      scaleImage(2.0,Vertices_Orig);
      break;
    case(79): // o = scale down
      printf("(o): scaling down...\n");
      scaleImage(0.5,Vertices_Orig);
      break;
    case(69): // e = exit application
      printf("(e): Exiting...\n");
      glfwTerminate();
      exit(1);
      break;
    }
    break;
  }
}

// Need to write a function that will compile our shader from the strange text above
GLint simpleShader(GLint shader_type, char* shader_src) {

  GLint compile_success = 0;

  // open GL gives back an integer number, not an address/pointer like malloc
  GLint shader_id = glCreateShader(shader_type);

  // open GL is a state machine, actions change it
  // set up the shader source
  glShaderSource(shader_id, 1, &shader_src, 0);

  // compile the source
  glCompileShader(shader_id);

  // have to check the error code to see if this compiled correctly or not. You have to be
  // vigilant about checking these error codes and printing out feedback because OGL will not do it
  // this is how you share a variable from one scope to another inside C program, address-of operator
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_success);

  // some of the best programmers are graphics programmers, because the API is difficult.
  // The API is that way for stability over time. Vulkan is even more difficult.
  //
  // look at documentation on Khronos website for info

  // good idea to create an error function
  if (compile_success == GL_FALSE) {
    // could also define as a pointer here, not an array
    // by passing in 0 for the length of the string returned, we are telling OGL that 
    // we don't care, could create a variable and capture it if you want
    GLchar message[256];
    glGetShaderInfoLog(shader_id, sizeof(message), 0, &message[0]);
    printf("glCompileShader Error: %s\n", message);
    exit(1); // bail if not rendering correctly
  }

  return shader_id;
}

// compile the actual program
GLint simpleProgram() {
  // common OGL idioms
  GLint link_success = 0;
  GLint program_id = glCreateProgram();
  GLint vertex_shader = simpleShader(GL_VERTEX_SHADER, vertex_shader_src);
  GLint fragment_shader = simpleShader(GL_FRAGMENT_SHADER, fragment_shader_src);

  // attach the shaders to the program
  glAttachShader(program_id, vertex_shader);
  glAttachShader(program_id, fragment_shader);

  // now do the explicit linking, remember the diagram of the two types of shaders together,
  // that's our program
  glLinkProgram(program_id);

  // now check for proper linking
  glGetProgramiv(program_id, GL_LINK_STATUS, &link_success);

  if (link_success == GL_FALSE) {
    GLchar message[256];
    glGetProgramInfoLog(program_id, sizeof(message), 0, &message[0]);
    printf("glLinkProgram Error: %s\n", message);
    exit(1);
  }

  return program_id;
}

// need to print out any errors whenever there is one thrown or you'll never figure them out
// callback design pattern, similar to listener design pattern (C doesn't have first
// class functions, so no closure)
static void error_callback(int error, const char* description) {
  fputs(description, stderr);
}

// PPMRW below
//------------
/*
---------------------------------------------------------------------------------------
ppmrw - from CS599 Project 1
R Mitchell Ralston (rmr5)
10/6/16
---------------------------------------------------------------------------------------
*/
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
  message("Usage","ezview texture.ppm");
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
  char tmp[16];
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


//-------------------------------------------------------------------
// Image processing functions
//-------------------------------------------------------------------
void scaleImage(float scale_amount, Vertex *input_vertices) {
  // vars
  //  Vertex *new_vertices = input_vertices;
  /*
  Vertex new_vertices[] = {
    {{1, 1, 0}, {1, 0, 0, 1}},
    {{-1, 1, 0}, {0, 1, 0, 1}},
    {{-1, -1, 0}, {0, 0, 0, 1}},
    {{1, -1, 0}, {0, 0, 1, 1}}
  };
  */
  
  // code body
  printf("Inside scaleImage at %f\n",scale_amount);
  //  const Vertex Vertices[] = {
  Vertex new_vertices[] = {
    {{1, -1, 0}, {1, 0, 0, 1}},
    {{1, 1, 0}, {0, 1, 0, 1}},
    {{-1, 1, 0}, {0, 0, 0, 1}},
    {{-1, -1, 0}, {0, 0, 1, 1}}
  };

  //  Vertices_Current = Vertices_Orig;
  Vertices_Current = new_vertices;
  //  Vertices_Current[0].color[0] = 0.5;
  //  Vertices_Current[1].color[2] = 0.75;
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices_Orig), Vertices_Current, GL_STATIC_DRAW);
  //glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices_Orig), new_vertices, GL_STATIC_DRAW);
}

void invertColors(Vertex *input_vertices) {
  // vars
  printf("Inverting image colors\n");
  const Vertex VerticesNot[] = {
    {{1, -1, 0}, {0, 1, 1, 1}},
    {{1, 1, 0}, {1, 0, 1, 1}},
    {{-1, 1, 0}, {1, 1, 1, 1}},
    {{-1, -1, 0}, {1, 1, 0, 1}}
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices_Orig), VerticesNot, GL_STATIC_DRAW);

}

// Rotate the image by way of vertices
void rotateImage(char axis) {
  if (axis == 'X') {
    message("Info","Rotating 45 degrees about the X axis..\n");
    //Vertices_Current[0].position[0] = Vertices_Current[0].position[0] * Rotation_Matrix_X[0];
    /*
    Vertex new_vertices[] = {
      {{1, 0, 0}, {1, 0, 0, 1}},
      {{0, cos45, -sin45}, {0, 1, 0, 1}},
      {{0, sin45, cos45}, {0, 0, 0, 1}},
      {{0,0,0}, {0, 0, 1, 1}}
    };
    */
    
    int checkval = 0;
    for (int iv = 0; iv < 28; iv++) {
      //Vertex value = Vertices_Test[iv];
      //printf("Found value[%d]: %f\n",iv,value);
      if (checkval < 3) {
	//Vertices_Test[iv] = Rotation_Matrix_X[iv];
	//Vertices_Current[0].position[0] = value;
      }
      //printf("Now it's %f from %f\n",Vertices_Test[iv],value);
      checkval++;
      if (checkval == 3) checkval = 0;
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices_Orig), Vertices_Test, GL_STATIC_DRAW);
  } else if (axis == 'Y') {
    //glTranslate(-1.5f,0.0f,0.0f);
    message("Info","Rotating 90 degrees about the Y axis..\n");
  } else {
    message("Error","Unrecognized rotation!");
  }
}

// Misc function to redraw the image to the original constant vertices
void origImage() {
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices_Orig), Vertices_Orig, GL_STATIC_DRAW);
}  

/*
Notes from 12/1/16
Ditch the color, we don't need it since we are loading color from texture
 the texdemo.c has gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n // casting to affine, vPos is vec2

he created texture on the fly, Gimp will actually export C code to create the image!!

this helper is just a wrapper around the glCompileShader function
void glCompileShaderOrDie(GLuint shader) {

 */
