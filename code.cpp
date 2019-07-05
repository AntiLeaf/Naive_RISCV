#include <cstdio>
#include <cstdlib>

#define MAX_LEN 0x20000 // 131072

// #define OUTPUT

void write_bin(int x, int c) {
	for (int i = c - 1; ~i; i--)
		printf("%d", x >> i & 1);
}

namespace i_o {

	int get_8(const unsigned char *s) {
		return (int)(char)*s;
	}

	int get_u8(const unsigned char *s) {
		return (int)*s;
	}

	int get_16(const unsigned char *s) {
		return (int)(*s | (int)(char)*(s + 1) << 8);
	}

	int get_u16(const unsigned char *s) {
		return (int)(*s | ((unsigned)*(s + 1) << 8));
	}

	int get_32(const unsigned char *s) {
		return (int)(*s | ((int)*(s + 1) << 8) | ((int)*(s + 2) << 16) | ((int)*(s + 3) << 24));
	}

	void write_8(unsigned char *s, int x) {
		*s = x & 255;
	}

	void write_16(unsigned char *s, int x) {
		*s = x & 255;
		*(s + 1) = (x & (65535 ^ 255)) >> 16;
	}

	void write_32(unsigned char *s, int x) {
		*s = x & 255;
		*(s + 1) = (x & (65535 ^ 255)) >> 8;
		*(s + 2) = (x & (16777215 ^ 65535)) >> 16;
		*(s + 3) = (x & ((-1) ^ 16777215)) >> 24;
	}

}
using namespace i_o;

namespace integer_handling {

	inline int get_high(int x, int l) {
		// x &= -(1 << l);
		return x >> l;
	}

	inline int get_low(int x, int r) {
		return x & ((1 << (r + 1)) - 1);
	}

	inline int get_range(int x, int l, int r) {
		return get_low(get_high(x, l), r - l);
	}

	inline int get_bit(int x, int t) {
		return (x >> t) & 1;
	}

	inline int get_highest_bit(int x, int t) {
		return ((x >> t) & 1) ? -1 : 0;
	}
}
using namespace integer_handling;

namespace operation_translating {
	
	int get_opcode(int x) {
		return get_low(x, 6);
	}

	int get_rs1(int x) {
		return get_range(x, 15, 19);
	}
	
	int get_rs2(int x) {
		return get_range(x, 20, 24);
	}

	int get_funct3(int x) {
		return get_range(x, 12, 14);
	}

	int get_rd(int x) {
		return get_range(x, 7, 11);
	}

	int get_inst_i(int x) {
		return get_high(x, 20);
	}

	int get_inst_s(int x) {
		return (get_high(x, 25) << 5) | get_range(x, 7, 11);
	}

	int get_inst_b(int x) {
		return (get_range(x, 8, 11) << 1) | (get_range(x, 25, 30) << 5) |
			(get_bit(x, 7) << 11) | (get_highest_bit(x, 31) << 12);
	}

	int get_inst_u(int x) {
		return x & - (1 << 12);
	}

	int get_inst_j(int x) {
		return (get_range(x, 21, 30) << 1) | (get_bit(x, 20) << 11) |
			(get_range(x, 12, 19) << 12) | (get_highest_bit(x, 31) << 20);
	}

}
using namespace operation_translating;

unsigned char RAM[MAX_LEN];

int reg[33];
const int pc = 32;

