#ifndef contract_def
#define contract_def

#include <vector>
#include "IBString.h"

/*
    SAME_POS    = open/close leg value is same as combo
    OPEN_POS    = open
    CLOSE_POS   = close
    UNKNOWN_POS = unknown

*/
enum LegOpenClose { SAME_POS, OPEN_POS, CLOSE_POS, UNKNOWN_POS };

struct ComboLeg
{

   ComboLeg()
      :
      conId(0),
	  ratio(1),
	  openClose(0),
	  shortSaleSlot(0),
	  exchange("SMART"),
	  designatedLocation("")
   {
   }

   long    conId;
   long    ratio;
   IBString action; //BUY/SELL/SSHORT

   IBString exchange;
   long    openClose; // LegOpenClose enum values

   // for stock legs when doing short sale
   long    shortSaleSlot; // 1 = clearing broker, 2 = third party
   IBString designatedLocation;

   bool operator==( const ComboLeg &other) const
   {
      return (conId == other.conId &&
         ratio == other.ratio &&
		 openClose == other.openClose &&
		 shortSaleSlot == other.shortSaleSlot &&
		 (Compare(action, other.action) == 0) &&
		 (Compare(exchange, other.exchange) == 0) &&
		 (Compare(designatedLocation, other.designatedLocation) == 0));
   }
};

struct UnderComp
{
	UnderComp()
		: conId(0)
		, delta(0)
		, price(0)
	{}

	long	conId;
	double	delta;
	double	price;
};

class Contract
{
public:
   Contract()
/*   : conId(0)
	  , secType("STK")
	  , expiry("")
	  , strike(0)
	  , right("")
	  , multiplier("")
	  , exchange("SMART")
	  , primaryExchange("ISLAND")
	  , currency("USD")
	  , localSymbol("")
	  , includeExpired(false)
	  , comboLegs(NULL)
	  , underComp(NULL)*/
   {
	   IBString t;
	   conId= 0;
	   t="STK"; secType = t.c_str();
	   t=""; expiry = t.c_str();
	   strike = 0;
	   t=""; right = t.c_str();
	   t=""; multiplier = t.c_str();
	   t="SMART"; exchange = t.c_str();
	   t="ISLAND"; primaryExchange = t.c_str();
	   //t="ISLAND"; exchange = t.c_str();
	   //t="ISLAND"; primaryExchange = t.c_str();
	   t="USD"; currency = t.c_str();
	   t=""; localSymbol = t.c_str();
	   includeExpired = false;
	   comboLegs = NULL;
	   underComp = NULL;
   }
   long    conId;
   IBString symbol;
   IBString secType;
   IBString expiry;
   double  strike;
   IBString right;
   IBString multiplier;
   IBString exchange;
   IBString primaryExchange; // pick an actual (ie non-aggregate) exchange that the contract trades on.  DO NOT SET TO SMART.
   IBString currency;
   IBString localSymbol;
   bool includeExpired;

   // COMBOS
   IBString comboLegsDescrip; // received in open order 14 and up for all combos

   typedef std::vector<ComboLeg*> ComboLegList;
   ComboLegList* comboLegs;

   // delta neutral
   UnderComp* underComp;

public:

    void setExchange(IBString exch) { exchange = exch.c_str(); };

	// Helpers
	static void CloneComboLegs(ComboLegList& dst, const ComboLegList& src);
    static void CleanupComboLegs(ComboLegList&);
};

struct ContractDetails
{
   ContractDetails()
      : minTick(0)
	  , priceMagnifier(0)
	  , underConId(0)
	  , callable(false)
	  , putable(false)
	  , coupon(0)
	  , convertible(false)
	  , nextOptionPartial(false)
		
   {
   }

   Contract	summary;
   IBString	marketName;
   IBString	tradingClass;
   double	minTick;
   IBString	orderTypes;
   IBString	validExchanges;
   long		priceMagnifier;
   int		underConId;
   IBString	longName;

   // BOND values
   IBString cusip;
   IBString ratings;
   IBString descAppend;
   IBString bondType;
   IBString couponType;
   bool callable;
   bool putable;
   double coupon;
   bool convertible;
   IBString maturity;
   IBString issueDate;
   IBString nextOptionDate;
   IBString nextOptionType;
   bool nextOptionPartial;
   IBString notes;
};

inline void
Contract::CloneComboLegs(ComboLegList& dst, const ComboLegList& src)
{
	CleanupComboLegs(dst);

	if (src.empty())
		return;

	dst.reserve(src.size());

	ComboLegList::const_iterator iter = src.begin();
	const ComboLegList::const_iterator iterEnd = src.end();

	for (; iter != iterEnd; ++iter) {
		const ComboLeg* leg = *iter;
		if (!leg)
			continue;
		dst.push_back(new ComboLeg(*leg));
	}
}

inline void
Contract::CleanupComboLegs(ComboLegList& legs)
{
	if (legs.empty())
		return;

	ComboLegList::iterator iter = legs.begin();
	const ComboLegList::iterator iterEnd = legs.end();

	for (; iter != iterEnd; ++iter) {
		delete *iter;
	}

	legs.clear();
}

#endif
