#include "memory.h"

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;

OrderId Cids::getNextId(int assetId)
{
	//OrderId nextId = m_nextId[assetId];
	//m_nextId[assetId] += m_BasketSize;
	OrderId nextId = m_nextId+assetId;
	m_nextId += m_BasketSize;

	return nextId;
}

int Cids::getAssetId(long orderId)
{
	int assetId;
	assetId = (orderId-m_origin) % m_BasketSize;
	return assetId;
}

int Cids::getAssetId(IBString localSymbol)
{
	int assetId = -1;
	for(int i=0; i<IM.BasketSize; i++)
	{
		if(!SM.marketData[i].contract->symbol.compare(localSymbol))
		{
			assetId = i;
			break;
		}
	}
	return assetId;
}

