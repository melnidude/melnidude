#include <iomanip>
#include <math.h>
#include "algoMF.h"
#include "memory.h"
#include "utils.h"
#include <time.h>

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;

CAlgoMF::CAlgoMF()
{
    run[ALL_CASH] = (bool (*)(long *, double *, double *, double *))&CAlgoMF::afterPCA_AllCash;
    run[PLACING_GETTING_INTO_POS_ORDERS] = (bool (*)(long *, double *, double *, double *))&CAlgoMF::afterPCA_PlacingPositionOrders;
    run[GETTING_INTO_POS] = (bool (*)(long *, double *, double *, double *))&CAlgoMF::afterPCA_GettingIntoPosition;
    run[IN_POS] = (bool (*)(long *, double *, double *, double *))&CAlgoMF::afterPCA_InPosition;
    run[PLACING_LIQ_ORDERS] = (bool (*)(long *, double *, double *, double *))&CAlgoMF::afterPCA_PlacingLiquidationOrders;
    run[LIQUIDATING_POS] = (bool (*)(long *, double *, double *, double *))&CAlgoMF::afterPCA_LiquidatingPosition;

    lastVectorAdj = new double[IM.BasketSize];
	positionOrth = new double[IM.BasketSize];
	positionPP = new double[IM.BasketSize];
	vwap = new double[IM.BasketSize];

	algState = ALL_CASH;
}


BracketHierarchy CAlgoMF::GenerateDesiredCumulativeOrders(int assetId, Order **orderB, Order **orderS)
{
	if((abs(SM.position[assetId])<IM.minOrderSize)&&(SM.DeviationDetected))  //what initial position to take
	{
		TakeOnPosition(assetId, orderB, orderS);
	}
	else if(SM.LiquidateDivergentPos) //one-sided
	{
		LiquidatePosition(assetId, orderB, orderS);
	}

	cancelNotNeededOrders(assetId, *orderB, *orderS);

	return LEGS_EQUAL;
}

void CAlgoMF::TakeOnPosition(int assetId, Order **orderB_ptr, Order **orderS_ptr)
{
	Order *orderB, *orderS;
	MyOrder BidAsk[2];
	int K = IM.BasketSize;
	int principal=K-1, orthogonal=K-2;
	OSTR_DEF;

	BidAsk[BID_ORDER].init(0);
	BidAsk[ASK_ORDER].init(INFINITY_D);

	BidAsk[BID_ORDER].size = 0;
	BidAsk[ASK_ORDER].size = 0;
	OSTR_RESET
	ostr << "OrderMgr " << SM.marketData[assetId].contract->symbol
			<< ": DeviationDetected=" << SM.DeviationDetected << ", lastOrth[assetId]="
			<< SM.lastOrth[assetId] << ", DeviationSize=" << SM.DeviationSize << "\n";
	OSTR_LOG(LOG_MSG);
	if(fabs(SM.lastOrth[assetId]) > MIN_NORM_FACTOR*SM.DeviationSize)
	{
		BidAsk[(SM.lastOrth[assetId] > TINY)].size = abs(SM.DesiredPosition[assetId]);
	}

	BidAsk[BID_ORDER].price = SM.marketData[assetId].market[BID]; //0.01;
	BidAsk[ASK_ORDER].price = SM.marketData[assetId].market[ASK]; //0.01;

	//stopgap logic
	if(BidAsk[BID_ORDER].size < IM.minOrderSize) BidAsk[BID_ORDER].size = 0;
	if(BidAsk[ASK_ORDER].size < IM.minOrderSize) BidAsk[ASK_ORDER].size = 0;

	//now determine price and order type
	if(BidAsk[BID_ORDER].size > 0)
	{
		orderB = *orderB_ptr = SM.marketData[assetId].orderB;
		orderB->totalQuantity = BidAsk[BID_ORDER].size;
		orderB->stateDesc = "EstPosn";
		//orderB->nbboPriceCap = 0.0;
		orderB->orderType = "REL";
		orderB->auxPrice = getAuxPrice(assetId);
		orderB->lmtPrice = 0.0; //BidAsk[BID_ORDER].price-IM.nbboOffset[assetId];

		OSTR_RESET
		ostr << "OrderMgr: desired: L " << orderB->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< " at " << orderB->lmtPrice << "\n";
		OSTR_LOG(LOG_MSG);
	}

	if(BidAsk[ASK_ORDER].size > 0)
	{
		orderS = *orderS_ptr = SM.marketData[assetId].orderS;
		orderS->totalQuantity = BidAsk[ASK_ORDER].size;
		orderS->stateDesc = "EstPosn";
		//orderS->nbboPriceCap = 0.0;
		orderS->orderType = "REL";
		orderS->auxPrice = getAuxPrice(assetId);
		orderS->lmtPrice = 0.0; //BidAsk[ASK_ORDER].price+IM.nbboOffset[assetId];

		OSTR_RESET
		ostr << "OrderMgr: desired: S " << orderS->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< " at " << orderS->lmtPrice << "\n";
		OSTR_LOG(LOG_MSG);
	}
}

