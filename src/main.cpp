#include "cpu.h"
#include <chrono>
#include <thread>
#include <iostream>

int main() {
	CPU cpu;
	cpu.init();
	cpu.read_rom("../demos/invaders.ch8");

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		return 0;
	}

	SDL_Window* window = SDL_CreateWindow(
		"chip8emu",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		1280, 640, SDL_WINDOW_SHOWN
	);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

	SDL_Texture* texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		64, 32);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	//TODO: timing is wrong probably I didnt read much into it
	while (true) {
		for (int i = 0; i < 10; i++) {
			cpu.input();
			cpu.cycle();
		}

		cpu.draw(texture, renderer);
		cpu.update_timers();

		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}