#include "memory.h"
#include "manager.h"

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;


void LiquidateAll()
{
	IM.algoInitDone = false;
	IM.client->cancelTickData();
	cancelAllOpenOrders();
	liquidateAllPositions();

	sleep(5);
}

void cancelAllOpenOrders()
{
	IM.client->reqAllOpenOrders();
	sleep(3);
	for(int i=0; i<1000; i++)
	{
		IM.client->processMessages();
		sched_yield();
	}
}

void liquidateAllPositions()
{
	IM.client->reqAccountUpdates(true);
	sleep(3);
	for(int i=0; i<1000; i++)
	{
		IM.client->processMessages();
		sched_yield();
	}
}