void execute_r(int x) {
	//ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
	int funct7 = get_high(x, 25), rs1 = get_rs1(x), rs2 = get_rs2(x), funct3 = get_funct3(x), rd = get_rd(x), opcode = get_opcode(x);

#ifdef OUTPUT
	printf("funct7 = ");write_bin(funct7, 7);
	printf(" rs1 = %d rs2 = %d funct3 = ", rs1, rs2);
	write_bin(funct3, 3);
	printf(" rd = %d reg[rs1] = %d reg[rs2] = %d opcode = ", rd, reg[rs1], reg[rs2]);write_bin(opcode, 7);printf(" ");
#endif
	
	if (funct3 == 0b000) { // ADD or SUB
		if (!get_bit(x, 30)) {// ADD
#ifdef OUTPUT
			printf("ADD\n");
#endif
			reg[rd] = reg[rs1] + reg[rs2];
		}
		else {// SUB
#ifdef OUTPUT
			printf("SUB\n");
#endif
			reg[rd] = reg[rs1] - reg[rs2];
		}
	}

	else if (funct3 == 0b001) { //SLL
#ifdef OUTPUT
		printf("SLL\n");
#endif
		reg[rd] = reg[rs1] << (reg[rs2] & 31);
	}

	else if (funct3 == 0b010) { // SLT
#ifdef OUTPUT
		printf("SLT\n");
#endif
		reg[rd] = (int)(reg[rs1] < reg[rs2]);
	}
	
	else if (funct3 == 0b011) { // SLTU
#ifdef OUTPUT
		printf("SLTU\n");
#endif
		reg[rd] = (int)((unsigned)reg[rs1] < (unsigned)reg[rs2]);
	}
	
	else if (funct3 == 0b100) { // XOR
#ifdef OUTPUT
		printf("XOR\n");
#endif
		reg[rd] = reg[rs1] ^ reg[rs2];
	}
	
	else if (funct3 == 0b101) { // SRL or SRA
		if (!get_bit(x, 30)) { // SRL
#ifdef OUTPUT
			printf("SRL\n");
#endif
			reg[rd] = (unsigned)reg[rs1] >> (reg[rs2] & 31);
		}
		
		else { //SRA
#ifdef OUTPUT
			printf("SRA\n");
#endif
			reg[rd] = reg[rs1] >> (reg[rs2] & 31);
		}
	}

	else if (funct3 == 0b110) { // OR
#ifdef OUTPUT
		printf("OR\n");
#endif
		reg[rd] = reg[rs1] | reg[rs2];
	}

	else if (funct3 == 0b111) { // AND
#ifdef OUTPUT
		printf("AND\n");
#endif
		reg[rd] = reg[rs1] & reg[rs2];
	}
	
	else abort();
}

void execute_i(int x) {
	//JALR, LB, LH, LW, LBU, LHU, ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
	int imm = get_inst_i(x), rs1 = get_rs1(x), funct3 = get_funct3(x), rd = get_rd(x), opcode = get_opcode(x);

#ifdef OUTPUT
	printf("imm = %d rs1 = %d funct3 = ", imm, rs1);
	write_bin(funct3, 3);
	printf(" rd = %d opcode = ", rd);write_bin(opcode, 7);printf(" ");
#endif

	if (opcode == 0b1100111) { // JALR
#ifdef OUTPUT
		printf("JALR\n");
#endif
		int tmp = reg[pc] + 4;
		reg[pc] = ((reg[rs1] + imm) & -2) - 4;
		reg[rd] = tmp;
	}

	else {

		if (opcode == 0b0000011) { // LB, LH, LW, LBU, LHU

			if (funct3 == 0b000) { // LB
#ifdef OUTPUT
				printf("LB\n");
#endif
				reg[rd] = get_8(RAM + reg[rs1] + imm);
			}
			
			else if (funct3 == 0b001) { // LH
#ifdef OUTPUT
				printf("LH\n");
#endif
				reg[rd] = get_16(RAM + reg[rs1] + imm);
			}
			
			else if (funct3 == 0b010) { // LW
#ifdef OUTPUT
				printf("LW\n");
#endif
				reg[rd] = get_32(RAM + reg[rs1] + imm);
			}
			
			else if (funct3 == 0b100) { // LBU
#ifdef OUTPUT
				printf("LBU\n");
#endif
				reg[rd] = get_u8(RAM + reg[rs1] + imm);
			}
			
			else if (funct3 == 0b101) { // LHU
#ifdef OUTPUT
				printf("LHU\n");
#endif
				reg[rd] = get_u16(RAM + reg[rs1] + imm);
			}
		}

		else if (opcode == 0b0010011) { // ADDI, SLTI, SLTIU, XORI, ORI, ANDI

			if (funct3 == 0b000) { // ADDI
#ifdef OUTPUT
				printf("ADDI\n");
#endif
				reg[rd] = reg[rs1] + imm;
			}
			
			else if (funct3 == 0b010) { // SLTI
#ifdef OUTPUT
				printf("SLTI\n");
#endif
				reg[rd] = (int)(reg[rs1] < imm);
			}
			
			else if (funct3 == 0b011) { // SLTIU
#ifdef OUTPUT
				printf("SLTIU\n");
#endif
				reg[rd] = (int)((unsigned)reg[rs1] < (unsigned)imm);
			}
			
			else if (funct3 == 0b100) { // XORI
#ifdef OUTPUT
				printf("XORI\n");
#endif
				reg[rd] = reg[rs1] ^ imm;
			}
			
			else if (funct3 == 0b110) { // ORI
#ifdef OUTPUT
				printf("ORI\n");
#endif
				reg[rd] = reg[rs1] | imm;
			}
			
			else if (funct3 == 0b111) { // ANDI
#ifdef OUTPUT
				printf("ANDI\n");
#endif
				reg[rd] = reg[rs1] & imm;
			}
		
			else if (funct3 == 0b001) { // SLLI
#ifdef OUTPUT
				printf("SLLI\n");
#endif
				reg[rd] = reg[rs1] << (imm & 31);
			}
			
			else if (funct3 == 0b101) {// SRLI or SRAI
				if (!get_bit(x, 30)) { // SRLI
#ifdef OUTPUT
					printf("SRLI\n");
#endif
					reg[rd] = (unsigned)reg[rs1] >> (imm & 31);
				}
				
				else { // SRAI
#ifdef OUTPUT
					printf("SRAI\n");
#endif
					reg[rd] = reg[rs1] >> (imm & 31);
				}
			}
			
			else abort();
		}
		
		else abort();
	}
}

void execute_s(int x) {
	//SB, SH, SW
	int imm = get_inst_s(x), rs1 = get_rs1(x), rs2 = get_rs2(x), funct3 = get_funct3(x), opcode = get_opcode(x);

#ifdef OUTPUT
	printf("imm = %d rs1 = %d rs2 = %d funct3 = ", imm, rs1, rs2);
	write_bin(funct3, 3);
	printf(" opcode = ");write_bin(opcode, 7);printf(" ");
	
	printf("reg[rs1] = %d ", reg[rs1]);
#endif
	
	if (funct3 == 0b000) { // SB
#ifdef OUTPUT
		printf("SB\n");
#endif
		write_8(RAM + reg[rs1] + imm, reg[rs2]);
	}
	
	else if (funct3 == 0b001) { // SH
#ifdef OUTPUT
		printf("SH\n");
#endif
		write_16(RAM + reg[rs1] + imm, reg[rs2]);
	}
	
	else if (funct3 == 0b010) { // SW
#ifdef OUTPUT
		printf("SW\n");
#endif
		write_32(RAM + reg[rs1] + imm, reg[rs2]);
	}
	
	else abort();
}

