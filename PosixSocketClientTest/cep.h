#ifndef __CEP_H_
#define __CEP_H_


class CCEP
{
public:
	CCEP();
	~CCEP();

	void processMsg(int idx, int tradeType, double price, int size);

private:
};

#endif //__CEP_H_
