#include <AntTweakBar.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <stdio.h>
#include <stdlib.h>

#include <GL/gl.h>
#include <GL/glu.h>

void load_texture(GLuint texture_id, SDL_Surface* image) {
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
}

/* globals */
SDL_Surface* original;
SDL_Surface* left_overlap = NULL;
SDL_Surface* right_overlap = NULL;
static float overlap = 0.0f;
GLuint original_texture_id;
GLuint left_overlap_texture_id;
GLuint right_overlap_texture_id;

int width  = 1000, height = 700;

void _copy_original() {
  if (left_overlap) SDL_FreeSurface(left_overlap);
  if (right_overlap) SDL_FreeSurface(right_overlap);
  left_overlap = SDL_ConvertSurface(original, original->format, SDL_SWSURFACE);
  right_overlap = SDL_ConvertSurface(original, original->format, SDL_SWSURFACE);
  int bpp = original->format->BytesPerPixel;
  int diff = (int)(original->w*overlap/2);
  int left_border = original->w/2 - diff;
  int right_border = original->w/2 + diff;
  printf("bpp: %d, be: %d, l: %d, r: %d\n", bpp, SDL_BYTEORDER == SDL_BIG_ENDIAN, left_border, right_border);
  /* Lock the screen for direct access to the pixels */
  if ( SDL_MUSTLOCK(left_overlap) ) {
    if ( SDL_LockSurface(left_overlap < 0 )) {
      fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
      return;
    }
  }
  if ( SDL_MUSTLOCK(right_overlap) ) {
    if ( SDL_LockSurface(right_overlap < 0 )) {
      fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
      return;
    }
  }

  int distance = right_border - left_border;
  for (int step = 0; step <= distance; step++ ) {
    int x = step + left_border;
    for (int y = 0; y < original->h; y++) {
      Uint8 *lp = (Uint8 *)left_overlap->pixels + y * left_overlap->pitch + x * bpp;
      Uint8 *rp = (Uint8 *)right_overlap->pixels + y * right_overlap->pitch + x * bpp;
      if (bpp == 3  && SDL_BYTEORDER != SDL_BIG_ENDIAN) {
        float share = ((float)step)/distance;
        for(int c = 0; c < 3; c++) {
          Uint8 value = (Uint8)(rp[c]*share);
          rp[c] = value;
          lp[c] -= value;
        }
      }
    }
  }
  /* draw window borders */
  /*
  for (int y = 0; y < 100; y++) {
    int lx = left_border;
    int rx = right_border;
    Uint8 *lp = (Uint8 *)left_overlap->pixels + y * original->pitch + lx * bpp;
    Uint8 *rp = (Uint8 *)right_overlap->pixels + y * original->pitch + rx * bpp;
    lp[0] = lp[1] = lp[2] = rp[0] = rp[1] = rp[2] = 0;
  }
  */
  SDL_UpdateRect(left_overlap, 0, 0, left_overlap->w, left_overlap->h);
  SDL_UpdateRect(right_overlap, 0, 0, right_overlap->w, right_overlap->h);
}

void TW_CALL _set_overlap_callback(const void *value, void *clientData){
  float new_overlap = *(const float *)value;
  if (new_overlap != overlap) {
    overlap = new_overlap;
    _copy_original();
    load_texture(left_overlap_texture_id, left_overlap);
    load_texture(right_overlap_texture_id, right_overlap);
  }
}

void TW_CALL _get_overlap_callback(void *value, void *clientData) { *(float *)value = overlap; }

