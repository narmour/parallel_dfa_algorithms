#include <mpi.h>
#include <iostream>
#include <iomanip>
#include <string>

#define K 1000


/* 
 * generates a transition matrix  D0 and D1 for a dfa with K states over {0,1}
 * STATE Q0 -> Q1 on 0   Q0->Q0 on 1       
 * STATE Q1->Q2 on 0   Q1->Q1 on 1 
 * STATE QK loop on 0 and 1
 * STATE QK will be the only accepting state 
 * etc...
 */
void gen_dfa(int D0[][K],int D1[][K]){
    // create D0 matrix
    for(int i =0;i<K;i++){
        for (int j=0;j<K;j++){
            if(j -i ==1 || (i == K -1 && j == K-1))
                D0[i][j] =  1 ;
            else
                D0[i][j] =  0;
        }
    }

    // create D1 matrix
    for(int i =0;i<K;i++){
        for (int j=0;j<K;j++){
            if(i==j)
                D1[i][j] =  1;
            else
                D1[i][j] =  0;
        }
    }

}

void print_matrix(int M[][K]){
    for(int i =K-1;i <K;i++){
        for(int j=0;j<K;j++)
            std::cout << M[i][j];
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

int D0[K][K];
int D1[K][K];
int RES[K][K]{{0}};
MPI_Status wrkr_status;


int main(int argc, char **argv){

    //RES={{0}};


    int task_id,num_proc,num_workers,row_work_start,row_per_wrkr,source,dest,i,j,k;
    MPI_Init(nullptr,nullptr);
    MPI_Comm_rank(MPI_COMM_WORLD,&task_id); //get this task_id
    MPI_Comm_size(MPI_COMM_WORLD,&num_proc); // get number of processes

    num_workers = num_proc -1; // dont want process 0 working

    if(task_id ==0){ // MAIN THREAD
        gen_dfa(D0,D1); // create D0 and D1
        //print_matrix(D0);
        //print_matrix(D1);

        // split up D0 data amongst workers
        row_per_wrkr = K/num_workers;
        row_work_start = 0;
        for(dest=1;dest<=num_workers;dest++){
            MPI_Send(&row_work_start,1,MPI_INT,dest,1,MPI_COMM_WORLD);
            MPI_Send(&row_per_wrkr,1,MPI_INT,dest,1,MPI_COMM_WORLD);
            MPI_Send(&D0[row_work_start][0],row_per_wrkr * K,MPI_INT,dest,1,MPI_COMM_WORLD);// chunk of D0
            MPI_Send(&D1,K* K,MPI_INT,dest,1,MPI_COMM_WORLD);// all of D1 
            row_work_start+= row_per_wrkr;
        }

        // wait for result
        for( i=1;i<=num_workers;i++){
            source = i;
            MPI_Recv(&row_work_start,1,MPI_INT,source,2,MPI_COMM_WORLD,&wrkr_status);
            MPI_Recv(&row_per_wrkr,1,MPI_INT,source,2,MPI_COMM_WORLD,&wrkr_status);
            MPI_Recv(&RES[row_work_start][0],row_per_wrkr * K,MPI_INT,source,2,MPI_COMM_WORLD,&wrkr_status);


        }

        std::cout << "RESULT MATRIX\n";
        print_matrix(RES);
    }
    else{ // WORKER THREAD
        source = 0;
        MPI_Recv(&row_work_start,1,MPI_INT,source,1,MPI_COMM_WORLD,&wrkr_status);
        MPI_Recv(&row_per_wrkr,1,MPI_INT,source,1,MPI_COMM_WORLD,&wrkr_status);
        MPI_Recv(&D0,row_per_wrkr * K,MPI_INT,source,1,MPI_COMM_WORLD,&wrkr_status);
        MPI_Recv(&D1,K* K,MPI_INT,source,1,MPI_COMM_WORLD,&wrkr_status);

        for(i =0;i<row_per_wrkr;i++)
            for(j=0;j<K;j++)
                //RES[i][j] = 0;
                for(k=0;k<K;k++)
                    RES[i][j] += D0[i][k] * D1[k][j];

        MPI_Send(&row_work_start,1,MPI_INT,0,2,MPI_COMM_WORLD);
        MPI_Send(&row_per_wrkr,1,MPI_INT,0,2,MPI_COMM_WORLD);
        MPI_Send(&RES,row_per_wrkr * K,MPI_INT,0,2,MPI_COMM_WORLD);


    }
    MPI_Finalize();
}


