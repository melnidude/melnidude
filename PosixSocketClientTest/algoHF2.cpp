#include <iomanip>
#include <math.h>
#include "algoHF2.h"
#include "memory.h"
#include "utils.h"
#include <time.h>

#include <algorithm>
#include <vector>

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;

CAlgoHF2::CAlgoHF2(int numNeutrDimensions)
{
    lastVectorAdj = new double[IM.BasketSize];
	positionOrth = new double[IM.BasketSize];
	positionPP = new double[IM.BasketSize];
	vwap = new double[IM.BasketSize];

	m_numNeutrDimensions = numNeutrDimensions;
	neutOrderSize = new long[m_numNeutrDimensions];
	m_neutrOrderCnt = 0;

	algState = ALL_CASH;

	DesiredPositionPrev = new long[IM.BasketSize];
	EstOrdersPlaced = new bool[IM.BasketSize];
	dimFullyNeutralizable = new bool[IM.BasketSize];
	PositionNeutralizable = new long[IM.BasketSize];
	for(int j=0; j<IM.BasketSize; j++)
	{
		PositionNeutralizable[j] = 0;
	}

	MarketValue = 0.0;
	PortfolioValue = 0.0;
	UnrealizedGain = 0.0;
	OneSidedValue = 0.0;

	timePrefix = SM.globalState->getTimePrefix();
}

CAlgoHF2::~CAlgoHF2()
{
    delete lastVectorAdj;
	delete positionOrth;
	delete positionPP;
	delete vwap;
	delete neutOrderSize;
}

BracketHierarchy CAlgoHF2::GenerateDesiredCumulativeOrders(int assetId, Order **orderB, Order **orderS)
{
	BracketHierarchy hierarchy = LEGS_EQUAL;

	IM.ostr << "GenerateDesiredCumulativeOrders: assetId=" << assetId << "\n";

#if !LOCAL_NEUTRALIZATION
	if(assetId==0)
	{
		NeutralizePosition(assetId, orderB, orderS);
		if(m_numNeutrDimensions>1) LiquidatePosition(assetId, orderB, orderS);
		EstOrdersPlaced[assetId] = false;
		IM.ostr << "GenerateDesiredCumulativeOrders: finished neutralizing\n";
	}
	else
#endif
		if(abs(SM.position[assetId])<IM.minOrderSize)
	{
		hierarchy = TakeOnPositionLayering(assetId, orderB, orderS);
	}
	else
	{
		//LiquidatePosition(assetId, orderB, orderS);
		LiquidateLayering(assetId, orderB, orderS);
		EstOrdersPlaced[assetId] = false;
	}
	IM.ostr << "GenerateDesiredCumulativeOrders: finished function\n";
	return hierarchy;
}


