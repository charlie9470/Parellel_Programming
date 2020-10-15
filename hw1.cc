#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <mpi.h>
#define DEBUG_MODE 0
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
	float *ans;
	ans = (float*)malloc(sizeof(float) * num);
//	//printf("Num: %d\n",num);

	MPI_Init(&argc,&argv);
	int rank, size;
	int round = 0;
	int index;
	int change = 0;
	int t_change;
	float *swapped;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if(size!=1){
	MPI_File f;
	MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &f);
	MPI_File_read_all(f, ans, num, MPI_FLOAT, MPI_STATUS_IGNORE);
	MPI_File_close(&f);
	//case A:size < max(num_OP,num_OE)
	//case B:size = max(num_OP,num_OE)
	//case C:size > min(num_OP,num_OE) //meaningless
	while(1){
		change = 0;
		t_change = 0;
		//Sort Odd Pairs
		int *send_cnt;
		int *displs;
		if(rank == 0){
			send_cnt = (int*)malloc(sizeof(int) * size);
			displs = (int*)malloc(sizeof(int) * size);
			int dist_num = num;
			int spread = 2 * (num / (2 * size));
			for(int i = 0;i < size;i++){
				send_cnt[i] = spread;
			}
			dist_num -= spread * size;
			for(int i = 0;dist_num >= 2;i++){
				send_cnt[i] += 2;
				dist_num -= 2;
			}
			displs[0] = 0;
			for(int i = 1;i < size;i++){
				displs[i] = displs[i-1] + send_cnt[i-1];
			}
			if(DEBUG_MODE){
			for(int i = 0;i<size;i++){
				printf("%d,", send_cnt[i]);
			}
			printf("\n");
			for(int i = 0;i<size;i++){
				printf("%d,", displs[i]);
			}
			printf("\n");
			}
		}
		int recv_num = 2 * (num / (2 * size));
		if(2 * rank + 1 < num % (2 * size)) recv_num+=2;
		swapped = (float*)malloc(sizeof(float) * recv_num);
		MPI_Scatterv(ans, send_cnt, displs, MPI_FLOAT, swapped, recv_num, MPI_FLOAT, 0, MPI_COMM_WORLD);
		for(int i = 0;i<recv_num;i+=2){
			if(swapped[i] > swapped[i+1]){
				swap(&swapped[i],&swapped[i+1]);
				change++;
				//printf("Swapped!!!\n");
			}
		}
		MPI_Gatherv(swapped, recv_num, MPI_FLOAT, ans, send_cnt, displs, MPI_FLOAT, 0, MPI_COMM_WORLD);
		if(DEBUG_MODE){
		if(rank == 0){
			printf("ANS after odd sort:----------------------------------\n");
			for(int i = 0;i<num;i++){
				printf("%f ",ans[i]);
			}
			printf("\n");
			printf("----------------------------------\n");
		}
		}
		//Sort Even Pairs
		if(rank == 0){
			send_cnt = (int*)malloc(sizeof(int) * size);
			displs = (int*)malloc(sizeof(int) * size);
			int dist_num/*num distributing to send_cnt*/ = num - 1;
			int spread = 2 * ((num-1) / (2 * size));
			for(int i = 0;i < size;i++){
				send_cnt[i] = spread;
			}
			dist_num -= spread * size;
			for(int i = 0;dist_num >= 2;i++){
				send_cnt[i] += 2;
				dist_num -= 2;
			}
			displs[0] = 1;
			
			for(int i = 1;i < size;i++){
				displs[i] = displs[i-1] + send_cnt[i-1];
			}
			if(DEBUG_MODE){
			for(int i = 0;i<size;i++){
				printf("%d,", send_cnt[i]);
			}
			printf("\n");
			for(int i = 0;i<size;i++){
				printf("%d,", displs[i]);
			}
			printf("\n");
			}
		}
		recv_num = 2 * ((num-1) / (2 * size));
		if(2 * rank + 1 < (num - 1) % (2 * size)) recv_num+=2;
//		MPI_Barrier(MPI_COMM_WORLD);
		//printf("rank %d received %d nums\n", rank, recv_num);
		swapped = (float*)malloc(sizeof(float) * recv_num);
		MPI_Scatterv(ans, send_cnt, displs, MPI_FLOAT, swapped, recv_num, MPI_FLOAT, 0, MPI_COMM_WORLD);
		for(int i = 0;i<recv_num;i+=2){
			if(swapped[i] > swapped[i+1]){
				swap(&swapped[i],&swapped[i+1]);
				change++;
				//printf("Swapped!!!\n");
			}
		}
		MPI_Gatherv(swapped, recv_num, MPI_FLOAT, ans, send_cnt, displs, MPI_FLOAT, 0, MPI_COMM_WORLD);

		MPI_Allreduce(&change, &t_change, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
		if(DEBUG_MODE){
		if(rank == 0){
			printf("ANS after even sort:----------------------------------\n");
			for(int i = 0;i<num;i++){
				printf("%f ",ans[i]);
			}
			printf("\n");
			printf("----------------------------------\n");
		}
		}
		if(t_change == 0) break;
	}
//	MPI_Barrier(MPI_COMM_WORLD);
//	//printf("Sorted and stored in ans\n");
	MPI_File_close(&f);
	MPI_File out_file;
	int err = MPI_File_open(MPI_COMM_WORLD, argv[3], MPI_MODE_CREATE | MPI_MODE_EXCL | MPI_MODE_WRONLY, MPI_INFO_NULL, &out_file);
	if(err != MPI_SUCCESS){
		MPI_File_open(MPI_COMM_WORLD, argv[3], MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &out_file);
		MPI_File_close(&out_file);
		MPI_File_open(MPI_COMM_WORLD, argv[3], MPI_MODE_CREATE, MPI_INFO_NULL, &out_file);
	}
	if(rank == 0){
		MPI_File_write(out_file, ans, num, MPI_FLOAT, MPI_STATUS_IGNORE);
	}
	MPI_File_close(&out_file);
	
	if(rank == 0){
		printf("ANS:----------------------------------\n");
		for(int i = 0;i<num;i++){
			printf("%.2f ",ans[i]);
			if((i!=0&&i%6==0)|i==num-1) printf("\n");
		}
		printf("----------------------------------\n");
	}
	
	MPI_Finalize();
	}
	else{
		MPI_File f;
		MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &f);
		MPI_File_read_all(f, ans, num, MPI_FLOAT, MPI_STATUS_IGNORE);
		MPI_File_close(&f);
		bool sorted = false;
		while(!sorted){
			sorted = true;
			for(int i = 0;i < num-1;i+=2){
				if(ans[i] > ans[i+1]){
					swap(&ans[i],&ans[i+1]);
					sorted = false;
				}
			}
			for(int i = 1;i < num-1;i+=2){
				if(ans[i] > ans[i+1]){
					swap(&ans[i],&ans[i+1]);
					sorted = false;
				}
			}
		}
		MPI_File out_file;
		int err = MPI_File_open(MPI_COMM_WORLD, argv[3], MPI_MODE_CREATE | MPI_MODE_EXCL | MPI_MODE_WRONLY, MPI_INFO_NULL, &out_file);
		if(err != MPI_SUCCESS){
			MPI_File_open(MPI_COMM_WORLD, argv[3], MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &out_file);
			MPI_File_close(&out_file);
			MPI_File_open(MPI_COMM_WORLD, argv[3], MPI_MODE_CREATE, MPI_INFO_NULL, &out_file);
		}
		MPI_File_write_all(out_file, ans, num, MPI_FLOAT, MPI_STATUS_IGNORE);
		MPI_File_close(&out_file);
		if(DEBUG_MODE){
		printf("ANS:----------------------------------\n");
		for(int i = 0;i<num;i++){
			printf("%.2f ",ans[i]);
			if((i!=0&&i%6==0)|i==num-1) printf("\n");
		}
		printf("----------------------------------\n");
		}
		MPI_Finalize();
	}
	return 0;
 }
