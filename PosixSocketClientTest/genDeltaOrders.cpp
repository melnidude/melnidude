#include "memory.h"

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;


void OrderBook::genDeltaOrders(Order *orderB, Order *orderS, BracketHierarchy hierarchy)
{
	long sizeDelta;

	if((orderB)&&(orderS))
	{
		IM.ostr << "bracket : orderB->totalQuantity=" << orderB->totalQuantity << ", orderS->totalQuantity=" << orderS->totalQuantity << "\n";
		if(orderB->totalQuantity!=0)
		{
			IM.ostr << "adding a bracket in " << SM.marketData[m_assetId].contract->symbol << ", hierarchy=" << hierarchy << "\n";
			if(hierarchy==B_PARENT_OF_S)
			{
				if(orderB->totalQuantity!=0)
					IM.Logger->writeMasterDimension(SM.globalState->getTime(), m_assetId, orderB->totalQuantity, orderB->lmtPrice, 0);
				if(orderS->totalQuantity!=0)
					IM.Logger->writeMasterDimension(SM.globalState->getTime(), m_assetId, -orderS->totalQuantity, orderS->lmtPrice, 0);

				orderB->parentId = 0;
				orderB->transmit = false;
				add(BID_ORDER,orderB);

				orderS->parentId = orderB->orderId;
				orderS->transmit = true;
				add(ASK_ORDER,orderS);

				/*
				orderS->parentId = orderB->orderId;
				orderS->transmit = true;
				orderS->orderType = "STP";
				orderS->auxPrice = orderB->lmtPrice - 0.02;
				orderS->lmtPrice = 0;
				add(ASK_ORDER,orderS);
				*/
			}
			else if(hierarchy==S_PARENT_OF_B)
			{
				if(orderS->totalQuantity!=0)
					IM.Logger->writeMasterDimension(SM.globalState->getTime(), m_assetId, -orderS->totalQuantity, orderS->lmtPrice, 0);
				if(orderB->totalQuantity!=0)
					IM.Logger->writeMasterDimension(SM.globalState->getTime(), m_assetId, orderB->totalQuantity, orderB->lmtPrice, 0);

				orderS->parentId = 0;
				orderS->transmit = false;
				add(ASK_ORDER,orderS);

				orderB->parentId = orderS->orderId;
				orderB->transmit = true;
				add(BID_ORDER,orderB);
			}
			else if(hierarchy==LEGS_EQUAL)
			{
				if(orderB->totalQuantity!=0)
					IM.Logger->writeMasterDimension(SM.globalState->getTime(), m_assetId, orderB->totalQuantity, orderB->lmtPrice, 0);
				if(orderS->totalQuantity!=0)
					IM.Logger->writeMasterDimension(SM.globalState->getTime(), m_assetId, -orderS->totalQuantity, orderS->lmtPrice, 0);

				orderB->parentId = 0;
				orderB->transmit = true;
				add(BID_ORDER,orderB);

				orderS->parentId = 0;
				orderS->transmit = true;
				add(ASK_ORDER,orderS);
			}
		}
	}
	else if(orderB)
	{
		IM.ostr << "adding new order to the book (B), orderB->totalQuantity=" << orderB->totalQuantity << "\n";
		orderB->transmit = true;
		if(orderB->totalQuantity!=0)
			IM.Logger->writeMasterDimension(SM.globalState->getTime(), m_assetId, orderB->totalQuantity, orderB->lmtPrice, 0);

#if NEUT_STOP
		if(m_assetId==0)
		{
			//orderB->transmit = false;
			orderB->ocaGroup = "Neut" + m_order_cnt;
			orderB->ocaType = 3;
		}
#endif
		orderB->parentId = 0;
		if(orderB->totalQuantity > 0)
		{
			IM.ostr << "adding new order to the book (B)\n";

			double bp = SM.cpa->getBuyingPower();
			if(orderB->totalQuantity*SM.marketData[m_assetId].market[LAST] > 0.90*bp)
			{
				orderB->totalQuantity = 0.90*bp/SM.marketData[m_assetId].market[LAST];
				IM.ostr << "orderBook: BP mod: to BUY " << orderB->totalQuantity << " of "
						<< SM.marketData[m_assetId].contract->symbol
						<< " at " << orderB->lmtPrice << " (" << bp << ")\n";
			}
			add(BID_ORDER,orderB);
#if NEUT_STOP
			if(m_assetId==0)
			{
				orderB->ocaGroup = "Neut" + m_order_cnt;
				orderB->ocaType = 3;
				orderB->orderType = "STP";
				orderB->lmtPrice = 0.0;
				orderB->auxPrice = SM.marketData[m_assetId].market[BID]+0.02;
				add(BID_ORDER,orderB);
			}
#endif
			SM.globalState->updateExposure(BID_ORDER,orderB->lmtPrice*orderB->totalQuantity);
		}
		else if(orderB->totalQuantity < 0)
		{
			sizeDelta = reduceSize(BID_ORDER,orderB);
			SM.globalState->updateExposure(BID_ORDER,-orderB->lmtPrice*sizeDelta);
		}
	}
	else if(orderS)
	{
		IM.ostr << "adding new order to the book (S), orderS->totalQuantity=" << orderS->totalQuantity << "\n";
		orderS->transmit = true;
		if(orderS->totalQuantity!=0)
			IM.Logger->writeMasterDimension(SM.globalState->getTime(), m_assetId, -orderS->totalQuantity, orderS->lmtPrice, 0);

#if NEUT_STOP
		if(m_assetId==0)
		{
			orderS->ocaGroup = "Neut" + m_order_cnt;
			orderS->ocaType = 3;
		}
#endif
		orderS->parentId = 0;
		if(orderS->totalQuantity > 0)
		{
			IM.ostr << "adding new order to the book (S)\n";

			double sp = SM.cpa->getShortingPower();
			if(orderS->totalQuantity*SM.marketData[m_assetId].market[LAST] > 0.90*sp)
			{
				orderS->totalQuantity = 0.90*sp/SM.marketData[m_assetId].market[LAST];
				IM.ostr << "orderBook: BP mod: to SELL " << orderS->totalQuantity << " of "
						<< SM.marketData[m_assetId].contract->symbol
						<< " at " << orderS->lmtPrice << " (" << sp << ")\n";
			}

			add(ASK_ORDER,orderS);
#if NEUT_STOP
			if(m_assetId==0)
			{
				orderS->ocaGroup = "Neut" + m_order_cnt;
				orderS->ocaType = 3;
				orderS->orderType = "STP";
				orderS->lmtPrice = 0.0;
				orderS->auxPrice = SM.marketData[m_assetId].market[ASK]-0.02;
				add(ASK_ORDER,orderS);
			}
#endif
			SM.globalState->updateExposure(ASK_ORDER,orderS->lmtPrice*orderS->totalQuantity);
		}
		else if(orderS->totalQuantity < 0)
		{
			sizeDelta = reduceSize(ASK_ORDER,orderS);
			SM.globalState->updateExposure(ASK_ORDER,-orderS->lmtPrice*sizeDelta);
		}
	}
}
