
# we need -O3
CXXFLAGS += -Wall -Wextra -pthread -fopenmp -g3 -DNDEBUG

.phony: all wsp release

all: release

release: wsp.c
	g++ wsp.c -o wsp $(CXXFLAGS)

clean:
	rm -f ./wsp
	rm -f ./wsp