void execute_b(int x) {
	//BEQ, BNE, BLT, BGE, BLTU, BGEU
	int imm = get_inst_b(x), rs1 = get_rs1(x), rs2 = get_rs2(x), funct3 = get_funct3(x), opcode = get_opcode(x);

#ifdef OUTPUT
	printf("imm = %d rs1 = %d rs2 = %d funct3 = ", imm, rs1, rs2);
	write_bin(funct3, 3);
	printf(" opcode = ");write_bin(opcode, 7);printf(" reg[rs1] = %d reg[rs2] = %d ", reg[rs1], reg[rs2]);
#endif

	if (funct3 == 0b000) { // BEQ
#ifdef OUTPUT
		printf("BEQ\n");
#endif
		if (reg[rs1] == reg[rs2])
			reg[pc] += imm - 4;
	}
	
	else if (funct3 == 0b001) { // BNE
#ifdef OUTPUT
		printf("BNE\n");
#endif
		if (reg[rs1] != reg[rs2])
			reg[pc] += imm - 4;
	}
	
	else if (funct3 == 0b100) { // BLT
#ifdef OUTPUT
		printf("BLT\n");
#endif
		if (reg[rs1] < reg[rs2])
			reg[pc] += imm - 4;
	}
	
	else if (funct3 == 0b101) { // BGE
#ifdef OUTPUT
		printf("BGE\n");
#endif
		if (reg[rs1] >= reg[rs2])
			reg[pc] += imm - 4;
	}
	
	else if (funct3 == 0b110) { // BLTU
#ifdef OUTPUT
		printf("BLTU\n");
#endif
		if ((unsigned)reg[rs1] < (unsigned)reg[rs2])
			reg[pc] += imm - 4;
	}
	
	else if (funct3 == 0b111) { // BGEU
#ifdef OUTPUT
		printf("BGEU\n");
#endif
		if ((unsigned)reg[rs1] >= (unsigned)reg[rs2])
			reg[pc] += imm - 4;
	}
	
	else abort();
}

void execute_u(int x) {
	//LUI, AUIPC
	int imm = get_inst_u(x), rd = get_rd(x), opcode = get_opcode(x);

#ifdef OUTPUT
	printf("imm = %d rd = %d opcode = ", imm, rd);write_bin(opcode, 7);printf(" ");
#endif
	
	if (opcode == 0b0110111) { // LUI
#ifdef OUTPUT
		printf("LUI\n");
#endif
		reg[rd] = imm;
	}
	
	else if (opcode == 0b0010111) { // AUIPC
#ifdef OUTPUT
		printf("AUIPC\n");
#endif
		reg[rd] = reg[pc] + imm;
	}
	
	else abort();
	
}

void execute_j(int x) {
	//JAL
	int imm = get_inst_j(x), rd = get_rd(x);
	
#ifdef OUTPUT
	printf("imm = %d rd = %d JAL ", imm, rd);
#endif
	reg[rd] = reg[pc] + 4;
	reg[pc] += imm - 4;
#ifdef OUTPUT
	printf("reg[rd] = %d\n", reg[rd]);
#endif
}

int main() {
	char c;
	do
		c = getchar();
	while (c != EOF && (c == ' ' || c == '\n'));
	
	while (c != EOF) {
		int pos;
		scanf("%x", &pos);
		// pos /= 8;
		
		int x;
		while (scanf("%x", &x) == 1)
			RAM[pos++] = x;
		
		do
			c = getchar();
		while (c != EOF && (c == ' ' || c == '\n'));
	}
	
	reg[0] = reg[pc] = 0;
	for (;;) {
		int command = get_32(RAM + reg[pc]), opcode = get_opcode(command);
		
#ifdef OUTPUT
		printf("pc = 0x%x command = ", reg[pc]);write_bin(command, 32);
		printf("(%d) opcode = ", command);write_bin(opcode, 7);printf("(%d)\n",opcode);
		//printf("command = %d opcode = %d\n", command, opcode);
#endif
		
		if (command == 0x00c68223) {
			printf("%d\n", reg[10] & 255);
			break;
		}
		
		if (opcode == 0b0110011)
			execute_r(command);

		else if (opcode == 0b1100111 || opcode == 0b0000011 || opcode == 0b0010011)
			execute_i(command);

		else if (opcode == 0b0100011)
			execute_s(command);

		else if (opcode == 0b1100011)
			execute_b(command);

		else if (opcode == 0b0110111 || opcode == 0b0010111)
			execute_u(command);

		else if (opcode == 0b1101111)
			execute_j(command);
		
		else {
			printf("Fuck you!\n");
			break;
		}
		
#ifdef OUTPUT
		printf("\n");
#endif
		
		reg[0] = 0;
		reg[pc] += 4;
		
		// getchar();
	}
	return 0;
}