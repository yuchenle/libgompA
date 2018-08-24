#include <omp.h>
#include <stdio.h>
int main()
{
	int cpt = 0;
#pragma omp parallel num_threads(4)
	{
		printf("parallel msg from %d\n",omp_get_thread_num());
#pragma omp single
		printf("single msg from %d\n",omp_get_thread_num());
	}
	return 0;
}
