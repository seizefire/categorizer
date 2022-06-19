default: categorizer.exe

categorizer.exe:
	gcc -mwindows src/main.c -o categorizer.exe
