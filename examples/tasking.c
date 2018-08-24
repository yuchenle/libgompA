#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 10000

int main()
{
	int *nb = (int*)calloc(4,sizeof(int));
#pragma omp parallel num_threads(4)
#pragma omp single
	for(int i=0;i<SIZE;i++)
	{
#pragma omp task
		{
			nb[omp_get_thread_num()]++;
		}
	}

	for(int i=0;i<4;i++)
	{
		printf("%d msg printed by %d\n",nb[i],i);
	}
	return 0;
}
      	
