#include <stdio.h>
#include <omp.h>
#include <stdlib.h>

int main()
{
#pragma omp parallel num_threads(4)
	{
#pragma omp master
	printf("msg 1, the size of team one is %d\n", omp_get_num_threads());
	}
#pragma omp parallel num_threads(8)
	{
#pragma omp master
	printf("msg 2, the size of team two is %d\n", omp_get_num_threads());
	}

	int *cpt = (int*) malloc(sizeof(int)*16);

#pragma omp parallel for schedule(static,2) num_threads(8)
	for(int i=0;i<16;i++)
	{
		cpt[i] = omp_get_thread_num();
	}

	for(int i=0;i<16;i++)
	{
		printf("for schedule(static,2) => cpt[%d] = %d, expeted : %d\n",i, cpt[i], (i/2)%4);
	}
	return 0;
}
