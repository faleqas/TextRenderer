#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>


static bool running = true;


void load_ttf(const char* path);
void main_loop();


int main(int argc, char** argv)
{
	SDL_Init(0);

	load_ttf("Minecraft.ttf");
	main_loop();

	SDL_Quit();
	return 0;
}


void load_ttf(const char* path)
{
	FILE* fp;
	fopen_s(&fp, path, "rb");
	if (!fp) return;

	_getw(fp); //sfnt_version

	uint16_t num_tables;
	fread(&num_tables, sizeof(uint16_t), 1, fp);

	fgetwc(fp); //search range
	fgetwc(fp); //entry selector
	fgetwc(fp); //range shift

	printf("NUM TABLES: %hu\n", num_tables);

	//read tables
	for (int i = 0; i < num_tables; i++)
	{

	}

	fclose(fp);
}


void main_loop()
{
	SDL_Window* window = SDL_CreateWindow("TextRenderer", 100, 100, 800, 600, 0);
	SDL_Surface* bitmap = SDL_GetWindowSurface(window);
	unsigned char* pixels = bitmap->pixels;
	const size_t bitmap_size = (bitmap->w * bitmap->h) * bitmap->format->BytesPerPixel;

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
			//BGR
			pixels[i] = 80;
			pixels[i + 1] = 80;
			pixels[i + 2] = 80;
		}

		SDL_UpdateWindowSurface(window);
	}
}