void _reshape(int w, int h, int bpp, Uint32 flags) {
    if( !SDL_SetVideoMode(w, h, bpp, flags) )
    {
        fprintf(stderr, "Video mode set failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //gluPerspective(40, (double)w/h, 1, 10);
    //gluLookAt(0,0,3, 0,0,0, 0,1,0);

    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


    TwWindowSize(w, h);
}

int main()
{
    const SDL_VideoInfo* video = NULL;
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
    _reshape(width, height, bpp, flags);
    SDL_WM_SetCaption("AntTweakBar simple example using SDL", "AntTweakBar+SDL");

    // Enable SDL unicode and key-repeat
    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    // Set OpenGL viewport and states
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

    // Create a tweak bar
    bar = TwNewBar("TweakBar");
    /*
    float distance = -0.0f;
    TwAddVarRW(bar, "distance", TW_TYPE_FLOAT, &distance,
               " max=0.0 min=-4.0 step=0.01");
    */
    TwAddVarCB(bar, "overlap", TW_TYPE_FLOAT, _set_overlap_callback, _get_overlap_callback, NULL,
               "max=0.25 min=0.0 step=0.001 keydecr='[' keyincr=']' ");

    original = IMG_Load("picture.jpg");
    if(!original) {
      printf("IMG_Load: %s\n", IMG_GetError());
      exit(1);
    }

    glGenTextures(1, &original_texture_id);
    glGenTextures(1, &left_overlap_texture_id);
    glGenTextures(1, &right_overlap_texture_id);
    load_texture(original_texture_id, original);
    // Main loop:
    // - Draw some cubes
    // - Process events
    while( !quit )
    {
        SDL_Event event;
        int handled;

        glClearColor(0.0f, 0.0f, 0.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);


    float texture_overlap = 2.0 * overlap;

    glPushMatrix();
    //glTranslatef( 0, 0, distance);
	glColor3d( 1, 1, 1 );
    glBindTexture(GL_TEXTURE_2D, original_texture_id);
	glBegin(GL_QUADS); {
      /* left */
      glTexCoord2f( 0.0, 0.0 );
      glVertex3f( -1.0, 1.0, 0 );
      glTexCoord2f( 0.5 - overlap/2, 0.0 );
      glVertex3f( 0.0 - texture_overlap, 1.0, 0 );
      glTexCoord2f( 0.5 - overlap/2, 1.0 );
      glVertex3f( 0.0 - texture_overlap, -1.0, 0 );
      glTexCoord2f( 0.0, 1.0 );
      glVertex3f( -1.0, -1.0, 0 );
      /* right */
      glTexCoord2f( 0.5 + overlap/2, 0.0 );
      glVertex3f( 0.0 + texture_overlap, 1.0, 0 );
      glTexCoord2f( 1.0, 0.0 );
      glVertex3f( 1.0, 1.0, 0 );
      glTexCoord2f( 1.0, 1.0 );
      glVertex3f( 1.0, -1.0, 0);
      glTexCoord2f( 0.5 + overlap/2, 1.0 );
      glVertex3f( 0.0 + texture_overlap, -1.0, 0);
    }
    glEnd();
    /* left-overlap */
    glBindTexture(GL_TEXTURE_2D, left_overlap_texture_id);
	glBegin(GL_QUADS); {
      glTexCoord2f( 0.5 - overlap/2, 0.0 );
      glVertex3f( 0.0 - texture_overlap, 1.0, 0 );
      glTexCoord2f( 0.5 + overlap/2, 0.0 );
      glVertex3f( 0.0, 1.0, 0 );
      glTexCoord2f( 0.5 + overlap/2, 1.0 );
      glVertex3f( 0.0 , -1.0, 0 );
      glTexCoord2f( 0.5 - overlap/2, 1.0 );
      glVertex3f( 0.0 - texture_overlap, -1.0, 0 );
    }
    glEnd();
      /* right-overlap */
    glBindTexture(GL_TEXTURE_2D, right_overlap_texture_id);
	glBegin(GL_QUADS); {
      glTexCoord2f( 0.5 - overlap/2, 0.0 );
      glVertex3f( 0.0, 1.0, 0 );
      glTexCoord2f( 0.5 + overlap/2, 0.0 );
      glVertex3f( 0.0 + texture_overlap, 1.0, 0 );
      glTexCoord2f( 0.5 + overlap/2, 1.0 );
      glVertex3f( 0.0 + texture_overlap, -1.0, 0 );
      glTexCoord2f( 0.5 - overlap/2, 1.0 );
      glVertex3f( 0.0, -1.0, 0 );
    }
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
                switch( event.type ){
                case SDL_QUIT:  // Window is closed
                  quit = 1;
                  break;
                case SDL_VIDEORESIZE:
                  int w = event.resize.w;
                  int h = event.resize.h;
                  printf("window-event to %d x %d\n", w, h);
                  _reshape(w, h, bpp, flags);
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
    SDL_FreeSurface(original);

    return 0;
}
