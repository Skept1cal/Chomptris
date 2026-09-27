tetris.elf: tetris.c tetris.h highscore.c highscore.h
	gcc tetris.c vector/vector.c highscore.c -o tetris.elf -g -Wall