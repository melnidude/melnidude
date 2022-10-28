#ifndef _VWAP_H_
#define _VWAP_H_


struct VWAP_Entry
{
	int m_size;
	double m_price;
	VWAP_Entry *nextElement;

	VWAP_Entry(int size, double price)
	{
		m_size = size;
		m_price = price;
	}
};

class CVWAP
{
public:
	CVWAP() {
		head = NULL;
		m_length = 0;
		m_VWAP = 0.0;

		m_proceeds = 0.0;
		m_size = 0;
	};
	~CVWAP() {};

	double getVWAP();
	void setAssetId(int assetId) { m_assetId = assetId; }
	void add(int size, double price);
	void remove(int size, double price);
	void newTrade(int size, double price);

private:
	double calcVWAP();

	int m_assetId;
	VWAP_Entry *head;
	int m_length;
	double m_VWAP;

	double m_proceeds;
	long m_size;
};


#endif //_VWAP_H_
