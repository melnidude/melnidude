#ifndef order_def
#define order_def

#include "shared_ptr.h"
#include "IBString.h"

#include <float.h>
#include <vector>

#define UNSET_DOUBLE DBL_MAX
#define UNSET_INTEGER INT_MAX

enum Origin { CUSTOMER,
              FIRM,
              UNKNOWN };

enum AuctionStrategy { AUCTION_UNSET = 0,
                       AUCTION_MATCH = 1,
                       AUCTION_IMPROVEMENT = 2,
                       AUCTION_TRANSPARENT = 3 };

enum ShortOrderType {
		ORDER_LMT = 0,
		ORDER_MKT,
		ORDER_STP,
		ORDER_REL,
		ORDER_TRAILLIMIT,
		ORDER_TRAILLIT
};

struct TagValue
{
	TagValue() {}
	TagValue(const IBString& p_tag, const IBString& p_value)
		: tag(p_tag), value(p_value)
	{}

	IBString tag;
	IBString value;
};

typedef shared_ptr<TagValue> TagValueSPtr;



struct Order
{
	Order()
	{
		// order identifier
		orderId  = 0;
		clientId = 0;
		permId   = 0;

		// main order fields
		totalQuantity = 0;
		lmtPrice      = 0;
		auxPrice      = 0;
		orderType     = "LMT";

		// extended order fields
		ocaType        = 0;
		transmit       = true;
		parentId       = 0;
		blockOrder     = false;
		sweepToFill    = false;
		displaySize    = 0;
		triggerMethod  = 0;
		outsideRth     = false;
		hidden         = false;
		allOrNone      = false;
		minQty         = UNSET_INTEGER;
		percentOffset  = UNSET_DOUBLE;
		overridePercentageConstraints = false;
		trailStopPrice = UNSET_DOUBLE;

		// institutional (ie non-cleared) only
		openClose     = "O";
		origin        = CUSTOMER;
		shortSaleSlot = 0;

		// SMART routing only
		discretionaryAmt = 0;
		eTradeOnly       = true;
		firmQuoteOnly    = true;
		nbboPriceCap     = UNSET_DOUBLE;

		// BOX exchange orders only
		auctionStrategy = AUCTION_UNSET;
		startingPrice   = UNSET_DOUBLE;
		stockRefPrice   = UNSET_DOUBLE;
		delta           = UNSET_DOUBLE;

		// pegged to stock and VOL orders only
		stockRangeLower = UNSET_DOUBLE;
		stockRangeUpper = UNSET_DOUBLE;

		// VOLATILITY ORDERS ONLY
		volatility            = UNSET_DOUBLE;
		volatilityType        = UNSET_INTEGER;     // 1=daily, 2=annual
		deltaNeutralOrderType = "";
		deltaNeutralAuxPrice  = UNSET_DOUBLE;
		continuousUpdate      = false;
		referencePriceType    = UNSET_INTEGER; // 1=Average, 2 = BidOrAsk

		// COMBO ORDERS ONLY
		basisPoints     = UNSET_DOUBLE;  // EFP orders only
		basisPointsType = UNSET_INTEGER; // EFP orders only

		// SCALE ORDERS ONLY
		scaleInitLevelSize  = UNSET_INTEGER;
		scaleSubsLevelSize  = UNSET_INTEGER;
		scalePriceIncrement = UNSET_DOUBLE;

		// What-if
		whatIf = false;

		// Not Held
		notHeld = false;
	}

	// order identifier
	long     orderId;
	long     clientId;
	long     permId;

	// main order fields
	IBString action;
	long     totalQuantity;
	IBString orderType;
	double   lmtPrice;
	double   auxPrice;

	// extended order fields
	IBString tif;           // "Time in Force" - DAY, GTC, etc.
	IBString ocaGroup;      // one cancels all group name
	int      ocaType;       // 1 = CANCEL_WITH_BLOCK, 2 = REDUCE_WITH_BLOCK, 3 = REDUCE_NON_BLOCK
	IBString orderRef;      // order reference
	bool     transmit;      // if false, order will be created but not transmited
	long     parentId;      // Parent order Id, to associate Auto STP or TRAIL orders with the original order.
	bool     blockOrder;
	bool     sweepToFill;
	int      displaySize;
	int      triggerMethod; // 0=Default, 1=Double_Bid_Ask, 2=Last, 3=Double_Last, 4=Bid_Ask, 7=Last_or_Bid_Ask, 8=Mid-point
	bool     outsideRth;
	bool     hidden;
	IBString goodAfterTime;    // Format: 20060505 08:00:00 {time zone}
	IBString goodTillDate;     // Format: 20060505 08:00:00 {time zone}
	IBString rule80A; // Individual = 'I', Agency = 'A', AgentOtherMember = 'W', IndividualPTIA = 'J', AgencyPTIA = 'U', AgentOtherMemberPTIA = 'M', IndividualPT = 'K', AgencyPT = 'Y', AgentOtherMemberPT = 'N'
	bool     allOrNone;
	int      minQty;
	double   percentOffset; // REL orders only
	bool     overridePercentageConstraints;
	double   trailStopPrice; // TRAILLIMIT orders only

