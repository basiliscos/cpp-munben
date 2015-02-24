#include <AntTweakBar.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <stdio.h>
#include <stdlib.h>

#include <GL/gl.h>
#include <GL/glu.h>

GLuint load_texture(SDL_Surface* image) {
  GLuint texture_id;
  glGenTextures(1, &texture_id);
  Uint8 number_of_colors = image->format->BytesPerPixel;
  GLenum texture_format = number_of_colors == 4
    ? ( image->format->Rmask == 0x000000ff ? GL_RGBA : GL_BGRA )
    : ( number_of_colors == 3 ?
       image->format->Rmask == 0x000000ff ? GL_RGB : GL_BGR
       : 0
       );
  if (!texture_format ) {
    printf("cannot determine texture image format\n");
    exit(1);
  }
  glBindTexture( GL_TEXTURE_2D, texture_id );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, texture_format, image->w, image->h,
                 0, texture_format, GL_UNSIGNED_BYTE, image->pixels);
  return texture_id;
}

int main()
{
    const SDL_VideoInfo* video = NULL;
    int width  = 640, height = 480;
    int bpp, flags;
    int quit = 0;
    TwBar *bar;

    // Initialize SDL, then get the current video mode and use it to create a SDL window.
    if( SDL_Init(SDL_INIT_VIDEO)<0 )
    {
        fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }
    video = SDL_GetVideoInfo();
    if( !video )
    {
        fprintf(stderr, "Video query failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    //SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    bpp = video->vfmt->BitsPerPixel;
    flags = SDL_OPENGL | SDL_HWSURFACE | SDL_RESIZABLE;
    //flags |= SDL_FULLSCREEN;
    if( !SDL_SetVideoMode(width, height, bpp, flags) )
    {
        fprintf(stderr, "Video mode set failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }
    SDL_WM_SetCaption("AntTweakBar simple example using SDL", "AntTweakBar+SDL");

    // Enable SDL unicode and key-repeat
    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    // Set OpenGL viewport and states
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0); // use default light diffuse and position
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

    // Initialize AntTweakBar
    TwInit(TW_OPENGL, NULL);

    // Tell the window size to AntTweakBar
    TwWindowSize(width, height);

    // Create a tweak bar
    bar = TwNewBar("TweakBar");
    float distance = -2.0f;
    TwAddVarRW(bar, "distance", TW_TYPE_FLOAT, &distance, 
               " max=-0.25 min=-4.0 step=0.01");
    float overlap = 0.0f;
    TwAddVarRW(bar, "overlap", TW_TYPE_FLOAT, &overlap, 
               "max=0.25 min=0.0 step=0.001");

    SDL_Surface* image = IMG_Load("/home/basiliscos/development/perl/group-dual/picture.jpg");
    if(!image) {
      printf("IMG_Load: %s\n", IMG_GetError());
      exit(1);
    }
               
    GLuint texture_id = load_texture(image);
    // Main loop:
    // - Draw some cubes
    // - Process events
    while( !quit )
    {
        SDL_Event event;
        int handled;

        glClearColor(0.0f, 0.0f, 0.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(40, (double)width/height, 1, 10);
        gluLookAt(0,0,3, 0,0,0, 0,1,0);

        glPushMatrix();
        glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


    float texture_overlap = 2.0 * overlap;
    
	glTranslatef( 0, 0, distance);
	glColor3d( 1, 1, 1 );
        glBindTexture(GL_TEXTURE_2D, texture_id);
	glBegin(GL_QUADS);
    /* left */
	glTexCoord2f( 0.0, 0.0 );
	glVertex3f( -1.0, 1.0, 0 );
	glTexCoord2f( 0.5 + texture_overlap, 0.0 );
	glVertex3f( 0.0, 1.0, 0 );
	glTexCoord2f( 0.5 + texture_overlap, 1.0 );
	glVertex3f( 0.0, -1.0, 0 );
	glTexCoord2f( 0.0, 1.0 );
	glVertex3f( -1.0, -1.0, 0 );
    /* right */
	glTexCoord2f( 0.5 - texture_overlap, 0.0 );
	glVertex3f( 0.0, 1.0, 0 );
	glTexCoord2f( 1.0, 0.0 );
	glVertex3f( 1.0, 1.0, 0 );
	glTexCoord2f( 1.0, 1.0 );
	glVertex3f( 1.0, -1.0, 0 );
	glTexCoord2f( 0.5 - texture_overlap, 1.0 );
	glVertex3f( 0.0, -1.0, 0 );
    
        glEnd();
        glPopMatrix();

        TwDraw();

        // // Present frame buffer
        SDL_GL_SwapBuffers();

        while( SDL_PollEvent(&event) )
        {
            // Send event to AntTweakBar
            handled = TwEventSDL(&event, SDL_MAJOR_VERSION, SDL_MINOR_VERSION);

            // If event has not been handled by AntTweakBar, process it
            if( !handled )
            {
                switch( event.type )
                {
                case SDL_QUIT:  // Window is closed
                    quit = 1;
                    break;

                }
            }
        }
        // quit = 1;
    } // End of main loop

    // Terminate AntTweakBar
    TwTerminate();

    // Terminate SDL
    SDL_Quit();

    return 0;
}
