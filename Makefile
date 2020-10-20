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

testcase = 33
output = ans.txt
NumofMach = 1
Numofthreads = 12
Num = 536869888
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
