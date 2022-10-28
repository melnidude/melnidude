#include <math.h>
#include "memory.h"

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;


void OrderBook::add(bool bidAsk, Order *order)
{
	BookElement *element, *newElement;
	long orderId;

	if((order->lmtPrice < TINY)&&((order->orderType == "LMT")||(order->orderType == "MKT")))
	{
		IM.ostr << "add(" << SM.marketData[m_assetId].contract->symbol << "): Order not placed: price=" << order->lmtPrice << "\n";
		return;
	}

	if(depth[bidAsk] >= IM.MaxSideOrders)
	{
		IM.ostr << "add(" << SM.marketData[m_assetId].contract->symbol << ") not placed : depth=" << depth[bidAsk] << "\n";
		element = (bidAsk==BID_ORDER) ? bid : ask;
		while(element)
		{
			IM.ostr << "\torder=" << element->m_order->orderId << ", type=" << element->m_order->action << ", qty=" << element->m_order->totalQuantity << ", status=" << element->orderStatus << "\n";
			element = element->nextElement;
		}
		return;
	}

	orderId = SM.ids->getNextId(m_assetId);
	order->orderId = orderId;

	//order->totalQuantity = MIN(order->totalQuantity, (long)(fabs(SM.weights[m_assetId])*IM.maxOrderSize + 0.5));
	if(order->totalQuantity < (IM.minOrderSize>>1)) { order->totalQuantity = 0; return; }
	else if(order->totalQuantity < IM.minOrderSize) { order->totalQuantity = IM.minOrderSize; }

	////////////////////////////////
	IM.client->placeOrder(m_assetId, *order, orderId);
	m_order_cnt++;

	////////////////////////////////

	element = (bidAsk==BID_ORDER) ? bid : ask;
	newElement = new BookElement(order);
	//IM.ostr << "add(" << SM.marketData[m_assetId].contract->symbol << "): element=" << element << ", newElement=" << newElement << ", bidAsk=" << bidAsk << "\n";

	if(element==NULL)
	{
		if(bidAsk==BID_ORDER) bid = newElement;
		else ask = newElement;
	}
	else
	{
		while(element->nextElement)
		{
			element = element->nextElement;
		}
		element->nextElement = newElement;
		newElement->prevElement = element;
	}

	IM.Logger->writePlaced(SM.globalState->getTime(),
			m_assetId, orderId, bidAsk, order->totalQuantity,
			order->lmtPrice, order->stateDesc);

	depth[bidAsk]++;
	IM.ostr << "add(" << SM.marketData[m_assetId].contract->symbol << "): depth=" << depth[bidAsk] << "\n";

#if ALGO2
	if(order->parentId==0)
	{
		Order *order_child;
		long orderId_child = SM.ids->getNextId(m_assetId);
		order_child = (bidAsk) ? SM.marketData[m_assetId].orderB : SM.marketData[m_assetId].orderS;
		order_child->totalQuantity = order->totalQuantity;
		order_child->orderType = "REL";
		order_child->auxPrice = 0.00;
		order_child->lmtPrice = order->lmtPrice + 0.01*(1-2*bidAsk);
		order_child->transmit = true;
		order_child->parentId = orderId;
		order_child->orderId = orderId_child;

		add(!bidAsk, order_child);
	}
#endif

}

