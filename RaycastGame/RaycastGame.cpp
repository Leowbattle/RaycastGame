#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstdint>
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

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

class Sprite {
public:
	vec2 pos;
};

class Game {
public:
	Game(Window* window);
	~Game();

	void init();
	void update();
	void draw();

	void drawFloor();
	void drawWalls();
	void drawSprites();

	vector<Sprite> sprites;

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
	vector<RGB> texture2;
	vector<RGB> barrelTexture;
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
	vector<uint8_t> lastKeystate;
	bool keyDown(int key);
	bool keyPressed(int key);

	SDL_Texture* screenTexture = nullptr;
	unique_ptr<RGB[]> pixelBuf;
	unique_ptr<float[]> depthBuf;
	RGB* pixelPtr = nullptr;

	bool gameRunning = true;
	float time = 0;
	float lastTime = 0;
	float timeAccumulator = 0;

	Game game;
};

Game::Game(Window* window) : window(window) {}

Game::~Game() {}

const float HEIGHT = 16;

const float speed = 256;
const float turnSpeed = M_PI / 2;

const float mouseSensitivity = 0.01f;
int mouseRelX;

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
	texture2 = loadTexture("wolf3d/eagle.png");
	barrelTexture = loadTexture("wolf3d/barrel.png");

	sprites.push_back(Sprite{ 4*64, 6*64 });
	sprites.push_back(Sprite{ 3*64, 6*64 });
	sprites.push_back(Sprite{ 4*64, 5*64 });
	sprites.push_back(Sprite{ 3*64, 5*64 });
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

	if (mouseRelX != 0) {
		setAngle(angle + mouseRelX * mouseSensitivity);
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
	drawSprites();
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
const int textureSize = 1 << texSizeLog;

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
	1, 0, 0, 0, 1, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};
const int mapSize = 10;

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
	o.x /= textureSize;
	o.y /= textureSize;
	vec2 d = r.d;

	int mapX = (int)floorf(o.x);
	int mapY = (int)floorf(o.y);

	float stepX = d.x > 0 ? 1 : -1;
	float stepY = d.y > 0 ? 1 : -1;

	/*float tmaxX = (ceilf(o.x) - o.x) / d.x * stepX;
	float tmaxY = (ceilf(o.y) - o.y) / d.y * stepY;*/

	float tmaxX = 0;
	float tmaxY = 0;

	if (stepX == 1) {
		tmaxX = (mapX - o.x + 1) / d.x;
	}
	else {
		tmaxX = (mapX - o.x) / d.x;
	}
	if (stepY == 1) {
		tmaxY = (mapY - o.y + 1) / d.y;
	}
	else {
		tmaxY = (mapY - o.y) / d.y;
	}

	float tDeltaX = 1 / d.x * stepX;
	float tDeltaY = 1 / d.y * stepY;

	float t = 0;
	int side = 0;
	while (mapX >= 0 && mapX < mapSize && mapY >= 0 && mapY < mapSize) {
		if (tmaxX < tmaxY) {
			tmaxX += tDeltaX;
			mapX += stepX;

			if (map[mapY * mapSize + mapX] > 0) {
				t = tmaxX - tDeltaX;
				side = 0;
				break;
			}
		}
		else {
			tmaxY += tDeltaY;
			mapY += stepY;

			if (map[mapY * mapSize + mapX] > 0) {
				t = tmaxY - tDeltaY;
				side = 1;
				break;
			}
		}
	}

	if (mapX < 0 || mapX > mapSize - 1 || mapY < 0 || mapY > mapSize - 1) {
		return { {0, 0}, -1 };
	}

	return {
		{(int)(o.x + t * d.x), (int)(o.y + t * d.y)},
		t * textureSize,
		side
	};
}

void Game::drawWalls() {
	int posx = (int)(pos.x * textureSize);
	int posy = (int)(pos.y * textureSize);

	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	float rSize = 1;
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
			SDL_Rect rect2 = {x* textureSize, y* textureSize, textureSize, textureSize };
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
		window->depthBuf[x] = d;

		float wallX;
		if (res.side == 0) {
			wallX = pos.y + d * rDirY;
		}
		else {
			wallX = pos.x + d * rDirX;
		}
		//wallX -= floorf(wallX);
		int texX = (int)(wallX * 1) % textureSize;
		/*if (res.side == 0 && rDirX > 0) texX = textureSize - texX - 1;
		if (res.side == 1 && rDirX < 0) texX = textureSize - texX - 1;*/

		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		SDL_RenderDrawLine(renderer, posx, posy, posx + (int)(res.t * textureSize * rDirX), posy + (int)(res.t * textureSize * rDirY));

		RGB colours[] = { {255, 0, 0}, {200, 0, 0} };
		RGB colour = colours[res.side];

		float wallHeight = 32;
		int y1 = max(camDist * (camZ - wallHeight) / d + height / 2, 0);
		int y2 = min(camZ * camDist / d + height / 2, height);

		float step = textureSize / (float)((camZ * camDist / d + height / 2) - (camDist * (camZ - wallHeight) / d + height / 2));
		//float texY = (y1 - camZ / 2 + height / 2) * step;
		float texY = 0;
		if (y1 == 0) {
			texY -= step * (camDist * (camZ - wallHeight) / d + height / 2);
		}
		for (int y = y1; y < y2; y++) {
			pixel(x, y) = texture2[(((int)texY) & (textureSize - 1)) * textureSize + texX];

			texY += step;
		}

		rDirX += rStepX;
		rDirY += rStepY;
	}
}

