#include <iomanip>
#include <math.h>
#include "memory.h"
#include <time.h>

#include "QuoteApiClientDemo_1_01.hh"

#ifdef _MATLAB_AMD64
#define MATLAB_PLOTS "/home/duroc/Matlab/R2010a/bin/matlab -nodesktop -r 'run Logs/runPlots' &"
#else
#define MATLAB_PLOTS "matlab -r 'run Logs/runPlots' &"
#endif

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;

CGlobalState::CGlobalState(int numPortfolioTickers)
{
	IM.ostr << "Allocating GlobalState variables...\n";

	clock_gettime( CLOCK_REALTIME, &programStartTime);

	m_Cash = -INFINITY_D;
	m_CashServer = -INFINITY_D;
	m_CashNoSlippage = -INFINITY_D;
	m_NLV = -INFINITY_D;
	m_NLVServer = -INFINITY_D;
	m_NLVinit = -INFINITY_D;
	m_NLVNoSlippage = -INFINITY_D;

	m_Exposure[0] = 0.0;
	m_Exposure[1] = 0.0;
	m_PandL = 0.0;
	m_PandLServer = 0.0;
	m_PandLNoSlippage = 0.0;
	m_numTrades = 0.0;
	m_numShares = 0.0;
	initialized = false;

	m_rho_num = 0.0;
	m_rho_den = 0.0;

	Dtime = mxCreateDoubleMatrix(1,1,mxREAL);
	PandL = mxCreateDoubleMatrix(1,1,mxREAL);
	PandLServer = mxCreateDoubleMatrix(1,1,mxREAL);
	NumTrades = mxCreateDoubleMatrix(1,1,mxREAL);
	NumShares = mxCreateDoubleMatrix(1,1,mxREAL);

	m_PandLDim = new double[numPortfolioTickers];
	m_CashDim = new double[numPortfolioTickers];
	m_numTradesDim = new long[numPortfolioTickers];
	m_numSharesDim = new long[numPortfolioTickers];
	m_chExtraText = new char[1000];
	m_TmpText = new char[500];
	for(int i=0; i<numPortfolioTickers; i++)
	{
		m_PandLDim[i] = 0.0;
		m_CashDim[i] = 0.0;
		m_numTradesDim[i] = 0;
		m_numSharesDim[i] = 0;
	}

	m_ImmLiq_Cnt = 0;
	m_Liq_Cnt = 0;
	m_PandL_ImmLiq = 0.0;
	m_PandL_Liq = 0.0;
	m_PandL_CostAve = 0.0;
	m_PandL_Neutr = 0.0;

	spawned = false;
};

void CGlobalState::setNLV(double NLV)
{
	m_NLVServer = NLV;
	IM.ostr << "server NLV: " << m_NLVServer << "\n";

	if(!initialized)
	{
		m_NLVinit = m_NLVServer;
		m_Cash = m_NLVServer*IM.PercentCash;
		m_CashNoSlippage = m_NLVServer*IM.PercentCash;
		m_NLV = m_Cash;
		m_NLVNoSlippage = m_CashNoSlippage;
		SM.cpa->initCash(m_NLVServer*IM.PercentCash, 0, 0, 0, 0);
		IM.ostr << "GlobalState: m_NLVinit=" << m_NLVinit << "\n";
		initialized = true;
	}
	else
	{
		m_PandLServer = m_NLVServer - m_NLVinit;

		double commission = IM.TiersCom[IM.numTiers-1];
		for(int i=0; i<IM.numTiers; i++)
		{
			if(m_numShares < IM.TiersVol[i])
			{
				commission = IM.TiersCom[i];
				break;
			}
		}

		double commission_adj = 0.0;
		commission_adj -= 0.005;
		//if(fabs(auxPrice)<TINY)
		commission_adj -= IM.Rebate;
		commission_adj += commission;
		commission_adj += IM.OtherFees;
		m_PandLServer -= commission_adj*m_numShares;

		updatePlots(false);
	}
}