long OrderBook::reduceSize(bool bidAsk, Order *order)
{
	BookElement *element = (bidAsk==BID_ORDER) ? bid : ask;
	long totalQuantity = abs(order->totalQuantity); //how much to reduce by
	long delta = totalQuantity;

	//starting from the most recent order
	while(element->nextElement) { element = element->nextElement; }

	while((element)&&(totalQuantity > 0))
	{
		//if(element->m_order->orderType==ShortOrder::strToOrderType(order->orderType)&&(element->orderStatus != CANCELLED_UNCONFIRMED))
		if(/*(element->m_order->neutrOrder==order->neutrOrder)&&*/(element->orderStatus != CANCELLED_UNCONFIRMED))
		{
			order->clientId = SM.clientId;
			if(element->m_order->totalQuantity <= totalQuantity)
			{	//cancel
				IM.client->cancelOrder(element->m_order->orderId);
				element->orderStatus = CANCELLED_UNCONFIRMED;
				totalQuantity -= element->m_order->totalQuantity;
			}
			else
			{	//modify
				if(totalQuantity < IM.minOrderSize)
				{	//delta is too small to lose position in the queue
					return delta;
				}

#if 1 //cancel and place new
				IM.client->cancelOrder(element->m_order->orderId);
				element->orderStatus = CANCELLED_UNCONFIRMED;
				order->totalQuantity = element->m_order->totalQuantity - totalQuantity;
				if(order->totalQuantity < IM.minOrderSize)
				{
					return delta;
				}
#if NEUT_STOP
				if(m_assetId==0)
				{
					//order->transmit = false;
					order->ocaGroup = "Neut" + m_order_cnt;
					order->ocaType = 3;
				}
#endif
				add(bidAsk, order);
#if NEUT_STOP
				if(m_assetId==0)
				{
					//order->transmit = true;
					order->ocaGroup = "Neut" + m_order_cnt;
					order->ocaType = 3;
					order->orderType = "STP";
					order->lmtPrice = 0.0;
					order->auxPrice = (bidAsk) ? SM.marketData[m_assetId].market[ASK]-0.02 : SM.marketData[m_assetId].market[BID]+0.02;
					add(bidAsk,order);
				}
#endif

#else //modify
				long orderId = element->m_order->orderId;
				order->orderId = orderId;
				OSTR_RESET
				ostr << "modify(" << SM.marketData[m_assetId].contract->symbol << "): " << orderId << ", old size=" << element->m_order->totalQuantity << ", new size=" << element->m_order->totalQuantity - totalQuantity << "\n";
				OSTR_LOG(LOG_MSG);

				//order->totalQuantity -= totalQuantity;
				order->totalQuantity = element->m_order->totalQuantity - totalQuantity;
				if(order->totalQuantity < (IM.minOrderSize>>1)) { order->totalQuantity = 0; return delta; }
				else if(order->totalQuantity < IM.minOrderSize) { order->totalQuantity = IM.minOrderSize; }

				element->m_order->totalQuantity = order->totalQuantity;
				IM.client->placeOrder(m_assetId, *order, orderId);
#endif
				totalQuantity = 0;
			}
		}
		element = element->prevElement;
	}
	return delta;
}


bool OrderBook::remove(long orderId)
{
	BookElement *element = bid;
	BookElement *prevElement = NULL;
	BookElement *nextElement;
	while(element)
	{
		/*IM.ostr << "(remove-bid) comparing " << orderId << " with " << element->m_order->orderId << "\n";*/

		if(element->m_order->orderId == orderId)
		{
			if(prevElement)
			{
				nextElement = element->nextElement;
				prevElement->nextElement = nextElement;
				if(nextElement) nextElement->prevElement = prevElement;
			}
			else
			{
				bid = element->nextElement;
				if(bid) bid->prevElement = NULL;
			}

			//IM.ostr << "removing buy order " << orderId << "\n";
			delete element;

			depth[BID_ORDER]--;

			return true;
		}
		prevElement = element;
		element = element->nextElement;
	}

	element = ask;
	prevElement = NULL;
	while(element)
	{
		/*IM.ostr << "(remove-ask) comparing " << orderId << " with " << element->m_order->orderId << "\n";*/

		if(element->m_order->orderId == orderId)
		{
			if(prevElement)
			{
				nextElement = element->nextElement;
				prevElement->nextElement = nextElement;
				if(nextElement) nextElement->prevElement = prevElement;
			}
			else
			{
				ask = element->nextElement;
				if(ask) ask->prevElement = NULL;
			}

			//IM.ostr << "removing sell order " << orderId << "\n";
			delete element;

			depth[ASK_ORDER]--;

			return true;
		}
		prevElement = element;
		element = element->nextElement;
	}
	return false;
}

long OrderBook::getSizeofOrderType(bool bidAsk, ShortOrderType orderType)
{
	BookElement *element = (bidAsk==BID_ORDER) ? bid : ask;
	long size = 0;

	while(element)
	{
		IM.ostr << "in getSizeofOrderType: comparing " << element->m_order->orderType << " to " << orderType << "\n";
		if(element->m_order->orderType==orderType)
		{
			size += element->m_order->totalQuantity;
		}
		element = element->nextElement;
	}
	return size;
}


