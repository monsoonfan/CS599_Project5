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
- he created texture on the fly, Gimp will actually export C code to create the image!!

Issues:
- stuck on tweening function
  - will work in one direction, but not immediatly back in the other until the absolute
    value works itself out again. Rotate right twice tween will work, rotate left 3 times
    the tween will work on the 3rd time
  - also, shear is causing the rotation animation to happen
- P3/P6 data is rotated by 90 degrees, actually so is "image"
- Large P3 data is only partially shown (test15 example)
- couldn't get ppmrw included as a lib, syntax error "expression evaluates to missing function"

Questions:
*/

// need these on windows
#define GLFW_DLL 1
#define GL_GLEXT_PROTOTYPES

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3
#define PI 3.1415

/*
  INCLUDES
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>
#include "linmath.h"

/*
  VARIABLES and TYPEDEFS
 */
GLFWwindow* window;
//int WIDTH = 640;
//int HEIGHT = 480;
int WIDTH = 1000;
int HEIGHT = 800;
float ROTATION_AMOUNT = 0;
float ROTATION_FACTOR;
float FINAL_ROTATION;
float SCALE_AMOUNT = 1;
float TRANSLATION_X = 0;
float TRANSLATION_Y = 0;
float SHEAR_X = 0;
float SHEAR_Y = 0;

typedef struct {
  float position[3];
  float texcoord[2];
} Vertex;

// in general, OpenGL likes big blobs of memory like this. What it doesn't like
// is information where things are defined separately. So, creating classes/objects
// for everything isn't a great idea because you spend time flattening all of that
// information into one place for OGL these are the points of a square
//
// Need to order the vertices into some consistent "winding order"
Vertex Vertexes[] = {
  {{1, -1}, {0.99999, 0.99999}},
  {{1, 1},  {0.99999, 0}},
  {{-1, 1}, {0, 0}},
  {{-1, 1}, {0, 0}},
  {{-1, -1}, {0, 0.99999}},
  {{1, -1}, {0.99999, 0.99999}}
};

// vertex shader code here
char* vertex_shader_src =
  "uniform mat4 MVP;\n"
  "attribute vec2 Position;\n"
  "attribute vec2 TexCoordIn;\n"
  "varying lowp vec2 TexCoordOut;\n"
  "\n"
  "\n"
  "void main(void) {\n"
  "    TexCoordOut = TexCoordIn;\n"
  "    gl_Position = MVP * vec4(Position, 0.0, 1.0);\n"
  "}\n";

// fragment shader code here
char* fragment_shader_src =
  "varying lowp vec2 TexCoordOut;\n"
  "uniform sampler2D Texture;\n"
  "\n"
  "void main(void) {\n"
  "    gl_FragColor = texture2D(Texture, TexCoordOut);\n"
  "}\n";

//--------------------------------------------------------------------------
/*
  PPMRW
 */
//--------------------------------------------------------------------------
// typdefs
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
void  skipWhitespace  (PPM_file_struct *input);
void  skipLine        (PPM_file_struct *input);
void  help            ();
int   computeDepth();
char* computeTuplType();
void  freeGlobalMemory ();
void  closeAndExit ();

// OpenGL functions
void keyHandler (GLFWwindow *window, int key, int code, int action, int mods);
GLuint simpleShader(GLint shader_type, char* shader_src);
GLuint simpleProgram();
static void error_callback(int error, const char* description);

// Image processing functions
void rotateImage(float rotation_factor);
void scaleImage(float scale_factor);
void translateImage(int pan_direction, float pan_factor);
void shearImage(int shear_direction, float shear_factor);
float tweenFunction(int rate);
float linearTween (int t, float b, float c, int d);

// Misc inline functions
static inline int fileExist(char *filename) {
  struct stat st;
  int result = stat(filename, &st);
  return result == 0;
}

// from example in class on 12/1
void glCompileShaderOrDie(GLuint shader) {
  GLint compiled;
  glCompileShader(shader);
  glGetShaderiv(shader,
		GL_COMPILE_STATUS,
		&compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader,
		  GL_INFO_LOG_LENGTH,
		  &infoLen);
    char* info = malloc(infoLen+1);
    GLint done;
    glGetShaderInfoLog(shader, infoLen, &done, info);
    printf("gCSOD: Unable to compile shader: %s\n", info);
    exit(1);
  }
}

