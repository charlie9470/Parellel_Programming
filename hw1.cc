#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <mpi.h>
#include <algorithm>
#define DEBUG_MODE 0
#define DETAIL_MODE 0
using namespace std;
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
	bool done = false;//if true return
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
	if(size!=1){
	MPI_File f;
	MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &f);
	MPI_File_read_all(f, ans, num, MPI_FLOAT, MPI_STATUS_IGNORE);
	MPI_File_close(&f);
	//case A:size < max(num_OP,num_OE)
	//case B:size = max(num_OP,num_OE)
	//case C:size > min(num_OP,num_OE) //meaningless
	//
	//
	int *send_cnt;
	int *displs;
	int index_no_work = 0;
	if(rank == 0){
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
		if(DETAIL_MODE){
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
	int recv_num = 0;
	int dist_num = num;
	if(num > 2 * size){
		recv_num = 2;
		dist_num -= 2*size;
		while(dist_num > size){
			dist_num -= size;
			recv_num++;
		}
		if(rank < dist_num) recv_num++;
	}
	else{
		if(rank < dist_num/2){
			recv_num = 2;
		}
		if(dist_num%2){
			if(rank == dist_num/2) recv_num = 1;
		}
	}
	swapped = (float*)malloc(sizeof(float) * recv_num);
	if(DEBUG_MODE) printf("Rank %d received %d\n", rank, recv_num);
	MPI_Scatterv(ans, send_cnt, displs, MPI_FLOAT, swapped, recv_num, MPI_FLOAT, 0, MPI_COMM_WORLD);
	//
	//
	MPI_Bcast(&index_no_work, 1, MPI_INT, 0, MPI_COMM_WORLD);
	//
	//
	while(1){
		change = 0;
		t_change = 0;
		sort(swapped, swapped + recv_num);
		//Odd phase
		MPI_Request send_req, recv_req;
		MPI_Status status;
		if(rank < index_no_work - 1 && rank % 2 == 0){
			float *pass_next = (float*)malloc(sizeof(float)*(recv_num+1));//recv_num + swapped
			float *get_back = (float*)malloc(sizeof(float)*(recv_num+1));//recv_num + swapped
			pass_next[0] = recv_num;
			get_back[0] = recv_num;
			for(int i = 0;i<recv_num;i++){
				pass_next[i+1] = swapped[i];
			}
			if(DETAIL_MODE) printf("Rank %d sent to Rank %d\n",rank, rank+1);
			if(DETAIL_MODE) printf("pass_next[0] is %d\n", (int)pass_next[0]);
			MPI_Send(pass_next, pass_next[0]+1, MPI_FLOAT, rank+1, 0, MPI_COMM_WORLD);
			if(DETAIL_MODE) printf("Rank %d successfully send and start waiting from Rank %d\n",rank, rank+1);
			MPI_Recv(get_back, get_back[0]+1, MPI_FLOAT, rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			if(DETAIL_MODE) printf("Rank %d successfully received from Rank %d\n",rank, rank+1);
			for(int i = 0;i<recv_num;i++){
				if(swapped[i]!=get_back[i+1]){
					swapped[i] = get_back[i+1];
					change++;
				}
			}/*
			if(DEBUG_MODE){
				printf("Rank %d after odd sort\n",rank);
				for(int i=0;i<recv_num;i++){
					printf("%f,", swapped[i]);
				}
				printf("\n---------------------\n");
			}*/
		}
		if(rank != 0 && rank < index_no_work&& rank % 2){
			if(DETAIL_MODE) printf("Rank %d tries to receive Rank %d\n",rank, rank-1);
			if(DETAIL_MODE) printf("memory alloced to recv_prev: %d\n",recv_num+3);
			float *recv_prev = (float*)malloc(sizeof(float)*(recv_num+3));//recv_num + swapped
			MPI_Recv(recv_prev, recv_num+3, MPI_FLOAT, rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			int count;
			MPI_Get_count(&status, MPI_FLOAT, &count);
			if(DETAIL_MODE) printf("Rank %d successfully received from Rank %d\n",rank, rank-1);
			if(DETAIL_MODE) printf("Count: %d\n",count);
			if(DETAIL_MODE) printf("Recv_prev[0] = %d\n",(int)recv_prev[0]);
			int recv_from_prev = (int)recv_prev[0];
			float *NEW = (float*)malloc(sizeof(float)*(recv_num+recv_from_prev));
			if(DETAIL_MODE) printf("size of NEW is %d\n",recv_num+recv_from_prev);

			for(int i = 0;i<recv_from_prev;i++){
				NEW[i] = recv_prev[i+1];
			}
			for(int i = 0;i<recv_num;i++){
				NEW[i+recv_from_prev] = swapped[i];
			}
			if(DETAIL_MODE) printf("Rank %d finished sorting\n",rank);
			sort(NEW, NEW + recv_num + recv_from_prev);
			/*
			if(DEBUG_MODE){
				printf("NEW: \n",rank);
				for(int i=0;i<recv_num + recv_from_prev;i++){
					printf("%f,", NEW[i]);
				}
				printf("\n---------------------\n");
			}
			*/
			for(int i = 0;i<recv_from_prev;i++){
				if(recv_prev[i+1]!=NEW[i]){
					recv_prev[i+1] = NEW[i];
					change++;
				}
			}
			for(int i = 0;i<recv_num;i++){
				swapped[i] = NEW[i+recv_from_prev];
			}

			if(DETAIL_MODE) printf("Rank %d sent back to Rank %d\n",rank, rank-1);
			MPI_Send(recv_prev, recv_prev[0]+1, MPI_FLOAT, rank-1, 0, MPI_COMM_WORLD);
			/*
			if(DEBUG_MODE){
				printf("Rank %d after odd sort\n",rank);
				for(int i=0;i<recv_num;i++){
					printf("%f,", swapped[i]);
				}
				printf("\n---------------------\n");
			}*/
		}
		MPI_Barrier(MPI_COMM_WORLD);

		//Even phase
		if(rank != 0&& rank < index_no_work&& rank % 2 == 0){
			float *pass_prev = (float*)malloc(sizeof(float)*(recv_num+1));//recv_num + swapped
			float *get_back = (float*)malloc(sizeof(float)*(recv_num+1));//recv_num + swapped
			pass_prev[0] = recv_num;
			get_back[0] = recv_num;
			for(int i = 0;i<recv_num;i++){
				pass_prev[i+1] = swapped[i];
			}
			if(DETAIL_MODE) printf("Rank %d sent to Rank %d\n",rank, rank-1);
			MPI_Send(pass_prev, pass_prev[0]+1, MPI_FLOAT, rank-1, 0, MPI_COMM_WORLD);
			if(DETAIL_MODE) printf("Rank %d successfully received from Rank %d\n",rank, rank-1);
			MPI_Recv(get_back, get_back[0]+10, MPI_FLOAT, rank-1, 0, MPI_COMM_WORLD, &status);
			for(int i = 0;i<recv_num;i++){
				if(swapped[i]!=get_back[i+1]){
					swapped[i] = get_back[i+1];
					change++;
				}
			}/*
			if(DEBUG_MODE){
				printf("Rank %d after odd sort\n",rank);
				for(int i=0;i<recv_num;i++){
					printf("%f,", swapped[i]);
				}
				printf("\n---------------------\n");
			}*/
		}
		if(rank != 0 && rank < index_no_work - 1&& rank % 2){
			float *recv_next = (float*)malloc(sizeof(float)*(recv_num+3));//recv_num + swapped
			if(DETAIL_MODE) printf("Rank %d tries to receive Rank %d\n",rank, rank+1);
			MPI_Recv(recv_next, recv_num+3, MPI_FLOAT, rank+1, 0, MPI_COMM_WORLD, &status);
			if(DETAIL_MODE) printf("Rank %d successfully received from Rank %d\n",rank, rank+1);

			int recv_from_next = (int)recv_next[0];
			float* NEW = (float*)malloc(sizeof(float)*(recv_num+recv_from_next));
			if(DETAIL_MODE) printf("size of NEW is %d\n",recv_num+recv_from_next);

			for(int i = 0;i<recv_num;i++){
				NEW[i] = swapped[i];
			}
			for(int i = 0;i<recv_from_next;i++){
				NEW[i+recv_num] = recv_next[i+1];
			}

			if(DETAIL_MODE) printf("Rank %d finished sorting\n",rank);
			sort(NEW, NEW + recv_num + recv_from_next);

			for(int i = 0;i<recv_num;i++){
				if(swapped[i]!=NEW[i]){
					swapped[i] = NEW[i];
					change++;
				}
			}
			for(int i = 0;i<recv_from_next;i++){
				recv_next[i+1] = NEW[i+recv_num];
			}

			if(DETAIL_MODE) printf("Rank %d sent back to Rank %d\n",rank, rank+1);
			MPI_Send(recv_next, recv_from_next+1, MPI_FLOAT, rank+1, 0, MPI_COMM_WORLD);
			/*
			if(DEBUG_MODE){
				printf("Rank %d after odd sort\n",rank);
				for(int i=0;i<recv_num;i++){
					printf("%f,", swapped[i]);
				}
				printf("\n---------------------\n");
			}*/
		}
		MPI_Allreduce(&change, &t_change, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
		printf("one round complete\n--------------------\n");
//		if(DEBUG_MODE) t_change = 0;
		if(t_change == 0) {
			MPI_Gatherv(swapped, recv_num, MPI_FLOAT, ans, send_cnt, displs, MPI_FLOAT, 0, MPI_COMM_WORLD);/*
			if(DEBUG_MODE){
				if(rank == 0){
					printf("ANS:----------------------------------\n");
					for(int i = 0;i<num;i++){
						printf("%f ",ans[i]);
					}
					printf("\n");
					printf("----------------------------------\n");
				}
			}*/
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
	/*
	if(DEBUG_MODE){
	if(rank == 0){
		printf("ANS:----------------------------------\n");
		for(int i = 0;i<num;i++){
			printf("%.2f ",ans[i]);
			if((i!=0&&i%6==0)|i==num-1) printf("\n");
		}
		printf("----------------------------------\n");
	}
	}
	*/
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
