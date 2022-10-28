#include <iomanip>
#include <math.h>
#include "algoHF.h"
#include "memory.h"
#include "utils.h"
#include <time.h>

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;

CAlgoHF::CAlgoHF()
{
	OSTR_DEF;
	ostr << "CAlgoHF: constructor: BasketSize=" << IM.BasketSize << "\n";
	OSTR_LOG(LOG_MSG);

    lastVectorAdj = new double[IM.BasketSize];
	positionOrth = new double[IM.BasketSize];
	positionPP = new double[IM.BasketSize];
	vwap = new double[IM.BasketSize];

	algState = ALL_CASH;

	algStateDesc[ALL_CASH] = "ALL_CASH";
	algStateDesc[PLACING_GETTING_INTO_POS_ORDERS] = "PLACING_GETTING_INTO_POS_ORDERS";
	algStateDesc[GETTING_INTO_POS] = "GETTING_INTO_POS";
	algStateDesc[IN_POS] = "IN_POS";
	algStateDesc[PLACING_LIQ_ORDERS] = "PLACING_LIQ_ORDERS";
	algStateDesc[LIQUIDATING_POS] = "LIQUIDATING_POS";
}


BracketHierarchy CAlgoHF::GenerateDesiredCumulativeOrders(int assetId, Order **orderB, Order **orderS)
{
	BracketHierarchy hierarchy = LEGS_EQUAL;
	OSTR_DEF;

	if((assetId==0)&&(!SM.LiquidateDivergentPos))
	{
		NeutralizePosition(assetId, orderB, orderS);
	}
	else if((abs(SM.position[assetId])<IM.minOrderSize)&&(SM.DeviationDetected))  //what initial position to take
	{
		hierarchy = TakeOnPosition(assetId, orderB, orderS);
	}
	else if(SM.LiquidateDivergentPos)
	{
		LiquidatePosition(assetId, orderB, orderS);
	}
	else
	{
		OSTR_RESET
		ostr << "CAlgoHF: invalid state\n";
		OSTR_LOG(LOG_MSG);
	}

	cancelNotNeededOrders(assetId, *orderB, *orderS);

	return hierarchy;
}

