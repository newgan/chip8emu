#include "cpu.h"
#include <vector>
#include <fstream>
#include <SDL2/SDL.h>
#include <iostream>

void CPU::init() {
	std::array<uint8_t, 80> sprites = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,
	0x20, 0x60, 0x20, 0x20, 0x70,
	0xF0, 0x10, 0xF0, 0x80, 0xF0,
	0xF0, 0x10, 0xF0, 0x10, 0xF0,
	0x90, 0x90, 0xF0, 0x10, 0x10,
	0xF0, 0x80, 0xF0, 0x10, 0xF0,
	0xF0, 0x80, 0xF0, 0x90, 0xF0,
	0xF0, 0x10, 0x20, 0x40, 0x40,
	0xF0, 0x90, 0xF0, 0x90, 0xF0,
	0xF0, 0x90, 0xF0, 0x10, 0xF0,
	0xF0, 0x90, 0xF0, 0x90, 0x90,
	0xE0, 0x90, 0xE0, 0x90, 0xE0,
	0xF0, 0x80, 0x80, 0x80, 0xF0,
	0xE0, 0x90, 0x90, 0x90, 0xE0,
	0xF0, 0x80, 0xF0, 0x80, 0xF0,
	0xF0, 0x80, 0xF0, 0x80, 0x80,
	};

	std::copy(sprites.begin(), sprites.end(), ram.begin());
}

Opcode CPU::fetch() {
	Opcode opcode((ram[pc] << 8) | (ram[pc + 1])); // 2 byte instructions

	pc += 2;
	return opcode;
}

void CPU::read_rom(const std::string& program_name) {
	std::ifstream infile(program_name, std::ios_base::binary);

	infile.seekg(0, infile.end);
	size_t length = infile.tellg();
	infile.seekg(0, infile.beg);

	if (length > 0) {
		infile.read(reinterpret_cast<char*>(&ram[512]), length);
	}
}

void CPU::draw(SDL_Texture* tex, SDL_Renderer* renderer) {
	if (should_redraw) {
		for (int i = 0; i < screen_pixel_count; i++) {
			uint8_t pixel = frame_buffer[i];
			pixel_buffer[i] = (0xFFFFFF00 * pixel) | 0x000000FF;
		}

		SDL_UpdateTexture(tex, NULL, pixel_buffer.data(), screen_width * sizeof(uint32_t));
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, tex, NULL, NULL);
		SDL_RenderPresent(renderer);

		should_redraw = false;
	}

}
void CPU::input() {
	// I really dont like this, it introduces delay
	//TODO: winproc

	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) exit(0);

		switch (e.type) {
		case SDL_KEYDOWN:
			switch (e.key.keysym.sym) {
			case SDLK_1: keypad[0x1] = true; break;
			case SDLK_2: keypad[0x2] = true; break;
			case SDLK_3: keypad[0x3] = true; break;
			case SDLK_4: keypad[0xC] = true; break;
			case SDLK_q: keypad[0x4] = true; break;
			case SDLK_w: keypad[0x5] = true; break;
			case SDLK_e: keypad[0x6] = true; break;
			case SDLK_r: keypad[0xD] = true; break;
			case SDLK_a: keypad[0x7] = true; break;
			case SDLK_s: keypad[0x8] = true; break;
			case SDLK_d: keypad[0x9] = true; break;
			case SDLK_f: keypad[0xE] = true; break;
			case SDLK_z: keypad[0xA] = true; break;
			case SDLK_x: keypad[0x0] = true; break;
			case SDLK_c: keypad[0xB] = true; break;
			case SDLK_v: keypad[0xF] = true; break;
			}
			break;
		case SDL_KEYUP:
			switch (e.key.keysym.sym) {
			case SDLK_1: keypad[0x1] = false; break;
			case SDLK_2: keypad[0x2] = false; break;
			case SDLK_3: keypad[0x3] = false; break;
			case SDLK_4: keypad[0xC] = false; break;
			case SDLK_q: keypad[0x4] = false; break;
			case SDLK_w: keypad[0x5] = false; break;
			case SDLK_e: keypad[0x6] = false; break;
			case SDLK_r: keypad[0xD] = false; break;
			case SDLK_a: keypad[0x7] = false; break;
			case SDLK_s: keypad[0x8] = false; break;
			case SDLK_d: keypad[0x9] = false; break;
			case SDLK_f: keypad[0xE] = false; break;
			case SDLK_z: keypad[0xA] = false; break;
			case SDLK_x: keypad[0x0] = false; break;
			case SDLK_c: keypad[0xB] = false; break;
			case SDLK_v: keypad[0xF] = false; break;
			}
			break;
		}
	}
}

void CPU::update_timers() {
	if (delay_timer > 0)
		delay_timer--;

	if (sound_timer > 0)
		sound_timer--;
}

void CPU::cycle() {
	cur_opcode = fetch();

	switch (cur_opcode.msb()) {
	case 0x0000: {
		switch (cur_opcode.nnn()) {
		case 0x0E0: { CLS(); break; }
		case 0x0EE: { RET(); break; }
		}
		break;
	}
	case 0x1000: { JMP(); break; }
	case 0x2000: { CALL(); break; };
	case 0x3000: { SE_Vx(); break; };
	case 0x4000: { SNE_Vx(); break; };
	case 0x5000: { SE_Vx_Vy(); break; };
	case 0x6000: { LD_Vx(); break; }
	case 0x7000: { ADD_Vx(); break; }
	case 0x8000: {
		switch (cur_opcode.n()) {
		case 0x0: { LD_Vx_Vy(); break; }
		case 0x1: { OR_Vx_Vy(); break; }
		case 0x2: { AND_Vx_Vy(); break; }
		case 0x3: { XOR_Vx_Vy(); break; }
		case 0x4: { ADD_Vx_Vy(); break; }
		case 0x5: { SUB_Vx_Vy(); break; }
		case 0x6: { SHR_Vx_Vy(); break; }
		case 0x7: { SUBN_Vx_Vy(); break; }
		case 0xE: { SHL_Vx_Vy(); break; }
		}
		break; };

	case 0x9000: { SNE_Vx_Vy(); break; };
	case 0xA000: { LD_I(); break; }
	case 0xB000: { JP_V0();  break; };
	case 0xC000: { RND_Vx();  break; };
	case 0xD000: { DRW_Vx_Vy(); break; }
	case 0xE000: {
		switch (cur_opcode.kk()) {
		case 0x9E: { SKP_Vx(); break; }
		case 0xA1: { SKNP_Vx(); break; }
		}
		break;
	};
	case 0xF000: {
		switch (cur_opcode.kk()) {
		case 0x07: { LD_Vx_DT(); break; }
		case 0x0A: { LD_Vx_K(); break; }
		case 0x15: { LD_DT_Vx(); break; }
		case 0x18: { LD_ST_Vx(); break; }
		case 0x1E: { ADD_I_Vx(); break; }
		case 0x29: { LD_F_Vx(); break; }
		case 0x33: { LD_B_Vx(); break; }
		case 0x55: { LD_I_Vx(); break; }
		case 0x65: { LD_Vx_I(); break; }
		}
		break;
	}
	default:
		break;
	}

}