#include <iomanip>
#include <math.h>
#include "algoMF.h"
#include "memory.h"
#include "utils.h"
#include <time.h>

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;

CAlgoBase::CAlgoBase()
{
	SM.lastTime = -1.0;
}

void CAlgoBase::updatePortfolioStats()
{
	double MV = 0.0, PV = 0.0, OSV = 0.0;
	double midpt;
	for(int i=0; i<IM.BasketSize; i++)
	{
		midpt = 0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]);
		MV += SM.position[i]*midpt;
		PV += SM.position[i]*SM.VWAP[i].getVWAP();
		OSV += abs(SM.position[i])*midpt;
	}
	MarketValue = MV;
	PortfolioValue = PV;
	UnrealizedGain = MV - PV;
	OneSidedValue = OSV;
}

void CAlgoBase::updatePortfolioStats(int assetId, long filled_s, bool neutralizable)
{
	updatePortfolioStats();
}

double CAlgoBase::getDistance(double PriceA, double PriceB, double PriceMkt)
{
	double distance;
	if(fabs(PriceA) < TINY) PriceA = PriceMkt;
	if(fabs(PriceB) < TINY) PriceB = PriceMkt;
	distance = fabs(PriceA-PriceB);
	return distance;
}

double CAlgoBase::getDifference(double PriceA, double PriceB, double PriceMkt)
{	//diff = PriceA - priceB
	double difference;
	if(fabs(PriceA) < TINY) PriceA = PriceMkt;
	if(fabs(PriceB) < TINY) PriceB = PriceMkt;
	difference = PriceA-PriceB;
	return difference;
}

bool CAlgoBase::inInterval(double Price, double PriceA, double PriceB, double PriceMkt)
{
	if(fabs(Price) < TINY) Price = PriceMkt;
	if(fabs(PriceA) < TINY) PriceA = PriceMkt;
	if(fabs(PriceB) < TINY) PriceB = PriceMkt;
	return ((Price>PriceA-TINY)&&(Price<PriceB+TINY));
}


void CAlgoBase::tickPrice(int assetId, TickType field, double price, int size, std::string quoteSource)
{
	//if(!IM.algoInitDone)
	//	return;

	IBString timeNow = SM.globalState->getTime();
	IBString timeOfDay = SM.globalState->getTime();

	if(assetId >= IM.BasketSize)
	{
		//error condition
		IM.ostr << "tickPrice: illegal assetId: " << assetId << "\n";
	}

	if(field > NOT_SET)
	{
		IM.ostr << "tickPrice: illegal tick type: " << field << "\n";
		IM.Logger->flushLogs();
		return;
	}

	SM.marketData[assetId].market[field] = price;
	SM.marketData[assetId].last_market_idx_chg = field;

	//if((field==BID)&&(assetId==5))
	//	std::cout << "tickPrice: update: " << timeOfDay << "," << SM.marketData[assetId].contract->symbol << "," << field << "," << price << "\n";

	//do nothing if only the size changed
	if((IM.pca_enable)&&(SM.marketData[assetId].marketPrev[field]==SM.marketData[assetId].market[field]))
	{
		IM.Logger->flushLogs();
		return;
	}


IM.ostr << "tickPrice: update: " << timeOfDay << "," << SM.marketData[assetId].contract->symbol << "," << field << "," << price << "\n";

	//IM.Logger->writeQuote(timeOfDay, assetId,field,price);

//2 cent spread
//when sitting on a gain use limit : oca


	if(((field==BID)||(field==ASK)||(field==LAST))&&
			((SM.marketData[assetId].market[BID]>TINY)&&(SM.marketData[assetId].market[ASK]>TINY))&&
			((SM.marketData[assetId].marketPrev[BID]>TINY)&&(SM.marketData[assetId].marketPrev[ASK]>TINY)))
	{
		IM.ostr << "tickPrice: before getDTime\n";
		double Dtime = SM.globalState->getDTime();

		IM.Logger->writeTAQ((unsigned int)(3600000*Dtime), assetId, (int)field, price*10000, -4, (unsigned int)size, quoteSource);

		IM.ostr << "tickPrice: before updateTAQ\n";
		SM.algo->updateTAQ(assetId, Dtime);

		IM.ostr << "tickPrice: before updatePortfolioStats\n";
		SM.algo->updatePortfolioStats();

		SM.pca->run();
		IM.ostr << "tickPrice: after run\n";

		SM.globalState->calcNLV(assetId);
		SM.globalState->updatePlots(true);
	}
	/*else
	{
		ostr << SM.globalState->getDTime() << "," << assetId << "," << field << "," << price << "\n";
		IM.Logger->writeTickData(ostr.str());
	}*/

	if((field==BID)||(field==ASK))
	{
		SM.marketData[assetId].marketPrev[BID] = SM.marketData[assetId].market[BID];
		SM.marketData[assetId].marketPrev[ASK] = SM.marketData[assetId].market[ASK];
	}
	IM.Logger->flushLogs();
}

