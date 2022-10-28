#include <math.h>
#include "memory.h"

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;


void CVWAP::add(int size, double price)
{
	VWAP_Entry *newElement = new VWAP_Entry(size, price);
	VWAP_Entry *element = head;

	//insert at the end of the linked list
	while(element)
	{
		if(element->nextElement)
			element = element->nextElement;
	}
	if(element)
		element->nextElement = newElement;
	else
		head = newElement;

	m_length++;

	calcVWAP();
}

void CVWAP::remove(int size, double price)
{
	VWAP_Entry *element = head;
	VWAP_Entry *prevElement = head;
	while((element) && (size>0))
	{
		if(element->m_size > size)
		{
			element->m_size -= size;
			size = 0;
			prevElement = element;
			element = element->nextElement;
		}
		else
		{
			size -= element->m_size;
			prevElement->nextElement = element->nextElement;
			delete element;
			m_length--;
			element = prevElement->nextElement;
		}
	}

	calcVWAP();
}

void CVWAP::newTrade(int size, double price)
{
	if(m_size*(m_size+size)<=0)
	{
		m_size += size;
		m_proceeds = m_size*price;
	}
	else if(abs(m_size+size)<abs(m_size))
	{
		m_size += size;
		m_proceeds = m_VWAP*m_size;
	}
	else
	{
		m_size += size;
		m_proceeds += size*price;
	}

	m_VWAP = 0;
	if(m_size!=0)
		m_VWAP = m_proceeds/m_size;
}

double CVWAP::calcVWAP()
{
	VWAP_Entry *element = head;
	double sum_prod = 0.0;
	long sum_pos = 0;
	double vwap;

	while(element)
	{
		sum_prod += element->m_size*fabs(element->m_price);
		sum_pos += element->m_size;
		element = element->nextElement;
	}

	if(sum_pos!=0)
		vwap = sum_prod/sum_pos;
	else
	{
		vwap = 0;
	}

	return vwap;
}

double CVWAP::getVWAP()
{
	return ((double)((int)(100*m_VWAP+0.5)))/100.0;
	//return m_VWAP;
}