BracketHierarchy CAlgoHF::TakeOnPosition(int assetId, Order **orderB_ptr, Order **orderS_ptr)
{
	Order *orderB, *orderS;
	MyOrder BidAsk[2];
	int K = IM.BasketSize;
	double absdev = fabs(SM.lastOrth[assetId]);
	BracketHierarchy hierarchy;
	OSTR_DEF;

	OSTR_RESET
	ostr << "CAlgoHF: TakeOnPosition\n";

	//if(absdev<0.01) return false;

	BidAsk[BID_ORDER].init(0);
	BidAsk[ASK_ORDER].init(INFINITY_D);

	BidAsk[BID_ORDER].size = 0;
	BidAsk[ASK_ORDER].size = 0;

	ostr << "OrderMgr " << SM.marketData[assetId].contract->symbol
			<< ": DeviationDetected=" << SM.DeviationDetected << ", lastOrth[assetId]="
			<< SM.lastOrth[assetId] << ", DeviationSize=" << SM.DeviationSize << "\n";
	OSTR_LOG(LOG_MSG);
	if(absdev > MIN_NORM_FACTOR*SM.DeviationSize)
	{
		BidAsk[(SM.lastOrth[assetId] > TINY)].size = abs(SM.DesiredPosition[assetId]);
	}
	OSTR_RESET
	ostr << "OrderMgr " << SM.marketData[assetId].contract->symbol
			<< ": BidAsk[].size=" << BidAsk[(SM.lastOrth[assetId] > TINY)].size << ", SM.DesiredPosition[assetId]="
			<< SM.DesiredPosition[assetId] << "\n";
	OSTR_LOG(LOG_MSG);

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
		orderB->auxPrice = 0.00;
		if(absdev > 0.03) orderB->lmtPrice = SM.marketData[assetId].market[BID]+0.03;
		else if(absdev > 0.01) orderB->lmtPrice = SM.marketData[assetId].market[BID]+0.01;
		else orderB->lmtPrice = SM.marketData[assetId].market[BID]+0.00;
		orderB->nbboWhenPlaced = SM.marketData[assetId].market[BID];
		OSTR_RESET
		ostr << "OrderMgr: desired: L " << orderB->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< " at " << orderB->lmtPrice << "\n";
		OSTR_LOG(LOG_MSG);


		orderS = *orderS_ptr = SM.marketData[assetId].orderS;
		orderS->totalQuantity = orderB->totalQuantity;
		orderS->stateDesc = "Liq";
		//orderS->nbboPriceCap = 0.0;
		orderS->orderType = "REL";
		orderS->auxPrice = 0.00;
		if(absdev > 0.03) orderS->lmtPrice = orderB->lmtPrice+0.02;
		else if(absdev > 0.01) orderS->lmtPrice = orderB->lmtPrice+0.01;
		else orderS->lmtPrice = orderB->lmtPrice+0.01;
		orderS->nbboWhenPlaced = SM.marketData[assetId].market[ASK];
		OSTR_RESET
		ostr << "OrderMgr: desired: S " << orderS->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< " at " << orderS->lmtPrice << "\n";
		OSTR_LOG(LOG_MSG);
		hierarchy = B_PARENT_OF_S;
	}

	if(BidAsk[ASK_ORDER].size > 0)
	{
		orderS = *orderS_ptr = SM.marketData[assetId].orderS;
		orderS->totalQuantity = BidAsk[ASK_ORDER].size;
		orderS->stateDesc = "EstPosn";
		//orderS->nbboPriceCap = 0.0;
		orderS->orderType = "REL";
		orderS->auxPrice = 0.00;
		if(absdev > 0.03) orderS->lmtPrice = SM.marketData[assetId].market[ASK]-0.03;
		else if(absdev > 0.01) orderS->lmtPrice = SM.marketData[assetId].market[ASK]-0.01;
		else orderS->lmtPrice = SM.marketData[assetId].market[ASK]-0.00;
		orderS->nbboWhenPlaced = SM.marketData[assetId].market[ASK];
		OSTR_RESET
		ostr << "OrderMgr: desired: S " << orderS->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< " at " << orderS->lmtPrice << "\n";
		OSTR_LOG(LOG_MSG);


		orderB = *orderB_ptr = SM.marketData[assetId].orderB;
		orderB->totalQuantity = orderS->totalQuantity;
		orderB->stateDesc = "Liq";
		//orderB->nbboPriceCap = 0.0;
		orderB->orderType = "REL";
		orderB->auxPrice = 0.00;
		if(absdev > 0.03) orderB->lmtPrice = orderS->lmtPrice-0.02;
		else if(absdev > 0.01) orderB->lmtPrice = orderS->lmtPrice-0.01;
		else orderB->lmtPrice = orderS->lmtPrice-0.01;
		orderB->nbboWhenPlaced = SM.marketData[assetId].market[BID];
		OSTR_RESET
		ostr << "OrderMgr: desired: L " << orderB->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< " at " << orderB->lmtPrice << "\n";
		OSTR_LOG(LOG_MSG);
		hierarchy = S_PARENT_OF_B;
	}
	return hierarchy;
}

void CAlgoHF::LiquidatePosition(int assetId, Order **orderB_ptr, Order **orderS_ptr)
{
	Order *orderB, *orderS;
	MyOrder BidAsk[2];
	bool side;
	OSTR_DEF;

	OSTR_RESET
	ostr << "CAlgoHF: LiquidatePosition\n";

	/*OSTR_RESET
	ostr << "CAlgoHF: liquidating open orders for " << SM.marketData[assetId].contract->symbol << "\n";
	OSTR_LOG(LOG_MSG);
	SM.orderBook[assetId].cancelSide(BID_ORDER);
	SM.orderBook[assetId].cancelSide(ASK_ORDER);*/

	//if(SM.position[assetId]==0)
	//	return;

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
		orderB->auxPrice = 0.00;
		orderB->lmtPrice = 0.00;
		orderB->nbboWhenPlaced = SM.marketData[assetId].market[BID];
		ostr << "OrderMgr: desired: L " << orderB->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< " at " << orderB->lmtPrice << "\n";
	}

	if(BidAsk[ASK_ORDER].size > 0)
	{
		orderS = *orderS_ptr = SM.marketData[assetId].orderS;
		orderS->totalQuantity = BidAsk[ASK_ORDER].size;
		//orderS->nbboPriceCap = 0.0;
		orderS->stateDesc = "Liq";
		orderS->orderType = "REL";
		orderS->auxPrice = 0.00;
		orderS->lmtPrice = 0.00;
		orderS->nbboWhenPlaced = SM.marketData[assetId].market[ASK];
		ostr << "OrderMgr: desired: S " << orderS->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< " at " << orderS->lmtPrice << "\n";
	}
	OSTR_LOG(LOG_MSG);
}