	// financial advisors only
	IBString faGroup;
	IBString faProfile;
	IBString faMethod;
	IBString faPercentage;

	// institutional (ie non-cleared) only
	IBString openClose; // O=Open, C=Close
	Origin   origin;    // 0=Customer, 1=Firm
	int      shortSaleSlot; // 1 if you hold the shares, 2 if they will be delivered from elsewhere.  Only for Action="SSHORT
	IBString designatedLocation; // set when slot=2 only.

	// SMART routing only
	double   discretionaryAmt;
	bool     eTradeOnly;
	bool     firmQuoteOnly;
	double   nbboPriceCap;

	// BOX exchange orders only
	int      auctionStrategy; // AUCTION_MATCH, AUCTION_IMPROVEMENT, AUCTION_TRANSPARENT
	double   startingPrice;
	double   stockRefPrice;
	double   delta;

	// pegged to stock and VOL orders only
	double   stockRangeLower;
	double   stockRangeUpper;

	// VOLATILITY ORDERS ONLY
	double   volatility;
	int      volatilityType;     // 1=daily, 2=annual
	IBString deltaNeutralOrderType;
	double   deltaNeutralAuxPrice;
	bool     continuousUpdate;
	int      referencePriceType; // 1=Average, 2 = BidOrAsk

	// COMBO ORDERS ONLY
	double   basisPoints;      // EFP orders only
	int      basisPointsType;  // EFP orders only

	// SCALE ORDERS ONLY
	int      scaleInitLevelSize;
	int      scaleSubsLevelSize;
	double   scalePriceIncrement;

	// Clearing info
	IBString account; // IB account
	IBString settlingFirm;
	IBString clearingAccount; // True beneficiary of the order
	IBString clearingIntent; // "" (Default), "IB", "Away", "PTA" (PostTrade)

	// ALGO ORDERS ONLY
	IBString algoStrategy;

	typedef std::vector<TagValueSPtr> TagValueList;
	typedef shared_ptr<TagValueList> TagValueListSPtr;

	TagValueListSPtr algoParams;

	// What-if
	bool     whatIf;

	// Not Held
	bool     notHeld;

	// state
	bool neutralizable;
	bool neutrOrder;
	IBString stateDesc;

	double nbboWhenPlaced;
};

class ShortOrder
{
public:
	ShortOrder()
	{
		// order identifier
		orderId  = 0;

		// main order fields
		totalQuantity = 0;
		lmtPrice      = 0.0;
		auxPrice       = 0.0;
		trailStopPrice = 0.0;
		cumulativeFill = 0;
		parentId = 0;

		clock_gettime( CLOCK_REALTIME, &timePlaced);
	}

	ShortOrder(Order *order)
	{
		// order identifier
		orderId  = order->orderId;

		// main order fields
		totalQuantity = order->totalQuantity;
		lmtPrice      = order->lmtPrice;
		auxPrice      = order->auxPrice;
		cumulativeFill = 0;

		action = (order->action == "SELL");
		orderType = strToOrderType(order->orderType);
		parentId = order->parentId;

		nbboWhenPlaced = order->nbboWhenPlaced;

		neutralizable = order->neutralizable;
		stateDesc = order->stateDesc;
		neutrOrder = order->neutrOrder;

		clock_gettime( CLOCK_REALTIME, &timePlaced);
	}

	static ShortOrderType strToOrderType(IBString strOrderType)
	{
		ShortOrderType type;

		if(strOrderType=="LMT") type = ORDER_LMT;
		else if(strOrderType=="MKT") type = ORDER_MKT;
		else if(strOrderType=="STP") type = ORDER_STP;
		else if(strOrderType=="REL") type = ORDER_REL;
		else if(strOrderType=="TRAILLIMIT") type = ORDER_TRAILLIMIT;
		else if(strOrderType=="TRAILLIT") type = ORDER_TRAILLIT;
		else
		{
			printf("strToOrderType: bad order type: %s\n",strOrderType.c_str());
			exit(0);
		}
		//orderType = type;
		return type;
	}

	// order identifier
	long     orderId;

	// main order fields
	bool     action;
	ShortOrderType orderType;
	long     totalQuantity;
	long     cumulativeFill;
	double   lmtPrice;
	double   auxPrice;
	double   trailStopPrice;
	long     parentId;

	struct timespec timePlaced;
	double nbboWhenPlaced;

	// state
	bool neutralizable;
	bool neutrOrder;
	IBString stateDesc;
};
#endif
