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

testcase = 38
output = ans.txt
NumofNode = 3
Numofthreads = 12
Num = 536831999
get_test:
	od -tfF testcases/$(testcase).in
get_ans:
	od -tfF testcases/$(testcase).out
get_My:
	od -tfF $(output)
compare:
	diff testcases/$(testcase).out $(output)
run:
	srun -N$(NumofNode) -n$(Numofthreads) ./hw1 $(Num) testcases/$(testcase).in $(output) Experiment.txt
	srun -N 1 -n 1 ./hw1 $(Num) testcases/$(testcase).in $(output) Result/$(testcase)_Ver2_1.txt
	srun -N 4 -n 4 ./hw1 $(Num) testcases/$(testcase).in $(output) Result/$(testcase)_Ver2_4.txt
	srun -N 4 -n 16 ./hw1 $(Num) testcases/$(testcase).in $(output) Result/$(testcase)_Ver2_16.txt
run_exp:
	srun -N 4 -n 48  ./hw1 $(Num) testcases/$(testcase).in $(output) Result/$(testcase)_Ver2_48.txt
recover:
	cp ../../share/hw1/testcases/$(testcase).in testcases
clean_job:
	scancel -u pp20s53