BracketHierarchy CAlgoHF2::TakeOnPositionLayering(int assetId, Order **orderB_ptr, Order **orderS_ptr)
{
	Order *orderB=NULL, *orderS=NULL;
	long BidAsk[2];
	BracketHierarchy hierarchy = LEGS_EQUAL;
	BookElement *element;
	double dev;
	long size;
	long layerTotalQuantity;
	long DesiredPosition = (!EstOrdersPlaced[assetId]) ? SM.DesiredPosition[assetId] : DesiredPositionPrev[assetId];

	//IM.ostr << "momentum=" << SM.momentum[assetId] << "\n";

	BidAsk[BID_ORDER] = 0;
	BidAsk[ASK_ORDER] = 0;
	BidAsk[(SM.lastOrth[assetId] > TINY)] = abs(DesiredPosition);

	IM.ostr << "TakeOn " << SM.marketData[assetId].contract->symbol << ": DesPos=" << DesiredPosition << "\n";
	//IM.ostr << "lastOrth[]=" << SM.lastOrth[assetId] << ", DeviationSize=" << SM.DeviationSize << ", sigmaPP=" << SM.sigmaPP << "\n";

	//stopgap logic
	if(BidAsk[BID_ORDER] < IM.minOrderSize) BidAsk[BID_ORDER] = 0;
	if(BidAsk[ASK_ORDER] < IM.minOrderSize) BidAsk[ASK_ORDER] = 0;

	//now determine price and order type
	//if(fabs(SM.lastOrth[assetId]) <  1.5*SM.sigmaOrigDim[assetId])
	if(1)
	{
		if(SM.marketData[assetId].marketPrev[BID]>SM.marketData[assetId].market[BID])
		{
			orderB = *orderB_ptr = SM.marketData[assetId].orderB;
			orderB->parentId = 0;
			orderB->ocaGroup = "";
			orderB->totalQuantity = abs(DesiredPosition); //BidAsk[BID_ORDER];
			orderB->neutrOrder = false;
			orderB->stateDesc = "EstPosn";
			//orderB->nbboPriceCap = 0.0;
			orderB->orderType = IM.PrefOrders;
			orderB->auxPrice = 0.00;
			orderB->lmtPrice = SM.marketData[assetId].market[BID]; //+0.01;
			orderB->nbboWhenPlaced = orderB->lmtPrice;
			//IM.ostr << "TakeOn: desired: L " << orderB->totalQuantity << " of " << SM.marketData[assetId].contract->symbol << "\n";
		}

		if(SM.marketData[assetId].marketPrev[ASK]<SM.marketData[assetId].market[ASK])
		{
			orderS = *orderS_ptr = SM.marketData[assetId].orderS;
			orderS->parentId = 0;
			orderS->ocaGroup = "";
			orderS->totalQuantity = abs(DesiredPosition); //orderB->totalQuantity;
			orderS->neutrOrder = false;
			orderS->stateDesc = "EstPosn";
			//orderS->nbboPriceCap = 0.0;
			orderS->orderType = IM.PrefOrders;
			orderS->auxPrice = 0.00;
			orderS->lmtPrice = SM.marketData[assetId].market[ASK]; //-0.01;
			orderS->nbboWhenPlaced = orderS->lmtPrice;
			//IM.ostr << "TakeOn: desired: S " << orderS->totalQuantity << " of " << SM.marketData[assetId].contract->symbol << "\n";
			hierarchy = LEGS_EQUAL;
		}
	}

	ShortOrder *orderBestB = SM.orderBook[assetId].getBestOrder(BID_ORDER);
	ShortOrder *orderBestS = SM.orderBook[assetId].getBestOrder(ASK_ORDER);
	//IM.ostr << "TakeOn: after getBestOrder (" << orderBestB << "," << orderBestS << ")\n";

	if(orderB)
	{
		//if(!orderS) SM.orderBook[assetId].cancelSide(ASK_ORDER);
		SM.orderBook[assetId].cancelSide(BID_ORDER, SM.marketData[assetId].market[BID]);

		double lmtMin = SM.marketData[assetId].market[BID] - IM.NumLayers*0.01;
		double lmtMax = SM.marketData[assetId].market[BID];
		layerTotalQuantity = orderB->totalQuantity/IM.NumLayers;

		for(double lmt=lmtMin; lmt<lmtMax+TINY; lmt+=0.01)
		{
			size = 0;
			element = SM.orderBook[assetId].bid;
			while(element)
			{
				if(!element->m_order->neutrOrder)
				{
					dev = getDistance(element->m_order->lmtPrice, lmt, SM.marketData[assetId].market[BID]);
					//IM.ostr << "TakeOnPosition: B dev=" << dev << "\n";
					if(dev < TINY)
					{
						size += element->m_order->totalQuantity;
						IM.ostr << "\texisting order B : " << element->m_order->totalQuantity << " at " << element->m_order->lmtPrice << "\n";
					}
				}
				element = element->nextElement;
			}

			orderB->lmtPrice = lmt;
			orderB->nbboWhenPlaced = orderB->lmtPrice;
			orderB->totalQuantity = layerTotalQuantity - size;
			orderB->neutralizable = false; //(fabs(lmt-SM.marketData[assetId].market[BID]) > TINY);

			if(abs(orderB->totalQuantity) < IM.minOrderSize) orderB->totalQuantity = 0;
			IM.ostr << "TakeOn: totalQuantity=" << orderB->totalQuantity << "\n";

			genDeltaOrder(assetId, orderB, size);
		}
	}

	if(orderS)
	{
		//if(!orderB) SM.orderBook[assetId].cancelSide(BID_ORDER);
		SM.orderBook[assetId].cancelSide(ASK_ORDER, SM.marketData[assetId].market[ASK]);

		double lmtMin = SM.marketData[assetId].market[ASK];
		double lmtMax = SM.marketData[assetId].market[ASK] + IM.NumLayers*0.01;
		layerTotalQuantity = orderS->totalQuantity/IM.NumLayers;

		for(double lmt=lmtMin; lmt<lmtMax+TINY; lmt+=0.01)
		{
			size = 0;
			element = SM.orderBook[assetId].ask;
			while(element)
			{
				if(!element->m_order->neutrOrder)
				{
					dev = getDistance(element->m_order->lmtPrice, lmt, SM.marketData[assetId].market[ASK]);
					//IM.ostr << "TakeOnPosition: S dev=" << dev << "\n";
					if(dev < TINY)
					{
						size += element->m_order->totalQuantity;
						IM.ostr << "\texisting order S : " << element->m_order->totalQuantity << " at " << element->m_order->lmtPrice << "\n";
					}
				}
				element = element->nextElement;
			}

			//if(lmt-lmtMin < TINY) layerTotalQuantity *= 0.5;
			//else if(lmtMax-lmt < TINY) layerTotalQuantity *= 1.5;

			orderS->lmtPrice = lmt;
			orderS->nbboWhenPlaced = orderS->lmtPrice;
			orderS->totalQuantity = layerTotalQuantity - size;
			orderS->neutralizable = false; //(fabs(lmt-SM.marketData[assetId].market[ASK]) > TINY);

			if(abs(orderS->totalQuantity) < IM.minOrderSize) orderS->totalQuantity = 0;
			IM.ostr << "TakeOn: totalQuantity=" << orderS->totalQuantity << "\n";

			genDeltaOrder(assetId, orderS, size);
		}
	}

	EstOrdersPlaced[assetId] = true;
	DesiredPositionPrev[assetId] = SM.DesiredPosition[assetId];

	return hierarchy;
}

