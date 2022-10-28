#ifndef _ACCOUNTING_H_
#define _ACCOUNTING_H_

class CAccounting
{
public:
	CAccounting(double IMR, double cash);
	~CAccounting(){};

	void initCash(double C, double L, double S, double D, double LnonM);
	void updateAccounting(double trade, int assetId, double commission);

	void markToMarket();
	double getCash() { return m_Cash; };
	double getRealizedGain() { return (m_Cash - m_CashInit); };
	double getBuyingPower();
	double getIBBuyingPower() { return m_IB_BP; };
	double getShortingPower();
	double getLongMarginRatio();
	double getShortMarginRatio();
	IBString getESDLC();
	void setIB_BP(double bp);

private:
	double m_Cash;
	double m_CashInit;
	double m_LongMarg;
	double m_LongNonMarg;
	double m_Short;
	double m_Debit;
	//double m_Equity;
	double m_BuyingPower;
	double m_ShortingPower;

	double m_IMR;

	double m_IB_BP;
	double m_IB_BP_offset;
	bool m_initDone;
};


#endif //_ACCOUNTING_H_
