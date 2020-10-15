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

testcase = 03
output = ans.txt
NumofMach = 4
Numofthreads = 28
Num = 21
get_test:
	od -tfF testcases/$(testcase).in
get_ans:
	od -tfF testcases/$(testcase).out
get_My:
	od -tfF $(output)
compare:
	diff testcases/$(testcase).out $(output)
run:
	srun -N $(NumofMach) -n $(Numofthreads) ./hw1 $(Num) testcases/$(testcase).in $(output)
recover:
	cp ../../share/hw1/testcases/$(testcase).in testcases
