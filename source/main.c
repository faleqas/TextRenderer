#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>


static bool running = true;


void main_loop();


int main(void)
{
	SDL_Init(0);

	main_loop();

	SDL_Quit();
	return 0;
}


void main_loop()
{
	SDL_Window* window = SDL_CreateWindow("TextRenderer", 100, 100, 800, 600, 0);
	SDL_Surface* bitmap = SDL_GetWindowSurface(window);
	unsigned char* pixels = bitmap->pixels;
	size_t bitmap_size = (bitmap->w * bitmap->h) * bitmap->format->BytesPerPixel;

	SDL_Event e;
	while (running)
	{
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
			{
				running = false;
			}
		}

		for (int i = 0; i < bitmap_size; i += bitmap->format->BytesPerPixel)
		{
			pixels[i] = 80;              //b
			pixels[i + 1] = 80;          //g 
			pixels[i + 2] = 80;          //r
		}

		SDL_UpdateWindowSurface(window);
	}
}