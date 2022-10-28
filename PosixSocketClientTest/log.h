#ifndef _LOG_H_
#define _LOG_H_

#include <ios>
#include <iostream>
#include <fstream>
#include <sstream>

#include "PosixTestClient.h"

using namespace std;

struct Quote
{
	double bid;
	double ask;
};

class CLog
{
public:
	CLog(bool ClientServer, IBString log_fname, IBString tick_data_file, IBString pandl_data_file, IBString count_data_file,
			IBString ugorth_data_file, IBString devsize_data_file, IBString lastorth_data_file, IBString lastpp_data_file,
			IBString slippage_data_file, IBString slippage2_data_file, IBString quotes_fname, int numDimensions, bool enableTAQ)
	{
		m_prefix = "Logs/";
		m_log_fname = m_prefix + log_fname;
		m_tick_data_file = m_prefix + tick_data_file;
		m_pandl_file = m_prefix + pandl_data_file;
		m_count_file = m_prefix + count_data_file;
		m_ugorth_file = m_prefix + ugorth_data_file;
		m_devsize_file = m_prefix + devsize_data_file;
		m_lastorth_file = m_prefix + lastorth_data_file;
		m_lastpp_file = m_prefix + lastpp_data_file;
		m_slippage_file = m_prefix + slippage_data_file;
		m_slippage2_file = m_prefix + slippage2_data_file;
		m_pca_file = m_prefix + "pca_log.txt";
		m_quotes_file = m_prefix + quotes_fname;
		m_quotesTxt_file = m_prefix + quotes_fname; m_quotesTxt_file = m_quotesTxt_file + ".csv";

		enableMiscLogs = false;

		m_LogSockfd = -1;
		m_PLSockfd = -1;

		if(ClientServer)
			setupSocket();

		//erase previous contents from the log file
		ofstream myfile;
		myfile.open (m_log_fname.c_str(), ios::out | ios::trunc);
		if (myfile.is_open())
		{
			myfile.close();
		}

		//erase previous contents from the tick data file
		if(enableMiscLogs)
		{
			myfile.open (m_tick_data_file.c_str(), ios::out | ios::trunc);
			if (myfile.is_open())
			{
				myfile.close();
			}
		}

		//erase previous contents from the pandl data file
		myfile.open (m_pandl_file.c_str(), ios::out | ios::trunc);
		if (myfile.is_open())
		{
			myfile.close();
		}

		//erase previous contents from the ugorth data file
		if(enableMiscLogs)
		{
			myfile.open (m_ugorth_file.c_str(), ios::out | ios::trunc);
			if (myfile.is_open())
			{
				myfile.close();
			}
		}

		//erase previous contents from the devsize data file
		if(enableMiscLogs)
		{
			myfile.open (m_devsize_file.c_str(), ios::out | ios::trunc);
			if (myfile.is_open())
			{
				myfile.close();
			}
		}

		//erase previous contents from the lastorth data file
		if(enableMiscLogs)
		{
			myfile.open (m_lastorth_file.c_str(), ios::out | ios::trunc);
			if (myfile.is_open())
			{
				myfile.close();
			}
		}

		//erase previous contents from the slippage data file
		if(enableMiscLogs)
		{
			myfile.open (m_slippage_file.c_str(), ios::out | ios::trunc);
			if (myfile.is_open())
			{
				myfile.close();
			}
		}

		//erase previous contents from the slippage2 data file
		if(enableMiscLogs)
		{
			myfile.open (m_slippage2_file.c_str(), ios::out | ios::trunc);
			if (myfile.is_open())
			{
				myfile.close();
			}
		}

		//erase previous contents from the quotes file
		/*if(enableTAQ)
		{
			myfile.open (m_quotes_file.c_str(), ios::out | ios::trunc | ios::binary);
			if (myfile.is_open())
			{
				myfile.close();
			}
		}*/

		//erase previous contents from the pca_log file
		if(enableMiscLogs)
		{
			myfile.open (m_pca_file.c_str(), ios::out | ios::trunc);
			if (myfile.is_open())
			{
				myfile.close();
			}
		}

		m_quote = new Quote[numDimensions];
		m_dimension_file = new IBString[numDimensions];

		//accounting
		if(enableMiscLogs)
		{
			m_accting_file = m_prefix + "accting.txt";
			myfile.open (m_accting_file.c_str(), ios::out | ios::trunc);
			if(myfile.is_open())
			{
				std::stringstream ostr;
				ostr << "Time,Symbol,NewPos,Filled,FilledQty,Comm,IB_BP,BP,SP,E,S,D,L,C,UG,RG,LMR,SMR\n";
				myfile << ostr.str();
				myfile.close();
			}
		}
	}
	~CLog()
	{
		delete m_quote;
		delete m_dimension_file;
	};

	void setSymbol(int idx, IBString symbol);
	void writeStr(IBString line);
	void writeTickData(IBString line);
	void writePandLData(IBString line);
	void writeCountData(IBString line);
	void writeUGorthData(IBString line);
	void writeDevSizeData(IBString line);
	void writeLastOrthData(IBString line);
	void writeLastPPData(IBString line);
	void writeSlippageData(IBString line);
	void writeSlippage2Data(IBString line);
	void writeQuote(IBString timeOfDay, int assetId, TickType field, double price);
	void writeFill(IBString timeOfDay, int assetId, OrderId orderId, bool action, int qty, double price);
	void writePlaced(IBString timeOfDay, int assetId, OrderId orderId, bool action, int qty, double price, IBString stateDesc);
	void writeCancel(IBString timeOfDay, int assetId, OrderId orderId, int qty, double price, bool action);
	void writeDesired(IBString timeOfDay, int assetId, int desB, int desA, int sizeAtPriceB, int sizeAtPriceS);
	void write_pca_log(IBString expr);
	void writeAccting(IBString timeOfDay, int assetId, bool action, int qty, double price, double commission);

	void writeMasterHeader();
	void writeMasterPositions(IBString timeOfDay, double mpvon);
	void writeMasterGetIntoInfo(IBString timeOfDay, double norm_lastPP, double sig_lastPP, double mpvon);
	void writeMasterDimension(IBString timeOfDay, int assetId, int des, double lmt, long existSize);

	void writeTAQ(unsigned int timestamp, int symbolIndex, int type, int32_t priceMantissa, int32_t priceExponent, unsigned int size, std::string quoteSource);
	void writeTAQTxt(unsigned int timestamp, int symbolIndex, int type, int32_t priceMantissa, int32_t priceExponent, unsigned int size, std::string quoteSource);

	void flushLogs();
	void writeToFile(IBString fname, IBString line, bool append);

	void setupSocket();

	bool enableMiscLogs;

private:
	IBString m_prefix;
	IBString m_log_fname;
	IBString m_tick_data_file;
	IBString m_pandl_file;
	IBString m_count_file;
	IBString m_ugorth_file;
	IBString m_devsize_file;
	IBString m_lastorth_file;
	IBString m_lastpp_file;
	IBString m_slippage_file;
	IBString m_slippage2_file;
	IBString m_quotes_file;
	IBString m_quotesTxt_file;

	IBString *m_dimension_file;
	IBString m_pca_file;
	IBString m_accting_file;
	IBString m_master_file;

	Quote *m_quote;

	ofstream taqfile;
	ofstream taqTxtfile;

	int m_LogSockfd;
	int m_PLSockfd;
};


#endif //_LOG_H_




