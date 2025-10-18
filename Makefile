CC := CC
CXXFLAGS := -c -std=c++17 -Wall -Wextra -O3
LFLAGS := -lstdc++

DEFINES := -DDELTA=10 -DEDGE_CLASSIFICATION=true -DHYBRIDIZATION=true -DTAU=0.8

ALL := sssp

all: $(ALL)

clean:
	rm -f *.o $(ALL)

sssp: sssp.o graph.o runner.o
	$(CC) $(LFLAGS) -o $@ $^

%.o: %.cpp graph.h runner.h
	$(CC) $(CXXFLAGS) $(DEFINES) $< -o $@