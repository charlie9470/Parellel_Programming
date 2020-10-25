#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <mpi.h>
#include <algorithm>
#define DEBUG_MODE 0
#define PRINT_TIME 1
#define DEBUG_HUGE_TESTCASE 1
#define DETAIL_MODE 0
using namespace std;
void swap(float* a,float* b){
	float temp = *a;
	*a = *b;
	*b = temp;
}
void mergeArr(float *arr1, int n1, float *arr2, int n2, float *arr3){
	int i = 0 , j = 0, k = 0;
	while(i<n1 && j<n2){
		if(arr1[i] < arr2[j]){
			arr3[k++] = arr1[i++];
		}
		else{
			arr3[k++] = arr2[j++];
		}
	}
	while(i < n1){
		arr3[k++] = arr1[i++];
	}
	while(j < n2){
		arr3[k++] = arr2[j++];
	}
}
int main(int argc, char** argv) {
	int num;//size of testcase
	sscanf(argv[1], "%d", &num);
	float *ans;
	ans = (float*)malloc(sizeof(float) * num);
//	printf("Num: %d\n",num);

	MPI_Init(&argc,&argv);
	int rank, size;
	int round = 0;
	int index;
	int change = 0;
	int t_change;
	float *swapped;
	double S_time, E_time, Sort_time, a_T, b_T, c_T, d_T;
	double IO_time = 0, CPU_time = 0, Comm_time = 0;
	double R_IO_time = 0, R_CPU_time = 0, R_Comm_time = 0;
	if(PRINT_TIME) S_time = MPI_Wtime();
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);


	if(PRINT_TIME)a_T = MPI_Wtime();
	MPI_File f;
	MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &f);
	MPI_File_read_all(f, ans, num, MPI_FLOAT, MPI_STATUS_IGNORE);
	MPI_File_close(&f);
	if(PRINT_TIME){b_T = MPI_Wtime();
	IO_time += (b_T-a_T);}
	if(PRINT_TIME)a_T = MPI_Wtime();
	int *send_cnt;
	int *displs;
	int index_no_work = 0;
	send_cnt = (int*)malloc(sizeof(int) * size);
	displs = (int*)malloc(sizeof(int) * size);
	int dist_num = num;
	for(int i = 0;i < size;i++){
		send_cnt[i] = 0;
		displs[i] = 0;
	}
	for(int i = 0;i < size && dist_num != 0;i++){
		send_cnt[i]=2;
		dist_num-=2;
		if(dist_num==1){
			if(i+1 < size) send_cnt[i+1] = 1;
			else send_cnt[i]++;
			dist_num--;
			index_no_work = i+2;
		}
		else index_no_work = i+1;
	}
	if(DEBUG_MODE)printf("rank %d: Index_no_work: %d\n", rank, index_no_work);
	for(int i = 0;dist_num>0;i++){
		if(i == size) i = 0;
		send_cnt[i]++;
		dist_num--;
	}
	displs[0] = 0;
	for(int i = 1;i < size;i++){
		displs[i] = displs[i-1] + send_cnt[i-1];
	}
	if(DEBUG_MODE){
	if(rank == size-1){
		printf("send_cnt: ");
		for(int i = 0;i<size;i++){
			printf("%d,", send_cnt[i]);
		}
		printf("\n");
		printf("displs: ");
		for(int i = 0;i<size;i++){
			printf("%d,", displs[i]);
		}
		printf("\n");
	}
	}
	int recv_num = send_cnt[rank];
	swapped = (float*)malloc(sizeof(float) * recv_num);
	if(PRINT_TIME)c_T = MPI_Wtime();
	MPI_Scatterv(ans, send_cnt, displs, MPI_FLOAT, swapped, recv_num, MPI_FLOAT, 0, MPI_COMM_WORLD);
	if(PRINT_TIME){d_T = MPI_Wtime();
	Comm_time += (d_T-c_T);}
	//
	//
	int odd_rank;
	int even_rank;
	int recv_odd;
	int recv_even;
	if(rank%2){
		odd_rank = rank - 1;
		even_rank = rank + 1;
	}
	else{
		odd_rank = rank + 1;
		even_rank = rank - 1;
	}
	if(rank==index_no_work-1&&index_no_work%2)recv_odd = 0;
	else recv_odd = send_cnt[odd_rank];
	if(rank!=0)recv_even = send_cnt[even_rank];
	MPI_Barrier(MPI_COMM_WORLD);
	float *send_buf;
	float *recv_buf;
	float *NEW;
	send_buf = (float*)malloc(sizeof(float)*recv_num);
	recv_buf = (float*)malloc(sizeof(float)*recv_num);
	NEW = (float*)malloc(sizeof(float)*(2*recv_num+5));
	
	sort(swapped, swapped + recv_num);
	if(DEBUG_MODE)printf("Local sort done\n");

	while(1){
		change = 0;
		t_change = 0;
		//Odd phase
		MPI_Status status;
		if(rank < index_no_work&&!(index_no_work%2&&rank==index_no_work-1)){
			if(DEBUG_MODE) printf("Rank %d tries to commute with Rank %d\n", rank, odd_rank);
			//Send all elements to odd rank
			for(int i = 0;i<recv_num;i++){
				send_buf[i] = swapped[i];
			}
			//Send and recv
			if(PRINT_TIME) c_T = MPI_Wtime();
			MPI_Sendrecv(send_buf, recv_num, MPI_FLOAT, odd_rank, 1, recv_buf, recv_odd, MPI_FLOAT, odd_rank, 1, MPI_COMM_WORLD, &status);
			if(PRINT_TIME){d_T = MPI_Wtime();
			Comm_time += (d_T-c_T);}
			if(DEBUG_MODE) printf("Rank %d finished tag 1\n", rank);
			//
			//sort the two arrays
			mergeArr(swapped, recv_num, recv_buf, recv_odd, NEW);
			//
			//put the second array back
			if(rank%2){
				for(int i = 0;i<recv_num;i++){
					if(swapped[i]!=NEW[i+recv_odd]){
						swapped[i] = NEW[i+recv_odd];
						change++;
					}
				}
			}
			//
			//put the first array back
			else{
				for(int i = 0;i<recv_num;i++){
					if(NEW[i]!=swapped[i]){
						swapped[i] = NEW[i];
						change++;
					}
				}
			}/*
			if(DEBUG_MODE&&!DEBUG_HUGE_TESTCASE){
				printf("Rank %d after odd sort\n",rank);
				for(int i=0;i<recv_num;i++){
					printf("%f,", swapped[i]);
				}
				printf("\n---------------------\n");
			}*/
			if(DEBUG_MODE){
				printf("Rank %d done communicating with Rank %d\n", rank, odd_rank);
			}
		}

		MPI_Barrier(MPI_COMM_WORLD);

		if(DEBUG_MODE&&rank == size-1) printf("odd phase done\n");
		//Even phase
		if(rank != 0 && rank < index_no_work&&!(rank == index_no_work-1&&(index_no_work-1)%2)){
			if(DEBUG_MODE) printf("Rank %d tries to commute with Rank %d\n", rank, even_rank);
			//
			//Send the array to Even rank
			for(int i = 0;i < recv_num;i++){
				send_buf[i] = swapped[i];
			}

			if(PRINT_TIME)c_T = MPI_Wtime();
			MPI_Sendrecv(send_buf, recv_num, MPI_FLOAT, even_rank, 1, recv_buf, recv_even, MPI_FLOAT, even_rank, 1, MPI_COMM_WORLD, &status);
			if(PRINT_TIME){d_T = MPI_Wtime();
			Comm_time += (d_T-c_T);}
			if(DEBUG_MODE) printf("Rank %d finished tag 1\n", rank);
			//
			//Sort the two arrays
			mergeArr(swapped, recv_num, recv_buf, recv_even, NEW);
			//
			//Put the first array back
			if(rank%2){
				for(int i = 0;i < recv_num;i++){
					if(swapped[i]!=NEW[i]){
						swapped[i] = NEW[i];
						change++;
					}
				}
			}
			else{
				for(int i = 0;i < recv_num;i++){
					if(swapped[i]!=NEW[i+recv_even]){
						swapped[i] = NEW[i+recv_even];
						change++;
					}
				}
			}/*
			if(DEBUG_MODE&&!DEBUG_HUGE_TESTCASE){
				printf("Rank %d after even sort\n",rank);
				for(int i=0;i<recv_num;i++){
					printf("%f,", swapped[i]);
				}
				printf("\n---------------------\n");
			}*/
			if(DEBUG_MODE){
				printf("Rank %d done communicating with Rank %d\n", rank, even_rank);
			}
		}

		MPI_Barrier(MPI_COMM_WORLD);

		if(PRINT_TIME)c_T = MPI_Wtime();
		MPI_Allreduce(&change, &t_change, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
		if(PRINT_TIME){d_T = MPI_Wtime();
		Comm_time += (d_T-c_T);}
		if(DEBUG_MODE && rank == size-1) printf("one round complete\n--------------------\n");
//		if(DEBUG_MODE) t_change = 0;
		if(t_change == 0) {
			if(PRINT_TIME) c_T = MPI_Wtime();
			MPI_Gatherv(swapped, recv_num, MPI_FLOAT, ans, send_cnt, displs, MPI_FLOAT, 0, MPI_COMM_WORLD);
			if(PRINT_TIME){d_T = MPI_Wtime();
			Comm_time += (d_T-c_T);}
			if(DEBUG_MODE&&!DEBUG_HUGE_TESTCASE){
				if(rank == 0){
					printf("ANS:----------------------------------\n");
					for(int i = 0;i<num;i++){
						printf("%f ",ans[i]);
					}
					printf("\n");
					printf("----------------------------------\n");
				}
			}
			break;
		}
	}
//	//printf("Sorted and stored in ans\n");
	if(PRINT_TIME){b_T = MPI_Wtime();
	CPU_time = (b_T-a_T-Comm_time);}
	if(PRINT_TIME) a_T = MPI_Wtime();
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
	if(PRINT_TIME){b_T = MPI_Wtime();
	IO_time+=(b_T-a_T);}
	if(PRINT_TIME) E_time = MPI_Wtime();
	if(PRINT_TIME){
		MPI_File_open(MPI_COMM_WORLD, argv[4], MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
		MPI_Reduce(&CPU_time, &R_CPU_time, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
		MPI_Reduce(&Comm_time, &R_Comm_time, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
		MPI_Reduce(&IO_time, &R_IO_time, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
		R_CPU_time/= size;
		R_Comm_time/= size;
		R_IO_time/= size;
		if(rank == 0){
			char Write_BUF[100];/*
			printf("CPU time: %f\n",CPU_time);
			printf("Comm time: %f\n",Comm_time);
			printf("IO time: %f\n",IO_time);*/
			int writesize = sizeof("CPU time: \nComm time: \nIO time: \n")+3*sizeof(float);
			sprintf(Write_BUF,"CPU time: %.2f\nComm time: %.2f\nIO time: %.2f\n", R_CPU_time, R_Comm_time, R_IO_time);
			printf("%s,sizeof: {%d,%d}\n",Write_BUF,sizeof(Write_BUF),writesize);
			MPI_File_write(f, Write_BUF, writesize, MPI_CHAR, MPI_STATUS_IGNORE);
			MPI_File_close(&f);
		}
	}
	if(PRINT_TIME&&rank == 0) printf("The task took %f\n",E_time - S_time);
	MPI_Finalize();

	return 0;
 }
