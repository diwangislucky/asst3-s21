
# we need -O3
CXXFLAGS += -Wall -Wextra -pthread -fopenmp -g3 -DNDEBUG

.phony: all experiment release

all: release

release: experiment.c
	g++ experiment.c -o experiment $(CXXFLAGS)

clean:
	rm -f ./wsp
	rm -f ./wsp
