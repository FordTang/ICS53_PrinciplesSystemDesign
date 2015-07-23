#include <iostream>
using namespace std;

bool binarySearch(int* anArray, int start, int end, int key)
{
	if (end < start)
	{
		return false;
	}
	else
	{
		int middle = (start + end)/2;
		
		if (anArray[middle] > key)
		{
			return binarySearch(anArray, start, middle-1, key);
		}
		
		else if (anArray[middle] < key)
		{
			return binarySearch(anArray, middle+1, end, key);
		}
		else
		{
			return true;
		}
	}
}

void bubbleSort(int* anArray, int start, int end)
{
	int temp;
	for (int i = 0; i < end; i++)
	{
		for (int j = 0; j < end - i - 1; j++)
		{
			if (anArray[j] > anArray[j + 1])
			{
				temp = anArray[j];
				anArray[j] = anArray[j + 1];
				anArray[j + 1] =  temp;
			}
		}
	}
}

int main()
{
	int anArray[] = {10,9,8,7,6,5,4,3,2,1};
	for (int i = 0; i < 10; i++)
	{
		cout << anArray[i] << " ";
	}
	cout << endl;
	bubbleSort(anArray, 1, 10);
	for (int i = 0; i < 10; i++)
	{
		cout << anArray[i] << " ";
	}
	cout << endl;
	bool result = binarySearch(anArray, 1, 10, 8);
	cout << result << endl;
	return 0;
}