void Game::drawSprites() {
	sort(sprites.begin(), sprites.end(), [&](const Sprite& a, const Sprite& b) {
		vec2 p = a.pos;
		p.x -= pos.x;
		p.y -= pos.y;
		float da = p.x * dir.x + p.y * dir.y;

		p = b.pos;
		p.x -= pos.x;
		p.y -= pos.y;
		float db = p.x * dir.x + p.y * dir.y;

		return da > db;
	});

	for (int i = 0; i < sprites.size(); i++) {
		const Sprite* sprite = &sprites[i];
		vec2 p = sprite->pos;

		p.x -= pos.x;
		p.y -= pos.y;

		float sy = p.x * dir.x + p.y * dir.y;
		float sx = p.y * dir.x - p.x * dir.y;

		if (sy < 0) {
			continue;
		}

		float q = sx / sy;

		printf("%f %f\n", sx, sy);

		q *= camDist/2;
		q += width / 2;

		/*int x1 = (int)fmaxf(q - textureSize / 2, 0);
		int x2 = (int)fminf(q + textureSize / 2, width);*/

		int x1 = (int)fmaxf((sx - textureSize/2) / sy * camDist/2 + width / 2, 0);
		int x2 = (int)fminf((sx + textureSize/2) / sy * camDist / 2 + width / 2, width);

		int y1 = (int)fmaxf(camDist * (camZ - textureSize/2) / sy + height / 2, 0);
		int y2 = (int)fminf(camZ * camDist / sy + height / 2, height);

		float texX = 0;
		float texY = 0;

		float stepX = (textureSize) / (float)(((sx + textureSize / 2) / sy * camDist / 2 + width / 2) - ((sx - textureSize / 2) / sy * camDist / 2 + width / 2));
		float stepY = (textureSize) / (float)((camZ * camDist / sy + height / 2) - (camDist * (camZ - textureSize / 2) / sy + height / 2));

		if (x1 == 0) {
			texX -= stepX * ((sx - textureSize / 2) / sy * camDist / 2 + width / 2);
		}
		if (y1 == 0) {
			texY -= stepY * (camDist * (camZ - textureSize / 2) / sy + height / 2);
		}
		float texY1 = texY;

		for (int x = x1; x < x2; x++) {
			if (window->depthBuf[x] < sy) {
				texX += stepX;
				texY = texY1;

				continue;
			}

			for (int y = y1; y < y2; y++) {
				RGB colour = barrelTexture[(((int)texY) & (textureSize - 1)) * textureSize + texX];
				if (colour.r == 0 && colour.g == 0 && colour.b == 0) {
					texY += stepY;
					continue;
				}
				pixel(x, y) = colour;

				texY += stepY;
			}
			texX += stepX;
			texY = texY1;
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

	depthBuf = make_unique<float[]>(width);

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

		mouseRelX = 0;

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				gameRunning = false;
				break;
			case SDL_MOUSEMOTION:
				mouseRelX = e.motion.xrel;
				break;
			}
		}

		lastKeystate = keyState;

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
		if (keyPressed(SDL_SCANCODE_T)) {
			firstPerson = !firstPerson;
		}
		if (firstPerson) SDL_RenderCopy(renderer, screenTexture, nullptr, nullptr);

		if (keyPressed(SDL_SCANCODE_ESCAPE)) {
			SDL_SetRelativeMouseMode((SDL_bool)!SDL_GetRelativeMouseMode());
		}

		uint64_t frameEnd = SDL_GetPerformanceCounter();
		float frameTime = (frameEnd - now) * period;
		//cout << frameTime * 1000 << "\n";

		SDL_RenderPresent(renderer);
	}
}

bool Window::keyDown(int key) {
	return keyState[key];
}

bool Window::keyPressed(int key) {
	return keyState[key] && !lastKeystate[key];
}

int main() {
	Window game;
	game.init();
	game.run();

	return 0;
}