BracketHierarchy CAlgoHF2::LiquidateLayering(int assetId, Order **orderB_ptr, Order **orderS_ptr)
{
	Order *orderB=NULL, *orderS=NULL;
	BracketHierarchy hierarchy = LEGS_EQUAL;
	BookElement *element;
	int cnt;
	double dev;
	long size;
	long layerTotalQuantity;
	long DesiredPosition = (!EstOrdersPlaced[assetId]) ? abs(SM.DesiredPosition[assetId]) : abs(DesiredPositionPrev[assetId]);
	long position = SM.position[assetId];
	long orderSize;
	bool side = (position > 0);

	//IM.ostr << "\nLiqLayering: " << SM.marketData[assetId].contract->symbol << "\n";

	//if()

	//now determine price and order type
	//if(fabs(SM.lastOrth[assetId]) <  1.5*SM.sigmaOrigDim[assetId])
	//if(!side)
	{
		orderB = *orderB_ptr = SM.marketData[assetId].orderB;
		orderB->parentId = 0;
		orderB->ocaGroup = "";
		//orderB->totalQuantity = abs(DesiredPosition); //BidAsk[BID_ORDER];
		orderB->neutrOrder = false;
		orderB->stateDesc = "EstPosn";
		//orderB->nbboPriceCap = 0.0;
		orderB->orderType = IM.PrefOrders;
		orderB->auxPrice = 0.00;
		//orderB->lmtPrice = SM.marketData[assetId].market[BID]; //+0.01;
		//orderB->nbboWhenPlaced = orderB->lmtPrice;
		IM.ostr << "LiqLayering: desired: L " << DesiredPosition << " of " << SM.marketData[assetId].contract->symbol << "\n";
		ShortOrder *orderBestB = SM.orderBook[assetId].getBestOrder(BID_ORDER);
		//IM.ostr << "LiqLayering: after getBestOrder (" << orderBestB << ")\n";

		double lmtMin = SM.marketData[assetId].market[BID] - IM.NumLayers*0.01;
		double lmtMax = SM.marketData[assetId].market[BID];

		//if(!orderS) SM.orderBook[assetId].cancelSide(ASK_ORDER);
#if 1
		if(position>0)
		{
			SM.orderBook[assetId].cancelSide(BID_ORDER, SM.marketData[assetId].market[BID]-0.01);
			lmtMax -= 0.01;
		}
		else
#endif
			SM.orderBook[assetId].cancelSide(BID_ORDER, SM.marketData[assetId].market[BID]);

		layerTotalQuantity = DesiredPosition/IM.NumLayers;

		cnt = 0;
		orderSize = MAX(position, 0);
		//for(double lmt=lmtMax; lmt>lmtMin-TINY; lmt-=0.01, cnt++)
		for(double lmt=lmtMin; lmt<lmtMax+TINY; lmt+=0.01, cnt++)
		{
			size = 0;
			element = SM.orderBook[assetId].bid;
			while(element)
			{
				if(!element->m_order->neutrOrder)
				{
					dev = getDistance(element->m_order->lmtPrice, lmt, SM.marketData[assetId].market[BID]);
					//IM.ostr << "TakeOnPosition: B dev=" << dev << "\n";
					if(dev < TINY)
					{
						size += element->m_order->totalQuantity;
						IM.ostr << "\texisting order B : " << element->m_order->totalQuantity << " at " << element->m_order->lmtPrice << "\n";
					}
					else if(element->orderStatus!=CANCELLED_UNCONFIRMED)
					{
						if(!inInterval(element->m_order->lmtPrice, lmtMin, lmtMax, SM.marketData[assetId].market[BID]))
						{
							IM.client->cancelOrder(element->m_order->orderId);
							element->orderStatus = CANCELLED_UNCONFIRMED;
						}
					}
				}
				element = element->nextElement;
			}

			orderB->lmtPrice = lmt;
			orderB->nbboWhenPlaced = orderB->lmtPrice;
			orderB->totalQuantity = layerTotalQuantity - size;

			//leave partial fills and existing orders alone
			if(size>0) { orderSize += size; continue; }
#if 0
			if(position>0)
			{
				//don't replenish at the bid when Long
				if((size==0)||(fabs(lmt-lmtMax) < TINY)) continue;
			}
			else
#endif
				if(position<0)
			{
				//backfill with larger orders when Short
				if(size==0) orderB->totalQuantity = 2.0*layerTotalQuantity - size;
			}

			//if(orderSize + layerTotalQuantity > DesiredPosition) break;
			if(position>0)
				if(orderSize > DesiredPosition) break;

			//orderB->neutralizable = (fabs(lmt-SM.marketData[assetId].market[BID]) > TINY);
			orderB->neutralizable = false;
			//if((cnt&0x3)==0) orderB->neutralizable = true;
			//if(fabs(lmt-lmtMax) < TINY) orderB->neutralizable = false;
			//if(fabs(lmt-lmtMin) < TINY) orderB->neutralizable = true;

			if(abs(orderB->totalQuantity) < IM.minOrderSize) orderB->totalQuantity = 0;
			IM.ostr << "Liq: " << orderB->totalQuantity << " at " << lmt << "\n";

			genDeltaOrder(assetId, orderB, size);
			orderSize += layerTotalQuantity;
		}
	}

	//else
	{
		orderS = *orderS_ptr = SM.marketData[assetId].orderS;
		orderS->parentId = 0;
		orderS->ocaGroup = "";
		//orderS->totalQuantity = abs(DesiredPosition); //orderB->totalQuantity;
		orderS->neutrOrder = false;
		orderS->stateDesc = "EstPosn";
		//orderS->nbboPriceCap = 0.0;
		orderS->orderType = IM.PrefOrders;
		orderS->auxPrice = 0.00;
		//orderS->lmtPrice = SM.marketData[assetId].market[ASK]; //-0.01;
		//orderS->nbboWhenPlaced = orderS->lmtPrice;
		IM.ostr << "TakeOn: desired: S " << DesiredPosition << " of " << SM.marketData[assetId].contract->symbol << "\n";
		//hierarchy = LEGS_EQUAL;
		ShortOrder *orderBestS = SM.orderBook[assetId].getBestOrder(ASK_ORDER);
		//IM.ostr << "LiquidateLayering: after getBestOrder (" << orderBestS << ")\n";

		double lmtMin = SM.marketData[assetId].market[ASK];
		double lmtMax = SM.marketData[assetId].market[ASK] + IM.NumLayers*0.01;

		//if(!orderB) SM.orderBook[assetId].cancelSide(BID_ORDER);
#if 1
		if(position<0)
		{
			SM.orderBook[assetId].cancelSide(ASK_ORDER, SM.marketData[assetId].market[ASK]+0.01);
			lmtMin += 0.01;
		}
		else
#endif
			SM.orderBook[assetId].cancelSide(ASK_ORDER, SM.marketData[assetId].market[ASK]);

		layerTotalQuantity = DesiredPosition/IM.NumLayers;

		cnt = 0;
		orderSize = -MIN(position, 0);
		for(double lmt=lmtMax; lmt>lmtMin-TINY; lmt-=0.01, cnt++)
		//for(double lmt=lmtMin; lmt<lmtMax+TINY; lmt+=0.01, cnt++)
		{
			size = 0;
			element = SM.orderBook[assetId].ask;
			while(element)
			{
				if(!element->m_order->neutrOrder)
				{
					dev = getDistance(element->m_order->lmtPrice, lmt, SM.marketData[assetId].market[ASK]);
					//IM.ostr << "TakeOnPosition: S dev=" << dev << "\n";
					if(dev < TINY)
					{
						size += element->m_order->totalQuantity;
						IM.ostr << "\texisting order S : " << element->m_order->totalQuantity << " at " << element->m_order->lmtPrice << "\n";
					}
					else if(element->orderStatus!=CANCELLED_UNCONFIRMED)
					{
						if(!inInterval(element->m_order->lmtPrice, lmtMin, lmtMax, SM.marketData[assetId].market[ASK]))
						{
							IM.client->cancelOrder(element->m_order->orderId);
							element->orderStatus = CANCELLED_UNCONFIRMED;
						}
					}
				}
				element = element->nextElement;
			}

			orderS->lmtPrice = lmt;
			orderS->nbboWhenPlaced = orderS->lmtPrice;
			orderS->totalQuantity = layerTotalQuantity - size;

			//leave partial fills and existing orders alone
			if(size>0) { orderSize += size; continue; }
#if 0
			if(position<0)
			{
				//don't replenish at the ask when Short
				if((size==0)||(fabs(lmt-lmtMin) < TINY)) continue;
			}
			else
#endif
				if(position>0)
			{
				//backfill with larger orders when Long
				if(size==0) orderS->totalQuantity = 2.0*layerTotalQuantity - size;
			}

			//if(orderSize + layerTotalQuantity > DesiredPosition) break;
			if(position<0)
				if(orderSize > DesiredPosition) break;

			//orderS->neutralizable = (fabs(lmt-SM.marketData[assetId].market[ASK]) > TINY);
			orderS->neutralizable = false;
			//if((cnt&0x3)==0) orderS->neutralizable = true;
			//if(fabs(lmt-lmtMin) < TINY) orderS->neutralizable = false;
			//if(fabs(lmt-lmtMax) < TINY) orderS->neutralizable = true;

			if(abs(orderS->totalQuantity) < IM.minOrderSize) orderS->totalQuantity = 0;
			IM.ostr << "Liq: totalQuantity " << orderS->totalQuantity << " at " << lmt << "\n";

			genDeltaOrder(assetId, orderS, size);
			orderSize += layerTotalQuantity;
		}
	}

	return hierarchy;
}

