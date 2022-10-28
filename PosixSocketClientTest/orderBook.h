#ifndef _ORDERBOOK_H_
#define _ORDERBOOK_H_

#include <math.h>

enum OrderStatus {
	SUBMITTED_UNCONFIRMED,
	SUBMITTED_CONFIRMED,
	CANCELLED_UNCONFIRMED
};

class BookElement
{
public:
	//Order *m_order;
	ShortOrder *m_order;
	OrderStatus orderStatus;
	BookElement *nextElement;
	BookElement *prevElement;

	BookElement(Order *order)
	{
		m_order = new ShortOrder(order);

		orderStatus = SUBMITTED_UNCONFIRMED;

		nextElement = NULL;
		prevElement = NULL;
	}
	~BookElement()
	{
		delete m_order;
	};

};

class OrderBook
{
public:
	OrderBook() {
		bid = NULL;
		ask = NULL;
		depth[0] = depth[1] = 0;
		m_order_cnt = 0;
	};
	~OrderBook() {};

	void setAssetId(int assetId) { m_assetId = assetId; }
	void setNeutralizer(OrderBook *neutralizer) { m_neutralizer = neutralizer; }
	void genDeltaOrders(Order *orderB, Order *orderS, BracketHierarchy hierarchy);
	void add(bool bidAsk, Order *order);
	long reduceSize(bool bidAsk, Order *order);
	bool remove(long orderId);
	BookElement* getBookElement(OrderId orderId, bool *found);
	long getSizeofOrderType(bool bidAsk, ShortOrderType orderType);
	//long getSizeAtPrice(bool bidAsk, double price);
	//long getSizeAtNotLessLikely(bool bidAsk, double price);
	double getBestLimit(bool side);
	ShortOrder* getBestOrder(bool side);

	void cancelSide(bool bidAsk);
	void cancelSide(bool bidAsk, bool neutrOrder);
	void cancelSide(bool bidAsk, double lmt);
	void cancelSidePrimary(bool bidAsk);
	int  cancelAllOtherPriceLevels(bool bidAsk, Order *order);
	int  cancelAllNonIdenticalOrdersREL(bool bidAsk, Order *order);
	int  cancelAllNonIdenticalOrdersTRAILLIMIT(bool bidAsk, Order *order);
	int  cancelAllNonIdenticalOrdersTRAILLIT(bool bidAsk, Order *order);
	void processOrderStatus(OrderId orderId, const IBString &status, long filledCumulative, double avgFillPrice);
	long countActiveOrders(BookElement *head);
	long countOrders(BookElement *head);

	BookElement *bid;
	BookElement *ask;

	int depth[2];

private:

	int m_assetId;

	OrderBook *m_neutralizer;
	unsigned long m_order_cnt;
};


#endif //_ORDERBOOK_H_
