#ifndef __ALGOBASE_H_
#define __ALGOBASE_H_

#include "defs.h"
#include "Order.h"
#include "orderBook.h"
#include "cep.h"

enum AlgState {
	ALL_CASH = 0,
	PLACING_GETTING_INTO_POS_ORDERS,
	GETTING_INTO_POS,
	IN_POS,
	PLACING_LIQ_ORDERS,
	LIQUIDATING_POS,
	ALG_STATE_CNT
};


class CAlgoBase
{
public:
	   CAlgoBase();
	   virtual void afterPCA(long *position, double *lastVector, double *eigVector, double *eigValue) = 0;
	   virtual BracketHierarchy GenerateDesiredCumulativeOrders(int assetId, Order **orderB, Order **orderS) = 0;
	   virtual void afterFill(int assetId) = 0;
	   virtual int  getNeutrDimId() = 0;

	   void orderManager(int assetId);

	   double getMarketValue() { return MarketValue; }
	   double getPortfolioValue() { return PortfolioValue; }
	   double getUnRealizedGain() { return UnrealizedGain; }
	   double getOneSidedValue() { return OneSidedValue; }

	   virtual void updatePortfolioStats();
	   virtual void updatePortfolioStats(int assetId, long filled_s, bool neutralizable) = 0;

	   double getDistance(double PriceA, double PriceB, double PriceMkt);
	   double getDifference(double PriceA, double PriceB, double PriceMkt);
	   bool inInterval(double Price, double PriceA, double PriceB, double PriceMkt);

	   void tickPrice(int assetId, TickType field, double price, int size, std::string quoteSource);
	   void updateTAQ(int assetId, double Dtime);

	   void CEP(int idx, int tradeType, double price, int size);

	   AlgState algState;
	   IBString algStateDesc[ALG_STATE_CNT];

	   //portfolio stats
	   double MarketValue;
	   double PortfolioValue;
	   double UnrealizedGain;
	   double OneSidedValue;

	   CCEP cep;
};

#endif //_ALGOBASE_H_