void CAlgoHF2::LiquidatePosition(int assetId, Order **orderB_ptr, Order **orderS_ptr)
{
	Order *order;
	long BidAsk[2];
	bool side;
	IBString desc, action;
	double lmtPrice;
	double auxPrice=0.00;
	double liq_thr, d;

	IM.ostr << "\nLiquidatePosition, SM.LiquidateDivergentPos=" << SM.LiquidateDivergentPos << "\n";

	side = (SM.position[assetId]>0);

	BidAsk[BID_ORDER] = BidAsk[ASK_ORDER] = 0;

#if 1//!LOCAL_NEUTRALIZATION
	bool liquidateImmediate = false;
	if(isPositionNeutral())
	{
		liquidateImmediate = SM.LiquidateDivergentPos;
	}

	IM.ostr << "LiquidatePosition: liquidateImmediate=" << liquidateImmediate << "\n";
	if(liquidateImmediate)
	{
		SM.orderBook[assetId].cancelSide(!side);

		BidAsk[side] = abs(SM.position[assetId]);
		desc = "ImmLiq";
		lmtPrice = 0.00;
		liq_thr = 0.00;
	}
	else
#endif
	{
		//SM.orderBook[assetId].cancelSide(!side);

		BidAsk[side] = abs(SM.position[assetId]);
		desc = "Liq";
		if(side) lmtPrice = MAX(SM.VWAP[assetId].getVWAP()+0.01, SM.marketData[assetId].market[ASK]);
		else lmtPrice = MIN(SM.VWAP[assetId].getVWAP()-0.01, SM.marketData[assetId].market[BID]);
		liq_thr = 0.02;
	}


	action = (side) ? "Short" : "Long";
	if(BidAsk[side] < IM.minOrderSize) BidAsk[side] = 0;

	if(side) order = *orderS_ptr = SM.marketData[assetId].orderS;
	else order = *orderB_ptr = SM.marketData[assetId].orderB;

	double nbboWhenPlaced;
	if(lmtPrice<TINY) nbboWhenPlaced = SM.marketData[assetId].market[1+side];
	else if(side==BID_ORDER) nbboWhenPlaced = MIN(SM.marketData[assetId].market[1+side], lmtPrice);
	else if(side==ASK_ORDER) nbboWhenPlaced = MAX(SM.marketData[assetId].market[1+side], lmtPrice);

	order->totalQuantity = BidAsk[side];
	order->ocaGroup = "";
	//order->nbboPriceCap = 0.0;
	order->neutrOrder = false;
	order->stateDesc = desc;
	order->orderType = "REL";
	order->auxPrice = auxPrice;
	order->lmtPrice = lmtPrice;
	order->nbboWhenPlaced = nbboWhenPlaced + (1-2*side)*auxPrice;
	IM.ostr << "LiquidatePosition: desired: to go " << action << " " << order->totalQuantity << " of "
			<< SM.marketData[assetId].contract->symbol << " at " << order->lmtPrice << "\n";


	long size = 0;
	BookElement *element = (side) ? SM.orderBook[assetId].ask : SM.orderBook[assetId].bid;

	while(element)
	{
		IM.ostr << "\tchecking order " << element->m_order->orderId << " : desc= " << element->m_order->stateDesc << "\n";
		//if(!element->m_order->neutrOrder)
		{
			//dev = getDifference(element->m_order->lmtPrice, order->lmtPrice, SM.marketData[assetId].market[1+side]);
			//if(side==BID_ORDER) dev = -dev;
			d = getDistance(element->m_order->lmtPrice, order->lmtPrice, SM.marketData[assetId].market[1+side]);
			if(d < 0.005+TINY) //at vwap
			{
				size += element->m_order->totalQuantity;
				IM.ostr << "\texisting order: " << element->m_order->totalQuantity << " at " << element->m_order->lmtPrice << "\n";
			}
			else
			{
				d = getDistance(element->m_order->lmtPrice, SM.marketData[assetId].market[1+side], SM.marketData[assetId].market[1+side]);
				if(d > liq_thr+TINY) //far from the market
				{
					if(element->orderStatus!=CANCELLED_UNCONFIRMED)
					{
						IM.client->cancelOrder(element->m_order->orderId);
						element->orderStatus = CANCELLED_UNCONFIRMED;
					}
				}
			}
		}
		element = element->nextElement;
	}


	IM.ostr << "existing orders (" << SM.marketData[assetId].contract->symbol << ") : to go " << action << " " << size << "\n";

	if(order->totalQuantity-size != 0)
	{
		long sizeAtPriceB = (side) ? 0 : size;
		long sizeAtPriceS = (side) ? size : 0;
		long desB = (side) ? 0 : order->totalQuantity;
		long desA = (side) ? order->totalQuantity : 0;
		IM.Logger->writeDesired(SM.globalState->getTime(), assetId, desB, desA, sizeAtPriceB, sizeAtPriceS);
	}

	order->totalQuantity -= size;
	if(abs(order->totalQuantity) < IM.minOrderSize) order->totalQuantity = 0;
	order->neutralizable = true;

	IM.ostr << "LiquidatePosition: order->totalQuantity=" << order->totalQuantity << "\n";

	genDeltaOrder(assetId, order, size);

}