void CGlobalState::updatePlots(bool nbboChgOnly)
{

	if(!initialized)
		return;

	if(m_numTrades<=0)
		return;

	if(nbboChgOnly)
		return;

	IM.ostr << "updatePlots: m_PandL=" << m_PandL << ",  m_PandLServer=" << m_PandLServer << ", m_PandLNoSlippage=" << m_PandLNoSlippage << ", m_numTrades=" << m_numTrades <<
		", m_numShares=" << m_numShares << "\n";

	double dtime = SM.globalState->getDTime();

	IM.pandl_ostr << dtime << "," << 82 << "," << m_PandLNoSlippage << "," << 0 << "\n";
	IM.pandl_ostr << dtime << "," << 83 << "," << m_PandL << "," << 0 << "\n";
	IM.pandl_ostr << dtime << "," << 84 << "," << m_PandLServer << "," << 0 << "\n";

	IM.cnt_ostr << dtime << "," << 85 << "," << m_numTrades << "," << 0 << "\n";
	IM.cnt_ostr << dtime << "," << 86 << "," << m_numShares << "," << 0 << "\n";

	IM.Logger->flushLogs();

	if(!spawned)
	{
		IM.ostr << "spawning\n";

		spawned = true;
		system(MATLAB_PLOTS);
#if 0
		pid_t pID = fork();
		IM.ostr << "\tpid=" << pID << "\n";
		if(pID == 0)                // child
		{
			while(1)
			{
				engEvalString(SM.ep, "runPlots(0);");
				sleep(5);
				IM.ostr << "\tpid=" << pID << ", ranPlots\n";
				IM.Logger->flushLogs();
			}
			exit(0);
		}
		else if(pID < 0)            // failed to fork
		{
			cerr << "Failed to fork" << endl;
			exit(1);
		}
		else                                   // parent
		{
			IM.ostr << "\tpid=" << pID << ", ranPlots\n";
		}
#endif
	}

	//engEvalString(SM.ep, "runPlots;");
	IM.ostr << "finished updatePlots\n";

}

void CGlobalState::updateCash(int assetId, double delta, double deltaNoSlippage)
{
	m_Cash -= delta;
	m_CashDim[assetId] -= delta;
	m_CashNoSlippage -= deltaNoSlippage;
	IM.ostr << "Cash: " << m_Cash << "\n";
}

void CGlobalState::updateExposure(bool bidAsk, double delta)
{
	//IBString side = (bidAsk) ? "Ask" : "Bid";
	m_Exposure[bidAsk] += delta;
}

void CGlobalState::setCash(double amount)
{
	if(m_CashServer < -VERY_LARGE)
	{
		//m_Cash = amount*IM.PercentCash;
	}
	m_CashServer = amount;
}

void CGlobalState::calcNLV(int assetId)
{
	double NLV = m_Cash;
	double NLVNoSlippage = m_CashNoSlippage;
	long pos;

	//IM.ostr << "GlobalState 1: NLV=" << NLV << ", NLVNoSlippage=" << NLVNoSlippage << "\n";

	for(int i=0; i<IM.BasketSize; i++)
	{
		pos = SM.position[i];
		NLV += pos*0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]);
		NLVNoSlippage += pos*0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]);
		/*IM.ostr << "calcNLV: pos=" << pos << ", bid=" << SM.marketData[i].market[BID] << ", ask=" << SM.marketData[i].market[ASK] << ", NLV="
				<< NLV << "\n";*/
	}
	m_NLV = NLV;
	m_NLVNoSlippage = NLVNoSlippage;

	m_PandL = m_NLV - m_NLVinit*IM.PercentCash;
	m_PandLNoSlippage = m_NLVNoSlippage - m_NLVinit*IM.PercentCash;

	m_PandLDim[assetId] = m_CashDim[assetId] +
			SM.position[assetId]*0.5*(SM.marketData[assetId].market[BID]+SM.marketData[assetId].market[ASK]);

	//IM.ostr << "GlobalState: PandL=" << m_PandL << ", m_PandLNoSlippage=" << m_PandLNoSlippage << ", m_NLV=" << m_NLV << ", m_NLVinit=" << m_NLVinit << ", IM.PercentCash=" << IM.PercentCash << "\n";
}

