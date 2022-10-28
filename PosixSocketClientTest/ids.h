#ifndef _IDS_H_
#define _IDS_H_

#include <math.h>

class Cids
{
public:
	Cids()
	{
		m_mktDataStartingId = -1;
	};
	~Cids() {};

	void init(long origin, int BasketSize)
	{
		m_origin = origin;
		m_BasketSize = BasketSize;
		m_nextId = origin+BasketSize;
	}

	long getMarketDataId(int assetId)
	{
		return (m_origin + assetId);
	}

	OrderId getNextId(int assetId);

	int getAssetId(long orderId);
	int getAssetId(IBString localSymbol);
	OrderId recdId(void) { return m_mktDataStartingId; }

	OrderId m_mktDataStartingId;

private:
	long m_origin;
	int m_BasketSize;
	OrderId m_nextId;
};


#endif //_IDS_H_
