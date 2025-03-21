#include <stdio.h>



int bubbleSort(int *pdata, int len)
{
	for(int i = 0; i < len - 1; i++)
	{
		for(int j = i + 1; j < len; j++)
		{
			if(pdata[i] < pdata[j])
			{
				int temp = pdata[i];
				pdata[i] = pdata[j];
				pdata[j] = temp;
			}
		}
	}
	return 0;
}

int main()
{
	int data[10] = {32, 83, 59, 10, 64, 76, 100, 91, 58, 7};
	bubbleSort(data, 10);
	for(int i = 0; i < 10; i++)
	{
		printf("%d ", data[i]);
	}
	printf("\n");
	return 0;
}