void CGlobalState::incrTrades(int assetId, int filled, ShortOrder *m_order)
{
	//trades, shares and P&L overall
	bool firstFill = (m_order->cumulativeFill==0);
	m_numTrades += firstFill;
	m_numShares += filled;

	m_numTradesDim[assetId] += firstFill;
	m_numSharesDim[assetId] += filled;

	//calcCorrCoefficient();

	updatePlots(false);
	IM.ostr << "finished incrTrades\n";
}

void CGlobalState::calcPandLcontributions(int assetId, double pl_delta, ShortOrder *order)
{
	if(order->stateDesc=="ImmLiq") { m_PandL_ImmLiq += pl_delta; m_ImmLiq_Cnt++; }
	else if(order->stateDesc=="Liq") { m_PandL_Liq += pl_delta; m_Liq_Cnt++; }
	else if(order->stateDesc=="CostAve") m_PandL_CostAve += pl_delta;
	else if(order->stateDesc=="Neutralize") m_PandL_Neutr += pl_delta;

	IM.ostr << "CGlobalState: order->stateDesc=" << order->stateDesc << "\n";
	IM.ostr << "CGlobalState: ImmLiq_Cnt=" << m_ImmLiq_Cnt << ", Liq_Cnt=" << m_Liq_Cnt << "\n";
	IM.ostr << "CGlobalState: PandL_ImmLiq=" << m_PandL_ImmLiq << ", PandL_Liq=" << m_PandL_Liq << ", PandL_CostAve=" << m_PandL_CostAve << ", PandL_Neutr=" << m_PandL_Neutr << "\n";

}

void CGlobalState::calcCorrCoefficient()
{
	m_rho_num = 0.0;
	double norm_pos = 0.0;
	double norm_last = 0.0;

	for(int i=0; i<IM.BasketSize; i++)
	{
		m_rho_num += SM.position[i]*SM.marketData[i].market[LAST];
		norm_pos += SM.position[i]*SM.position[i];
   		norm_last += SM.marketData[i].market[LAST]*SM.marketData[i].market[LAST];
	}
	m_rho_den = pow(norm_last*norm_pos,0.5);
	IM.ostr << "m_rho_num=" << m_rho_num << ", m_rho_den=" << m_rho_den << "\n";
}

double CGlobalState::getTimeSinceStarting()
{
	struct timespec present;
	double time_in_seconds;

	clock_gettime( CLOCK_REALTIME, &present);

	time_in_seconds = ( present.tv_sec - programStartTime.tv_sec )
                     + (double)( present.tv_nsec - programStartTime.tv_nsec )
                       / (double)BILLION;
	return time_in_seconds;
}

double CGlobalState::getTimeSinceStarting(struct timespec refTime)
{
	struct timespec present;
	double time_in_seconds;

	clock_gettime( CLOCK_REALTIME, &present);

	time_in_seconds = ( present.tv_sec - refTime.tv_sec )
                     + (double)( present.tv_nsec - refTime.tv_nsec )
                       / (double)BILLION;
	return time_in_seconds;
}

IBString CGlobalState::getTime()
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);

	char day[4], mon[4];
	int wday, hh, mm, ss, year;
	std::stringstream ostr;

	sscanf(ctime((time_t*) &(ts.tv_sec)), "%s %s %d %d:%d:%d %d",
	day, mon, &wday, &hh, &mm, &ss, &year);

	ostr << hh << ':' << mm << ':' << ss << '.' <<
			(ts.tv_nsec/10000000);

	return ostr.str();
}