void CAlgoMF::LiquidatePosition(int assetId, Order **orderB_ptr, Order **orderS_ptr)
{
	Order *orderB, *orderS;
	MyOrder BidAsk[2];
	int K = IM.BasketSize;
	int principal=K-1, orthogonal=K-2;
	bool side;
	OSTR_DEF;

	BidAsk[BID_ORDER].init(0);
	BidAsk[ASK_ORDER].init(INFINITY_D);
	BidAsk[BID_ORDER].size = BidAsk[ASK_ORDER].size = 0;

	int Sgn = 2*(SM.position[assetId]>0)-1;
	side = (SM.position[assetId]>0);

	BidAsk[side].size = abs(SM.position[assetId]); //liquidate

	//stopgap logic
	if(BidAsk[BID_ORDER].size < IM.minOrderSize) BidAsk[BID_ORDER].size = 0;
	if(BidAsk[ASK_ORDER].size < IM.minOrderSize) BidAsk[ASK_ORDER].size = 0;

	//now determine price and order type
	if(BidAsk[BID_ORDER].size > 0)
	{
		orderB = *orderB_ptr = SM.marketData[assetId].orderB;
		orderB->totalQuantity = BidAsk[BID_ORDER].size;
		//orderB->nbboPriceCap = 0.0;
		orderB->stateDesc = "Liq";
		orderB->orderType = "REL";
		orderB->auxPrice = getAuxPrice(assetId);
		orderB->lmtPrice = 0.0;
		OSTR_RESET
		ostr << "OrderMgr: desired: L " << orderB->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< " at " << orderB->lmtPrice << "\n";
		OSTR_LOG(LOG_MSG);
	}

	if(BidAsk[ASK_ORDER].size > 0)
	{
		orderS = *orderS_ptr = SM.marketData[assetId].orderS;
		orderS->totalQuantity = BidAsk[ASK_ORDER].size;
		//orderS->nbboPriceCap = 0.0;
		orderS->stateDesc = "Liq";
		orderS->orderType = "REL";
		orderS->auxPrice = getAuxPrice(assetId);
		orderS->lmtPrice = 0.0;
		OSTR_RESET
		ostr << "OrderMgr: desired: S " << orderS->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< " at " << orderS->lmtPrice << "\n";
		OSTR_LOG(LOG_MSG);
	}
}

