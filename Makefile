clear:
	rm -rf tetris.exe tetris.elf highscores.txt hs.txt

src:
	gcc -DINCLUDE_HVEC -DINCLUDE_SVEC tetris.c vector/vector.c highscore.c -o tetris.elf -g -Wall -lm -Wno-unused-function -Wno-format
	x86_64-w64-mingw32-gcc -DINCLUDE_HVEC -DINCLUDE_SVEC tetris.c vector/vector.c highscore.c -o tetris.exe -g -Wall -lm -lwinmm -Wno-unused-function -Wno-format

stable:
	gcc -DINCLUDE_HVEC -DINCLUDE_SVEC tetris.c vector/vector.c highscore.c -o tetris.elf -s -static -w -lm -O3
	x86_64-w64-mingw32-gcc -DINCLUDE_HVEC -DINCLUDE_SVEC tetris.c vector/vector.c highscore.c -o tetris.exe -s -static -w -lm -lwinmm -O3

pack: stable
	cd ../prep && make && ./prep.elf && cd ../src