void CAlgoBase::updateTAQ(int assetId, double Dtime)
{
	//handle the crossed nbbo case
	if(SM.marketData[assetId].market[ASK]<SM.marketData[assetId].market[BID])
	{
		if(SM.marketData[assetId].market[BID] - SM.marketData[assetId].marketPrev[BID] > TINY) //bid increased, ask stale
			SM.marketData[assetId].market[ASK] = SM.marketData[assetId].market[BID]+0.01;
		else if(SM.marketData[assetId].marketPrev[ASK] - SM.marketData[assetId].market[ASK] > TINY) //ask decreased, bid stale
			SM.marketData[assetId].market[BID] = SM.marketData[assetId].market[ASK]-0.01;
	}

	//handle bid=ask case
	if(SM.marketData[assetId].market[ASK]-SM.marketData[assetId].market[BID]<TINY)
	{
		if(SM.marketData[assetId].market[BID] - SM.marketData[assetId].marketPrev[BID] > TINY) //bid increased
			SM.marketData[assetId].market[ASK] += 0.01;
		else if(SM.marketData[assetId].marketPrev[ASK] - SM.marketData[assetId].market[ASK] > TINY) //ask decreased
			SM.marketData[assetId].market[BID] -= 0.01;
		else
			SM.marketData[assetId].market[BID] = SM.marketData[assetId].market[ASK] - 0.01;
	}


	int increments = (int)((Dtime - SM.lastTime)/0.00006) + 1;  // Hardcoded for now
	if(SM.lastTime < 0) increments = 1;
	increments = MAX(increments, 1);
	//IM.ostr.precision(7);
	IM.ostr << "PCA: Dtime=" << Dtime << ", Dtime_prev=" << SM.lastTime << ", increments=" << increments << "\n";
	//IM.ostr.precision(2);
	if(Dtime < SM.lastTime)
		IM.ostr << "PCA: time reversal\n";

	IM.ostr << "TestClient(" << SM.marketData[assetId].contract->symbol
		<< ") : increments=" << increments
		<< ", (Dtime - SM.lastTime)=" << (Dtime - SM.lastTime)
		<< ", SM.lastTime=" << SM.lastTime
		<< ", SM.last_cnt[assetId]=" << SM.last_cnt[assetId]
		<< "\n";

	SM.lastTime = Dtime;

	Momentum momentum_prev0 = SM.momentum[0];
	Momentum momentum_prev1 = SM.momentum[1];
	Momentum momentum_prev2 = SM.momentum[2];

	IM.ostr << "PCA: Midpt:\n";
	for(int j=0; j<IM.BasketSize; j++)
	{
		double midpt = 0.5*(SM.marketData[j].market[BID]+SM.marketData[j].market[ASK]);
		IM.ostr << midpt << " ";
	}
	IM.ostr << "\n";
	IM.ostr << "PCA: SM.momentum:\n";

	//shift down a column of last values
	//another option: write a custom T'*T and work w/ circular buffer here
	for(int j=0; j<IM.BasketSize; j++)
	{
		double lastPrev = SM.last[j*IM.pca_window];
		double midpt = 0.5*(SM.marketData[j].market[BID]+SM.marketData[j].market[ASK]);
		if(SM.last_cnt[assetId] < IM.pca_window) increments = MIN(increments, IM.pca_window - SM.last_cnt[assetId]);
		else if(increments > IM.pca_window) increments = IM.pca_window;
		for(int i=IM.pca_window-1; i>increments-1; i--) SM.last[i+j*IM.pca_window] = SM.last[i+j*IM.pca_window-increments];
		SM.last[j*IM.pca_window] = midpt;
		for(int i=1; i<increments; i++) SM.last[i+j*IM.pca_window] = SM.last[j*IM.pca_window];

		if(((SM.last_cnt[j]>0)||(j==assetId))&&(SM.last_cnt[j]<IM.pca_window)) SM.last_cnt[j] += increments;

		if(SM.last[j*IM.pca_window]>lastPrev+TINY) SM.momentum[j] = MOMENTUM_UP;
		else if(SM.last[j*IM.pca_window]<lastPrev-TINY) SM.momentum[j] = MOMENTUM_DOWN;
		IM.ostr << SM.momentum[j] << " ";
	}
	IM.ostr << "\n";

	if(SM.momentum[0]!=momentum_prev0) { for(int j=1; j<IM.BasketSize; j++) SM.momentum[j] = SM.momentum[0]; }
	else if(SM.momentum[1]!=momentum_prev1)
	{
		for(int j=0; j<IM.BasketSize; j++)
		{
			if(j==1) continue;
			SM.momentum[j] = SM.momentum[1];
		}
	}
	else if(SM.momentum[2]!=momentum_prev2)
	{
		for(int j=0; j<IM.BasketSize; j++)
		{
			if(j==2) continue;
			SM.momentum[j] = SM.momentum[2];
		}
	}

}


void CAlgoBase::CEP(int idx, int tradeType, double price, int size)
{
	cep.processMsg(idx, tradeType, price, size);
}
