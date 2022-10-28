#ifndef __ALGOHF_H_
#define __ALGOHF_H_

#include "memory.h"

class CAlgoHF : public CAlgoBase
{
public:
	CAlgoHF();
	   ~CAlgoHF();

	   //interface
	   void afterPCA(long *position, double *lastVector, double *eigVector, double *eigValue);
	   BracketHierarchy GenerateDesiredCumulativeOrders(int assetId, Order **orderB, Order **orderS);
	   void afterFill(int assetId);
	   int getNeutrDimId() { return 0; };
	   using CAlgoBase::updatePortfolioStats;
	   void updatePortfolioStats(int assetId, long filled_s, bool neutralizable) = 0;

private:
	   void cancelNotNeededOrders(int assetId, Order *orderB,  Order *orderS);

	   //run[state]
	   bool afterPCA_AllCash(long *position, double *lastVector, double *eigVector, double *eigValue);
	   bool afterPCA_PlacingPositionOrders(long *position, double *lastVector, double *eigVector, double *eigValue);
	   bool afterPCA_GettingIntoPosition(long *position, double *lastVector, double *eigVector, double *eigValue);
	   bool afterPCA_InPosition(long *position, double *lastVector, double *eigVector, double *eigValue);
	   bool afterPCA_PlacingLiquidationOrders(long *position, double *lastVector, double *eigVector, double *eigValue);
	   bool afterPCA_LiquidatingPosition(long *position, double *lastVector, double *eigVector, double *eigValue);
	   bool afterPCA_default(long *position, double *lastVector, double *eigVector, double *eigValue);

	   void calcDesiredPosition(double *last, double *eigVector, long *desPos);
	   void finalize(double *eigVector, double *lastVector);
	   bool isPositionNeutral();

	   BracketHierarchy TakeOnPosition(int assetId, Order **orderB_ptr, Order **orderS_ptr);
	   void LiquidatePosition(int assetId, Order **orderB, Order **orderS);
	   void NeutralizePosition(int assetId, Order **orderB, Order **orderS);

	   double *lastVectorAdj;
	   double *positionOrth;
	   double  vpvon, mpvon;
	   double *positionPP;
	   double *vwap;

	   bool m_desiredPositionAchieved;
};

#endif //_ALGOHF_H_
