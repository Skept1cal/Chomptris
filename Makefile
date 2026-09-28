clear:
	rm -rf tetris.exe tetris.elf highscores.txt hs.txt

src:
	gcc tetris.c vector/vector.c highscore.c -o tetris.elf -g -Wall -Wno-unused-function -Wno-format
	x86_64-w64-mingw32-gcc tetris.c vector/vector.c highscore.c -o tetris.exe -g -Wall -lwinmm -Wno-unused-function -Wno-format

stable:
	gcc tetris.c vector/vector.c highscore.c -o tetris.elf -s -static -w
	x86_64-w64-mingw32-gcc tetris.c vector/vector.c highscore.c -o tetris.exe -s -static -w -lwinmm

pack: stable
	cd ../prep && make && ./prep.elf && cd ../src