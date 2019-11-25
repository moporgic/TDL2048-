all:
	g++ -std=c++1y -mtune=native -mpopcnt -mbmi2 -O3 -lpthread -Wall -fmessage-length=0 -o 2048 2048.cpp
noshm:
	g++ -std=c++1y -mtune=native -mpopcnt -mbmi2 -O3 -lpthread -DNOSHM -Wall -fmessage-length=0 -o 2048 2048.cpp
native:
	g++ -std=c++1y -march=native -mpopcnt -mbmi2 -O3 -lpthread -Wall -fmessage-length=0 -o 2048 2048.cpp
dump:
	g++ -std=c++1y -mtune=native -mpopcnt -mbmi2 -O3 -lpthread -Wall -fmessage-length=0 -g -o 2048 2048.cpp
	objdump -S 2048 > 2048.dump
static:
	g++ -std=c++1y -static -mpopcnt -mbmi2 -O3 -lpthread -Wall -fmessage-length=0 -o 2048 2048.cpp
clean:
	rm 2048 || rm 2048.exe ||:
