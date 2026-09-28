clear:
	rm -rf tetris.exe tetris.elf highscores.txt hs.txt

src:
	gcc tetris.c vector/vector.c highscore.c -o tetris.elf -g -Wall -lm -Wno-unused-function -Wno-format
	x86_64-w64-mingw32-gcc tetris.c vector/vector.c highscore.c -o tetris.exe -g -Wall -lm -lwinmm -Wno-unused-function -Wno-format

stable:
	gcc tetris.c vector/vector.c highscore.c -o tetris.elf -s -static -w -lm
	x86_64-w64-mingw32-gcc tetris.c vector/vector.c highscore.c -o tetris.exe -s -static -w -lm -lwinmm

pack: stable
	cd ../prep && make && ./prep.elf && cd ../src