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
const size_t pixelBufSize = width * height * sizeof(RGB);

class Window;

class Game {
public:
	Game(Window* window);
	~Game();

	void init();
	void draw();

	RGB& pixel(int x, int y);

	Window* window;
	RGB* pixelPtr = nullptr;
};

class Window {
public:
	Window();
	~Window();

	void init();
	void run();

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	SDL_Texture* screenTexture = nullptr;
	unique_ptr<RGB[]> pixelBuf;
	RGB* pixelPtr = nullptr;

	bool gameRunning = true;

	Game game;
};

Game::Game(Window* window) : window(window) {}

Game::~Game() {}

void Game::init() {
	pixelPtr = window->pixelPtr;
}

void Game::draw() {
	pixel(0, 0) = { 255, 0, 0 };
}

RGB& Game::pixel(int x, int y) {
	return pixelPtr[y * width + x];
}

Window::Window() : game(this) {}

Window::~Window() {
	if (screenTexture) SDL_DestroyTexture(screenTexture);
	if (renderer) SDL_DestroyRenderer(renderer);
	if (window) SDL_DestroyWindow(window);

	SDL_Quit();
}

void Window::init() {
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

	game.init();
}

void Window::run() {
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

		memset(pixelPtr, 0, pixelBufSize);
		game.draw();
		sdl_e(SDL_UpdateTexture(screenTexture, nullptr, pixelPtr, pixelPitch));
		SDL_RenderCopy(renderer, screenTexture, nullptr, nullptr);

		SDL_RenderPresent(renderer);
	}
}

int main() {
	Window game;
	game.init();
	game.run();

	return 0;
}