void CAlgoMF::NeutralizePosition(int assetId, Order **orderB_ptr, Order **orderS_ptr)
{
	Order *orderB, *orderS;
	MyOrder BidAsk[2];
	double MV = getMarketValue();
	bool side;

	if(assetId != 0) return;

	BidAsk[BID_ORDER].init(0);
	BidAsk[ASK_ORDER].init(INFINITY_D);
	BidAsk[BID_ORDER].size = BidAsk[ASK_ORDER].size = 0;

	OSTR_DEF;
	ostr << "OrderMgr: portfolio MV=" << MV << "\n";
	OSTR_LOG(LOG_MSG);

	side = (MV>0);
	BidAsk[side].size = (long)fabs(MV/SM.marketData[assetId].market[1+side]+0.5);

	OSTR_RESET
	ostr << "Dim0: ";
	for(int i=0; i<IM.BasketSize; i++)
	{
		ostr << SM.marketData[i].contract->symbol << "=" << SM.position[i] << " ";
	}
	ostr << "\n";
	OSTR_LOG(LOG_MSG);

	if(BidAsk[BID_ORDER].size >= 0)
	{
		orderB = *orderB_ptr = SM.marketData[assetId].orderB;
		orderB->totalQuantity = BidAsk[BID_ORDER].size;
		orderB->stateDesc = "Neutralize";
		orderB->orderType = "REL";
		orderB->auxPrice = 0.00;
		orderB->lmtPrice = 0.00;

		OSTR_RESET
		ostr << "OrderMgr: desired: L " << orderB->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< ", bid= " << SM.marketData[assetId].market[BID] << "\n";
		OSTR_LOG(LOG_MSG);
	}
	if(BidAsk[ASK_ORDER].size >= 0)
	{
		orderS = *orderS_ptr = SM.marketData[assetId].orderS;
		orderS->totalQuantity = BidAsk[ASK_ORDER].size;
		orderS->stateDesc = "Neutralize";
		orderS->orderType = "REL";
		orderS->auxPrice = 0.00;
		orderS->lmtPrice = 0.00;

		OSTR_RESET
		ostr << "OrderMgr: desired: S " << orderS->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< ", ask= " << SM.marketData[assetId].market[ASK] << "\n";
		OSTR_LOG(LOG_MSG);
	}
}

void CAlgoMF::cancelNotNeededOrders(int assetId, Order *orderB,  Order *orderS)
{
	Order *order;
	BookElement *element;
	int size=0;
	ShortOrderType orderType = ShortOrder::strToOrderType(order->orderType);
	bool action;
	long sizeAtPriceB, sizeAtPriceS;
	OSTR_DEF;

	if((orderB!=NULL)&&(orderS!=NULL))
	{
		OSTR_RESET
		ostr << "CAlgoHF: canceling all orders for a new bracket\n";
		OSTR_LOG(LOG_MSG);
		element = SM.orderBook[assetId].bid;
		while(element)
		{
			if(element->orderStatus!=CANCELLED_UNCONFIRMED) IM.client->cancelOrder(element->m_order->orderId);
			element = element->nextElement;
		}
		element = SM.orderBook[assetId].ask;
		while(element)
		{
			if(element->orderStatus!=CANCELLED_UNCONFIRMED) IM.client->cancelOrder(element->m_order->orderId);
			element = element->nextElement;
		}
		sizeAtPriceB = 0;
		sizeAtPriceS = 0;
	}
	else
	{
		OSTR_RESET
		ostr << "CAlgoHF: canceling all opposite side orders\n";
		OSTR_LOG(LOG_MSG);
		element = (orderB==NULL) ? SM.orderBook[assetId].bid : SM.orderBook[assetId].ask;
		while(element)
		{
			if(element->orderStatus!=CANCELLED_UNCONFIRMED) IM.client->cancelOrder(element->m_order->orderId);
			element = element->nextElement;
		}

		OSTR_RESET
		ostr << "CAlgoHF: canceling all opposite side orders\n";
		OSTR_LOG(LOG_MSG);
		if(orderB!=NULL)
		{
			element = SM.orderBook[assetId].bid;
			order = orderB;
			action = 0;
		}
		else
		{
			element = SM.orderBook[assetId].ask;
			order = orderS;
			action = 1;
		}
		while(element)
		{
			if(element->orderStatus!=CANCELLED_UNCONFIRMED)
			{
				if((fabs(element->m_order->lmtPrice-order->lmtPrice) > TINY)||
						(element->m_order->orderType!=orderType)||
						(fabs(element->m_order->auxPrice-order->auxPrice) > TINY)||
						(element->m_order->action!=action))
					IM.client->cancelOrder(element->m_order->orderId);
				else
					size += element->m_order->totalQuantity;
			}
			element = element->nextElement;
		}
		sizeAtPriceB = (action) ? 0 : size;
		sizeAtPriceS = (action) ? size : 0;
	}

	int desB=0,desA=0;
	if(orderB!=NULL)
	{
		desB = orderB->totalQuantity;
		orderB->totalQuantity = orderB->totalQuantity - sizeAtPriceB;
	}
	if(orderS!=NULL)
	{
		desA = -orderS->totalQuantity;
		orderS->totalQuantity = orderS->totalQuantity - sizeAtPriceS;
	}

	OSTR_RESET
	ostr << "existing orders (" << SM.marketData[assetId].contract->symbol << ") : to BUY " <<
		sizeAtPriceB << ", to SELL " << sizeAtPriceS << "\n";
	OSTR_LOG(LOG_MSG);

	if((desB != 0)||(desA != 0))
	{
		IM.Logger->writeDesired(SM.globalState->getTime(), assetId, desB, desA, sizeAtPriceB, sizeAtPriceS);
	}
}


