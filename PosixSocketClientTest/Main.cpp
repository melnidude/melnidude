#ifdef _WIN32
# include <Windows.h>
# define sleep( seconds) Sleep( seconds * 1000);
#else
# include <unistd.h>
#endif

#include <ios>
#include <iostream>
#include <fstream>
#include <sstream>

#include "ConfigFile.h"
#include "PosixTestClient.h"
#include "memory.h"
#include "globalState.h"
#include "vwap.h"
#include "manager.h"
#include "marketData.h"
#include "algoMF.h"
#include "algoHF.h"
#include "algoHF2.h"

#include "QuoteApiClientDemo_1_01.hh"

#ifdef _MATLAB_AMD64
#define MATLAB_COMMANDLINE "/home/duroc/Matlab/R2010a/bin/matlab -nodesktop"
#else
#define MATLAB_COMMANDLINE "\0"
#endif

using namespace std;

SharedMemoryStruct SM;
InvariantMemoryStruct IM;

const unsigned MAX_ATTEMPTS = 50;
const unsigned SLEEP_TIME = 10;

//IBString log_fname;

#define PCA_WINDOW 20


int main(int argc, char** argv)
{
	ifstream myfile;
	int NumNeutrDim;

	//initialize algorithm
	ConfigFile config("config.txt");
	IBString log_fname, tick_data_file, pandl_data_file, count_data_file, ugorth_data_file, devsize_data_file, lastorth_data_file, lastpp_data_file, slippage_data_file, slippage2_data_file, quotes_file, algorithm;
	config.readInto(SM.clientId, "ClientId");
	config.readInto(algorithm, "Algorithm");
	config.readInto(IM.enableIB, "EnableIB");
	config.readInto(IM.quoteSource, "QuoteSource");
	config.readInto(IM.enableTAQ, "EnableTAQ");
	config.readInto(IM.ClientServer, "ClientServer");
	config.readInto(log_fname, "LogFileName");
	config.readInto(tick_data_file, "TickDataFile");
	config.readInto(pandl_data_file, "PandLDataFile");
	config.readInto(count_data_file, "CountDataFile");
	config.readInto(ugorth_data_file, "UGorthFile");
	config.readInto(devsize_data_file, "DevSizeFile");
	config.readInto(lastpp_data_file, "LastPPFile");
	config.readInto(lastorth_data_file, "LastOrthFile");
	config.readInto(slippage_data_file, "SlippageFile");
	config.readInto(slippage2_data_file, "Slippage2File");
	config.readInto(quotes_file, "QuotesFile");
	config.readInto(IM.PercentCash, "PercentCash");
	config.readInto(IM.MaxLeverage, "MaxLeverage");
	config.readInto(IM.MaxPurchase, "MaxPurchase");
	config.readInto(IM.IdBatchSize, "IdBatchSize");
	config.readInto(IM.IdReplenishThreshold, "IdReplThr");
	config.readInto(IM.pca_window, "PCAwindow");
	config.readInto(IM.pca_window_init, "PCAwindowInit");
	config.readInto(IM.OrderSizeSF, "OrderSizeSF");
	config.readInto(IM.ReductionFactor, "ReductionF");
	config.readInto(IM.RelOrders, "RelOrders");
	config.readInto(IM.Exchange, "Exchange");

	if(IM.enableIB) IM.enableMatlab = true;
	else IM.enableMatlab = false;
IM.enableMatlab = false;

	IBString Basket, ticker;
	config.readInto(Basket, "Basket");
	std::stringstream os(Basket);
	int numPortfolioTickers = 0;
	while (os >> ticker) numPortfolioTickers++;
	IM.BasketSize = numPortfolioTickers;

	//initialize logging
	IM.Logger = new CLog(IM.ClientServer, log_fname, tick_data_file, pandl_data_file, count_data_file, ugorth_data_file, devsize_data_file, lastorth_data_file,
			lastpp_data_file, slippage_data_file, slippage2_data_file, quotes_file, numPortfolioTickers, IM.enableTAQ);

	IM.ostr.setf(std::ios::fixed);
	IM.ostr.precision(3);

	IM.master_ostr.setf(std::ios::fixed);
	IM.master_ostr.precision(7);

	IM.ugorth_ostr.setf(std::ios::fixed);
	IM.ugorth_ostr.precision(7);

	IM.devSize_ostr.setf(std::ios::fixed);
	IM.devSize_ostr.precision(7);

	IM.lastOrth_ostr.setf(std::ios::fixed);
	IM.lastOrth_ostr.precision(7);

	IM.lastPP_ostr.setf(std::ios::fixed);
	IM.lastPP_ostr.precision(7);

	IM.pandl_ostr.setf(std::ios::fixed);
	IM.pandl_ostr.precision(7);

	IM.cnt_ostr.setf(std::ios::fixed);
	IM.cnt_ostr.precision(7);

	IM.slippage_ostr.setf(std::ios::fixed);
	IM.slippage_ostr.precision(7);
	IM.slippage2_ostr.setf(std::ios::fixed);
	IM.slippage2_ostr.precision(7);

	IM.quoteSystem = new QuoteApiClientDemo(true);
	SM.marketData = new CMarketData[numPortfolioTickers];

	numPortfolioTickers = 0;
	std::stringstream os1(Basket);
	while (os1 >> ticker)
	{
		SM.marketData[numPortfolioTickers].contract->symbol = ticker.c_str();
#if COMBO
		IM.client->reqContractDetails(SM.ids->getNextId(numPortfolioTickers), *SM.marketData[numPortfolioTickers].contract);
#endif
		IM.Logger->setSymbol(numPortfolioTickers, SM.marketData[numPortfolioTickers].contract->symbol);
		numPortfolioTickers++;
	}

	IM.Logger->writeMasterHeader();

	IM.client = new PosixTestClient;
	IM.host = argc > 1 ? argv[1] : "";
	IM.port = 7496;

	SM.last = new double[numPortfolioTickers*IM.pca_window];
	SM.last_cnt = new int[numPortfolioTickers];
	SM.lastPP = new double[numPortfolioTickers];
	SM.lastOrth = new double[numPortfolioTickers];
	SM.lastOrthPrev = new double[numPortfolioTickers];
	SM.position = new long[numPortfolioTickers];
	SM.DesiredPosition = new long[numPortfolioTickers];
	SM.DesiredPositionTmp = new long[numPortfolioTickers];
	SM.DesiredPositionAll = new long[numPortfolioTickers];
	SM.sigmaOrigDim = new double[numPortfolioTickers];
	SM.nbboCurrent = new double[numPortfolioTickers];
	SM.nbboWhenPlaced = new double[numPortfolioTickers];
	SM.position_prev = new long[numPortfolioTickers];
	SM.position_srv_prev = new long[numPortfolioTickers];
	SM.allowTrading = new bool[numPortfolioTickers];
	SM.positionDiscrepancyCnt = new int[numPortfolioTickers];
	SM.weights = new double[numPortfolioTickers];
	SM.orderBook = new OrderBook[numPortfolioTickers];
	SM.momentum = new Momentum[numPortfolioTickers];
	SM.momentumWhenPlaced = new Momentum[numPortfolioTickers];
	for(int j=0; j<numPortfolioTickers; j++)
	{
		SM.position[j] = 0;
		SM.DesiredPosition[j] = 0;
		SM.position_prev[j] = 0;
		SM.position_srv_prev[j] = 0;
		SM.positionDiscrepancyCnt[j] = 0;
		SM.orderBook[j].setAssetId(j);
		if(j>0) SM.orderBook[j].setNeutralizer(&SM.orderBook[0]);
		SM.momentum[j] = MOMENTUM_NONE;
	}

	SM.VWAP = new CVWAP[numPortfolioTickers];
	for(int j=0; j<numPortfolioTickers; j++) SM.VWAP[j].setAssetId(j);

	IM.costAvgOffset = new double[numPortfolioTickers];
	IM.nbboOffset = new double[numPortfolioTickers];
	IM.vwapOffset = new double[numPortfolioTickers];
	IM.lossOffset = new double[numPortfolioTickers];
	config.readArray(IM.costAvgOffset, "costAvgOffset", numPortfolioTickers);
	config.readArray(IM.nbboOffset, "nbboOffset", numPortfolioTickers);
	config.readArray(IM.lossOffset, "lossOffset", numPortfolioTickers);
	config.readArray(IM.vwapOffset, "vwapOffset", numPortfolioTickers);
	config.readArray(IM.sigmaFactor, "SigmaFactor", 2);
	config.readInto(IM.minOrderSize, "minOrderSize");
	config.readInto(IM.maxOrderSize, "maxOrderSize");
	config.readInto(IM.MaxSideOrders, "MaxSideOrders");
	config.readInto(IM.ManagerSleepTime, "MgrSleepTime");
	config.readInto(IM.Overshoot, "Overshoot");
	config.readInto(IM.CostAverage, "CostAverage");
	config.readInto(IM.UseMomentum, "UseMomentum");
	config.readInto(IM.CloseInitPos, "CloseInitPos");
	config.readInto(IM.NumLayers, "NumLayers");
	config.readInto(IM.PrefOrders, "PrefOrders");
	config.readInto(NumNeutrDim, "NumNeutrDim");
	NumNeutrDim = MAX(MIN(NumNeutrDim, IM.BasketSize), 1);

	//set the algorithm
	//if(algorithm == "MF") SM.algo = new CAlgoMF;
	//else if(algorithm == "HF") SM.algo = new CAlgoHF;
	//else if(algorithm == "HF2")
		SM.algo = new CAlgoHF2(NumNeutrDim);

	//accounting-related
	double IMR;
	config.readInto(IMR, "InitMarginReq");
	SM.cpa = new CAccounting(IMR, 0);
	SM.marginable = new bool[IM.BasketSize];
	bool specified = config.readArray(SM.marginable, "Marginable", numPortfolioTickers);
	if(!specified) for(int i=0; i<IM.BasketSize; i++) SM.marginable[i] = false;

	SM.globalState->readCommissionSchedule(config);


	/******* Initialize shared memory *********/
	IM.algoInitDone = false;

	IM.pca_enable = false;
	IM.debugPCA = false;

	//PCA related
	SM.eigVectors = new double[IM.BasketSize*IM.BasketSize];
	SM.SigmaValue = new double[IM.BasketSize];
	SM.deltaPrincipal = new double[IM.BasketSize];
	SM.vOrth = new double[IM.BasketSize];
	SM.vOrthNorm = new double[IM.BasketSize];

	SM.cancelDimension = -1;

	for(int i=0; i<IM.BasketSize; i++)
	{
		SM.last_cnt[i] = 0;
		SM.position[i] = 0;
		SM.allowTrading[i] = true;
		//for(int j=0; j<IM.pca_window; j++) SM.last[j+i*IM.pca_window] = i+j;
	}

	SM.pca = new CPCA;
	//SM.dummy_array = new double[IM.BasketSize];
	SM.pca_A = new double[IM.BasketSize*IM.BasketSize];
	//SM.pca_UU = new double[IM.BasketSize*IM.BasketSize];
	//SM.pca_VV = new double[IM.BasketSize*IM.BasketSize];
	//SM.singular_values = new double[IM.BasketSize];

    /**********************************************/
	SM.globalState = new CGlobalState(numPortfolioTickers);
	SM.ids = new Cids;

	if(IM.enableIB)
	{
		IM.client->disconnect();
		IM.ostr << "Connecting with clientId " << SM.clientId << "\n";
		IM.client->connect(IM.host, IM.port, SM.clientId);
		while(!IM.client->isConnected()) { sleep(1); }
	}

	for(int i=0; i<IM.BasketSize; i++)
	{
		SM.marketData[i].orderB->clientId = SM.clientId;
		SM.marketData[i].orderS->clientId = SM.clientId;
	}


	// start Matlab
	if(IM.enableMatlab)
	{
		IM.ostr << "Starting the Matlab engine...\n";
		if (!(SM.ep = engOpen(MATLAB_COMMANDLINE))) {
			printf("\nCan't start MATLAB engine\n");
			exit(1);
		}

		// initialized Matlab variables
		engEvalString(SM.ep, "cd Logs;");
	}

	if(IM.enableIB)
	{
		// Allocate and init market data ids
		IM.client->reserveIds(IM.IdBatchSize);
		OrderId mktDataStartingId;
		while((mktDataStartingId=SM.ids->recdId()) < 0)
			IM.client->processMessages();
		SM.ids->init(mktDataStartingId, IM.BasketSize);
		for(int i=0; i<IM.BasketSize; i++)
			SM.marketData[i].mktDataId = SM.ids->getMarketDataId(i);

		// Start the market data streams
		//IM.client->reqTickData();

		//liquidate previous session's orders and positions
		cancelAllOpenOrders();
		liquidateAllPositions();
		IM.client->reqAccountUpdates(true);
		IM.client->reqTickData();
	}

	IM.quoteSystem->LimeLogin(IM.quoteSource);

	IM.algoInitDone = true;

	IM.ostr << "Starting the main loop\n";

	if(IM.enableIB)
	{
		while(IM.client->isConnected()) {
			IM.client->processMessages();
		}
	}
	else
	{
		while(SM.globalState->getDTime() < 15.00) { custom_sleep(1); };
	}

	IM.ostr << "Done with the main while loop\n";

	if(IM.enableMatlab) engClose(SM.ep);

	if(IM.enableIB)
	{
		if(!IM.LiqOrders.empty()) IM.LiqOrders.clear();
		if(!IM.LiqPositions.empty()) IM.LiqPositions.clear();
	}

	if(IM.quoteSource!="IB")
    {
		IM.quoteSystem->unsubscribeAll(IM.quoteSource);
		IM.quoteSystem->logout();
    }

	printf ( "End of POSIX Socket Client Test\n");
}