// 4 x 4 image..
int image_width = 4;
int image_height = 4;
unsigned char image[] = {
  255, 0, 0, 255,
  255, 0, 0, 255,
  255, 0, 0, 255,
  255, 0, 0, 255,

  0, 255, 0, 255,
  0, 255, 0, 255,
  0, 255, 0, 255,
  0, 255, 0, 255,

  0, 0, 255, 255,
  0, 0, 255, 255,
  0, 0, 255, 255,
  0, 0, 255, 255,

  255, 0, 255, 255,
  255, 0, 255, 255,
  255, 0, 255, 255,
  255, 0, 255, 255
};

// image from pixmap
unsigned char *pixmap;


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
  GLint program_id, position_slot, color_slot, tex_coord_slot;
  GLuint vertex_buffer, index_buffer;
  GLint mvp_location, vpos_location, vcol_location;

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

  // Now create the actual window>
  window = glfwCreateWindow(WIDTH,
                            HEIGHT,
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
  glfwSwapInterval(1);

  // WARNING: Do NOT do openGL calls before the window is set up!! OGL expects that canvas
  // and if it's not there, it will cause problems
  program_id = simpleProgram();

  // tell it we want to use the program we just created (which has the shaders)
  // remember that OGL is essentially a state machine, so this will change the state
  glUseProgram(program_id);

  // create a single buffer for the vertex/index buffers
  glGenBuffers(1, &vertex_buffer);

  // Now bind the OGL buffer to the right kind of buffer: GL_ARRAY_BUFFER
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  // now send the buffer data to that bound buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertexes), Vertexes, GL_STATIC_DRAW);

  // Now create the index buffer same as the vertices and bind it to the API
  glGenBuffers(1, &index_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
  
  // Texturing
  mvp_location = glGetUniformLocation(program_id, "MVP");
  assert(mvp_location != -1);
  
  // get the attribute from the program and look for a variable called Position
  // this is not a memory position like C, this is OGL so it's an index
  vpos_location = glGetAttribLocation(program_id, "Position");
  assert(vpos_location != -1);
  
  GLint texcoord_location = glGetAttribLocation(program_id, "TexCoordIn");
  assert(texcoord_location != -1);
  
  GLint tex_location = glGetUniformLocation(program_id, "Texture");
  assert(tex_location != -1);
  
  // next need to use a vertex array to pass information into slots, tell OGL to expect that
  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Vertex),
			(void*) 0);
  
  glEnableVertexAttribArray(texcoord_location);
  glVertexAttribPointer(texcoord_location,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Vertex),
			(void*) (sizeof(float) * 2));
  // set up the texture
  GLuint texID;
  glGenTextures(1, &texID);
  glBindTexture(GL_TEXTURE_2D, texID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // apply the texture
  //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
  glTexImage2D(
	       GL_TEXTURE_2D,
	       0,
	       GL_RGB,
	       INPUT_FILE_DATA.width,
	       INPUT_FILE_DATA.height,
	       0,
	       GL_RGB,
	       GL_UNSIGNED_BYTE,
	       pixmap
	       );
  
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texID);
  glUniform1i(tex_location, 0);
  
  // Now need to create an Event Loop
  // ask for input, display output, back and forth, super popular in any kind of UI, game, etc...
  while (!glfwWindowShouldClose(window)) {
    // variables
    mat4x4 rot_matrix; // store rotation
    mat4x4 scl_matrix; // store scale
    mat4x4 trn_matrix; // store translation
    mat4x4 shr_matrix; // store shear
    mat4x4 rotXshr_matrix; // product of rotation and shear
    mat4x4 rotXshrXscl_m;  // product or rotation, shear, and scale
    mat4x4 mvp;            // all affines applied
    
    // handle keyboard events
    glfwSetKeyCallback(window, keyHandler);
    
    // this has been std since like OGL 1.0
    //    glClearColor(0, 104.0/255.0, 55.0/255.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // next, set the viewport to match our window. This could be outside the while
    // if it's never changing
    glViewport(0, 0, WIDTH, HEIGHT);

    // TODO remove
    float time = (float)glfwGetTime();
    //printf("time: %f\n",time);

    /*
      ROTATION
      --------
      implment the rotation, the final rotation amount comes from rotateImage(), but
      the animation/tweening must be applied here in the 'while'
    */
    mat4x4_identity(rot_matrix);
    float orig_rotation = ROTATION_AMOUNT;
    //float final_rotation = orig_rotation + (ROTATION_FACTOR * PI / 180);
    float increment = 0.001;
    // Animation
    //    printf("DBG or_i: %f, RA: %f, RF: %f, fr: %f\n",orig_rotation, ROTATION_AMOUNT, ROTATION_FACTOR, FINAL_ROTATION);
    while( fabsf(ROTATION_AMOUNT) < fabsf(FINAL_ROTATION) ) {
      ROTATION_AMOUNT += (increment * ROTATION_FACTOR) * PI / 180;
      //      printf("DBG or: %f, RA: %f, fr: %f\n",orig_rotation, ROTATION_AMOUNT, FINAL_ROTATION);
      //mat4x4_rotate_Z(rot_matrix, rot_matrix, ROTATION_AMOUNT);
      mat4x4_rotate_Z(rot_matrix, rot_matrix, (float)glfwGetTime()*ROTATION_AMOUNT);
    }
    // stop at final rotation
    ROTATION_AMOUNT = FINAL_ROTATION;
    mat4x4_rotate_Z(rot_matrix, rot_matrix, ROTATION_AMOUNT);
    
    // set up scale
    mat4x4_identity(scl_matrix);
    scl_matrix[0][0] = scl_matrix[0][0]*SCALE_AMOUNT;
    scl_matrix[1][1] = scl_matrix[1][1]*SCALE_AMOUNT;

    // set up translation
    mat4x4_identity(trn_matrix);
    mat4x4_translate(trn_matrix, TRANSLATION_X, TRANSLATION_Y, 0);
    
    // set up shear
    mat4x4_identity(shr_matrix);
    shr_matrix[0][1] = SHEAR_X;
    shr_matrix[1][0] = SHEAR_Y;
	
    // implement the affine transformations
    mat4x4_mul(rotXshr_matrix, rot_matrix, shr_matrix);
    mat4x4_mul(rotXshrXscl_m, rotXshr_matrix, scl_matrix);
    mat4x4_mul(mvp, rotXshrXscl_m, trn_matrix);
    
    // finally we can draw the stuff!
    glUseProgram(program_id);
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
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
    case(GLFW_KEY_UP): // up arrow = pan up
      printf("(up arrow): panning up...\n");
      translateImage(UP, 0.25);
      break;
    case(GLFW_KEY_RIGHT): // right arrow = pan right
      printf("(right arrow): panning right...\n");
      translateImage(RIGHT, 0.25);
      break;
    case(GLFW_KEY_LEFT): // left arrow = pan left
      printf("(left arrow): panning left...\n");
      translateImage(LEFT, 0.25);
      break;
    case(GLFW_KEY_DOWN): // down arrow = pan down
      printf("(down arrow): panning down...\n");
      translateImage(DOWN, 0.25);
      break;
    case(GLFW_KEY_R): // r = rotate clock
      printf("(r): rotating clockwise...\n");
      rotateImage(-90);
      break;
    case(GLFW_KEY_C): // c = rotate counter
      printf("(c): rotating counter-clockwise...\n");
      rotateImage(90);
      break;
    case(GLFW_KEY_S): // s = shear top to right
      printf("(s): shear applied at top of image to right...\n");
      shearImage(RIGHT, 0.15);
      break;
    case(GLFW_KEY_D): // d = shear top to left
      printf("(d): shear applied at top of image to left...\n");
      shearImage(LEFT, 0.15);
      break;
    case(GLFW_KEY_A): // a = shear right upward
      printf("(a): shear applied at right of image upward...\n");
      shearImage(UP, 0.15);
      break;
    case(GLFW_KEY_F): // f = shear right downward
      printf("(f): shear applied at right of image downward...\n");
      shearImage(DOWN, 0.15);
      break;
    case(GLFW_KEY_I): // i = scale up (zoom in)
      printf("(i): scaling up...\n");
      scaleImage(2.0);
      break;
    case(GLFW_KEY_O): // o = scale down (zoom out)
      printf("(o): scaling down...\n");
      scaleImage(0.5);
      break;
    case(GLFW_KEY_E): // e = exit application
      printf("(e): Exiting...\n");
      glfwTerminate();
      exit(1);
      break;
    case(GLFW_KEY_ESCAPE): // e = exit application
      printf("(e): Exiting...\n");
      glfwTerminate();
      exit(1);
      break;
    }
    break;
  }
}