void CAlgoHF2::NeutralizePosition(int assetId, Order **orderB_ptr, Order **orderS_ptr)
{
	int K = IM.BasketSize;
	Order *order;
	double MVneutr = 0.0, MVall;
	double dev = SM.lastOrth[assetId];
	double thr;
	bool side;
	IBString action;

	IM.ostr << "\nNeutralizePosition\n";

	/*for(int i=0; i<K; i++)
	{
		if(SM.position[i] > 0) MVneutr += SM.position[i]*SM.marketData[i].market[BID];
		else if(SM.position[i] < 0) MVneutr += SM.position[i]*SM.marketData[i].market[ASK];
	}
	*/

	MVall = getMarketValue();
	MVneutr = getMarketValueNeutralizable();
	IM.ostr << "MVneutr=" << MVneutr << ", MVall=" << MVall << "\n";


	/*side = (MVneutr < 0);
	for(int i=0; i<K; i++)
	{
		SM.orderBook[i].cancelSide(side);
	}*/


	for(int i=0; i<m_numNeutrDimensions; i++)
	{
		neutOrderSize[i] = 0;
		if(MVneutr > TINY) neutOrderSize[i] = -(long)(MVneutr/SM.marketData[i].market[BID]);
		else if(MVneutr < -TINY) neutOrderSize[i] = -(long)(MVneutr/SM.marketData[i].market[ASK]);
	}

	IM.ostr << "Neutr: ";
	for(int i=0; i<m_numNeutrDimensions; i++) IM.ostr << SM.marketData[i].contract->symbol << "=" << neutOrderSize[i] << " ";
	IM.ostr << "\n";
	IM.ostr << "Pos: ";
	for(int i=0; i<K; i++) IM.ostr << SM.marketData[i].contract->symbol << "=" << SM.position[i] << " ";
	IM.ostr << "\n";

	std::stringstream ocaGroup;
	ocaGroup << timePrefix << "_Neut" << m_neutrOrderCnt;
	for(int i=0; i<m_numNeutrDimensions; i++)
	{
		if(neutOrderSize[i]==0) continue;

		IM.ostr << "\nNeutralizing with " << SM.marketData[i].contract->symbol << "\n";

		side = (neutOrderSize[i] < 0);
		action = (side) ? "Short" : "Long";

		SM.orderBook[i].cancelSide(!side, true); //cancel neutralizing orders on the other side

		if(side) order = *orderS_ptr = SM.marketData[i].orderS;
		else order = *orderB_ptr = SM.marketData[i].orderB;

		order->totalQuantity = abs(neutOrderSize[i]);
		order->neutrOrder = true;
		order->stateDesc = "Neutralize";
		order->ocaGroup = ocaGroup.str();
		order->orderType = "REL";
		order->lmtPrice = 0.00;
		order->auxPrice = 0.00;
		//if((side==BID_ORDER)&&(SM.momentum[assetId] == MOMENTUM_UP)/*&&(dev<-0.01+TINY)*/) order->auxPrice = 0.01;
		//else if((side==ASK_ORDER)&&(SM.momentum[assetId] == MOMENTUM_DOWN)/*&&(dev>0.01-TINY)*/) order->auxPrice = 0.01;
		order->nbboWhenPlaced = SM.marketData[i].market[1+side]+(1-2*side)*order->auxPrice;
		IM.ostr << "NeutralizePosition: desired: " << action << " " << order->totalQuantity << " of "
				<< SM.marketData[i].contract->symbol << ", mkt= " << SM.marketData[i].market[1+side] << ", oca=" << order->ocaGroup << "\n";

		long size = 0;
		BookElement *element = (side) ? SM.orderBook[i].ask : SM.orderBook[i].bid;

		thr = 0.00;
		while(element)
		{
			IM.ostr << "\tchecking order " << element->m_order->orderId << " : desc= " << element->m_order->stateDesc << "\n";
			if(element->m_order->neutrOrder)
			{
				dev = getDistance(element->m_order->lmtPrice, order->lmtPrice, SM.marketData[i].market[1+side]);
				if(dev > thr+TINY)
				{
					if(element->orderStatus!=CANCELLED_UNCONFIRMED)
					{
						IM.client->cancelOrder(element->m_order->orderId);
						element->orderStatus = CANCELLED_UNCONFIRMED;
					}
				}
				else
				{
					size += element->m_order->totalQuantity;
					IM.ostr << "\texisting order: " << element->m_order->totalQuantity << " at " << element->m_order->lmtPrice << "\n";
				}
			}
			element = element->nextElement;
		}

		IM.ostr << "existing orders (" << SM.marketData[i].contract->symbol << ") : to go " << action << " " << size << "\n";

		if(IM.Logger->enableMiscLogs)
		{
			if(order->totalQuantity != 0)
			{
				long sizeAtPriceB = (side) ? 0 : size;
				long sizeAtPriceS = (side) ? size : 0;
				long desB = (side) ? 0 : order->totalQuantity;
				long desA = (side) ? order->totalQuantity : 0;
				IM.Logger->writeDesired(SM.globalState->getTime(), assetId, desB, desA, sizeAtPriceB, sizeAtPriceS);
			}
		}

		order->totalQuantity -= size;
		if(abs(order->totalQuantity) < IM.minOrderSize) order->totalQuantity = 0;

		if(order->totalQuantity != 0) m_neutrOrderCnt++;

		order->neutralizable = true;

		genDeltaOrder(i, order, size);
	}
	IM.ostr << "CAlgoHF2: finished NeutralizePosition\n";
}