void CAlgoMF::afterFill(int assetId)
{
	return;
}

void CAlgoMF::afterPCA(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	int K = IM.BasketSize;
	OSTR_DEF;

	if(algState==PLACING_GETTING_INTO_POS_ORDERS) algState = GETTING_INTO_POS;
	if(algState==PLACING_LIQ_ORDERS) algState = LIQUIDATING_POS;

	bool desiredAchieved = true;
	for(int i=0; i<K; i++)
	{
		OSTR_RESET
		ostr << "PCA: pos=" << SM.position[i] << ", desired pos=" << SM.DesiredPosition[i] << "\n";
		OSTR_LOG(LOG_MSG);
		if(abs(abs(SM.DesiredPosition[i])-abs(SM.position[i])) > IM.minOrderSize) desiredAchieved = false;
	}
	if((algState==GETTING_INTO_POS)&&(desiredAchieved))
		algState = IN_POS;
	else if((algState==LIQUIDATING_POS)&&(desiredAchieved))
		algState = ALL_CASH;

	OSTR_RESET
	ostr << "PCA: state=" << algState << "\n";
	OSTR_LOG(LOG_MSG);

	bool status = run[algState](position, lastVector, eigVector, eigValue);

	if(status)
	{
		int neutrDimId;
		if(SM.DeviationDetected) neutrDimId = 0;
		else neutrDimId = -1;
		for(int j=IM.BasketSize-1; j>neutrDimId; j--)
		{
			orderManager(j);
		}
	}
}