void CAlgoHF::NeutralizePosition(int assetId, Order **orderB_ptr, Order **orderS_ptr)
{
	Order *orderB, *orderS;
	MyOrder BidAsk[2];
	double MV = getMarketValue(); // SM.globalState->getMarketValue();
	bool side;
	long neutOrderSize;
	double dev = SM.lastOrth[assetId];
	OSTR_DEF;

	OSTR_RESET
	ostr << "CAlgoHF: NeutralizePosition\n";

	/*long bidOrders = SM.orderBook[assetId].countActiveOrders(SM.orderBook[assetId].bid);
	long askOrders = SM.orderBook[assetId].countActiveOrders(SM.orderBook[assetId].ask);
	neutOrderSize = (long)(MV/SM.marketData[assetId].market[BID] + bidOrders - askOrders);*/
	neutOrderSize = (long)(MV/SM.marketData[assetId].market[BID]);
	if((SM.position[assetId]!=0)&&(abs(SM.position[assetId]-neutOrderSize)<=IM.minOrderSize))
		neutOrderSize = SM.position[assetId];

	BidAsk[BID_ORDER].init(0);
	BidAsk[ASK_ORDER].init(INFINITY_D);
	BidAsk[BID_ORDER].size = BidAsk[ASK_ORDER].size = 0;

	//ostr << "OrderMgr: portfolio MV=" << MV << ", neutOrderSize=" << neutOrderSize << ", bidOrders=" << bidOrders << ", askOrders=" << askOrders << "\n";
	ostr << "OrderMgr: portfolio MV=" << MV << ", neutOrderSize=" << neutOrderSize << "\n";

	BidAsk[neutOrderSize>0].size = abs(neutOrderSize);

	ostr << "Dim0: ";
	for(int i=0; i<IM.BasketSize; i++) ostr << SM.marketData[i].contract->symbol << "=" << SM.position[i] << " ";
	ostr << "\n";

	if(BidAsk[BID_ORDER].size > 0)
	{
		orderB = *orderB_ptr = SM.marketData[assetId].orderB;
		orderB->totalQuantity = BidAsk[BID_ORDER].size;
		orderB->stateDesc = "Neutralize";
		orderB->auxPrice = 0.00;
		if(dev<-0.01)
		//if(1)
		{
			orderB->orderType = "REL";
			orderB->lmtPrice = 0.00;
		}
		else
		{
			orderB->orderType = "STP";
			orderB->lmtPrice = 0.0;
			orderB->auxPrice = SM.marketData[assetId].market[BID]+0.03;
		}
		orderB->nbboWhenPlaced = SM.marketData[assetId].market[BID];
		ostr << "CAlgoHF: desired: L " << orderB->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< ", bid= " << SM.marketData[assetId].market[BID] << "\n";
	}
	if(BidAsk[ASK_ORDER].size > 0)
	{
		orderS = *orderS_ptr = SM.marketData[assetId].orderS;
		orderS->totalQuantity = BidAsk[ASK_ORDER].size;
		orderS->stateDesc = "Neutralize";
		orderS->auxPrice = 0.00;
		if(dev>0.01)
		//if(1)
		{
			orderS->orderType = "REL";
			orderS->lmtPrice = 0.00;
		}
		else
		{
			orderS->orderType = "STP";
			orderS->lmtPrice = 0.0;
			orderS->auxPrice = SM.marketData[assetId].market[ASK]-0.03;
		}
		orderS->nbboWhenPlaced = SM.marketData[assetId].market[ASK];
		ostr << "CAlgoHF: desired: S " << orderS->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol
				<< ", ask= " << SM.marketData[assetId].market[ASK] << "\n";
	}
	OSTR_LOG(LOG_MSG);
}