void CAlgoHF2::afterFill(int assetId)
{
	SM.pca->run();
	IM.ostr << "afterFill: after run\n";
	//orderManager(0);
	//orderManager(assetId);
}

void CAlgoHF2::afterPCA(long *position, double *lastVector, double *eigVector, double *eigValue)
{
	int K = IM.BasketSize;
	double Dtime = SM.globalState->getDTime();

	IM.ostr << "\n";

	//calculate the desired position
	calcDesiredPosition(lastVector, eigVector, SM.DesiredPositionTmp); //calc SM.weights, SM.AtomicSize
	for(int i=0; i<K; i++)
	{
		lastVectorAdj[i] = (SM.DesiredPositionTmp[i]>0) ? SM.marketData[i].market[ASK] : SM.marketData[i].market[BID];
	}

	calcDesiredPosition(lastVectorAdj, eigVector, SM.DesiredPosition); //calc SM.weights, SM.AtomicSize
	/*for(int i=1; i<K; i++)
	{
		if((SM.momentum[i] == MOMENTUM_UP)&&(SM.DesiredPosition[i]<0))  SM.DesiredPosition[i] = 0;
		else if((SM.momentum[i] == MOMENTUM_DOWN)&&(SM.DesiredPosition[i]>0))  SM.DesiredPosition[i] = 0;
	}*/

	bool zeroPosition = true;
	for(int i=0; i<K; i++) { if(abs(SM.position[i]) > IM.minOrderSize) zeroPosition = false; }
	if(zeroPosition)
	{
		//calc positionOrth
		Utils::calcOrth(positionOrth, eigVector, K, SM.DesiredPosition);

		mpvon = Utils::calcInnerProd(lastVectorAdj,positionOrth,K,1,1);
		for(int i=0; i<K; i++) SM.lastPP[i] = Utils::calcInnerProd(&eigVector[i*K],lastVectorAdj,K,1,1);
		SM.DeviationSize = Utils::calcVectorNorm(SM.lastPP,K-1);
		SM.LiquidateDivergentPos = false;
		double sigma = 0.0;
		for(int i=0; i<K-1; i++) sigma += pow(SM.lastPP[i]*eigValue[i],2.0);
		sigma = (fabs(SM.DeviationSize) > TINY) ? pow(sigma,0.5)/SM.DeviationSize : 0.0;
		SM.sigmaPP = sigma;
		IM.Logger->writeMasterGetIntoInfo(SM.globalState->getTime(), SM.DeviationSize, sigma, mpvon);
	}
	else
	{
		bool posNeutral = isPositionNeutral();
		//calc positionOrth
		if(posNeutral)
			Utils::calcOrth(positionOrth, eigVector, K, SM.position);
		else
		{
			double MV = getMarketValue();
			long posPrev = SM.position[0];
			SM.position[0] = (MV>0) ? -(long)(MV/SM.marketData[0].market[BID]) : -(long)(MV/SM.marketData[0].market[ASK]);
			Utils::calcOrth(positionOrth, eigVector, K, SM.position);
			SM.position[0] = posPrev;
		}

		for(int i=0; i<K; i++)
		{
			if(abs(SM.position[i]) <= IM.minOrderSize)
			{
				vwap[i] = 0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]);
				lastVectorAdj[i] = 0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]);
			}
			else
			{
				vwap[i] = SM.VWAP[i].getVWAP();
				lastVectorAdj[i] = (SM.position[i]>0) ? SM.marketData[i].market[BID] : SM.marketData[i].market[ASK];
			}
		}
		double norm = Utils::calcVectorNorm(positionOrth,K);
		double sigma = 0.0;
		for(int i=0; i<K-1; i++)
		{
			positionPP[i] = Utils::calcInnerProd(&eigVector[i*K],positionOrth,K,1,1);
			sigma += pow(positionPP[i]*eigValue[i],2.0);
		}
		sigma = pow(sigma,0.5);
		vpvon = Utils::calcInnerProd(vwap,positionOrth,K,1,1);
		mpvon = Utils::calcInnerProd(lastVectorAdj,positionOrth,K,1,1);

		for(int i=0; i<K; i++) SM.lastPP[i] = Utils::calcInnerProd(&eigVector[i*K],lastVectorAdj,K,1,1);
		SM.DeviationSize = Utils::calcVectorNorm(SM.lastPP,K-1);
		SM.LiquidateDivergentPos = ((mpvon > (vpvon + sigma))||(vpvon > 2*sigma))&&(norm > TINY);

		if(IM.Logger->enableMiscLogs)
			IM.ugorth_ostr << Dtime << "," << 95 << "," << mpvon << "," << vpvon << "\n";
	}

	if(IM.debugPCA)
	{
		IM.ostr << "Eigenvectors: \n";
		for(int i=0; i<K; i++)
		{
			for(int j=0; j<K; j++)
			{
				IM.ostr << eigVector[i*K+j] << " ";
			}
			IM.ostr << "\n";
		}

		IM.ostr << "Eigenvalues: \n";
		for(int i=0; i<K; i++)
		{
			IM.ostr << eigValue[i] << " ";
		}
		IM.ostr << "\n";
	}

	for(int i=0; i<K; i++) SM.sigmaOrigDim[i] = fabs(Utils::calcInnerProd(&eigVector[i],&eigValue[0],K-1,K,1));
	//IM.ostr << "sigmaOrigDim: ";
	//for(int i=0; i<K; i++) IM.ostr << SM.sigmaOrigDim[i] << " ";
	//IM.ostr << "\n";

	IM.ostr << "Desired pos: ";
	for(int i=0; i<K; i++) IM.ostr << SM.DesiredPosition[i] << " ";
	IM.ostr << "\n";

	algState = (SM.LiquidateDivergentPos) ? LIQUIDATING_POS : ALL_CASH;

	if(IM.Logger->enableMiscLogs)
	{
		IM.devSize_ostr << Dtime << "," << 94 << "," << SM.DeviationSize << "," << vpvon << "\n";

		//lastOrth
		for(int i=0; i<K; i++)  IM.lastOrth_ostr << Dtime << "," << 90+i << "," << SM.lastOrth[i] << "," << 0 << "\n";

		//last principal projection
		double pp = 0.0;
		for(int i=0; i<K; i++) pp += (lastVector[i] - SM.SigmaValue[K-1]*eigVector[(K-1)*K+i])*eigVector[(K-1)*K+i];
		IM.lastPP_ostr << Dtime << "," << 89 << "," << pp << "," << 0 << "\n";
	}


	//IM.ostr << "nbbo Now(nbbo_when_placed): ";
	for(int i=0; i<K; i++)
	{
		if(SM.DesiredPosition[i]==0) { SM.nbboCurrent[i]=0; continue; }
		SM.nbboCurrent[i] = (SM.DesiredPosition[i]>0) ? SM.marketData[i].market[BID] : SM.marketData[i].market[ASK];
		SM.nbboWhenPlaced[i] = (SM.DesiredPosition[i]>0) ? SM.orderBook[i].getBestLimit(BID_ORDER) : SM.orderBook[i].getBestLimit(ASK_ORDER);
		//if((SM.nbboWhenPlaced[i] > TINY)&&(SM.nbboWhenPlaced[i] < INFINITY_D-TINY))
		//	IM.ostr << SM.nbboCurrent[i] << "(" << SM.nbboWhenPlaced[i] << ")  ";
	}
	//IM.ostr << "\n";

	IM.pca_enable = true;

	for(int j=IM.BasketSize-1; j>=0; j--)
	{
		orderManager(j);
	}