double OrderBook::getBestLimit(bool side)
{
	BookElement *element;
	double limit;
	bool found = false;

	if(side==BID_ORDER)
	{
		limit = 0.0;
		element = bid;
		while(element)
		{
			if(element->orderStatus != CANCELLED_UNCONFIRMED)
			{
				found = true;
				if(element->m_order->lmtPrice < TINY) limit = INFINITY_D;
				else limit = MAX(limit,element->m_order->lmtPrice);
			}
			element = element->nextElement;
		}
		if(!found) limit = INFINITY_D;
	}
	else
	{
		limit = INFINITY_D;
		element = ask;
		while(element)
		{
			if(element->orderStatus != CANCELLED_UNCONFIRMED)
			{
				found = true;
				if(element->m_order->lmtPrice < TINY) limit = 0.0;
				else limit = MIN(limit,element->m_order->lmtPrice);
			}
			element = element->nextElement;
		}
		if(!found) limit = 0.0;
	}

	return limit;
}

ShortOrder* OrderBook::getBestOrder(bool side)
{
	BookElement *element;
	double limit;
	ShortOrder *bestOrder = NULL;

	if(side==BID_ORDER)
	{
		limit = 0.0;
		element = bid;
		//IM.ostr << "getBestOrder: bid=" << bid << "\n";
		while(element)
		{
			//IM.ostr << "\t(" << element->m_order->orderId << ") orderStatus=" << element->orderStatus << "\n";
			if(element->orderStatus != CANCELLED_UNCONFIRMED)
			{
				//IM.ostr << "\tlmtPrice=" << element->m_order->lmtPrice << "\n";
				if(element->m_order->lmtPrice < TINY)
				{
					limit = INFINITY_D;
					bestOrder = element->m_order;
				}
				if(element->m_order->lmtPrice > limit + TINY)
				{
					limit = element->m_order->lmtPrice;
					bestOrder = element->m_order;
				}
			}
			element = element->nextElement;
		}
	}
	else
	{
		limit = INFINITY_D;
		element = ask;
		//IM.ostr << "getBestOrder ask=" << ask << "\n";
		while(element)
		{
			//IM.ostr << "\t(" << element->m_order->orderId << ") orderStatus=" << element->orderStatus << "\n";
			if(element->orderStatus != CANCELLED_UNCONFIRMED)
			{
				//IM.ostr << "\tlmtPrice=" << element->m_order->lmtPrice << "\n";
				if(element->m_order->lmtPrice < TINY)
				{
					limit = 0.0;
					bestOrder = element->m_order;
				}
				if(element->m_order->lmtPrice < limit - TINY)
				{
					limit = element->m_order->lmtPrice;
					bestOrder = element->m_order;
				}
			}
			element = element->nextElement;
		}
	}

	return bestOrder;
}



BookElement* OrderBook::getBookElement(OrderId orderId, bool *found)
{
	BookElement *element = bid;
	*found = false;
	while(element)
	{
		if(element->m_order->orderId == orderId)
		{
			*found = true;
			return element;
		}
		element = element->nextElement;
	}

	element = ask;
	while(element)
	{
		if(element->m_order->orderId == orderId)
		{
			*found = true;
			return element;
		}
		element = element->nextElement;
	}
	return NULL;
}

long OrderBook::countActiveOrders(BookElement *head)
{
	BookElement *element = head;
	long size = 0;

	while(element)
	{
		if(element->orderStatus!=CANCELLED_UNCONFIRMED)
		{
			size += element->m_order->totalQuantity;
		}
		element = element->nextElement;
	}
	IM.ostr << "countActiveOrders: size=" << size << "\n";
	return size;
}

long OrderBook::countOrders(BookElement *head)
{
	BookElement *element;
	long size = 0;

	element = bid;
	while(element)
	{
		size += element->m_order->totalQuantity;
		element = element->nextElement;
	}

	element = ask;
	while(element)
	{
		size += element->m_order->totalQuantity;
		element = element->nextElement;
	}

	return size;
}

