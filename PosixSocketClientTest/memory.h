#ifndef _DATA_H_
#define _DATA_H_

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <algorithm>


#include "defs.h"
#include "EPosixClientSocket.h"
#include "EWrapper.h"
#include "PosixTestClient.h"
#include "marketData.h"
#include "Contract.h"
#include "Order.h"
#include "ids.h"
#include "globalState.h"
#include "vwap.h"
#include "accounting.h"
#include "log.h"
#include "pca.h"
#include "algoBase.h"
#include "QuoteApiClientDemo_1_01.hh"

/********* SHARED MEMORY DEFINITIONS ********/

class DimDeviation
{
public:
	int index;
	double deviation;
	bool operator<(DimDeviation rhs) const { return this->deviation < rhs.deviation; }
};

struct Holding
{
	IBString localSymbol;
	int position;
};

enum TradingPeriod {
	NORMAL_TRADING,
	UNWINDING,
	DUMPING
};

enum BidAsk {
	BID_ORDER = 0,
	ASK_ORDER
};

enum Momentum {
	MOMENTUM_UP = 0,
	MOMENTUM_DOWN,
	MOMENTUM_NONE,
	MOMENTUM_ALIGNED,
	MOMENTUM_OPPOSITE
};

class MyOrder
{
public:
	OrderId id;
	double price;
	int size;

	MyOrder() {
		id = 0;
		price = 0.0;
		size = 0;
	}

	void init(double init_price)
	{
		id = 0;
		price = init_price;
		size = 0;
	}
};

void log(IBString line);
bool isNowTimeFor(TradingPeriod period);


struct InvariantMemoryStruct
{
	const char* host;
	unsigned int port;

	PosixTestClient *client;

	int BasketSize;
	bool algoInitDone;

	bool pca_enable;
	int pca_window;
	int pca_window_init;
	bool debugPCA;

	bool ClientServer;
	bool enableMatlab;
	bool enableIB;
	IBString quoteSource;
	bool enableTAQ;
	CLog *Logger;

	pthread_t manager_thread;
	pthread_t dfh_thread;
	pthread_t *om_thread;
	pthread_t pca_thread;

	double PercentCash;
	double MaxLeverage;
	double MaxPurchase;
	double OrderSizeSF; //M
	double ReductionFactor;
	bool   Overshoot;
	bool   CostAverage;
	bool   UseMomentum;
	//double *Offset;
	double *costAvgOffset;
	double *lossOffset;
	double *nbboOffset;
	double *vwapOffset;
	double sigmaFactor[2];
	long   minOrderSize;
	long   maxOrderSize;
	int    MaxSideOrders;
	int    IdBatchSize;
	int    IdReplenishThreshold;
	int    ManagerSleepTime;
	bool   RelOrders;
	IBString Exchange;
	bool   CloseInitPos;
	int   NumLayers;
	std::string PrefOrders;

	//commission-related
	int    numTiers;
	long  *TiersVol;
	double *TiersCom;
	double Rebate;
	double OtherFees;

	std::map<std::string,int> symbolIdx;

	vector<OrderId> LiqOrders;
	vector<Holding> LiqPositions;

	std::stringstream ostr;
	std::stringstream master_ostr;
	std::stringstream ugorth_ostr;
	std::stringstream devSize_ostr;
	std::stringstream lastOrth_ostr;
	std::stringstream lastPP_ostr;
	std::stringstream pandl_ostr;
	std::stringstream cnt_ostr;
	std::stringstream slippage_ostr;
	std::stringstream slippage2_ostr;

	QuoteApiClientDemo *quoteSystem;
};


#include "orderBook.h"


struct SharedMemoryStruct
{
	int clientId;
	CMarketData *marketData;

	//PCA related
	int *last_cnt;
	double *last;
	double *lastPP;
	double *lastOrth;
	double *lastOrthPrev;
	double lastTime;
	double *eigVectors;
	double *SigmaValue;
	double *deltaPrincipal;

	bool momentumSide;
	Momentum *momentum;
	Momentum *momentumWhenPlaced;
	long *DesiredPosition;
	long *DesiredPositionTmp;
	long *DesiredPositionAll;
	double *sigmaOrigDim;
	double *nbboCurrent;
	double *nbboWhenPlaced;

	CPCA *pca;
	double *dummy_array;
	double *pca_A;
	double *pca_UU;
	double *pca_VV;
	double *singular_values;

	double *vOrth;
	double *vOrthNorm;
	bool DeviationDetected;
	double DeviationSize;
	double sigmaPP;
	double DeviationRange;
	double DeviationSizePrev;
	bool LiquidateDivergentPos;

	//neutralization
	//std::map<double,int> DimDev;
	vector<DimDeviation> DimDev;


	//Orders related
	Cids *ids;
	OrderBook *orderBook;

	//global state
	long *position;
	long *position_prev;
	long *position_srv_prev;
	bool *allowTrading;
	int *positionDiscrepancyCnt;
	int cancelDimension;

	CGlobalState *globalState;

	long AtomicSize;
	double *weights;
	CVWAP *VWAP;

	//Matlab related
	Engine *ep;

	CAccounting *cpa;
	bool *marginable;

	CAlgoBase *algo;

};

#endif //_DATA_H_