#if 0//LOCAL_NEUTRALIZATION
	neutralization();
#endif
	IM.ostr << "finished afterPCA\n";
}


void CAlgoHF2::calcDesiredPosition(double *last, double *eigVector, long *desPos)
{
	int K = IM.BasketSize;

	//compute lastOrth
	Utils::calcOrth(SM.lastOrth, eigVector, K, last);

	double pp = Utils::calcInnerProd(&eigVector[(K-1)*K],last,K,1,1);
	for(int i=0; i<K; i++) SM.deltaPrincipal[i] = (pp-SM.SigmaValue[K-1])*eigVector[(K-1)*K+i];

	//compute weights
#if 0
	double maxPrice = 0.0;
	for(int i=0; i<K; i++) maxPrice = MAX(fabs(SM.lastOrth[i]),maxPrice);
	for(int i=0; i<K; i++) SM.weights[i] = (maxPrice > TINY) ? fabs(SM.lastOrth[i]) / maxPrice : 0.0;
#else
	double maxPrice = 0.0;
	for(int i=0; i<K; i++) maxPrice = MAX(fabs(last[i]),maxPrice);
	for(int i=0; i<K; i++) SM.weights[i] = (fabs(last[i]) > TINY) ? maxPrice / fabs(last[i]) : 0.0;
#endif

	//compute AtomicSize
	double NLV = SM.globalState->getNLV();
	double w_sum = 0.0;
	for(int i=0; i<K; i++) w_sum += fabs(SM.weights[i])*last[i];
	SM.AtomicSize = (long)(min(IM.MaxPurchase,IM.MaxLeverage)*NLV/(IM.OrderSizeSF*w_sum)+0.5);

	for(int i=0; i<K; i++)
	{
		int sgn;
		sgn = 1-2*(SM.lastOrth[i] > TINY);
		desPos[i] = (long)(fabs(SM.weights[i]*SM.AtomicSize)/K+0.5)*sgn;
	}
}

bool CAlgoHF2::isPositionNeutral()
{
	double MV = getMarketValue();
	return (fabs(MV) < 1000);
}

long CAlgoHF2::neutrMinShares(long neutOrderSize, long neutOrderSizeAll, long pos, Momentum momentum)
{
	long neutrMin, neutrMax, neutr=0;

	neutrMin = MIN(neutOrderSize, neutOrderSizeAll);
	neutrMax = MAX(neutOrderSize, neutOrderSizeAll);

	//if(pos > neutrMax) neutr = neutrMax - pos;
	//else if(pos < neutrMin) neutr = neutrMin - pos;
	//if((pos!=0)&&(abs(pos+neutr)<=IM.minOrderSize)) neutr = -pos;

	if(neutrMax < 0) neutr = neutrMax;
	else if(neutrMin > 0) neutr = neutrMin;

	return neutr;
}

long CAlgoHF2::neutrWithMomentum(long neutOrderSize, long neutOrderSizeAll, long pos, Momentum momentum)
{
	long neutrMin, neutrMax, neutr=0;

	//neutrMin = MIN(neutOrderSize, neutOrderSizeAll);
	//neutrMax = MAX(neutOrderSize, neutOrderSizeAll);
	neutrMin = neutrMax = neutOrderSizeAll;

	if(momentum==MOMENTUM_UP)
	{
		if(neutrMax<0) neutr = 0.8*neutrMax;
		else if(neutrMax>0) neutr = 1.2*neutrMax;
		//neutr = neutr - pos;
	}
	else if(momentum==MOMENTUM_DOWN)
	{
		if(neutrMin<0) neutr = 1.2*neutrMin;
		else if(neutrMin>0) neutr = 0.8*neutrMin;
		//neutr = neutr - pos;
	}
	else
	{
		neutr = neutrMinShares(neutOrderSize, neutOrderSizeAll, pos, momentum);
	}

	return neutr;
}

