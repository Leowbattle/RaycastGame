#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstdint>
#define _USE_MATH_DEFINES
#include <cmath>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

struct vec2i {
	int x;
	int y;
};

float degToRad(float d) {
	return d * M_PI / 180.0f;
}

float lerp(float a, float b, float t) {
	return a + t * (b - a);
}

int min(int a, int b) {
	return a < b ? a : b;
}

int max(int a, int b) {
	return a > b ? a : b;
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

	void drawFloor();
	void drawWalls();

	RGB& pixel(int x, int y);

	Window* window;
	RGB* pixelPtr = nullptr;
	SDL_Renderer* renderer;

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

	vector<RGB> texture;
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

const float HEIGHT = 32;

const float speed = 256;
const float turnSpeed = M_PI / 2;

float vz = 0;
float gravity = -320;
bool canJump = true;

vector<RGB> loadTexture(const char* path) {
	int x, y, n;
	RGB* data = (RGB*)stbi_load(path, &x, &y, &n, 0);
	vector<RGB> pixels;
	pixels.assign(data, data + y * x);
	return pixels;
}

void Game::init() {
	pixelPtr = window->pixelPtr;
	renderer = window->renderer;

	setFovX(degToRad(70));
	setPos({ 0, 0 });
	setAngle(0);
	setHeight(HEIGHT);

	texture = loadTexture("wolf3d/wood.png");
}

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
		vz += 160.0f;
		canJump = false;
	}
	camZ += vz * dt;
	vz += gravity * dt;
	if (camZ < HEIGHT) {
		camZ = HEIGHT;
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

void Game::draw() {
	drawFloor();
	drawWalls();
}

//const uint8_t floorTexture[] = {
//	0, 0, 0, 0, 0, 0, 0, 0,
//	0, 0, 0, 0, 0, 0, 0, 0,
//	0, 0, 255, 255, 255, 255, 0, 0,
//	0, 0, 255, 0, 0, 255, 0, 0,
//	0, 0, 255, 0, 0, 255, 0, 0,
//	0, 0, 255, 255, 255, 255, 0, 0,
//	0, 0, 0, 0, 0, 0, 0, 0,
//	0, 0, 0, 0, 0, 0, 0, 0,
//};
//const int texSizeLog = 3;
//const uint8_t floorTexture[] = {
//	127, 255,
//	255, 255
//};
//const int texSizeLog = 1;
//const int texMask = (1 << texSizeLog) - 1;
const int texSizeLog = 6;
const int texMask = (1 << texSizeLog) - 1;

const int halfHeight = height / 2;

void Game::drawFloor() {
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

			RGB pix = texture[((fy2 & texMask) << texSizeLog) + (fx2 & texMask)];
			pixel(x, i) = pix;

			/*uint8_t pix = floorTexture[((fy2 & texMask) << texSizeLog) + (fx2 & texMask)];
			pixel(x, i) = { pix, pix, 0 };*/

			//pixel(x, i) = { (uint8_t)(fx * 255), (uint8_t)(fy * 255), 0 };

			fx += stepX;
			fy += stepY;
		}
	}
}

const uint8_t map[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 0, 1, 0, 0, 0, 1, 0, 0, 1,
	1, 0, 1, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 1, 1, 1, 1, 1, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};
const int mapSize = 10;
const int tileSize = 10;

struct Raycast {
	vec2 o;
	vec2 d;
};

struct RaycastResult {
	vec2i tile;
	float t;
	int side;
};

