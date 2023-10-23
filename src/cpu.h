#include <cstddef>
#include <array>
#include <string>
#include <memory>
#include <stack>
#include <chrono>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <unordered_map>

class Opcode {
private:
	uint16_t opcode;

public:
	Opcode() : opcode(0x0) {};
	Opcode(uint16_t opcode) : opcode(opcode) {};
	~Opcode() {};

	uint16_t msb() {
		return opcode & 0xF000;
	}

	uint16_t nnn() {					//the lowest 12 bits of the instruction
		return opcode & 0x0FFF;
	}

	uint8_t n() {						//the lowest 4 bits of the instruction
		return opcode & 0x000F;
	}

	uint8_t x() {						//the lower 4 bits of the high byte of the instruction
		return (opcode & 0x0F00) >> 8;
	}

	uint8_t y() {						//the upper 4 bits of the low byte of the instruction
		return (opcode & 0x00F0) >> 4;
	}

	uint8_t kk() {						//the lowest 8 bits of the instruction
		return opcode & 0x00FF;
	}

	uint16_t data() {
		return opcode;
	}
};

class CPU {
private:
	static constexpr int screen_width = 64;
	static constexpr int screen_height = 32;
	static constexpr int screen_pixel_count = screen_width * screen_height;

	std::array<uint8_t, 16> V;			// data registers

	uint8_t delay_timer = 0;			//timers
	uint8_t sound_timer = 0;

	uint16_t pc = 0x200;		// program counter
	uint16_t I = 0;						// address register
	uint16_t sp;						// stack ptr

	std::array<uint16_t, 16> stack;
	std::array<uint8_t, 0x1000> ram;
	std::array<uint8_t, screen_pixel_count> frame_buffer;
	std::array<uint32_t, screen_pixel_count> pixel_buffer;

	std::array<uint8_t, 16> keys;

	Opcode cur_opcode;

	bool should_redraw;

	std::array<bool, 0x10> keypad{};

public:
	CPU() {};
	~CPU() {};

	void read_rom(const std::string& program_name);
	void init();

	Opcode fetch();
	void cycle();
	void draw(SDL_Texture* tex, SDL_Renderer* renderer);
	void input();

	void update_timers();

	inline void CLS() { //00E0
		std::fill(frame_buffer.begin(), frame_buffer.end(), 0);
		should_redraw = true;
	}

	inline void RET() { //00EE
		sp--;
		pc = stack[sp];
	}

	inline void JMP() { //1NNN
		pc = cur_opcode.nnn();
	}

	inline void CALL() { //2NNN
		stack[sp] = pc;
		sp++;
		pc = cur_opcode.nnn();
	}

	inline void SE_Vx() { //3Xkk
		if (V[cur_opcode.x()] == cur_opcode.kk())
			pc += 2;
	}

	inline void SNE_Vx() { //4xk
		if (V[cur_opcode.x()] != cur_opcode.kk())
			pc += 2;
	}

	inline void SE_Vx_Vy() { //5xy0 
		if (V[cur_opcode.x()] == V[cur_opcode.y()])
			pc += 2;
	}

	inline void LD_Vx() { //6Xkk
		V[cur_opcode.x()] = cur_opcode.kk();
	}

	inline void ADD_Vx() { //7xkk 
		V[cur_opcode.x()] += cur_opcode.kk();
	}

	inline void LD_Vx_Vy() { //8xy0 
		V[cur_opcode.x()] = V[cur_opcode.y()];
		V[0xF] = 0;
	}

	inline void OR_Vx_Vy() { //8xy1 
		V[cur_opcode.x()] |= V[cur_opcode.y()];
		V[0xF] = 0;
	}

	inline void AND_Vx_Vy() { //8xy2
		V[cur_opcode.x()] &= V[cur_opcode.y()];
		V[0xF] = 0;
	}

	inline void XOR_Vx_Vy() { //8xy3
		V[cur_opcode.x()] ^= V[cur_opcode.y()];
		V[0xF] = 0;
	}

	inline void ADD_Vx_Vy() { //8xy4	
		int sum = V[cur_opcode.x()] + V[cur_opcode.y()];

		V[cur_opcode.x()] = sum & 0xFF;

		V[0xF] = sum > 0xFF;
	}

	inline void SUB_Vx_Vy() { //8xy5
		bool comp = V[cur_opcode.y()] < V[cur_opcode.x()];

		V[cur_opcode.x()] = V[cur_opcode.x()] - V[cur_opcode.y()];
		V[0xF] = comp;
	}

