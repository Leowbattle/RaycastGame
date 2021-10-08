#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstdint>
#define _USE_MATH_DEFINES
#include <cmath>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

using namespace std;

const int width = 640;
const int height = 360;

const int FPS = 60;
const float dt = 1.0f / FPS;

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

struct vec2 {
	float x;
	float y;
};

float degToRad(float d) {
	return d * M_PI / 180.0f;
}

float lerp(float a, float b, float t) {
	return a + t * (b - a);
}

const int pixelPitch = width * sizeof(RGB);
const size_t pixelBufSize = width * height * sizeof(RGB);

class Window;

class Game {
public:
	Game(Window* window);
	~Game();

	void init();
	void update();
	void draw();

	RGB& pixel(int x, int y);

	Window* window;
	RGB* pixelPtr = nullptr;

	float fovX;
	float fovY;
	void setFovX(float f);

	vec2 pos;
	float angle;
	vec2 dir;
	float camDist;
	float invCamDist;
	float camZ;

	void setPos(vec2 p);
	void setAngle(float a);
	void setHeight(float h);
};

class Window {
public:
	Window();
	~Window();

	void init();
	void run();

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	vector<uint8_t> keyState;
	bool keyDown(int key);

	SDL_Texture* screenTexture = nullptr;
	unique_ptr<RGB[]> pixelBuf;
	RGB* pixelPtr = nullptr;

	bool gameRunning = true;
	float time = 0;
	float lastTime = 0;
	float timeAccumulator = 0;

	Game game;
};

Game::Game(Window* window) : window(window) {}

Game::~Game() {}

void Game::init() {
	pixelPtr = window->pixelPtr;

	setFovX(degToRad(70));
	setPos({ 0, 0 });
	setAngle(0);
	setHeight(1);
}

const float speed = 5;
const float turnSpeed = M_PI / 2;

float vz = 0;
float gravity = -10;
bool canJump = true;
void Game::update() {
	//cout << window->time << "\n";

	if (window->keyDown(SDL_SCANCODE_W)) {
		pos.x += dir.x * speed * dt;
		pos.y += dir.y * speed * dt;
	}
	if (window->keyDown(SDL_SCANCODE_S)) {
		pos.x -= dir.x * speed * dt;
		pos.y -= dir.y * speed * dt;
	}
	if (window->keyDown(SDL_SCANCODE_A)) {
		pos.x += dir.y * speed * dt;
		pos.y -= dir.x * speed * dt;
	}
	if (window->keyDown(SDL_SCANCODE_D)) {
		pos.x -= dir.y * speed * dt;
		pos.y += dir.x * speed * dt;
	}

	int turnDir = 0;
	turnDir += window->keyDown(SDL_SCANCODE_RIGHT);
	turnDir -= window->keyDown(SDL_SCANCODE_LEFT);
	if (turnDir != 0) {
		setAngle(angle + turnDir * turnSpeed * dt);
	}

	if (canJump && window->keyDown(SDL_SCANCODE_SPACE)) {
		vz += 5.0f;
		canJump = false;
	}
	camZ += vz * dt;
	vz += gravity * dt;
	if (camZ < 1) {
		camZ = 1;
		vz = 0;
		canJump = true;
	}

	if (window->keyDown(SDL_SCANCODE_I)) {
		setFovX(fovX + degToRad(1));
	}
	if (window->keyDown(SDL_SCANCODE_K)) {
		setFovX(fovX - degToRad(1));
	}
}

const uint8_t floorTexture[] = {
	255, 255,
	255, 127
};
const int halfHeight = height / 2;
void Game::draw() {
	for (int i = halfHeight; i < height; i++) {
		float y = i - halfHeight;

		float d = camZ * camDist / y;

		float f1 = width * d * invCamDist;

		float flx = pos.x + dir.x * d - dir.y * -f1;
		float fly = pos.y + dir.y * d + dir.x * -f1;

		float frx = pos.x + dir.x * d - dir.y * f1;
		float fry = pos.y + dir.y * d + dir.x * f1;

		float stepX = (frx - flx) / width;
		float stepY = (fry - fly) / width;

		float fx = flx;
		float fy = fly;

		for (int x = 0; x < width; x++) {
			int fx2 = floorf(fx);
			int fy2 = floorf(fy);

			uint8_t pix = floorTexture[((fy2 & 1) << 1) + (fx2 & 1)];
			pixel(x, i) = { pix, pix, 0 };

			//pixel(x, i) = { (uint8_t)(fx * 255), (uint8_t)(fy * 255), 0 };

			fx += stepX;
			fy += stepY;
		}
	}
}

RGB& Game::pixel(int x, int y) {
	return pixelPtr[y * width + x];
}

const float invAspectRatio = height / width;
void Game::setFovX(float f) {
	// tan half f
	float thf = tanf(f * 0.5);

	fovX = f;
	fovY = 2 * atanf(thf * invAspectRatio);
	camDist = width / (2 * thf);
	invCamDist = 1 / camDist;
}

void Game::setPos(vec2 p) {
	pos = p;
}

void Game::setAngle(float a) {
	angle = a;
	dir = { cosf(a), sinf(a) };
}

void Game::setHeight(float h) {
	camZ = h;
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

	int scale = 1;
	window = sdl_e(SDL_CreateWindow("Raycast Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width * scale, height * scale, SDL_WINDOW_RESIZABLE));
	renderer = sdl_e(SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC));

	sdl_e(SDL_RenderSetLogicalSize(renderer, width, height));
	//sdl_e(SDL_RenderSetIntegerScale(renderer, SDL_TRUE));
	SDL_SetWindowMinimumSize(window, width, height);

	screenTexture = sdl_e(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height));
	pixelBuf = make_unique<RGB[]>(width * height);
	pixelPtr = pixelBuf.get();

	game.init();
}

void Window::run() {
	float period = 1.0f / SDL_GetPerformanceFrequency();
	uint64_t t0 = SDL_GetPerformanceCounter();

	while (gameRunning) {
		lastTime = time;
		uint64_t now = SDL_GetPerformanceCounter();
		time = (now - t0) * period;

		timeAccumulator += time - lastTime;

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				gameRunning = false;
				break;
			}
		}

		int numKeys;
		const uint8_t* keyStatePtr = SDL_GetKeyboardState(&numKeys);
		keyState.resize(numKeys);
		copy(keyStatePtr, keyStatePtr + numKeys, keyState.begin());

		while (timeAccumulator > dt) {
			timeAccumulator -= dt;
			game.update();
		}
		//cout << "FRAME\n";

		SDL_RenderClear(renderer);

		memset(pixelPtr, 0, pixelBufSize);
		game.draw();
		sdl_e(SDL_UpdateTexture(screenTexture, nullptr, pixelPtr, pixelPitch));
		SDL_RenderCopy(renderer, screenTexture, nullptr, nullptr);

		uint64_t frameEnd = SDL_GetPerformanceCounter();
		float frameTime = (frameEnd - now) * period;
		cout << frameTime * 1000 << "\n";

		SDL_RenderPresent(renderer);
	}
}

bool Window::keyDown(int key) {
	return keyState[key];
}

int main() {
	Window game;
	game.init();
	game.run();

	return 0;
}