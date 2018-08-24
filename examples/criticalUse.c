#include <stdio.h>
#include <omp.h>

#define SIZE 10000
int main()
{
	int cpt = 0;
#pragma omp parallel for num_threads(4)
	for(int i=0;i<SIZE;i++)
	{
#pragma omp critical
		cpt++;
	}

	printf("cpt = %d, expected value is %d\n",cpt, SIZE);
	return 0;
}
