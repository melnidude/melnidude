#include "memory.h"

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;


void OrderBook::cancelSide(bool bidAsk)
{
	BookElement *element = (bidAsk==BID_ORDER) ? bid : ask;

	while(element)
	{
		if(element->orderStatus!=CANCELLED_UNCONFIRMED)
		{
			IM.client->cancelOrder(element->m_order->orderId);
			element->orderStatus = CANCELLED_UNCONFIRMED;
			IM.ostr << "cancelSide: cancelling order " << element->m_order->orderId << "\n";
		}
		element = element->nextElement;
	}
}

void OrderBook::cancelSide(bool bidAsk, double lmt)
{
	BookElement *element = (bidAsk==BID_ORDER) ? bid : ask;
	bool lmt_cond;

	while(element)
	{
		lmt_cond = ((bidAsk==BID_ORDER)&&(element->m_order->lmtPrice>lmt+TINY))||
				((bidAsk==ASK_ORDER)&&(element->m_order->lmtPrice<lmt-TINY)&&(element->m_order->lmtPrice > TINY));
		if((element->orderStatus!=CANCELLED_UNCONFIRMED)&&(lmt_cond))
		{
			IM.client->cancelOrder(element->m_order->orderId);
			element->orderStatus = CANCELLED_UNCONFIRMED;
			IM.ostr << "cancelSide: cancelling order " << element->m_order->orderId << "\n";
		}
		element = element->nextElement;
	}
}

void OrderBook::cancelSide(bool bidAsk, bool neutrOrder)
{
	BookElement *element = (bidAsk==BID_ORDER) ? bid : ask;

	while(element)
	{
		if((element->orderStatus!=CANCELLED_UNCONFIRMED)&&(element->m_order->neutrOrder==neutrOrder))
		{
			IM.client->cancelOrder(element->m_order->orderId);
			element->orderStatus = CANCELLED_UNCONFIRMED;
			IM.ostr << "cancelSide: cancelling order " << element->m_order->orderId << "\n";
		}
		element = element->nextElement;
	}
}

void OrderBook::cancelSidePrimary(bool bidAsk)
{
	BookElement *element = (bidAsk==BID_ORDER) ? bid : ask;

	while(element)
	{
		if((element->orderStatus!=CANCELLED_UNCONFIRMED)&&(element->m_order->parentId==0))
		{
			IM.client->cancelOrder(element->m_order->orderId);
			element->orderStatus = CANCELLED_UNCONFIRMED;
			IM.ostr << "cancelSide: cancelling order " << element->m_order->orderId << "\n";
		}
		element = element->nextElement;
	}
}


int OrderBook::cancelAllOtherPriceLevels(bool bidAsk, Order *order)
{
	BookElement *element = (bidAsk==BID_ORDER) ? bid : ask;
	double price = order->lmtPrice;
	int size=0;

	while(element)
	{
		if(((fabs(element->m_order->lmtPrice-price) > TINY)||
				(element->m_order->orderType!=ORDER_LMT))&&
				(element->orderStatus!=CANCELLED_UNCONFIRMED))
		{
			IM.client->cancelOrder(element->m_order->orderId);
			element->orderStatus = CANCELLED_UNCONFIRMED;
		}
		else
		{
			size += element->m_order->totalQuantity;
		}
		element = element->nextElement;
	}
	return size;
}