	//TODO: IMPLEMENT SHIFT QUIRK OPTION
	inline void SHR_Vx_Vy() { //8xy6
		uint8_t Vf = V[cur_opcode.x()] & 1;

		V[cur_opcode.x()] = V[cur_opcode.y()];
		V[cur_opcode.x()] >>= 1;

		V[0xF] = Vf;
	}

	inline void SUBN_Vx_Vy() { //8xy7
		bool comp = V[cur_opcode.y()] > V[cur_opcode.x()];

		V[cur_opcode.x()] = V[cur_opcode.y()] - V[cur_opcode.x()];
		V[0xF] = comp;
	}

	//TODO: IMPLEMENT SHIFT QUIRK OPTION
	inline void SHL_Vx_Vy() { //8xyE
		uint8_t Vf = V[cur_opcode.x()] >> 7;

		V[cur_opcode.x()] = V[cur_opcode.y()];
		V[cur_opcode.x()] <<= 1;

		V[0xF] = Vf;
	}

	inline void SNE_Vx_Vy() { //9xy0 
		if (V[cur_opcode.x()] != V[cur_opcode.y()]) {
			pc += 2;
		}
	}

	inline void LD_I() { //Annn 
		I = cur_opcode.nnn();
	}

	inline void JP_V0() { //Bnnn 
		pc = cur_opcode.nnn() + V[0];
	}

	inline void RND_Vx() { //Cxkk 
		V[cur_opcode.x()] = rand() % 256 & cur_opcode.kk();
	}

	inline void DRW_Vx_Vy() { //Dxyn 
		const int width = 8;
		int height = cur_opcode.n();

		uint8_t vx = V[cur_opcode.x()] % screen_width;
		uint8_t vy = V[cur_opcode.y()] % screen_height;
		uint8_t pixel;

		V[0xF] = 0;

		for (int y_coord = 0; y_coord < height; y_coord++) {
			pixel = ram[I + y_coord];

			auto row = (y_coord + vy);// % screen_height;
			if (row >= screen_height)
				break;

			for (int x_coord = 0; x_coord < width; x_coord++) {

				auto col = (x_coord + vx);// % screen_width;
				if (col >= screen_width)
					continue;

				if ((pixel & (0x80 >> x_coord)) > 0) {
					auto screen_pos = row
						* screen_width
						+ col;

					if (frame_buffer[screen_pos]) {
						V[0xF] = 1;
					}

					frame_buffer[screen_pos] ^= 1;
				}
			}
		}

		should_redraw = true;
	}

	inline void SKP_Vx() { //Ex9E 
		if (keypad[V[cur_opcode.x()]])
			pc += 2;
	}

	inline void SKNP_Vx() { //ExA1
		if (!keypad[V[cur_opcode.x()]])
			pc += 2;
	}

	inline void LD_Vx_DT() { //Fx07 
		V[cur_opcode.x()] = delay_timer;
	}

	inline void LD_Vx_K() { //Fx0A 
		pc -= 2;

		for (auto i = 0; i < 0x0F; ++i) {
			if (keypad[V[cur_opcode.x()]]) {
				V[cur_opcode.x()] = i;
				pc += 2;
			}
		}
	}

	inline void LD_DT_Vx() { //Fx15 
		delay_timer = V[cur_opcode.x()];
	}

	inline void LD_ST_Vx() { //Fx18 
		sound_timer = V[cur_opcode.x()];
	}

	inline void ADD_I_Vx() { //Fx1E 
		I += V[cur_opcode.x()];
	}

	inline void LD_F_Vx() { //Fx29 
		I = V[cur_opcode.x()] * 5;
	}

	inline void LD_B_Vx() { //Fx33 
		ram[I] = V[cur_opcode.x()] / 100;
		ram[I + 1] = (V[(cur_opcode.x())] / 10) % 10;
		ram[I + 2] = V[cur_opcode.x()] % 10;
	}

	inline void LD_I_Vx() { //Fx55 
		for (int i = 0; i <= cur_opcode.x(); i++) {
			ram[I + i] = V[i];
		}

		I += cur_opcode.x() + 1;
	}

	inline void LD_Vx_I() { //Fx65 
		for (int i = 0; i <= cur_opcode.x(); i++) {
			V[i] = ram[I + i];
		}

		I += cur_opcode.x() + 1;
	}
};