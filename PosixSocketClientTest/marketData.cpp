#include "memory.h"

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;


CMarketData::CMarketData()
{
	orderB = new Order;
	orderS = new Order;
	orderB->action = "BUY";
	orderS->action = "SELL";
	leg = new ComboLeg;

	contract = new Contract;
	contract->setExchange(IM.Exchange);
	for(int i=0; i<NOT_SET+1; i++)
	{
		market[i] = -1;
		marketPrev[i] = -1;
	}
}
