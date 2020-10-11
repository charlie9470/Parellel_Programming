#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <mpi.h>
void swap(float* a,float* b){
	float temp = *a;
	*a = *b;
	*b = temp;
}
int main(int argc, char** argv) {
	/*Attempt 1
	 * split the sorting job to each mpi_objects.
	 * To prevent write-read conflict, only allow the sorting of even pairs after odd pairs sort is complete
	 * And vice versa.
	 * Which means there are at most only num/2 mpi are able to run simultaneously,
	 * Should be ways to improve throughput.
	 * */
	/* Problem *** Even outside of MPI_Init It's individual So the table doesn't work ****/
	/*Fixed Attempt 1
	 * Be a good boy, ask from file everytime and write from file.
	 * */
	/*Attemp 2
	 * Making rank 0 Master node who sends data and asks them to sort for it.
	 * */

	int num;//size of testcase
	sscanf(argv[1], "%d", &num);
	int numP = num-1;//size of pairs should be num_OP + num_OE
	int num_OP = floor(num/2);//num of odd pairs
	int num_OE = floor((num-1)/2);//num of even pairs

	int num_SO = 0;//num of sorted pairs in the round
	int num_SE = 0;//num of sorted pairs in the round

	/*
	bool E_O = false;//epoch of odd
	bool E_E = false;//epoch of even
	*/
	bool so_O = 1;//0 or 1. 0 means sorting even pairs, 1 means sorting odd pairs.
//	int index_O = 0;//points to the index of the sorted pair ex. (4,5) is to be sorted index_O = 4;
//	int index_E = 0;//points to the index of the sorted pair ex. (5,6) is to be sorted index_E = 5;

	bool done = false;//if true return
	float * ans = (float*)malloc(sizeof(float)*num);
	printf("Num: %d\n",num);
	int test_int = 0;

	MPI_Init(&argc,&argv);
	int rank, size;
	int round = 0;
	int index;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_File f;
	MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &f);/*
	MPI_File_read_all_begin(f, ans, num, MPI_FLOAT);
	if(rank == 0){
		printf("list:");
		for(int i = 0;i<num;i++){
			printf("%f ",ans[i]);
		}
		printf("\n");
	}
	printf("TESTINT is %d\n",test_int+=rank);*/
	//case A:size < max(num_OP,num_OE)
	//case B:size = max(num_OP,num_OE)
	//case C:size > min(num_OP,num_OE) //meaningless
	while(1){
		if(rank == 0&& round == 0){
			printf("list before odd/even pairs sorted:");
			for(int i = 0;i<num;i++){
				printf("%f ",ans[i]);
			}
			printf("\n");
		}
		MPI_Barrier(MPI_COMM_WORLD);
//		printf("Rank %d began round %d\n", rank, round);
//		if(rank == 0) printf("ROUND %d, OE:%d\n",round,so_O);
		if(so_O){//sort odd pairs
			//0 2 4 6 8 10
			index = 2 * round * size + 2 * rank;
			if(index+1>numP) {
				round = 0;
				continue;
			}
			printf("rank %d gets (%f,%f) from (%d,%d)\n",rank,ans[index],ans[index+1],index,index+1);
			if(ans[index]>ans[index+1]){
				swap(&ans[index],&ans[index+1]);
				printf("Swap!\nrank %d swapped(%d,%d)\n",rank,index,index+1);
				done = false;
			}
			if(index==numP||index==numP-1){
				so_O = false;
				printf("Start sorting even pairs\n");
				round = 0;
				continue;
			}
		}
		else{//sort even pairs
			//1 3 5 7 9 11
			index = 2 * round * size + 2 * rank + 1;
			if(index+1>numP) {
				round = 0;
				continue;
			}
			printf("rank %d gets (%f,%f) from (%d,%d)\n",rank,ans[index],ans[index+1],index,index+1);
			if(ans[index]>ans[index+1]){
				swap(&ans[index],&ans[index+1]);
				printf("Swap!\nrank %d swapped(%d,%d)\n",rank,index,index+1);
				done = false;
			}
			if(index==numP||index==numP-1){
				if(done) break;
				so_O = true;
				done = true;
				printf("Start sorting odd pairs\n");
				round = 0;
				continue;
			}
		}
		round++;
	}
	MPI_File_sync(f);
	MPI_File out_file;
	MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_WRONLY, MPI_INFO_NULL, &out_file);
	MPI_File_write_all(out_file, &ans, num, MPI_FLOAT, MPI_STATUS_IGNORE);
	MPI_File_close(&f);
	MPI_File_close(&out_file);
	MPI_Finalize();*/
	return 0;
 }