void CAlgoHF2::genDeltaOrder(int assetId, Order *order, long existSize)
{
	if(order==NULL) return;
	int bidAsk = (order->action=="SELL");

	if(order)
	{
		IM.ostr << "desired " << order->action << " " << order->totalQuantity << "\n";
		if(order->totalQuantity!=0)
		{
			IM.Logger->writeMasterDimension(SM.globalState->getTime(), assetId, order->totalQuantity*(1-2*bidAsk), order->lmtPrice, existSize);

			order->parentId = 0;
			if(order->totalQuantity > 0)
			{
				IM.ostr << "adding a new order\n";

				double bp = SM.cpa->getBuyingPower();
				if(order->totalQuantity*SM.marketData[assetId].market[LAST] > 0.90*bp)
				{
					order->totalQuantity = 0.90*bp/SM.marketData[assetId].market[LAST];
					IM.ostr << "orderBook: BP mod: to BUY " << order->totalQuantity << " of " << SM.marketData[assetId].contract->symbol
							<< " at " << order->lmtPrice << " (" << bp << ")\n";
				}
				SM.orderBook[assetId].add(bidAsk,order);
				SM.globalState->updateExposure(bidAsk,order->lmtPrice*order->totalQuantity);
			}
			else if(order->totalQuantity < 0)
			{
				IM.ostr << "reducing book size\n";
				long sizeDelta = SM.orderBook[assetId].reduceSize(bidAsk,order);
				SM.globalState->updateExposure(bidAsk,-order->lmtPrice*sizeDelta);
			}
		}
	}
}

void CAlgoHF2::updatePortfolioStats(int assetId, long filled_s, bool neutralizable)
{
	updatePortfolioStats();

	double MVneutr = 0.0;
	if(neutralizable) PositionNeutralizable[assetId] = SM.position[assetId];
	else PositionNeutralizable[assetId] += filled_s;

	for(int i=0; i<IM.BasketSize; i++)
	{
		MVneutr += PositionNeutralizable[i]*SM.marketData[i].market[LAST];
	}
	MarketValueNeutralizable = MVneutr;
}

void CAlgoHF2::neutralization()
{
	Order *order;
	double MVneutr = 0.0, MVall, MVneutrAbs;
	double dev;
	double thr;
	bool side;
	IBString action;
	long neutrTradeSize;
	int assetId;
	double midpt;

	IM.ostr << "\nneutralization\n";

	MVall = getMarketValue();
	MVneutr = getMarketValueNeutralizable();
	IM.ostr << "neutralization: MVneutr=" << MVneutr << "\n";
	if(fabs(MVneutr) < TINY)
	{
		IM.ostr << "neutralization: nothing to neutralize.\n";
		return;
	}

	//sort dimensions by deviation from nbbo
	SM.DimDev.clear();
	for(int i=0; i<IM.BasketSize; i++)
	{
		if(SM.position[i]==0) continue;
		if(SM.position[i]>0) dev = SM.marketData[i].market[ASK] - SM.VWAP[i].getVWAP();  //dev=gain
		else dev = SM.VWAP[i].getVWAP() - SM.marketData[i].market[BID];

		DimDeviation element  = {i, dev };
		SM.DimDev.push_back(element);
		IM.ostr << "\tinserted: dev=" << dev << ", i=" << i << "\n";
	}

	std::sort(SM.DimDev.begin(), SM.DimDev.end());

	for(vector<DimDeviation>::const_iterator cur = SM.DimDev.begin(); cur != SM.DimDev.end(); cur++)
	{
		assetId = cur->index;
		IM.ostr << "map: retrieved: deviation=" << cur->deviation << ", index=" << assetId << "\n";

		if(MVneutr*SM.position[assetId]<0) continue;

		midpt = 0.5*(SM.marketData[assetId].market[BID]+SM.marketData[assetId].market[ASK]);
		MVneutrAbs = fabs(MVneutr);

		if(MVneutrAbs < IM.minOrderSize*midpt) break;
		side = (MVneutr > 0);
		action = (side) ? "Short" : "Long";

		neutrTradeSize = MIN((long)(MVneutrAbs/midpt), abs(SM.position[assetId]));
		//if(MVneutr > 0) neutrTradeSize = -neutrTradeSize;
		IM.ostr << "\tneutralization: " << SM.marketData[assetId].contract->symbol << ": Pos=" << SM.position[assetId] <<
				", Neutr=" << neutrTradeSize << ", " << action << " (MVneutr=" << MVneutr << ")\n";

		SM.orderBook[assetId].cancelSide(!side);

		if(side) order = SM.marketData[assetId].orderS;
		else order = SM.marketData[assetId].orderB;
		order->totalQuantity = neutrTradeSize;
		order->neutrOrder = true;
		order->stateDesc = "Neutralize";
		order->ocaGroup = "";
		order->orderType = "REL";
		order->lmtPrice = 0.00;
		order->auxPrice = 0.00; //0.01;
		order->nbboWhenPlaced = SM.marketData[assetId].market[1+side]+(1-2*side)*order->auxPrice;
		IM.ostr << "\tneutralization: desired: " << action << " " << order->totalQuantity << " of "
				<< SM.marketData[assetId].contract->symbol << ", mkt=" << SM.marketData[assetId].market[1+side] << "\n";

		long size = 0;
		BookElement *element = (side) ? SM.orderBook[assetId].ask : SM.orderBook[assetId].bid;

		thr = 0.00;
		while(element)
		{
			IM.ostr << "\tchecking order " << element->m_order->orderId << " : desc= " << element->m_order->stateDesc << "\n";
			dev = getDistance(element->m_order->lmtPrice, order->lmtPrice, SM.marketData[assetId].market[1+side]);
			if(dev > thr+TINY)
			{
				/*if(element->orderStatus!=CANCELLED_UNCONFIRMED)
				{
					IM.client->cancelOrder(element->m_order->orderId);
					element->orderStatus = CANCELLED_UNCONFIRMED;
				}*/
			}
			else
			{
				size += element->m_order->totalQuantity;
				IM.ostr << "\texisting order: " << element->m_order->totalQuantity << " at " << element->m_order->lmtPrice << "\n";
			}
			element = element->nextElement;
		}

		IM.ostr << "existing orders (" << SM.marketData[assetId].contract->symbol << ") : to go " << action << " " << size << "\n";

		order->totalQuantity -= size;
		if(abs(order->totalQuantity) < IM.minOrderSize) order->totalQuantity = 0;

		if(order->totalQuantity != 0) m_neutrOrderCnt++;

		order->neutralizable = true;

		genDeltaOrder(assetId, order, size);

		MVneutr += neutrTradeSize*midpt*(2*side-1);
	}

	IM.ostr << "CAlgoHF2: finished neutralization\n";
}


