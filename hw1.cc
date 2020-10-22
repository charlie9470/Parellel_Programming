#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <mpi.h>
#include <algorithm>
#define DEBUG_MODE 0
#define DEBUG_HUGE_TESTCASE 0
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
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);


	MPI_File f;
	MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &f);
	MPI_File_read_all(f, ans, num, MPI_FLOAT, MPI_STATUS_IGNORE);
	MPI_File_close(&f);
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
	if(DETAIL_MODE)printf("Index_no_work: %d\n", index_no_work);
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
	if(rank == size/2){
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
	MPI_Scatterv(ans, send_cnt, displs, MPI_FLOAT, swapped, recv_num, MPI_FLOAT, 0, MPI_COMM_WORLD);
	//
	//
	MPI_Bcast(&index_no_work, 1, MPI_INT, 0, MPI_COMM_WORLD);
	//
	//
	int odd_rank;
	int even_rank;
	int index_mid = recv_num/2;//First index of the second half of swapped
	int num_recv_from_next,num_recv_from_prev;
	int num_pass_to_next,num_pass_to_prev;
	if(rank%2){
		odd_rank = rank - 1;
		even_rank = rank + 1;
		num_recv_from_next = send_cnt[even_rank]/2;
		num_recv_from_prev = send_cnt[odd_rank]/2 + send_cnt[odd_rank]%2;
		num_pass_to_prev = recv_num/2;
		num_pass_to_next = recv_num - num_pass_to_prev;
	}
	else{
		odd_rank = rank + 1;
		even_rank = rank - 1;
		num_recv_from_next = send_cnt[odd_rank]/2;
		num_recv_from_prev = send_cnt[even_rank]/2 + send_cnt[even_rank]%2;
		num_pass_to_prev = recv_num/2;
		num_pass_to_next = recv_num - num_pass_to_prev;
	}

	float *send_buf;
	float *recv_buf;
	float *NEW;
	send_buf = (float*)malloc(sizeof(float)*(num_pass_to_next+5));
	recv_buf = (float*)malloc(sizeof(float)*(max(num_recv_from_next,num_recv_from_prev)+5));
	NEW = (float*)malloc(sizeof(float)*(num_pass_to_next + max(num_recv_from_next,num_recv_from_prev + 5)));

	while(1){
		change = 0;
		t_change = 0;
		sort(swapped, swapped + recv_num);
		//Odd phase
		MPI_Request send_req, recv_req;
		MPI_Status status;
		if(rank < index_no_work - 1 && rank % 2 == 0){
			//Send recv_num/2+recv_num%2 to rank+1
			//Recv num_recv_from_next from rank+1
			//Send num_recv_from_next back
			//Recv recv_num/2+recv_num%2 from rank+1
			for(int i = 0;i<num_pass_to_next;i++){
				send_buf[i] = swapped[index_mid+i];
			}
			MPI_Send(send_buf, num_pass_to_next, MPI_FLOAT, odd_rank, 1, MPI_COMM_WORLD);
			MPI_Recv(recv_buf, num_recv_from_next, MPI_FLOAT, odd_rank, 1, MPI_COMM_WORLD, &status);
/*
			for(int i = 0;i<index_mid;i++){
				NEW[i] = swapped[i];
			}
			for(int i = 0;i<num_recv_from_next;i++){
				NEW[index_mid+i] = recv_buf[i];
			}
			sort(NEW, NEW + index_mid + num_recv_from_next);
*/
			mergeArr(swapped, index_mid, recv_buf, num_recv_from_next, NEW);

			for(int i = 0;i<index_mid;i++){
				if(NEW[i]!=swapped[i]){
					swapped[i] = NEW[i];
					change++;
				}
			}
			for(int i = 0;i<num_recv_from_next;i++){
				recv_buf[i] = NEW[index_mid+i];
			}
			MPI_Send(recv_buf, num_recv_from_next, MPI_FLOAT, odd_rank, 2, MPI_COMM_WORLD);
			
			MPI_Recv(send_buf, num_pass_to_next, MPI_FLOAT, odd_rank, 2, MPI_COMM_WORLD, &status);
			for(int i = 0;i<num_pass_to_next;i++){
				if(swapped[index_mid+i]!=send_buf[i]){
					swapped[index_mid+i] = send_buf[i];
					change++;
				}
			}
			
			MPI_Recv(recv_buf, num_recv_from_next, MPI_FLOAT, odd_rank, 3, MPI_COMM_WORLD, &status);
/*
			for(int i = 0;i < num_pass_to_next;i++){
				NEW[i] = swapped[i+index_mid];
			}
			for(int i = 0;i < num_recv_from_next;i++){
				NEW[i+num_pass_to_next] = recv_buf[i];
			}
			sort(NEW, NEW + num_pass_to_next + num_recv_from_next);
*/
			if(DEBUG_MODE){
				printf("index_mid is %d\n",index_mid);
				printf("swapped+index_mid stores %f\n",*(swapped+index_mid));
			}
			mergeArr(swapped + index_mid, num_pass_to_next, recv_buf, num_recv_from_next, NEW);

			for(int i = 0;i < num_pass_to_next;i++){
				if(NEW[i]!=swapped[i+index_mid]){
					swapped[i+index_mid] = NEW[i];
					change++;
				}
			}
			for(int i = 0;i < num_recv_from_next;i++){
				send_buf[i] = NEW[i+num_pass_to_next];
			}
			MPI_Send(send_buf, num_recv_from_next, MPI_FLOAT, odd_rank, 3, MPI_COMM_WORLD);
			if(DEBUG_MODE&&!DEBUG_HUGE_TESTCASE){
				printf("Rank %d after odd sort\n",rank);
				for(int i=0;i<recv_num;i++){
					printf("%f,", swapped[i]);
				}
				printf("\n---------------------\n");
			}
		}
		if(rank != 0 && rank < index_no_work&& rank % 2){
			//Send recv_num/2 to rank-1
			//Recv num_recv_from_next from rank-1
			//Send num_recv_from_next back
			//Recv recv_num/2 from rank-1
			for(int i = 0;i<num_pass_to_prev;i++){
				send_buf[i] = swapped[i];
			}
			MPI_Send(send_buf, num_pass_to_prev, MPI_FLOAT, odd_rank, 1, MPI_COMM_WORLD);
			MPI_Recv(recv_buf, num_recv_from_prev, MPI_FLOAT, odd_rank, 1, MPI_COMM_WORLD, &status);
/*
			for(int i = 0;i<num_recv_from_prev;i++){
				NEW[i] = recv_buf[i];
			}
			for(int i = index_mid;i<recv_num;i++){
				NEW[num_recv_from_prev+i-index_mid] = swapped[i];
			}
			sort(NEW, NEW + num_recv_from_prev + recv_num - recv_num/2);
*/
			mergeArr(recv_buf, num_recv_from_prev, swapped + index_mid, num_pass_to_next, NEW);

			for(int i = 0;i<num_recv_from_prev;i++){
				recv_buf[i] = NEW[i];
			}
			for(int i = index_mid;i<recv_num;i++){
				if(swapped[i]!=NEW[num_recv_from_prev+i-index_mid]){
					swapped[i] = NEW[num_recv_from_prev+i-index_mid];
					change++;
				}
			}
			MPI_Send(recv_buf, num_recv_from_prev, MPI_FLOAT, odd_rank, 2, MPI_COMM_WORLD);
			MPI_Recv(send_buf, num_pass_to_prev, MPI_FLOAT, odd_rank, 2, MPI_COMM_WORLD, &status);
			for(int i = 0;i<num_pass_to_prev;i++){
				if(swapped[i]!=send_buf[i]){
					swapped[i] = send_buf[i];
					change++;
				}
			}

			for(int i = 0;i<num_pass_to_prev;i++){
				send_buf[i] = swapped[i];
			}
			MPI_Send(send_buf, num_pass_to_prev, MPI_FLOAT, odd_rank, 3, MPI_COMM_WORLD);
			MPI_Recv(recv_buf, num_pass_to_prev, MPI_FLOAT, odd_rank, 3, MPI_COMM_WORLD, &status);
			for(int i = 0;i<num_pass_to_prev;i++){
				if(recv_buf[i]!=swapped[i]){
					swapped[i] = recv_buf[i];
					change++;
				}
			}
			if(DEBUG_MODE&&!DEBUG_HUGE_TESTCASE){
				printf("Rank %d after odd sort\n",rank);
				for(int i=0;i<recv_num;i++){
					printf("%f,", swapped[i]);
				}
				printf("\n---------------------\n");
			}
		}

		MPI_Barrier(MPI_COMM_WORLD);
		//Even phase
		if(rank != 0&& rank < index_no_work&& rank % 2 == 0){
			//Send num_pass_to_prev to rank-1
			//Recv num_recv_from_next from rank-1
			//Send num_recv_from_next back
			//Recv num_pass_to_prev from rank-1
			for(int i = 0;i < num_pass_to_prev;i++){
				send_buf[i] = swapped[i];
			}
			MPI_Send(send_buf, num_pass_to_prev, MPI_FLOAT, even_rank, 1, MPI_COMM_WORLD);
			MPI_Recv(recv_buf, num_recv_from_prev, MPI_FLOAT, even_rank, 1, MPI_COMM_WORLD, &status);
/*
			for(int i = 0;i < num_recv_from_prev;i++){
				NEW[i] = recv_buf[i];
			}

			for(int i = 0;i < num_pass_to_next;i++){
				NEW[i+num_recv_from_prev] = swapped[i+index_mid];
			}
			sort(NEW, NEW + num_recv_from_prev + num_pass_to_next);
*/
			mergeArr(swapped + index_mid, num_pass_to_next, recv_buf, num_recv_from_prev, NEW);

			for(int i = 0;i < num_recv_from_prev;i++){
				recv_buf[i] = NEW[i];
			}
			for(int i = 0;i < num_pass_to_next;i++){
				if(NEW[i+num_recv_from_prev]!=swapped[i+index_mid]){
					swapped[i+index_mid] = NEW[i+num_recv_from_prev];
					change++;
				}
			}

			MPI_Send(recv_buf, num_recv_from_prev, MPI_FLOAT, even_rank, 2, MPI_COMM_WORLD);
			MPI_Recv(send_buf, num_pass_to_prev, MPI_FLOAT, even_rank, 2, MPI_COMM_WORLD, &status);

			for(int i = 0;i<num_pass_to_prev;i++){
				if(send_buf[i]!=swapped[i]){
					swapped[i] = send_buf[i];
					change++;
				}
			}

			for(int i = 0;i<num_pass_to_prev;i++){
				send_buf[i] = swapped[i];
			}
			MPI_Send(send_buf, num_pass_to_prev, MPI_FLOAT, even_rank, 3, MPI_COMM_WORLD);
			MPI_Recv(recv_buf, num_pass_to_prev, MPI_FLOAT, even_rank, 3, MPI_COMM_WORLD, &status);
			for(int i = 0;i<num_pass_to_prev;i++){
				if(swapped[i]!=recv_buf[i]){
					swapped[i] = recv_buf[i];
					change++;
				}
			}
			if(DEBUG_MODE&&!DEBUG_HUGE_TESTCASE){
				printf("Rank %d after even sort\n",rank);
				for(int i=0;i<recv_num;i++){
					printf("%f,", swapped[i]);
				}
				printf("\n---------------------\n");
			}
		}
		if(rank != 0 && rank < index_no_work - 1&& rank % 2){
			//Send recv_num/2+recv_num%2 to rank+1
			//Recv num_recv_from_next from rank+1
			//Send num_recv_from_next back
			//Recv recv_num/2+recv_num%2 from rank+1
			
			for(int i = 0;i < num_pass_to_next;i++){
				send_buf[i] = swapped[i+index_mid];
			}
			MPI_Send(send_buf, num_pass_to_next, MPI_FLOAT, even_rank, 1, MPI_COMM_WORLD);
			MPI_Recv(recv_buf, num_recv_from_next, MPI_FLOAT, even_rank, 1, MPI_COMM_WORLD, &status);
/*
			for(int i = 0;i < index_mid;i++){
				NEW[i] = swapped[i];
			}
			for(int i = 0;i < num_recv_from_next;i++){
				NEW[i+index_mid] = recv_buf[i];
			}
			sort(NEW, NEW + index_mid + num_recv_from_next);
*/
			mergeArr(swapped, index_mid, recv_buf, num_recv_from_next, NEW);
			for(int i = 0;i < index_mid;i++){
				if(swapped[i]!=NEW[i]){
					swapped[i] = NEW[i];
					change++;
				}
			}
			for(int i = 0;i < num_recv_from_next;i++){
				recv_buf[i] = NEW[index_mid+i];
			}

			MPI_Send(recv_buf, num_recv_from_next, MPI_FLOAT, even_rank, 2, MPI_COMM_WORLD);
			MPI_Recv(send_buf, num_pass_to_next, MPI_FLOAT, even_rank, 2, MPI_COMM_WORLD, &status);

			for(int i = 0;i < num_pass_to_next;i++){
				if(swapped[index_mid+i]!=send_buf[i]){
					swapped[index_mid+i] = send_buf[i];
					change++;
				}
			}

			MPI_Recv(recv_buf, num_recv_from_next, MPI_FLOAT, even_rank, 3, MPI_COMM_WORLD,&status);
/*
			for(int i = 0;i<num_pass_to_next;i++){
				NEW[i] = swapped[i+index_mid];
			}
			for(int i =0;i<num_recv_from_next;i++){
				NEW[i+num_pass_to_next] = recv_buf[i];
			}
			sort(NEW, NEW + num_recv_from_next + num_pass_to_next);
*/
			mergeArr(swapped + index_mid, num_pass_to_next, recv_buf, num_recv_from_next, NEW);
			for(int i = 0;i<num_pass_to_next;i++){
				if(NEW[i]!=swapped[i+index_mid]){
					swapped[i+index_mid] = NEW[i];
					change++;
				}
			}

			for(int i = 0;i<num_recv_from_next;i++){
				send_buf[i] = NEW[i+num_pass_to_next];
			}
			MPI_Send(send_buf, num_recv_from_next, MPI_FLOAT, even_rank, 3, MPI_COMM_WORLD);
			if(DEBUG_MODE&&!DEBUG_HUGE_TESTCASE){
				printf("Rank %d after even sort\n",rank);
				for(int i=0;i<recv_num;i++){
					printf("%f,", swapped[i]);
				}
				printf("\n---------------------\n");
			}
		}

		MPI_Allreduce(&change, &t_change, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
		if(DEBUG_MODE && rank == size-1) printf("one round complete\n--------------------\n");
//		if(DEBUG_MODE) t_change = 0;
		if(t_change == 0) {
			MPI_Gatherv(swapped, recv_num, MPI_FLOAT, ans, send_cnt, displs, MPI_FLOAT, 0, MPI_COMM_WORLD);
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
	MPI_Finalize();

	return 0;
 }