bool CAlgoMF::afterPCA_AllCash(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	double sigma;
	int K = IM.BasketSize;
	double Dtime = SM.globalState->getDTime();
	bool placeOrders = false;
	OSTR_DEF;

	calcDesiredPosition(lastVector, eigVector, SM.DesiredPositionTmp); //calc SM.weights, SM.AtomicSize
	//calcOrth(positionOrth, eigVector, K, SM.DesiredPositionTmp);  //calc positionOrth
	for(int i=0; i<K; i++)
		lastVectorAdj[i] = (SM.DesiredPositionTmp[i]>0) ? SM.marketData[i].market[ASK] : SM.marketData[i].market[BID];
	calcDesiredPosition(lastVectorAdj, eigVector, SM.DesiredPositionTmp); //calc SM.weights, SM.AtomicSize
	Utils::calcOrth(positionOrth, eigVector, K, SM.DesiredPositionTmp);  //calc positionOrth

	mpvon = Utils::calcInnerProd(lastVectorAdj,positionOrth,K,1,1);

	//project basket's last
	for(int i=0; i<K; i++) SM.lastPP[i] = Utils::calcInnerProd(&eigVector[i*K],lastVectorAdj,K,1,1);

	SM.DeviationSize = Utils::calcVectorNorm(SM.lastPP,K-1);

	sigma = 0.0;
	for(int i=0; i<K-1; i++) sigma += pow(SM.lastPP[i]*eigValue[i],2.0);
	sigma = (fabs(SM.DeviationSize) > TINY) ? pow(sigma,0.5)/SM.DeviationSize : 0.0;

	SM.LiquidateDivergentPos = false;
	SM.DeviationDetected = (SM.DeviationSize > IM.sigmaFactor[0]*MAX(sigma,0.005));   // hardcoded for now

	IM.Logger->writeMasterGetIntoInfo(SM.globalState->getTime(), SM.DeviationSize, sigma, mpvon);

	OSTR_RESET
	ostr << "mpvon=" << mpvon << "\n";
	IM.Logger->write_pca_log(ostr.str());

	OSTR_RESET
	ostr << "PCA: lastPP:\n";
	for(int i=0; i<K; i++) ostr << SM.lastPP[i] << " ";
	ostr << "\n";

	ostr << "SM.lastPP norm =" << SM.DeviationSize << "\n";
	ostr << "last orth (SM.lastPP) sigma=" << sigma << "\n";

	IM.Logger->write_pca_log(ostr.str());
	OSTR_RESET
	ostr << "\nDeviationDetected = " << SM.DeviationDetected << ", SM.DeviationSize=" << SM.DeviationSize << "\n";
	OSTR_LOG(LOG_MSG);

	if(SM.DeviationDetected)
	{
		OSTR_RESET
		ostr << "PCA : Atomic=(" << SM.AtomicSize << ")\n";
		IM.Logger->write_pca_log(ostr.str());

		for(int i=0; i<K; i++) SM.DesiredPosition[i] = SM.DesiredPositionTmp[i];

		OSTR_RESET
		for(int i=0; i<K; i++)
		{
			if(fabs(SM.lastOrth[i]) > MIN_NORM_FACTOR*SM.DeviationSize)
			{
				if(SM.lastOrth[i] < -TINY)
					ostr << Dtime << "," << i << ",96," << SM.marketData[i].market[BID] << "\n";
				else if(SM.lastOrth[i] > TINY)
					ostr << Dtime << "," << i << ",97," << SM.marketData[i].market[ASK] << "\n";
			}
		}
		IM.Logger->writeTickData(ostr.str());
		algState = PLACING_GETTING_INTO_POS_ORDERS;
		placeOrders = true;

		if(IM.debugPCA)
		{
			OSTR_RESET
			ostr << "PCA: projection of Last:\n";
			for(int i=0; i<K; i++) ostr << SM.lastPP[i] << " ";
			ostr << "\n";

			ostr << "PCA: Weights:\n";
			for(int i=0; i<K; i++) ostr << SM.weights[i] << " ";
			ostr << "\n";

			ostr << "PCA: Eigen-vectors:\n";
			for(int i=0; i<K; i++)
			{
				for(int j=0; j<K; j++) ostr << SM.eigVectors[i+j*K] << " ";
				ostr << "\n";
			}

			ostr << "PCA: Eigen-values:\n";
			for(int i=0; i<K; i++) ostr << SM.SigmaValue[i] << " ";
			ostr << "\n";
			IM.Logger->write_pca_log(ostr.str());
		}
	}

	finalize(eigVector, lastVector);

	return placeOrders;
}

