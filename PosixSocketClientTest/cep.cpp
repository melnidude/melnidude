#include "cep.h"
#include "memory.h"
#include "utils.h"
#include <time.h>

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;

CCEP::CCEP()
{
}

CCEP::~CCEP()
{
}


enum MarketMicroStructureState {
	MMS_NBBO_CHANGE = 0
};

void CCEP::processMsg(int idx, int tradeType, double price, int size)
{
	if((tradeType==1)||(tradeType==2))
	{
		//newNBBO();
	}
	else if(tradeType==3)
	{

	}
	else if(tradeType==4)
	{

	}
}
