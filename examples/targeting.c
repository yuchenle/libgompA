#include <stdio.h>
#include <omp.h>

int main()
{
	int cpt=0;
	#pragma omp target map(tofrom:cpt)
	cpt++;
	printf("b = %d\n",cpt);
	return 0;
}
	
