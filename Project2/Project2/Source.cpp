//============================================================================
// Name        : Source.cpp
// Author      : Ford Tang, Ray Chou, Michael Oh
// Version     :
// Copyright   : Your copyright notice
// Description : Second Project Lab
//============================================================================

#include <iostream>
#include <string>
#include <fstream>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
using namespace std;


struct vehicle
{
	int id;
	int direc;
};


ifstream ifile;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int cars_on_bridge=0;
int current_dir=0;


void ArriveBridge(int id,int direct)
{	
	pthread_mutex_lock(&mutex);
	cout<<"Car "<<id<<" arrives traveling direction "<<direct<<endl;
	//cout << "Cars on bridge: " << cars_on_bridge << endl;
	if (cars_on_bridge == 0 && current_dir != direct)
	{
		current_dir = direct;
		cout<<"Traffic Direction is being changed to "<< direct <<endl;
	}
	if (cars_on_bridge==3 || direct!=current_dir)
		cout<<"Car "<<id<<" waits to travel in direction "<<direct<<endl;
	while (cars_on_bridge>=3 || direct!=current_dir)
	{
		pthread_cond_wait(&cond, &mutex);
		if (cars_on_bridge == 0 && current_dir != direct)
		{
			cout<<"Traffic Direction is being changed to "<< direct <<endl;
			current_dir = direct;
		}
	}
	pthread_mutex_unlock(&mutex);	
}


void CrossBridge(int id,int direct)
{
	pthread_mutex_lock(&mutex);
	cout<<"Car "<<id<<" crossing bridge in direction "<<direct<<endl;
	cars_on_bridge++;
	pthread_mutex_unlock(&mutex);
	sleep(2);	
}


void ExitBridge(int id,int direct)
{
	pthread_mutex_lock(&mutex);
	cars_on_bridge--;
	cout << "Car " << id << " leaves the bridge." << endl;
	if (cars_on_bridge==0)
	{
		if(direct==0)
		{
			current_dir=1;
			cout<<"Traffic Direction is being changed to 1"<<endl;
			pthread_cond_signal(&cond);
		}
		else
		{
			current_dir=0;
			cout<<"Traffic Direction is being changed to 0"<<endl;
			pthread_cond_signal(&cond);
		}
	}
	pthread_mutex_unlock(&mutex);
}


void *OneVehicle( void *ptr )
{
	vehicle *car;
	car = (vehicle *) ptr;
	//cout << "id: " << car->id << " direc: " << car->direc << endl;
	ArriveBridge(car->id, car->direc);
	CrossBridge(car->id, car->direc);
	ExitBridge(car->id, car->direc);
	pthread_exit(0);
}


int main()
{
	pthread_t thread1, thread2, thread3;
	string item;
	ifile.open("bridge.in");
	ifile >> item;
	int count = 0;
	bool process = false;
	int number = atoi(item.c_str());
	vehicle car1,car2,car3;
	for (int i = 0; i < number; i++)
	{
		if (count == 0)
		{
			ifile >> item;
			int id = atoi(item.c_str());
			car1.id = id;
			ifile >> item;
			int direc = atoi(item.c_str());
			car1.direc = direc;
			if (i == number - 1)
				process = true;
			else
				process = false;
			count++;
		}
		else if (count == 1)
		{
			ifile >> item;
			int id = atoi(item.c_str());
			car2.id = id;
			ifile >> item;
			int direc = atoi(item.c_str());
			car2.direc = direc;
			if (i == number - 1)
				process = true;
			else
				process = false;
			count++;
		}
		else if (count == 2)
		{
			ifile >> item;
			int id = atoi(item.c_str());
			car3.id = id;
			ifile >> item;
			int direc = atoi(item.c_str());
			car3.direc = direc;
			process = true;
			count = 0;
		}
		if (process)
		{
			if (count == 0)
			{
				//cout << "running 3 thread" << endl;
				pthread_create(&thread1, NULL, &OneVehicle, &car1);
				pthread_create(&thread2, NULL, &OneVehicle, &car2);
				pthread_create(&thread3, NULL, &OneVehicle, &car3);
				pthread_join(thread1, NULL);
				pthread_join(thread2, NULL);
				pthread_join(thread3, NULL);
			}
			else if (count == 1)
			{
				//cout << "running 1 thread" << endl;
				pthread_create(&thread1, NULL, &OneVehicle, &car1);
				pthread_join( thread1,NULL);
			}
			else if (count == 2)
			{
				//cout << "running 2 thread" << endl;
				pthread_create(&thread1, NULL, &OneVehicle, &car1);
				pthread_create(&thread2, NULL, &OneVehicle, &car2);
				pthread_join( thread1,NULL);
				pthread_join( thread2,NULL);
			}
			process=false;
		}
	}
	ifile.close();
	return 0;
}