#ifndef _MARKETDATA_H_
#define _MARKETDATA_H_

#include "Contract.h"
#include "Order.h"

struct CMarketData
{
	int mktDataId;
	Order *orderB;
	Order *orderS;
	Contract *contract;
	ComboLeg *leg;
	double market[NOT_SET+1]; //bid,ask,last,high,low,close,etc.
	double marketPrev[NOT_SET+1]; //bid,ask,last,high,low,close,etc.
	int last_market_idx_chg;

	CMarketData();
	~CMarketData()
	{
		delete orderB;
		delete orderS;
		delete contract;
	}
};

#endif //_MARKETDATA_H_