IBString CGlobalState::getPreciseTime()
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);

	char day[4], mon[4];
	int wday, hh, mm, ss, year;
	std::stringstream ostr;

	sscanf(ctime((time_t*) &(ts.tv_sec)), "%s %s %d %d:%d:%d %d",
	day, mon, &wday, &hh, &mm, &ss, &year);

	ostr << hh << ':' << mm << ':' << ss << '.' << ts.tv_nsec;

	return ostr.str();
}

IBString CGlobalState::getTimePrefix()
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);

	char day[4], mon[4];
	int wday, hh, mm, ss, year;
	std::stringstream ostr;

	sscanf(ctime((time_t*) &(ts.tv_sec)), "%s %s %d %d:%d:%d %d",
	day, mon, &wday, &hh, &mm, &ss, &year);

	ostr << hh << mm;

	return ostr.str();
}

double CGlobalState::getDTime()
{
	struct timespec ts;
	double dtime;

	clock_gettime(CLOCK_REALTIME, &ts);

	char day[4], mon[4];
	int wday, hh, mm, ss, year;
	std::stringstream ostr;

	sscanf(ctime((time_t*) &(ts.tv_sec)), "%s %s %d %d:%d:%d %d",
			day, mon, &wday, &hh, &mm, &ss, &year);

IM.ostr << "getDTime: ss=" << ss << ", tv_nsec=" << ts.tv_nsec << "\n";

	dtime = hh + ((double)mm)/60.0 + ((double)ss)/3600.0 + ((double)ts.tv_nsec)/(3600000000000.0);

	return dtime;
}

void CGlobalState::readCommissionSchedule(ConfigFile config)
{
	IBString TiersVol, TiersCom, tier;
	config.readInto(TiersVol, "TiersVol");
	std::stringstream os_tier1(TiersVol);

	int numTiers = 0;
	while (os_tier1 >> tier) numTiers++;
	IM.numTiers = numTiers;
	IM.TiersVol = new long[numTiers];
	IM.TiersCom = new double[numTiers];

	config.readArray(IM.TiersVol, "TiersVol", numTiers);
	config.readArray(IM.TiersCom, "TiersCom", numTiers);

	config.readInto(IM.Rebate, "Rebate");
	config.readInto(IM.OtherFees, "OtherFees");
}

double CGlobalState::calcCommission(double auxPrice)
{
	double commission = IM.TiersCom[IM.numTiers-1];
	for(int i=0; i<IM.numTiers; i++)
	{
		if(m_numShares < IM.TiersVol[i])
		{
			commission = IM.TiersCom[i];
			break;
		}
	}

	if(fabs(auxPrice)<TINY)
		commission -= IM.Rebate;
	commission += IM.OtherFees;
	//commission -= 0.005;
	return commission;
}


void CGlobalState::LimeLogin(void *quoteSystem1, std::string quoteSource)
{
	QuoteApiClientDemo *quoteSystem = (QuoteApiClientDemo *)quoteSystem1;
	if(IM.quoteSource=="Lime")
	{
		string hostname = "74.120.51.3:7047";
		string password = "limeapi";
		string username = "GREENRIVER";
		while(quoteSystem->login(hostname, username, password) < 0)  {
			std::cout << SM.globalState->getDTime() << " : error logging in" << std::endl;
			sleep(30);
			//exit(1);
		}
		QuoteSystemApi::SymbolType symbol_type = QuoteSystemApi::symbolTypeNormal;
		QuoteSystemApi::SubscriptionType subscription_type = QuoteSystemApi::subscriptionTypeMarketData;
		std::vector<std::string> symbols;

		for(int i=0; i<IM.BasketSize; i++)
		{
			symbols.push_back(SM.marketData[i].contract->symbol);
			IM.symbolIdx[SM.marketData[i].contract->symbol] = i;
		}

		quoteSystem->enableTopOfBook();
		int sub_result = quoteSystem->subscribe(quoteSource, symbols, symbol_type, subscription_type);
		if(sub_result < 0) {
			std::cout << "error subscribing" << std::endl;
			exit(1);
		}
	}
}

