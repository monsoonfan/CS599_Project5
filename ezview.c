/*
CS599 Project 5 - ezview.c
R Mitchell Ralston
rmr5@nau.edu

Usage: ezview <input.ppm>

Implementation:
Take demo.c from Dr. P's starter kit, add my PPM reader, and adapt it to the project needs

Notes:

Issues:

Questions:
- do we need the whole starter kit in our repo? (no)
- approach for rendering something out of PPM? (texture map to a pair of triangles)
#include "ppmrw.h"
*/

// need these on windows
#define GLFW_DLL 1
#define GL_GLEXT_PROTOTYPES

#include <stdlib.h>
#include <stdio.h>
#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>
#include <test.h>

GLFWwindow* window;

typedef struct {
  float position[3];
  float color[4];
} Vertex;

// in general, OpenGL likes big blobs of memory like this. What it doesn't like is information
// where things are defined separately. So, creating classes/objects for everything isn't a great idea
// because you spend time flattening all of that information into one place for OGL
// these are the points of a square
const Vertex Vertices[] = {
  {{1, -1, 0}, {1, 0, 0, 1}},
  {{1, 1, 0}, {0, 1, 0, 1}},
  {{-1, 1, 0}, {0, 0, 1, 1}},
  {{-1, -1, 0}, {0, 0, 0, 1}}
};

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
  "\n"
  "varying vec4 DestinationColor;\n"
  "\n"
  "void main(void) {\n"
  "    DestinationColor = SourceColor;\n"
  "    gl_Position = Position;\n"
  "}\n";

// fragment shader code here
char* fragment_shader_src =
  "varying lowp vec4 DestinationColor;\n"
  "\n"
  "void main(void) {\n"
  "    gl_FragColor = DestinationColor;\n"
  "}\n";


// Need to write a function that will compile our shader from the strange text above
GLint simple_shader(GLint shader_type, char* shader_src) {

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
    GLchar message[256]; // or you could define as a pointer, not an array
    // by passing in 0 for the length of the string returned, we are telling OGL that 
    // we don't care, could create a variable and capture it if you want
    glGetShaderInfoLog(shader_id, sizeof(message), 0, &message[0]);
    printf("glCompileShader Error: %s\n", message);
    exit(1); // bail if not rendering correctly
  }

  return shader_id;
}


// compile the actual program
GLint simple_program() {
  // common OGL idioms
  GLint link_success = 0;
  GLint program_id = glCreateProgram();
  GLint vertex_shader = simple_shader(GL_VERTEX_SHADER, vertex_shader_src);
  GLint fragment_shader = simple_shader(GL_FRAGMENT_SHADER, fragment_shader_src);

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


int main(void) {

  GLint program_id, position_slot, color_slot;
  GLuint vertex_buffer;
  GLuint index_buffer;

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

  // test a function
  testFunc;

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
  program_id = simple_program();

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

  // Now bind the OGL buffer to the right kind of buffer: GL_ARRAY_BUFFER
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  // now send the buffer data to that bound buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

  // Now create the index buffer same as the vertices and bind it to the API
  glGenBuffers(1, &index_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

  // Now need to create an Event Loop
  // ask for input, display output, back and forth, super popular in any kind of UI, game, etc...
  while (!glfwWindowShouldClose(window)) {
    // this is where we draw stuff!

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
                          (GLvoid*) (sizeof(float) * 3));  // this is the offset to get to color from Vertex, not posision

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


//Main from PPMRW
/*
  if (argc != 4) {
    help();
    return(1);
  }
  
  // Get parameters from arguments with error checking
  OUTPUT_MAGIC_NUMBER = atoi(argv[1]);
  if (OUTPUT_MAGIC_NUMBER != 3 && OUTPUT_MAGIC_NUMBER != 6 && OUTPUT_MAGIC_NUMBER != 7) {
    printf("Error: File type not supported!\n");
    return EXIT_FAILURE;
  }
  char *infile = argv[2];
  char *outfile = argv[3];
  if (strcmp(infile,outfile)  == 0) {printf("Error: input and output file names the same!\n"); return EXIT_FAILURE;}
  printf("Info: Converting file to format %d ...\n",OUTPUT_MAGIC_NUMBER);
  printf("          Input : %s\n",infile);
  printf("          Output: %s\n",outfile);
  
  // Open the input file and traverse it, storing the image to buffer
  readPPM(infile,&INPUT_FILE_DATA);
  
  // Write the contents of the new file in desired format
  writePPM(outfile,&INPUT_FILE_DATA);

  // free all globally allocated memory
  freeGlobalMemory();
    
  // Successful exit was reached
  return EXIT_SUCCESS;
*/
