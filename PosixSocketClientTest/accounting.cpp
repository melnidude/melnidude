#include "defs.h"
#include "memory.h"

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;

CAccounting::CAccounting(double IMR, double cash)
{
	m_initDone = false;

	if(fabs(IMR) < TINY)
	{
		IM.ostr << "Accounting: Invalid IMR (" << IMR << ").\n";
		exit(0);
	}
	m_IMR = IMR;
	m_Cash = cash;
	m_LongMarg = 0.0;
	m_LongNonMarg = 0.0;
	m_Short = 0.0;
	m_Debit = 0.0;
	//m_Equity = 0.0;
	m_BuyingPower = 0.0;
	m_ShortingPower = 0.0;
};

void CAccounting::initCash(double C, double L, double S, double D, double LnonM)
{
	m_Cash = C;
	m_CashInit = C;
	m_LongMarg = L;
	m_Short = S;
	m_Debit = D;
	m_LongNonMarg = LnonM;
}

void CAccounting::updateAccounting(double trade, int assetId, double commission)
{
	int position = SM.position[assetId];

	IM.ostr << "Accounting: before: trade=" << trade << ", position=" << position <<
			", C=" << m_Cash << ", L=" << m_LongMarg << ", S=" << m_Short <<
			", D=" << m_Debit << "\n";

	if(trade > 0)
	{
		if(position>=0) //adding to a long position
		{
			if(SM.marginable[assetId]) m_LongMarg += trade;
			else m_LongNonMarg += trade;
		}
		else //reducing a short position
		{
			m_Short -= trade;
			if(m_Short < 0.0)
			{
				if(SM.marginable[assetId]) m_LongMarg -= m_Short;
				else m_LongNonMarg -= m_Short;
				m_Short = 0.0;
			}
		}
		m_Cash -= trade;
		if(m_Cash < 0.0)
		{
			m_Debit -= m_Cash;
			m_Cash = 0.0;
		}
	}
	else if(trade < 0)
	{
		if(position>=0) //reducing a long position
		{
			m_Debit += trade;
			if(m_Debit < 0.0)
			{
				m_Cash -= m_Debit;
				m_Debit = 0.0;
			}
			double LongAmt = (SM.marginable[assetId]) ? m_LongMarg : m_LongNonMarg;
			LongAmt += trade;
			if(LongAmt < 0.0)
			{
				m_Short -= LongAmt;
				if(SM.marginable[assetId]) m_LongMarg = 0.0;
				else m_LongNonMarg = 0.0;
			}
			else
			{
				if(SM.marginable[assetId]) m_LongMarg = LongAmt;
				else m_LongNonMarg = LongAmt;
			}
		}
		else //adding to a short position
		{
			m_Short -= trade;
			m_Cash -= trade;
		}
	}

	m_Cash -= commission;
	if(m_Cash < 0.0)
	{
		m_Debit -= m_Cash;
		m_Cash = 0.0;
	}

	IM.ostr << "Accounting: after: C=" << m_Cash << ", L=" << m_LongMarg << ", S=" << m_Short <<
			", D=" << m_Debit << "\n";
}

void CAccounting::markToMarket()
{
	double L=0.0, Lnm=0.0, S=0.0;
	for(int i=0; i<IM.BasketSize; i++)
	{
		if((SM.position[i] > 0)||(SM.marginable[i])) L += SM.position[i]*SM.marketData[i].market[LAST];
		if((SM.position[i] > 0)||(!SM.marginable[i])) Lnm += SM.position[i]*SM.marketData[i].market[LAST];
		else if(SM.position[i] < 0) S -= SM.position[i]*SM.marketData[i].market[LAST];
	}
	m_LongMarg = L;
	m_LongNonMarg = Lnm;
	m_Short = S;
}

double CAccounting::getBuyingPower()
{
	markToMarket();
#if 0
	double E = m_LongMarg + m_Cash - m_Short - m_Debit;
	m_BuyingPower = m_Cash - m_Debit + E*(1-m_IMR)/m_IMR;
	return m_BuyingPower;
#else
	return m_IB_BP;
#endif
}

double CAccounting::getShortingPower()
{
	markToMarket();
#if 0
	double E = m_LongMarg + m_Cash - m_Short - m_Debit;
	m_ShortingPower = (E/m_IMR) - m_Short;
	return m_ShortingPower;
#else
	return m_IB_BP;
#endif
}

IBString CAccounting::getESDLC()
{
	stringstream ESDLC;
	double E = m_LongMarg + m_Cash - m_Short - m_Debit;
	ESDLC << E << "," << m_Short << "," << m_Debit << "," <<
			m_LongMarg << "," << m_Cash;
	return ESDLC.str();
}

double CAccounting::getLongMarginRatio()
{
	double E = m_LongMarg + m_Cash - m_Short - m_Debit;
	if(fabs(E+m_Debit) > TINY)
		return E/(E+m_Debit);
	else
		return 0.0;
}

double CAccounting::getShortMarginRatio()
{
	double E = m_LongMarg + m_Cash - m_Short - m_Debit;
	if(fabs(m_Short) > TINY)
		return E/m_Short;
	else
		return 0.0;
}

void CAccounting::setIB_BP(double bp)
{
	if(!m_initDone)
	{
		m_IB_BP = bp*IM.PercentCash;
		m_IB_BP_offset = bp - m_IB_BP;
		m_initDone = true;
	}
	else
	{
		m_IB_BP = bp - m_IB_BP_offset;
	}
}



