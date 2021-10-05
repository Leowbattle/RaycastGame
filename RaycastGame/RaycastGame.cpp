#include <iostream>
#include <stdexcept>
#include <memory>
#include <cstdint>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

using namespace std;

const int width = 320;
const int height = 240;

template <class T>
T sdl_e(T x) {
	if (x != 0) {
		cerr << SDL_GetError() << "\n";
		throw runtime_error(SDL_GetError());
	}
	return x;
}

template <class T>
T* sdl_e(T* x) {
	if (x == nullptr) {
		cerr << SDL_GetError() << "\n";
		throw runtime_error(SDL_GetError());
	}
	return x;
}

struct RGB {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

const int pixelPitch = width * sizeof(RGB);

class Game {
public:
	Game() {
		sdl_e(SDL_Init(SDL_INIT_EVERYTHING));

		int scale = 3;
		window = sdl_e(SDL_CreateWindow("Raycast Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width * scale, height * scale, SDL_WINDOW_RESIZABLE));
		renderer = sdl_e(SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC));

		sdl_e(SDL_RenderSetLogicalSize(renderer, width, height));
		sdl_e(SDL_RenderSetIntegerScale(renderer, SDL_TRUE));
		SDL_SetWindowMinimumSize(window, width, height);

		screenTexture = sdl_e(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height));
		pixelBuf = make_unique<RGB[]>(width * height);
		pixelPtr = pixelBuf.get();
	}

	~Game() {
		if (screenTexture) SDL_DestroyTexture(screenTexture);
		if (renderer) SDL_DestroyRenderer(renderer);
		if (window) SDL_DestroyWindow(window);

		SDL_Quit();
	}

	void run() {
		for (uint8_t i = 0; i < height; i++) {
			pixel(i, i) = { i, 0, 0 };
		}

		while (gameRunning) {
			SDL_Event e;
			while (SDL_PollEvent(&e)) {
				switch (e.type) {
				case SDL_QUIT:
					gameRunning = false;
					break;
				}
			}

			SDL_RenderClear(renderer);
			
			sdl_e(SDL_UpdateTexture(screenTexture, nullptr, pixelPtr, pixelPitch));
			SDL_RenderCopy(renderer, screenTexture, nullptr, nullptr);

			SDL_RenderPresent(renderer);
		}
	}

	RGB& pixel(int x, int y) {
		return pixelPtr[y * width + x];
	}

private:
	SDL_Window* window;
	SDL_Renderer* renderer;

	SDL_Texture* screenTexture;
	unique_ptr<RGB[]> pixelBuf;
	RGB* pixelPtr;

	bool gameRunning = true;
};

int main() {
	Game game;
	game.run();

	return 0;
}