// Need to write a function that will compile our shader from the strange text above
GLuint simpleShader(GLint shader_type, char* shader_src) {

  GLint compile_success = 0;

  // open GL gives back an integer number, not an address/pointer like malloc
  GLuint shader_id = glCreateShader(shader_type);

  // open GL is a state machine, actions change it
  // set up the shader source
  glShaderSource(shader_id, 1, &shader_src, 0);

  // compile the source
  glCompileShaderOrDie(shader_id);

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
GLuint simpleProgram() {
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
  free(pixmap);
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
  int total_pixels = (input->width) * (input->height) * 3;
  int number_count = 0;
  int pm_index = 0;

  // allocate the image buffer memory
  pixmap = malloc(sizeof(unsigned char *) * total_pixels);
  
  // This switch handles parsing the various input file formats for the image data
  switch(input->magic_number) {
  case(3):
    message("Info","  format version: 3<<");
    //    while(PREV_CHAR != EOF && number_count < total_pixels) {
    while(PREV_CHAR != EOF) {
      skipWhitespace(input);
      unsigned char value = getNumber(255,input);
      //INPUT_FILE_DATA->buffer[pm_index] = value;
      pixmap[pm_index] = value;
      //      printf("DBG: stored[%d][%d] %d to pixmap\n",pm_index,value,pixmap[pm_index]);
      pm_index++;
      number_count++;
    }
    printf("read %d numbers\n",number_count);
    message("Info","Done reading PPM 3");
    break; // magic number case(3)
  case(6):
    message("Info","  format version: 6");
    int err = 0;
    // read the whole image in one whack
    number_count = fread(pixmap, 1, total_pixels, input->fh_in);
    printf("Info: read %d bytes\n",number_count);
    if (err) {message("Error","Binary data read error");}    
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
// Rotate - this happens by changing two global vars and letting the animation function
// in the main while loop take care of the animation
void rotateImage(float rotation_factor) {
  printf("Info: Rotating %f degrees about the X axis..\n", -rotation_factor);
  float orig_rotation = ROTATION_AMOUNT;

  ROTATION_FACTOR = rotation_factor;
  FINAL_ROTATION = orig_rotation + (ROTATION_FACTOR * PI / 180);
}

void scaleImage(float scale_factor) {
  printf("Zoom by %f\n",scale_factor);
  SCALE_AMOUNT *= scale_factor;
}

void translateImage(int pan_direction, float pan_factor) {
  // decide the direction
  /*#define UP 0
    #define DOWN 1
    #define LEFT 2
    #define RIGHT 3
  */
  switch(pan_direction) {
  case(0):
    TRANSLATION_Y += pan_factor;
    break;
  case(1):
    TRANSLATION_Y -= pan_factor;
    break;
  case(2):
    TRANSLATION_X -= pan_factor;
    break;
  case(3):
    TRANSLATION_X += pan_factor;
    break;
  }
}

void shearImage(int shear_direction, float shear_factor) {
  // same directions as translate
  switch(shear_direction) {
  case(0):
    ROTATION_AMOUNT = 0;
    SHEAR_X -= shear_factor;
    break;
  case(1):
    ROTATION_AMOUNT = 0;
    SHEAR_X += shear_factor;
    break;
  case(2):
    ROTATION_AMOUNT = 0;
    SHEAR_Y -= shear_factor;
    break;
  case(3):
    ROTATION_AMOUNT = 0;
    SHEAR_Y += shear_factor;
    break;
  }
}

// Simple tweening function, returns a number between 0 and 1 as time goes by
// starts at 0, ends at 1
float tweenFunction(int rate) {
  float rval = 0.1;

  return rval;
}

/*
t = current time
b = start value
c = change in value
d = duration
t/d can be frames or milliseconds/seconds
*/
float linearTween (int t, float b, float c, int d){
  //printf("DBG t: %d, b: %f, c: %f, d: %d\n",t,b,c,d);
  return c*t/d + b;
}