void CAlgoHF::cancelNotNeededOrders(int assetId, Order *orderB,  Order *orderS)
{
	Order *order;
	BookElement *element;
	int size=0;
	ShortOrderType orderType;
	bool action;
	long sizeAtPriceB, sizeAtPriceS;
	OSTR_DEF;

	if(((orderB!=NULL)&&(orderS!=NULL))||((orderB==NULL)&&(orderS==NULL)))
	{
		OSTR_RESET
		ostr << "CAlgoHF: canceling all orders on both sides.\n";
		OSTR_LOG(LOG_MSG);
		element = SM.orderBook[assetId].bid;
		while(element)
		{
			if(element->orderStatus!=CANCELLED_UNCONFIRMED)
			{
				IM.client->cancelOrder(element->m_order->orderId);
				element->orderStatus = CANCELLED_UNCONFIRMED;
			}
			element = element->nextElement;
		}
		element = SM.orderBook[assetId].ask;
		while(element)
		{
			if(element->orderStatus!=CANCELLED_UNCONFIRMED)
			{
				IM.client->cancelOrder(element->m_order->orderId);
				element->orderStatus = CANCELLED_UNCONFIRMED;
			}
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
			if(element->orderStatus!=CANCELLED_UNCONFIRMED)
			{
				IM.client->cancelOrder(element->m_order->orderId);
				element->orderStatus = CANCELLED_UNCONFIRMED;
			}
			element = element->nextElement;
		}

		OSTR_RESET
		ostr << "CAlgoHF: counting and canceling same side orders\n";
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
		orderType = ShortOrder::strToOrderType(order->orderType);
		while(element)
		{
			if(element->orderStatus!=CANCELLED_UNCONFIRMED)
			{
				if((fabs(element->m_order->lmtPrice-order->lmtPrice) > TINY)||
						(element->m_order->orderType!=orderType)||
						(fabs(element->m_order->auxPrice-order->auxPrice) > TINY)||
						(element->m_order->action!=action))
				{
					IM.client->cancelOrder(element->m_order->orderId);
					element->orderStatus = CANCELLED_UNCONFIRMED;
				}
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

void CAlgoHF::afterFill(int assetId)
{
	//if(!SM.LiquidateDivergentPos)
	if((assetId!=0)&&(!SM.LiquidateDivergentPos))
	{
		orderManager(0);
	}
}

void CAlgoHF::afterPCA(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	int K = IM.BasketSize;
	bool status;
	OSTR_DEF;
	OSTR_RESET
	ostr << "\n";

	if(algState==PLACING_GETTING_INTO_POS_ORDERS) algState = GETTING_INTO_POS;
	if(algState==PLACING_LIQ_ORDERS) algState = LIQUIDATING_POS;

	m_desiredPositionAchieved = true;
	for(int i=0; i<K; i++)
	{
		ostr << "PCA: pos=" << SM.position[i] << ", desired pos=" << SM.DesiredPosition[i] << "\n";
		if(abs(abs(SM.DesiredPosition[i])-abs(SM.position[i])) > IM.minOrderSize) m_desiredPositionAchieved = false;
	}
	if((algState==GETTING_INTO_POS)&&(m_desiredPositionAchieved))
		algState = IN_POS;

	ostr << "PCA: state=" << algStateDesc[algState] << "\n";
	OSTR_LOG(LOG_MSG);
	IM.Logger->write_pca_log(ostr.str());

	switch (algState)
	{
	case ALL_CASH: { status=afterPCA_AllCash(position, lastVector, eigVector, eigValue); break; }
	case PLACING_GETTING_INTO_POS_ORDERS: { status=afterPCA_PlacingPositionOrders(position, lastVector, eigVector, eigValue); break; }
	case GETTING_INTO_POS: { status=afterPCA_GettingIntoPosition(position, lastVector, eigVector, eigValue); break; }
	case IN_POS: { status=afterPCA_InPosition(position, lastVector, eigVector, eigValue); break; }
	case PLACING_LIQ_ORDERS: { status=afterPCA_PlacingLiquidationOrders(position, lastVector, eigVector, eigValue); break; }
	case LIQUIDATING_POS: { status=afterPCA_LiquidatingPosition(position, lastVector, eigVector, eigValue); break; }
	}


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

bool CAlgoHF::afterPCA_AllCash(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	OSTR_DEF;
	double sigma;
	int K = IM.BasketSize;
	double Dtime = SM.globalState->getDTime();
	bool placeOrders = false;

	calcDesiredPosition(lastVector, eigVector, SM.DesiredPositionTmp); //calc SM.weights, SM.AtomicSize

	//calcOrth(positionOrth, eigVector, K, SM.DesiredPositionTmp);  //calc positionOrth
	for(int i=0; i<K; i++)
	{
		lastVectorAdj[i] = (SM.DesiredPositionTmp[i]>0) ? SM.marketData[i].market[ASK] : SM.marketData[i].market[BID];
	}

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
	SM.DeviationDetected = (SM.DeviationSize > sigma/4);

	IM.Logger->writeMasterGetIntoInfo(SM.globalState->getTime(), SM.DeviationSize, sigma, mpvon);

	OSTR_RESET_HiPrec
	//ostr << "sigma=" << sigma << ", mpvon=" << mpvon << "\n";
	//ostr << "PCA: lastPP:\n";
	//for(int i=0; i<K; i++) ostr << SM.lastPP[i] << " ";
	//ostr << "\n";
	ostr << "SM.DeviationDetected=" << SM.DeviationDetected << ", SM.DeviationSize=" << SM.DeviationSize << ", sigma=" << sigma << "\n";
	IM.Logger->write_pca_log(ostr.str());

	if(SM.DeviationDetected)
	{
		for(int i=0; i<K; i++) SM.DesiredPositionAll[i] = SM.DesiredPositionTmp[i];
		int minDevIdx=-1;
		double lastOrthMin=INFINITY_D;
		for(int i=1; i<K; i++)
		{
			if(fabs(SM.lastOrth[i])<lastOrthMin) { lastOrthMin = fabs(SM.lastOrth[i]); minDevIdx = i; }
		}
		if(minDevIdx>0) SM.DesiredPositionAll[minDevIdx] = 0;

		SM.momentumSide = MOMENTUM_ALIGNED;
		for(int i=0; i<K; i++)
		{
			if((SM.momentum[i] == MOMENTUM_UP)&&(SM.DesiredPositionAll[i]<0))  SM.DesiredPosition[i] = 0;
			else if((SM.momentum[i] == MOMENTUM_DOWN)&&(SM.DesiredPositionAll[i]>0))  SM.DesiredPosition[i] = 0;
			else SM.DesiredPosition[i] = SM.DesiredPositionAll[i];
			SM.momentumWhenPlaced[i] = SM.momentum[i];
		}


		OSTR_RESET_HiPrec
		ostr << "PCA : Atomic=(" << SM.AtomicSize << ")\n";
		ostr << "PCA: DesiredPosition:\n";
		for(int i=0; i<K; i++) ostr << SM.DesiredPosition[i] << " ";
		ostr << "\n";
		IM.Logger->write_pca_log(ostr.str());

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

		placeOrders = false;
		for(int i=1; i<K; i++) { if(abs(SM.DesiredPosition[i])>IM.minOrderSize) placeOrders = true; }
		if(placeOrders) algState = PLACING_GETTING_INTO_POS_ORDERS;

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

bool CAlgoHF::afterPCA_default(long *position, double *lastVector, double *eigVector, double *eigValue)
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
	/*ostr << "PCA: lastPP:\n";
	for(int i=0; i<K; i++) ostr << SM.lastPP[i] << " ";*/
	ostr << "\n";
	IM.Logger->write_pca_log(ostr.str());


	for(int i=0; i<K; i++) SM.lastPP[i] = Utils::calcInnerProd(&eigVector[i*K],lastVectorAdj,K,1,1);
	SM.DeviationSize = Utils::calcVectorNorm(SM.lastPP,K-1);
	ostr << "SM.DeviationSize(default)=" << SM.DeviationSize << "\n";

	finalize(eigVector, lastVector);

	IM.Logger->writeMasterPositions(SM.globalState->getTime(), mpvon);

	return false;
}

bool CAlgoHF::afterPCA_InPosition(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	int K = IM.BasketSize;
	double Dtime = SM.globalState->getDTime();
	bool placeOrders = false;
	bool zeroPosition = true;
	OSTR_DEF

	for(int i=0; i<K; i++) { if(abs(SM.position[i]) > IM.minOrderSize) zeroPosition = false; }
	if(zeroPosition)
	{
		for(int i=0; i<K; i++)
		{
			SM.orderBook[i].cancelSide(BID_ORDER);
			SM.orderBook[i].cancelSide(ASK_ORDER);
		}
		algState = ALL_CASH;
		return false;
	}

	for(int i=0; i<K; i++)
	{
		if(abs(SM.position[i]) <= IM.minOrderSize)
			vwap[i] = 0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]);
		else
			vwap[i] = SM.VWAP[i].getVWAP();
	}

	//calc positionOrth
	if(isPositionNeutral())
		Utils::calcOrth(positionOrth, eigVector, K, SM.position);
	else
	{
		double MV = getMarketValue();
		long posPrev = SM.position[0];
		SM.position[0] = (long)(MV/SM.marketData[0].market[BID]);
		Utils::calcOrth(positionOrth, eigVector, K, SM.position);
		SM.position[0] = posPrev;
	}

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
	SM.LiquidateDivergentPos = ((mpvon > (vpvon + sigma))||(vpvon > sigma))&&(norm > TINY);

	if(SM.LiquidateDivergentPos)
	{
		if((m_desiredPositionAchieved)&&(mpvon > (vpvon + sigma))&&(SM.momentumSide == MOMENTUM_ALIGNED))
		{
			ostr << "InPosition: placing orders against momentum\n";
			SM.momentumSide = MOMENTUM_OPPOSITE;
			algState = PLACING_GETTING_INTO_POS_ORDERS;
			for(int i=0; i<K; i++)
			{
				if((SM.momentum[i] == MOMENTUM_UP)&&(SM.DesiredPositionAll[i]>0))  SM.DesiredPosition[i] = 0;
				else if((SM.momentum[i] == MOMENTUM_DOWN)&&(SM.DesiredPositionAll[i]<0))  SM.DesiredPosition[i] = 0;
				else if(SM.momentum[i]!=SM.momentumWhenPlaced[i])  SM.DesiredPosition[i] = 0;
				else SM.DesiredPosition[i] = SM.DesiredPositionAll[i];
			}
		}
		else
		{
			ostr << "InPosition: switching to order liquidation\n";
			algState = PLACING_LIQ_ORDERS;
			for(int i=0; i<K; i++) SM.DesiredPosition[i] = 0;
		}
		placeOrders = true;
	}

	for(int i=0; i<K; i++) SM.lastPP[i] = Utils::calcInnerProd(&eigVector[i*K],lastVectorAdj,K,1,1);
	SM.DeviationSize = Utils::calcVectorNorm(SM.lastPP,K-1);

	ostr << "LiquidateDivergentPos=" << SM.LiquidateDivergentPos << ", DeviationSize=" << SM.DeviationSize << ", vpvon=" << vpvon << ", mpvon=" << mpvon << "\n";
	IM.Logger->write_pca_log(ostr.str());

	OSTR_RESET_HiPrec;
	Dtime = SM.globalState->getDTime();
	ostr << Dtime << "," << 95 << "," << mpvon << "," << vpvon << "\n";
	IM.Logger->writeTickData(ostr.str());

	finalize(eigVector, lastVector);

	return placeOrders;
}

bool CAlgoHF::afterPCA_PlacingPositionOrders(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	SM.LiquidateDivergentPos = false;
	afterPCA_default(position, lastVector, eigVector, eigValue);
	return true;
}

bool CAlgoHF::afterPCA_GettingIntoPosition(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	OSTR_DEF;
	int K = IM.BasketSize;
	bool zeroPosition = true;

	for(int i=0; i<K; i++) { if(abs(SM.position[i]) > IM.minOrderSize) zeroPosition = false; }
	if(!zeroPosition)
	{
		//OSTR_RESET
		//ostr << "PCA: state=" << algStateDesc[algState] << "\n";
		//IM.Logger->write_pca_log(ostr.str());
		return afterPCA_InPosition(position, lastVector, eigVector, eigValue);
	}

	bool marketMoved = false;
	double marketMovedThr = 0.0;
	OSTR_RESET
	ostr << "Now(Then): ";
	for(int i=0; i<K; i++)
	{
		if(SM.DesiredPosition[i]==0) { SM.nbboCurrent[i]=0; continue; }
		SM.nbboCurrent[i] = (SM.DesiredPosition[i]>0) ? SM.marketData[i].market[BID] : SM.marketData[i].market[ASK];
		SM.nbboWhenPlaced[i] = (SM.DesiredPosition[i]>0) ? SM.orderBook[i].getBestLimit(BID_ORDER) : SM.orderBook[i].getBestLimit(ASK_ORDER);

		if(SM.DesiredPosition[i] > 0)
			marketMoved = (SM.nbboCurrent[i] > SM.orderBook[i].getBestLimit(BID_ORDER)+marketMovedThr+TINY);
		else if(SM.DesiredPosition[i] < 0)
			marketMoved = (SM.nbboCurrent[i] < SM.orderBook[i].getBestLimit(ASK_ORDER)-marketMovedThr-TINY);
		//if(fabs(SM.nbboCurrent[i]-SM.nbboWhenPlaced[i])>0.01+TINY) marketMoved = true;
		ostr << SM.nbboCurrent[i] << "(" << SM.nbboWhenPlaced[i] << ")  ";
	}
	ostr << "\n";
	OSTR_LOG(LOG_MSG);

	if(marketMoved)
	{
		OSTR_RESET
		ostr << "CAlgoHF: market moved\n";
		OSTR_LOG(LOG_MSG);
		SM.DeviationDetected = false;
		SM.LiquidateDivergentPos = true;
		algState = LIQUIDATING_POS;
		for(int i=0; i<K; i++) SM.DesiredPosition[i] = 0;

		for(int i=0; i<K; i++)
		{
			SM.orderBook[i].cancelSide(BID_ORDER);
			SM.orderBook[i].cancelSide(ASK_ORDER);
		}
		return false;
	}
	else
	{
		OSTR_RESET
		ostr << "CAlgoHF: market hasn't moved\n";
		OSTR_LOG(LOG_MSG);
		afterPCA_default(position, lastVector, eigVector, eigValue);
		return false;
	}
}

bool CAlgoHF::afterPCA_PlacingLiquidationOrders(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	SM.DeviationDetected = false;
	SM.LiquidateDivergentPos = true;
	afterPCA_default(position, lastVector, eigVector, eigValue);
	return true;
}

bool CAlgoHF::afterPCA_LiquidatingPosition(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	int K = IM.BasketSize;
	OSTR_DEF;
	OSTR_RESET

	int numOpenOrders = 0;
	for(int i=0; i<K; i++)
		numOpenOrders += (SM.orderBook[i].countActiveOrders(SM.orderBook[i].bid)+SM.orderBook[i].countActiveOrders(SM.orderBook[i].ask));
	int numOrders = 0;
	for(int i=0; i<K; i++) numOrders += SM.orderBook[i].countOrders(NULL);
	ostr << "PCA: numOpenOrders=" << numOpenOrders << ", numOrders=" << numOrders << "\n";
	OSTR_LOG(LOG_MSG);
	IM.Logger->write_pca_log(ostr.str());

	if(m_desiredPositionAchieved)
	{
		if(numOrders==0)
		{
			algState = ALL_CASH;
			return false;
		}
		else if(numOpenOrders>0)
		{
			for(int i=0; i<K; i++)
			{
				SM.orderBook[i].cancelSide(BID_ORDER);
				SM.orderBook[i].cancelSide(ASK_ORDER);
			}
		}
	}

	SM.DeviationDetected = false;
	SM.LiquidateDivergentPos = true;
	afterPCA_default(position, lastVector, eigVector, eigValue);
	return true;
}


void CAlgoHF::finalize(double *eigVector, double *lastVector)
{
	int K = IM.BasketSize;
	OSTR_DEF

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

void CAlgoHF::calcDesiredPosition(double *last, double *eigVector, long *desPos)
{
	int K = IM.BasketSize;
	OSTR_DEF

	//compute lastOrth
	Utils::calcOrth(SM.lastOrth, eigVector, K, last);

	double pp = Utils::calcInnerProd(&eigVector[(K-1)*K],last,K,1,1);
	for(int i=0; i<K; i++) SM.deltaPrincipal[i] = (pp-SM.SigmaValue[K-1])*eigVector[(K-1)*K+i];
	//double pp = 0.0;
	//for(int i=0; i<K; i++) pp += (last[i] - SM.SigmaValue[K-1]*eigVector[(K-1)*K+i])*eigVector[(K-1)*K+i];

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

			//don't go against momentum
			if(SM.deltaPrincipal[i]>0.05) desPos[i] = MAX(desPos[i],0);
			else if(SM.deltaPrincipal[i]<-0.05) desPos[i] = MIN(desPos[i],0);
		}
	}
}

bool CAlgoHF::isPositionNeutral()
{
	double MV = getMarketValue();
	return (fabs(MV) < 1000);
}