#if 0
int OrderBook::cancelAllNonIdenticalOrdersREL(bool bidAsk, Order *order)
{
	BookElement *element = (bidAsk==BID_ORDER) ? bid : ask;
	int size=0;
	//ShortOrderType orderType = ShortOrder::strToOrderType(order->orderType);
	ShortOrderType orderType = ORDER_REL;

	while(element)
	{
		if(element->orderStatus!=CANCELLED_UNCONFIRMED)
		{
			if(((fabs(element->m_order->lmtPrice-order->lmtPrice) > TINY)||
					(element->m_order->orderType!=orderType)||(fabs(element->m_order->auxPrice-order->auxPrice)>TINY)))
			{
				IM.client->cancelOrder(element->m_order->orderId);
				element->orderStatus = CANCELLED_UNCONFIRMED;
			}
			else
			{
				size += element->m_order->totalQuantity;
			}
		}
		element = element->nextElement;
	}
	return size;
}
#else
int OrderBook::cancelAllNonIdenticalOrdersREL(bool bidAsk, Order *order)
{
	BookElement *element = (bidAsk==BID_ORDER) ? bid : ask;
	int size=0;
	//ShortOrderType orderType = ShortOrder::strToOrderType(order->orderType);
	ShortOrderType orderType = ORDER_REL;

	if(bidAsk==0) //buying
	{
		while(element)
		{
			if(element->orderStatus!=CANCELLED_UNCONFIRMED)
			{
				if(((order->lmtPrice-element->m_order->lmtPrice > TINY)||
						((fabs(order->lmtPrice)<TINY)&&(fabs(element->m_order->lmtPrice)>TINY))||
						(element->m_order->orderType!=orderType)||(fabs(element->m_order->auxPrice-order->auxPrice)>TINY))&&
						(element->orderStatus!=CANCELLED_UNCONFIRMED))
				{
					IM.client->cancelOrder(element->m_order->orderId);
					element->orderStatus = CANCELLED_UNCONFIRMED;
				}
				else
				{
					size += element->m_order->totalQuantity;
				}
			}
			element = element->nextElement;
		}
	}
	else //selling
	{
		while(element)
		{
			if(element->orderStatus!=CANCELLED_UNCONFIRMED)
			{
				if(((element->m_order->lmtPrice-order->lmtPrice > TINY)||
						((fabs(order->lmtPrice)<TINY)&&(fabs(element->m_order->lmtPrice)>TINY))||
						(element->m_order->orderType!=orderType)||(fabs(element->m_order->auxPrice-order->auxPrice)>TINY))&&
						(element->orderStatus!=CANCELLED_UNCONFIRMED))
				{
					IM.client->cancelOrder(element->m_order->orderId);
					element->orderStatus = CANCELLED_UNCONFIRMED;
				}
				else
				{
					size += element->m_order->totalQuantity;
				}
			}
			element = element->nextElement;
		}
	}

	return size;
}
#endif

int OrderBook::cancelAllNonIdenticalOrdersTRAILLIMIT(bool bidAsk, Order *order)
{
	BookElement *element = (bidAsk==BID_ORDER) ? bid : ask;
	int size=0;
	//ShortOrderType orderType = ShortOrder::strToOrderType(order->orderType);
	ShortOrderType orderType = ORDER_TRAILLIMIT;

	while(element)
	{
		//if(((element->m_order->orderType!=orderType)||
		//		(fabs(element->m_order->auxPrice-order->auxPrice)>TINY)||
		//		(fabs(element->m_order->trailStopPrice-order->trailStopPrice)>TINY))&&
		//		(element->orderStatus!=CANCELLED_UNCONFIRMED))
		//if((element->m_order->orderType!=orderType)&&
		//			(element->orderStatus!=CANCELLED_UNCONFIRMED))
		if(0)
		{
			IM.client->cancelOrder(element->m_order->orderId);
			element->orderStatus = CANCELLED_UNCONFIRMED;
		}
		else
		{
			size += element->m_order->totalQuantity;
		}
		element = element->nextElement;
	}
	return size;
}

int OrderBook::cancelAllNonIdenticalOrdersTRAILLIT(bool bidAsk, Order *order)
{
	BookElement *element = (bidAsk==BID_ORDER) ? bid : ask;
	int size=0;
	//ShortOrderType orderType = ShortOrder::strToOrderType(order->orderType);
	ShortOrderType orderType = ORDER_TRAILLIT;

	while(element)
	{
		//if(((element->m_order->orderType!=orderType)||
		//		(fabs(element->m_order->auxPrice-order->auxPrice)>TINY)||
		//		(fabs(element->m_order->trailStopPrice-order->trailStopPrice)>TINY))&&
		//		(element->orderStatus!=CANCELLED_UNCONFIRMED))
		//if((element->m_order->orderType!=orderType)&&
		//		(element->orderStatus!=CANCELLED_UNCONFIRMED))
		if(0)
		{
			IM.client->cancelOrder(element->m_order->orderId);
			element->orderStatus = CANCELLED_UNCONFIRMED;
		}
		else
		{
			size += element->m_order->totalQuantity;
		}
		element = element->nextElement;
	}
	return size;
}
