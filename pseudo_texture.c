// PSEUDO of texturing

// step 0 - hard code all vertices and texture coords and colors, so these do not come from image
//          sit down with graph paper and map the sqaure
//          could re-use example
//
// so we are painting 2 trangles on the screen
// our image is the texture map, it will texture them
// using uniform, we are pulling in the texture data so the fragment shader pulls in the
// image data
// have to manipulate the vertex coordinates such that the cat's aspect ratio does not get
// messed up
//
//
// step 1 - load PPM
// step 2 - openGL setup, GLFW setup
// 
// step 3 - Transformations in GLSL code





// Vertex shader modifications...
// 1) pass in texture coords
attribute vec2 TexCoordIn; // 2 element tuple, floating points
varying vec2 TexCoordOut; // output of vertex shader, input to fragment shader
// other stuff like last example, above is just new stuff...

// shader main
void main(void) {
  // Our code to do stuff...
  // specifically, transformations, (scaling, rotation, etc...)
  // we'll have to do research to figure out how to do the math to do operations
  // will proably have to do OpenGL tutorials, this is like 1st/2nd thing to do

  // will also need to pass along the texture coords
  TexCoordOut = TexCoordIn;
}

// Also need Fragment shader mods...
varying lowp vec2 TexCoordOut;
// need uniform in order to map in texture data into program
uniform sampler2D Texture;
// other stuff like last example...

// fragment shader main
void main(void) {
  // other stuff, will need to modify from example as well
  // new stuff
  // this function uses TeXCoord to look up correct color in the texture (the linear mapping
  // from notes). Done by interpolation between the 2 shaders
  gl_FragColor = texture2D(Texture, TexCoordOut);
}



// still alot that has to happen on the C side of things
typedef struct {
  float position[3];
  float color[4];
  // now we add one more thing, texture coordinates
  // we do this because need to find out how 
  float texcoord[2];
} Vertex;

Vertex vertexes[] = {
  {{1,-1,0}, {0, 0, 0, 1}, {1, 0}},
  // keep doing this
  // we will have to construct all of these vertices so we can actually display
}
  
// texture setup also happens somewhere in the code
  GLuint myTexture;
glGenTextures(1,&myTexture);
glBindTexture(GL_TEXTURE_2D, myTexture); // fancy way of saying we want to use the texture
// set up integer parameter, does some mapping from low res textures to higher res images
// "mit-mapping"?? trying to load small amount of a large texture when image is small (far away)
// used "space-alien" example, when far away, use small texture, when close, use large texture,
// can interpolate between 2 detail levels for other distances
glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // nearest neighbor algo
// google this for options (opengl.org)
glTexImage2D(GL_TEXTURE_2D,
	     0, // no Level of detail
	     GL_RGBA, // depends on the format of your PPM image, GL_RGB if no alpha (internal)
	     width, // got this from PPM header
	     height, 
	     0, // no borders for us
	     GL_RGBA, // other format code (external, how we loaded data)
	     GL_UNSIGNED_BYTE, // what kind of data we used in order to represent GL_RGBA (float, double, unsigned char, he used byte but I think I need char
	     &our_image_data // I assume we use address of, may not be
	     );
	     
// Still more we have to do, when we compile shaders have to hook up texture coord slots so
// it matches up with the texture that we loaded
// probably this bit of code goes before linking the shaders, during compiling them
// something like this...
texCoordSlot = glGetAttributeLocation(programID, "TexCoordIn");
glEnableVertexAttribArray(texCoordSlot);
// set up the uniform slot we'll use to connect to 2D
textureUniform = glGetUniformLocation(programID, "Texture");


// Before glDrawElements
// use this to load in texture coords, instead of color like before
glVertexAttribPointer(texCoordSlot, 2, GL_FLOAT, GL_FALSE,
// "stride" of one element to the next
		      sizeof(Vertex),
		      // 		      initial offset
		      sizeof(float)*7, // this comes from Vertex, texture is at index 7
		      );

glActivateTexture(GL_TEXTURE);
glBindTexture(GL_TEXTURE_2D, myTexture);
// hook up texture to uniform variable
glUniformli(textureUniform, 0);

// DRAW IT!!




// still have to do all the points from last time
