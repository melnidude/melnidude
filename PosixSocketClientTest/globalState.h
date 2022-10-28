#ifndef _GLOBALSTATE_H_
#define _GLOBALSTATE_H_

#include <math.h>
#include "defs.h"
#include "engine.h"
#include "ConfigFile.h"

#include "EPosixClientSocket.h"
#include "EWrapper.h"
#include "PosixTestClient.h"
#include "Contract.h"
#include "Order.h"
#include "ids.h"
#include "globalState.h"


class CGlobalState
{
public:
	CGlobalState(int numPortfolioTickers);
	~CGlobalState()
	{
		delete m_PandLDim;
		delete m_CashDim;
		delete m_numTradesDim;
		delete m_numSharesDim;
		delete m_chExtraText;
		delete m_TmpText;
	};

	double getCash() { return m_Cash; }
	double getNLV() { return m_NLV; }
	double getPandL() { return m_PandL; }
	double getNoSlippagePandL() { return m_PandLNoSlippage; }

	void setNLV(double NLV);
	void setCash(double amount);
	void updateCash(int assetId, double delta, double deltaNoSlippage);
	void calcNLV(int assetId);
	void updateExposure(bool bidAsk, double delta);
	double getTimeSinceStarting();
	double getTimeSinceStarting(struct timespec refTime);
	IBString getTime();
	IBString getPreciseTime();
	double getDTime();
	void incrTrades(int assetId, int filled, ShortOrder *m_order);
	void calcPandLcontributions(int assetId, double pl_delta, ShortOrder *order);
	void readCommissionSchedule(ConfigFile config);
	double calcCommission(double auxPrice);
	void calcCorrCoefficient();
	void updatePlots(bool nbboChgOnly);
	IBString getTimePrefix();
	void LimeLogin(void *quoteSystem, std::string quoteSource);

	double m_rho_num;
	double m_rho_den;

private:
	//double m_Budget;
	double m_Cash;
	double m_CashServer;
	double m_CashNoSlippage;
	double m_NLV;
	double m_NLVNoSlippage;
	double m_NLVServer;
	double m_NLVinit;
	double m_Exposure[2];
	double m_PandL;
	double m_PandLServer;
	double m_PandLNoSlippage;
	bool initialized;
	double m_numTrades;
	double m_numShares;

	double *m_PandLDim;
	double *m_CashDim;
	long *m_numTradesDim;
	long *m_numSharesDim;

	mxArray *Dtime;
	mxArray *PandL;
	mxArray *PandLServer;
	mxArray *NumTrades;
	mxArray *NumShares;
	mxArray *ExtraText;
	char *m_chExtraText;
	char *m_TmpText;

	double m_commission;

	struct timespec programStartTime;

	//vector<OrderId> LiqOrders;
	//vector<IBString> LiqPositions;

	long m_ImmLiq_Cnt;
	long m_Liq_Cnt;
	double m_PandL_ImmLiq;
	double m_PandL_Liq;
	double m_PandL_CostAve;
	double m_PandL_Neutr;

	bool spawned;
};


#endif //_GLOBALSTATE_H_
