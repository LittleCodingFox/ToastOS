#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 1000

int main (int argc, char **argv)
{
    int windowFlags = SDL_WINDOW_OPENGL;
    SDL_Window *window = SDL_CreateWindow("OpenGL Test", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, windowFlags);

    if(window == NULL)
    {
        printf("Error: %s\n", SDL_GetError());

        SDL_Quit();

        return 1;
    }

    int width = 0, height = 0;

    SDL_GetWindowSize(window, &width, &height);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    SDL_GL_MakeCurrent(window, context);
  
    bool running = true;

    while (running)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_ESCAPE:

                    running = false;

                    break;

                default:

                    break;
                }
            }
            else if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        glViewport(0, 0, width, height);
        glClearColor(1.f, 0.f, 1.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(window);
    }

    SDL_Quit();

    return 0;
}