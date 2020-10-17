date_cheater.so: date_cheater.c
	gcc -W -Wall -fPIC date_cheater.c -ldl -shared -Wl,-soname,date_cheater.so -o date_cheater.so

default: date_cheater.so
clean: 
	rm *.so