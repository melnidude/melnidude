#ifndef __ALGOMF_H_
#define __ALGOMF_H_

#include "memory.h"

class CAlgoMF : public CAlgoBase
{
public:
	   CAlgoMF();
	   ~CAlgoMF();

	   //interface
	   void afterPCA(long *position, double *lastVector, double *eigVector, double *eigValue);
	   BracketHierarchy GenerateDesiredCumulativeOrders(int assetId, Order **orderB, Order **orderS);
	   void afterFill(int assetId);
	   int getNeutrDimId() { return -1; };

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

	   bool (*run[ALG_STATE_CNT])(long *position, double *lastVector, double *eigVector, double *eigValue);
	   void calcDesiredPosition(double *last, double *eigVector, long *desPos);
	   void finalize(double *eigVector, double *lastVector);

	   void TakeOnPosition(int assetId, Order **orderB, Order **orderS);
	   void LiquidatePosition(int assetId, Order **orderB, Order **orderS);
	   void NeutralizePosition(int assetId, Order **orderB, Order **orderS);

	   double getAuxPrice(int assetId);

	   double *lastVectorAdj;
	   double *positionOrth;
	   double  vpvon, mpvon;
	   double *positionPP;
	   double *vwap;
};

#endif //_ALGOMF_H_