bool CAlgoMF::afterPCA_default(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	int K = IM.BasketSize;
	OSTR_DEF

	calcDesiredPosition(lastVector, eigVector, SM.DesiredPositionTmp); //calc SM.weights, SM.AtomicSize
	//calcOrth(positionOrth, eigVector, K, SM.DesiredPositionTmp);  //calc positionOrth
	for(int i=0; i<K; i++)
		lastVectorAdj[i] = (SM.DesiredPositionTmp[i]>0) ? SM.marketData[i].market[ASK] : SM.marketData[i].market[BID];
	calcDesiredPosition(lastVectorAdj, eigVector, SM.DesiredPositionTmp); //calc SM.weights, SM.AtomicSize
	Utils::calcOrth(positionOrth, eigVector, K, SM.DesiredPositionTmp);  //calc positionOrth

	mpvon = Utils::calcInnerProd(lastVectorAdj,positionOrth,K,1,1);
	OSTR_RESET
	ostr << "mpvon=" << mpvon << "\n";
	IM.Logger->write_pca_log(ostr.str());


	for(int i=0; i<K; i++) SM.lastPP[i] = Utils::calcInnerProd(&eigVector[i*K],lastVectorAdj,K,1,1);
	SM.DeviationSize = Utils::calcVectorNorm(SM.lastPP,K-1);

	finalize(eigVector, lastVector);

	IM.Logger->writeMasterPositions(SM.globalState->getTime(), mpvon);

	return false;
}

bool CAlgoMF::afterPCA_InPosition(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	int K = IM.BasketSize;
	double Dtime = SM.globalState->getDTime();
	bool placeOrders = false;
	OSTR_DEF

	for(int i=0; i<K; i++) vwap[i] = SM.VWAP[i].getVWAP();

	Utils::calcOrth(positionOrth, eigVector, K, SM.position);  //calc positionOrth

	double norm = Utils::calcVectorNorm(positionOrth,K);

	for(int i=0; i<K-1; i++) positionPP[i] = Utils::calcInnerProd(&eigVector[i*K],positionOrth,K,1,1);

	double sigma = 0.0;
	for(int i=0; i<K-1; i++) sigma += pow(positionPP[i]*eigValue[i],2.0);
	sigma = pow(sigma,0.5);

	for(int i=0; i<K; i++)
		lastVectorAdj[i] = (SM.position[i]>0) ? SM.marketData[i].market[BID] : SM.marketData[i].market[ASK];

	vpvon = Utils::calcInnerProd(vwap,positionOrth,K,1,1);
	mpvon = Utils::calcInnerProd(lastVectorAdj,positionOrth,K,1,1);

	SM.DeviationDetected = false;
	SM.LiquidateDivergentPos = (mpvon > (vpvon + sigma))||(vpvon > 0.0);
	if(SM.LiquidateDivergentPos)
	{
		algState = PLACING_LIQ_ORDERS;
		for(int i=0; i<K; i++) SM.DesiredPosition[i] = 0;
		placeOrders = true;
	}

	for(int i=0; i<K; i++) SM.lastPP[i] = Utils::calcInnerProd(&lastVector[i*K],lastVectorAdj,K,1,1);
	SM.DeviationSize = Utils::calcVectorNorm(SM.lastPP,K-1);


	ostr << "PCA: positionOrth:\n";
	for(int i=0; i<K; i++) ostr << positionOrth[i] << " ";
	ostr << "\n";

	ostr << "positionOrth norm =" << norm << "\n";

	ostr << "PCA: positionPP:\n";
	for(int i=0; i<K-1; i++) ostr << positionPP[i] << " ";
	ostr << "\n";

	ostr << "position orth (positionPP) sigma=" << sigma << "\n";
	ostr << "vpvon=" << vpvon << "\n";
	ostr << "mpvon=" << mpvon << "\n";
	IM.Logger->write_pca_log(ostr.str());

	OSTR_RESET
	ostr << "\nLiquidateDivergentPos = " << SM.LiquidateDivergentPos << "\n";
	OSTR_LOG(LOG_MSG);

	OSTR_RESET_HiPrec;
	Dtime = SM.globalState->getDTime();
	ostr << Dtime << "," << 95 << "," << mpvon << "," << vpvon << "\n";
	IM.Logger->writeTickData(ostr.str());

	finalize(eigVector, lastVector);

	return placeOrders;
}

