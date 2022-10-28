#ifndef __ALGOHF2_H_
#define __ALGOHF2_H_

#include "memory.h"

class CAlgoHF2 : public CAlgoBase
{
public:
	   CAlgoHF2(int numNeutrDimensions);
	   ~CAlgoHF2();

	   //interface
	   void afterPCA(long *position, double *lastVector, double *eigVector, double *eigValue);
	   BracketHierarchy GenerateDesiredCumulativeOrders(int assetId, Order **orderB, Order **orderS);
	   void afterFill(int assetId);
	   int getNeutrDimId() { return 0; };
	   double getMarketValueNeutralizable() { return MarketValueNeutralizable; }

private:
	   void cancelNotNeededOrders(int assetId, Order *orderB,  Order *orderS);

	   void calcDesiredPosition(double *last, double *eigVector, long *desPos);
	   bool isPositionNeutral();

	   BracketHierarchy TakeOnPosition(int assetId, Order **orderB_ptr, Order **orderS_ptr);
	   BracketHierarchy TakeOnPositionLayering(int assetId, Order **orderB_ptr, Order **orderS_ptr);
	   BracketHierarchy LiquidateLayering(int assetId, Order **orderB_ptr, Order **orderS_ptr);
	   void LiquidatePosition(int assetId, Order **orderB, Order **orderS);
	   void NeutralizePosition(int assetId, Order **orderB, Order **orderS);
	   long neutrMinShares(long neutOrderSize, long neutOrderSizeAll, long pos, Momentum momentum);
	   long neutrWithMomentum(long neutOrderSize, long neutOrderSizeAll, long pos, Momentum momentum);

	   void genDeltaOrder(int assetId, Order *order, long existSize);
	   using CAlgoBase::updatePortfolioStats;
	   void updatePortfolioStats(int assetId, long filled_s, bool neutralizable);

	   void neutralization();

	   double *lastVectorAdj;
	   double *positionOrth;
	   double  vpvon, mpvon;
	   double *positionPP;
	   double *vwap;

	   bool m_desiredPositionAchieved;

	   int m_numNeutrDimensions;
	   long *neutOrderSize;
	   unsigned long m_neutrOrderCnt;

	   long *DesiredPositionPrev;
	   bool *EstOrdersPlaced;
	   bool *dimFullyNeutralizable;

	   //portfolio stats
	   double MarketValueNeutralizable;
	   long *PositionNeutralizable;

	   IBString timePrefix;
};

#endif //_ALGOHF2_H_