RaycastResult raycastMap(Raycast r) {
	vec2 o = r.o;
	/*o.x /= tileSize;
	o.y /= tileSize;*/
	vec2 d = r.d;

	int x = (int)floorf(o.x);
	int y = (int)floorf(o.y);

	float stepX = d.x > 0 ? 1 : -1;
	float stepY = d.y > 0 ? 1 : -1;

	/*float tmaxX = (ceilf(o.x) - o.x) / d.x * stepX;
	float tmaxY = (ceilf(o.y) - o.y) / d.y * stepY;*/

	float tmaxX = 0;
	float tmaxY = 0;

	if (stepX == 1) {
		tmaxX = (x - o.x + 1) / d.x;
	}
	else {
		tmaxX = (x - o.x) / d.x;
	}
	if (stepY == 1) {
		tmaxY = (y - o.y + 1) / d.y;
	}
	else {
		tmaxY = (y - o.y) / d.y;
	}

	float tDeltaX = 1 / d.x * stepX;
	float tDeltaY = 1 / d.y * stepY;

	float t = 0;
	int side = 0;
	while (x >= 0 && x < mapSize && y >= 0 && y < mapSize) {
		if (tmaxX < tmaxY) {
			tmaxX += tDeltaX;
			x += stepX;

			if (map[y * mapSize + x] > 0) {
				t = tmaxX - tDeltaX;
				side = 0;
				break;
			}
		}
		else {
			tmaxY += tDeltaY;
			y += stepY;

			if (map[y * mapSize + x] > 0) {
				t = tmaxY - tDeltaY;
				side = 1;
				break;
			}
		}
	}

	if (x < 0 || x > mapSize - 1 || y < 0 || y > mapSize - 1) {
		return { {0, 0}, -1 };
	}

	return {
		{x, y},
		t,
		side
	};
}

void Game::drawWalls() {
	int posx = (int)(pos.x * tileSize);
	int posy = (int)(pos.y * tileSize);

	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	float rSize = 5;
	float hRSize = rSize / 2;
	SDL_Rect rect = {posx - hRSize, posy - hRSize, rSize, rSize};
	SDL_RenderFillRect(renderer, &rect);

	float dirLen = camDist / 10;
	SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
	SDL_RenderDrawLine(renderer, posx, posy, posx + (int)(dirLen * dir.x), posy + (int)(dirLen * dir.y));

	float u = tan(fovX / 2) * 2;
	float rDirX = dir.x + dir.y * u;
	float rDirY = dir.y - dir.x * u;

	float rDirX_R = dir.x - dir.y * u;
	float rDirY_R = dir.y + dir.x * u;
	float rStepX = (rDirX_R - rDirX) / width;
	float rStepY = (rDirY_R - rDirY) / width;

	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
	for (int y = 0; y < mapSize; y++) {
		for (int x = 0; x < mapSize; x++) {
			if (map[y * mapSize + x] == 0) continue;
			SDL_Rect rect2 = {x*tileSize, y*tileSize, tileSize, tileSize};
			SDL_RenderFillRect(renderer, &rect2);
		}
	}

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	for (int x = 0; x < width; x++) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		//SDL_RenderDrawLine(renderer, posx, posy, posx + (int)(dirLen * rDirX), posy + (int)(dirLen * rDirY));

		Raycast ray = {
			pos,
			{rDirX, rDirY}
		};
		RaycastResult res = raycastMap(ray);
		if (res.t == -1) {
			rDirX += rStepX;
			rDirY += rStepY;
			continue;
		}

		float d = dir.x * res.t * rDirX + dir.y * res.t * rDirY;
		float h = (int)(camDist / d);

		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		SDL_RenderDrawLine(renderer, posx, posy, posx + (int)(res.t * tileSize * rDirX), posy + (int)(res.t * tileSize * rDirY));

		RGB colours[] = { {255, 0, 0}, {200, 0, 0} };
		RGB colour = colours[res.side];

		float wallHeight = 0.5f;
		int y1 = max(camDist * (camZ - wallHeight) / d + height / 2, 0);
		int y2 = min(camZ * camDist / d + height / 2, height);
		for (int y = y1; y < y2; y++) {
			pixel(x, y) = colour;
		}

		rDirX += rStepX;
		rDirY += rStepY;
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

bool firstPerson = true;
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

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);

		memset(pixelPtr, 0, pixelBufSize);
		game.draw();
		sdl_e(SDL_UpdateTexture(screenTexture, nullptr, pixelPtr, pixelPitch));
		if (keyDown(SDL_SCANCODE_T)) {
			firstPerson = !firstPerson;
		}
		if (firstPerson) SDL_RenderCopy(renderer, screenTexture, nullptr, nullptr);

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