bool CAlgoMF::afterPCA_PlacingPositionOrders(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	SM.LiquidateDivergentPos = false;
	afterPCA_default(position, lastVector, eigVector, eigValue);
}

bool CAlgoMF::afterPCA_GettingIntoPosition(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	SM.DeviationDetected = false;
	SM.LiquidateDivergentPos = false;
	afterPCA_default(position, lastVector, eigVector, eigValue);
}

bool CAlgoMF::afterPCA_PlacingLiquidationOrders(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	SM.DeviationDetected = false;
	afterPCA_default(position, lastVector, eigVector, eigValue);
}

bool CAlgoMF::afterPCA_LiquidatingPosition(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	SM.DeviationDetected = false;
	afterPCA_default(position, lastVector, eigVector, eigValue);
}

void CAlgoMF::finalize(double *eigVector, double *lastVector)
{
	int K = IM.BasketSize;
	OSTR_DEF;

	OSTR_RESET_HiPrec;
	ostr << SM.globalState->getDTime() << "," << 94 << "," << SM.DeviationSize << "," << vpvon << "\n";
	IM.Logger->writeTickData(ostr.str());

	//lastOrth components
	OSTR_RESET_HiPrec;
	for(int i=0; i<K; i++)  ostr << SM.globalState->getDTime() << "," << 90+i << "," << SM.lastOrth[i] << "," << 0 << "\n";
	IM.Logger->writeTickData(ostr.str());

	//last principal projection
	double pp = 0.0;
	for(int i=0; i<K; i++) pp += (lastVector[i] - SM.SigmaValue[K-1]*eigVector[(K-1)*K+i])*eigVector[(K-1)*K+i];
	OSTR_RESET_HiPrec;
	ostr << SM.globalState->getDTime() << "," << 89 << "," << pp << "," << 0 << "\n";
	IM.Logger->writeTickData(ostr.str());

	IM.pca_enable = true;
}

void CAlgoMF::calcDesiredPosition(double *last, double *eigVector, long *desPos)
{
	int K = IM.BasketSize;

	//compute lastOrth
	Utils::calcOrth(SM.lastOrth, eigVector, K, last);

	//compute weights
	double maxPrice = 0.0;
	for(int i=0; i<K; i++) maxPrice = MAX(fabs(SM.lastOrth[i]),maxPrice);
	for(int i=0; i<K; i++) SM.weights[i] = (maxPrice > TINY) ? fabs(SM.lastOrth[i]) / maxPrice : 0.0;

	//compute AtomicSize
	double NLV = SM.globalState->getNLV();
	double w_sum = 0.0;
	for(int i=0; i<K; i++) w_sum += fabs(SM.weights[i])*last[i];
	SM.AtomicSize = (long)(min(IM.MaxPurchase,IM.MaxLeverage)*NLV/(IM.OrderSizeSF*w_sum)+0.5);

	for(int i=0; i<K; i++)
	{
		int sgn = 1-2*(SM.lastOrth[i] > TINY);
		desPos[i] = 0;
		if(fabs(SM.lastOrth[i]) > MIN_NORM_FACTOR*SM.DeviationSize)
		{
			desPos[i] = (long)(fabs(SM.weights[i]*SM.AtomicSize)/K+0.5)*sgn;
		}
	}
}

double CAlgoMF::getAuxPrice(int assetId)
{
	//if(assetId==0) return 0.00;
	//else
		return 0.01;
}

