CC = mpicc
CXX = mpicxx
CXXFLAGS = -O3 -lm
CFLAGS = -O3 -lm
TARGETS = hw1

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS)
get_test:
	od -tfF ../testcases/03.in
run:
	srun -N 1 -n 6 ./hw1 21 ../testcases/03.in ans.txt
