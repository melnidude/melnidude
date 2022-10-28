#include "log.h"
#include "memory.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "socketUtils.h"

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;

void CLog::setSymbol(int idx, IBString symbol)
{
	if(enableMiscLogs)
	{
		m_dimension_file[idx] = symbol + ".txt";
		m_dimension_file[idx] = m_prefix + m_dimension_file[idx];

		//erase previous contents from the log file
		ofstream myfile;
		myfile.open (m_dimension_file[idx].c_str(), ios::out | ios::trunc);
		if(myfile.is_open())
		{
			OSTR_DEF;
			ostr << "Timestamp,Bid,Ask,Position,VWAP,Last,Filled,FilledQty,Placed,PlacedQty,State,Cancel,CancelQty,OrderId,DesBQty,DesAQty,BkBQty,BkAQty,IB_BP,BP,SP,E,S,D,L,C\n";
			myfile << ostr.str();
			myfile.close();
		}
	}
}

void CLog::writeStr(IBString line)
{
	ofstream myfile;

	myfile.open (m_log_fname.c_str(), ios::out | ios::app);

	if(myfile.is_open())
	{
		myfile << line;
		myfile.close();
	}
}

void CLog::write_pca_log(IBString expr)
{
	ofstream myfile;

	myfile.open(m_pca_file.c_str(), ios::out | ios::app);

	if(myfile.is_open())
	{
		myfile << expr;
		myfile.close();
	}

	IM.Logger->writeStr(expr);
}

void CLog::writeTickData(IBString line)
{
	if(!enableMiscLogs) return;
	ofstream myfile;

	myfile.open (m_tick_data_file.c_str(), ios::out | ios::app);

	if(myfile.is_open())
	{
		myfile << line;
		myfile.close();
	}
}

void CLog::writeTAQ(unsigned int timestamp, int symbolIndex, int type, int32_t priceMantissa, int32_t priceExponent, unsigned int size, std::string quoteSource)
{
	static int taqCnt;

	if(taqCnt==0)
		taqfile.open(m_quotes_file.c_str(), ios::out | ios::binary | ios::trunc);
	if(!taqfile.is_open())
	{
		std::cout <<  "Can not open TAQ file.\n" << std::endl;
	}

std::cout << timestamp << "," << symbolIndex << "," << type << "," << priceMantissa << "," << size << "\n";


	taqfile.write((char *)&timestamp, sizeof(unsigned int));
	taqfile.write((char *)&symbolIndex, sizeof(int));
	taqfile.write((char *)&type, sizeof(int));
	taqfile.write((char *)&priceMantissa, sizeof(int32_t));
	taqfile.write((char *)&priceExponent, sizeof(int32_t));
	taqfile.write((char *)&size, sizeof(unsigned int));

	if((taqCnt & 0xFF)==0)
	{
		taqfile.close();
		taqfile.open(m_quotes_file.c_str(), ios::out | ios::app |  ios::binary);
		if(!taqfile.is_open())
		{
			std::cout <<  "Can not open TAQ file.\n" << std::endl;
		}
	}

	taqCnt++;

	//writeTAQTxt(timestamp,symbolIndex,type,priceMantissa,priceExponent,size,quoteSource);
}

void CLog::writeTAQTxt(unsigned int timestamp, int symbolIndex, int type, int32_t priceMantissa, int32_t priceExponent, unsigned int size, std::string quoteSource)
{
	static int taqCnt;

	if(taqCnt==0)
		taqTxtfile.open(m_quotesTxt_file.c_str(), ios::out | ios::trunc);
	else
		taqTxtfile.open(m_quotesTxt_file.c_str(), ios::out | ios::app);
	if(!taqTxtfile.is_open())
	{
		std::cout <<  "Can not open TAQ txt file.\n" << std::endl;
	}

	taqTxtfile << timestamp << "," << symbolIndex << "," << type << "," << priceMantissa << "," << priceExponent << "," << size << "," << quoteSource << "\n";
	taqTxtfile.close();

	taqCnt++;
}

void CLog::writePandLData(IBString line)
{
	ofstream myfile;

	myfile.open (m_pandl_file.c_str(), ios::out | ios::app);

	if(myfile.is_open())
	{
		myfile << line;
		myfile.close();
	}
}

void CLog::writeCountData(IBString line)
{
	ofstream myfile;

	myfile.open (m_count_file.c_str(), ios::out);

	if(myfile.is_open())
	{
		myfile << line;
		myfile.close();
	}
}

void CLog::writeUGorthData(IBString line)
{
	if(!enableMiscLogs) return;
	ofstream myfile;

	myfile.open (m_ugorth_file.c_str(), ios::out | ios::app);

	if(myfile.is_open())
	{
		myfile << line;
		myfile.close();
	}
}

void CLog::writeDevSizeData(IBString line)
{
	if(!enableMiscLogs) return;
	ofstream myfile;

	myfile.open (m_devsize_file.c_str(), ios::out | ios::app);

	if(myfile.is_open())
	{
		myfile << line;
		myfile.close();
	}
}

void CLog::writeLastOrthData(IBString line)
{
	if(!enableMiscLogs) return;
	ofstream myfile;

	myfile.open (m_lastorth_file.c_str(), ios::out | ios::app);

	if(myfile.is_open())
	{
		myfile << line;
		myfile.close();
	}
}

void CLog::writeLastPPData(IBString line)
{
	if(!enableMiscLogs) return;
	ofstream myfile;

	myfile.open (m_lastpp_file.c_str(), ios::out | ios::app);

	if(myfile.is_open())
	{
		myfile << line;
		myfile.close();
	}
}

void CLog::writeSlippageData(IBString line)
{
	if(!enableMiscLogs) return;
	ofstream myfile;

	myfile.open (m_slippage_file.c_str(), ios::out | ios::app);

	if(myfile.is_open())
	{
		myfile << line;
		myfile.close();
	}
}

void CLog::writeSlippage2Data(IBString line)
{
	if(!enableMiscLogs) return;
	ofstream myfile;

	myfile.open (m_slippage2_file.c_str(), ios::out | ios::app);

	if(myfile.is_open())
	{
		myfile << line;
		myfile.close();
	}
}

void CLog::writeQuote(IBString timeOfDay, int assetId, TickType field, double price)
{
	if(!enableMiscLogs) return;
	if((assetId<0)||(assetId>=IM.BasketSize))
		return;

	if(field==BID) { m_quote[assetId].bid = price; }
	else if(field==ASK) { m_quote[assetId].ask = price; }
	else if(field==LAST) {}
	else { return; }

	std::stringstream vwap;
	if(SM.VWAP[assetId].getVWAP()!=0)
		vwap << SM.VWAP[assetId].getVWAP();
	else
		vwap << "";

	ofstream myfile;
	myfile.open (m_dimension_file[assetId].c_str(), ios::out | ios::app);
	if(myfile.is_open())
	{
		OSTR_DEF;
		if(field == LAST)
		{
			ostr << timeOfDay << "," << m_quote[assetId].bid << "," << m_quote[assetId].ask << "," <<
				SM.position[assetId] << "," << vwap.str() << "," << price << ",,,,,,,,,,,,,,,,,,,,\n";
		}
		else
		{
			ostr << timeOfDay << "," << m_quote[assetId].bid << "," << m_quote[assetId].ask << "," <<
				SM.position[assetId] << "," << vwap.str() << "," << ",,,,,,,,,,,,,,,,,,,,\n";
		}
		myfile << ostr.str();
		myfile.close();
	}
}

void CLog::writeFill(IBString timeOfDay, int assetId, OrderId orderId, bool action, int qty, double price)
{
	if(!enableMiscLogs) return;
	if((assetId<0)||(assetId>=IM.BasketSize))
		return;

	ofstream myfile;
	myfile.open (m_dimension_file[assetId].c_str(), ios::out | ios::app);
	if(myfile.is_open())
	{
		OSTR_DEF;
		if(action==0) //buy
		{
			ostr << timeOfDay << ",,,,,," << price << ",+" << qty << ",,,,,," << orderId << ",,,,,,,,,,,,,\n";
		}
		else //sell
		{
			ostr << timeOfDay << ",,,,,," << price << ",-" << qty << ",,,,,," << orderId << ",,,,,,,,,,,,,\n";
		}
		myfile << ostr.str();
		myfile.close();
	}
}

void CLog::writeAccting(IBString timeOfDay, int assetId, bool action, int qty, double price, double commission)
{
	if(!enableMiscLogs) return;
	if((assetId<0)||(assetId>=IM.BasketSize))
		return;

	//ostr << "Time,Symbol,Position,Filled,Comm,FilledQty,IB_BP,BP,SP,E,S,D,L,C,UG,RG,LMR,SMR\n";

	ofstream myfile;
	myfile.open (m_accting_file.c_str(), ios::out | ios::app);
	if(myfile.is_open())
	{
		OSTR_DEF;
		if(action==0) //buy
		{
			ostr << timeOfDay << "," << SM.marketData[assetId].contract->symbol <<
					"," << SM.position[assetId] << "," << price << ",+" << qty << "," << commission << "," <<
					SM.cpa->getIBBuyingPower() << "," << SM.cpa->getBuyingPower() << "," << SM.cpa->getShortingPower() << "," <<
					SM.cpa->getESDLC() << "," << SM.algo->getUnRealizedGain() <<
					"," << SM.cpa->getRealizedGain()  << "," << SM.cpa->getLongMarginRatio() <<
					"," << SM.cpa->getShortMarginRatio() << "\n";
		}
		else //sell
		{
			ostr << timeOfDay << "," << SM.marketData[assetId].contract->symbol <<
					"," << SM.position[assetId] << "," << price << ",-" << qty << "," << commission << "," <<
					SM.cpa->getIBBuyingPower() << "," << SM.cpa->getBuyingPower() << "," << SM.cpa->getShortingPower() << "," <<
					SM.cpa->getESDLC() << "," << SM.algo->getUnRealizedGain() <<
					"," << SM.cpa->getRealizedGain()  << "," << SM.cpa->getLongMarginRatio() <<
					"," << SM.cpa->getShortMarginRatio() << "\n";
		}
		myfile << ostr.str();
		myfile.close();
	}
}

void CLog::writePlaced(IBString timeOfDay, int assetId, OrderId orderId, bool action, int qty, double price, IBString stateDesc)
{
	if(!enableMiscLogs) return;
	if((assetId<0)||(assetId>=IM.BasketSize))
		return;

	ofstream myfile;
	myfile.open (m_dimension_file[assetId].c_str(), ios::out | ios::app);
	if(myfile.is_open())
	{
		OSTR_DEF;
		if(action==0) //buy
		{
			ostr << timeOfDay << ",,,,,,,," << price << ",+" << qty << "," << stateDesc << ",,," << orderId << ",,,,," <<
					SM.cpa->getIBBuyingPower() << "," << SM.cpa->getBuyingPower() << "," << SM.cpa->getShortingPower() << "," <<
					SM.cpa->getESDLC() << "\n";
		}
		else
		{
			ostr << timeOfDay << ",,,,,,,," << price << ",-" << qty << "," << stateDesc << ",,," << orderId << ",,,,," <<
					SM.cpa->getIBBuyingPower() << "," << SM.cpa->getBuyingPower() << "," << SM.cpa->getShortingPower() << "," <<
					SM.cpa->getESDLC() << "\n";
		}
		myfile << ostr.str();
		myfile.close();
	}
}

void CLog::writeDesired(IBString timeOfDay, int assetId, int desB, int desA, int sizeAtPriceB, int sizeAtPriceS)
{
	if(!enableMiscLogs) return;
	if((assetId<0)||(assetId>=IM.BasketSize))
		return;

	ofstream myfile;
	myfile.open (m_dimension_file[assetId].c_str(), ios::out | ios::app);
	if(myfile.is_open())
	{
		OSTR_DEF;

		if(desB!=0)
			ostr << timeOfDay << ",,,,,,,,,,,,,," << desB << ",";
		else
			ostr << timeOfDay << ",,,,,,,,,,,,,,,";

		if(desA!=0)
			ostr << desA << ",";
		else
			ostr << ",";

		if(sizeAtPriceB > 0) ostr << sizeAtPriceB << ",";
		else ostr << ",";
		if(sizeAtPriceS > 0) ostr << sizeAtPriceS << ",";
		else ostr << ",";

		ostr << ",,,,,,,,\n";

		myfile << ostr.str();
		myfile.close();
	}
}

void CLog::writeCancel(IBString timeOfDay, int assetId, OrderId orderId, int qty, double price, bool action)
{
	if(!enableMiscLogs) return;
	if((assetId<0)||(assetId>=IM.BasketSize))
		return;

	ofstream myfile;
	myfile.open (m_dimension_file[assetId].c_str(), ios::out | ios::app);
	if(myfile.is_open())
	{
		OSTR_DEF;
		if(action==0)
		{
			ostr << timeOfDay << ",,,,,,,,,,," << price << ",+" << qty << "," << orderId << ",,,,,,,,,,,,\n";
		}
		else
		{
			ostr << timeOfDay << ",,,,,,,,,,," << price << ",-" << qty << "," << orderId << ",,,,,,,,,,,,\n";
		}
		myfile << ostr.str();
		myfile.close();
	}
}


void CLog::writeMasterHeader()
{
	ofstream myfile;

	IM.master_ostr.setf(std::ios::fixed);
	IM.master_ostr.precision(2);

	m_master_file = m_prefix + "master.txt";
	myfile.open (m_master_file.c_str(), ios::out | ios::trunc);
	if(myfile.is_open())
	{
		std::stringstream ostr;
		//ostr << "Time,P&L,MV,PV,PosSPY,PosQQQ,PosIWM,PosEEM,SPY,QQQ,IWM,EEM,ordSPY,ordQQQ,ordIWM,ordEEM,At,Exist\n";
		ostr << "Time,P&L,MV,PV,";
		for(int i=0; i<IM.BasketSize; i++) ostr << "Pos" << SM.marketData[i].contract->symbol << ",";
		for(int i=0; i<IM.BasketSize; i++) { ostr << SM.marketData[i].contract->symbol << ","; }
		for(int i=0; i<IM.BasketSize; i++) { ostr << "ord" << SM.marketData[i].contract->symbol << ","; }
		ostr << "At,Exist\n";
		myfile << ostr.str();
		myfile.close();
	}
}


//ostr << "Time,P&L,MV,PV,PosSPY,PosQQQ,PosIWM,PosEEM,SPY,QQQ,IWM,EEM,ordSPY,ordQQQ,ordIWM,ordEEM,At,Exist\n";
void CLog::writeMasterPositions(IBString timeOfDay, double mpvon)
{
	double MV=0.0, PV=0.0;
	for(int i=0; i<IM.BasketSize; i++)
	{
		MV += SM.position[i]*0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]);
		PV += SM.position[i]*SM.VWAP[i].getVWAP();
	}
	IM.master_ostr << timeOfDay << "," << SM.globalState->getPandL() << "," << MV << "," << PV << ",";
	for(int i=0; i<IM.BasketSize; i++) IM.master_ostr << SM.position[i] << ",";
#if 0
	for(int i=0; i<IM.BasketSize; i++) IM.master_ostr << 0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]) << ",";
#else
	for(int i=0; i<IM.BasketSize; i++)
	{
		double midpt = SM.marketData[i].market[BID]; //0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]);
		double vwap = SM.VWAP[i].getVWAP();
		double relprice;
		/*if(SM.position[i] > 0)*/ relprice = SM.marketData[i].market[BID];
		if(SM.position[i] < 0) relprice = SM.marketData[i].market[ASK];
		double diff;
		if(SM.position[i]==0) diff = 0.0;
		else if(SM.position[i]>0) diff = relprice-vwap;
		else diff = vwap-relprice;
		if(fabs(vwap)>TINY) IM.master_ostr << relprice << "(" << diff << "),";
		else IM.master_ostr << midpt << ",";
	}
#endif
	for(int i=0; i<IM.BasketSize; i++) IM.master_ostr << ",";
	IM.master_ostr << ",\n";
}


//ostr << "Time,P&L,MV,PV,PosSPY,PosQQQ,PosIWM,PosEEM,SPY,QQQ,IWM,EEM,ordSPY,ordQQQ,ordIWM,ordEEM,At,Exist\n";
void CLog::writeMasterGetIntoInfo(IBString timeOfDay, double norm_lastPP, double sig_lastPP, double mpvon)
{
	double MV=0.0, PV=0.0;
	for(int i=0; i<IM.BasketSize; i++)
	{
		MV += SM.position[i]*0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]);
		PV += SM.position[i]*SM.VWAP[i].getVWAP();
	}
	IM.master_ostr << timeOfDay << "," << SM.globalState->getPandL() << "," << MV << "," << PV << ",";
	for(int i=0; i<IM.BasketSize; i++) IM.master_ostr << SM.position[i] << ",";
#if 0
	for(int i=0; i<IM.BasketSize; i++) IM.master_ostr << 0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]) << ",";
#else
	for(int i=0; i<IM.BasketSize; i++)
	{
		double midpt = SM.marketData[i].market[BID]; //0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]);
		double vwap = SM.VWAP[i].getVWAP();
		double relprice;
		/*if(SM.position[i] > 0)*/ relprice = SM.marketData[i].market[BID];
		if(SM.position[i] < 0) relprice = SM.marketData[i].market[ASK];
		double diff;
		if(SM.position[i]==0) diff = 0.0;
		else if(SM.position[i]>0) diff = relprice-vwap;
		else diff = vwap-relprice;
		if(fabs(vwap)>TINY) IM.master_ostr << relprice << "(" << diff << "),";
		else IM.master_ostr << midpt << ",";
	}
#endif

	for(int i=0; i<IM.BasketSize; i++) IM.master_ostr << ",";
	IM.master_ostr << ",\n";
}

//ostr << "Time,P&L,MV,PV,PosSPY,PosQQQ,PosIWM,PosEEM,SPY,QQQ,IWM,EEM,ordSPY,ordQQQ,ordIWM,ordEEM,At,Exist\n";
void CLog::writeMasterDimension(IBString timeOfDay, int assetId, int des, double lmt, long existSize)
{
	OSTR_DEF;
	double MV=0.0, PV=0.0;
	for(int i=0; i<IM.BasketSize; i++)
	{
		MV += SM.position[i]*0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]);
		PV += SM.position[i]*SM.VWAP[i].getVWAP();
	}

	IM.master_ostr << timeOfDay << "," << SM.globalState->getPandL() << "," << MV << "," << PV << ",";
	for(int i=0; i<IM.BasketSize; i++) IM.master_ostr << SM.position[i] << ",";
#if 0
	for(int i=0; i<IM.BasketSize; i++) IM.master_ostr << 0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]) << ",";
#else
	for(int i=0; i<IM.BasketSize; i++)
	{
		double midpt = SM.marketData[i].market[BID]; //0.5*(SM.marketData[i].market[BID]+SM.marketData[i].market[ASK]);
		double vwap = SM.VWAP[i].getVWAP();
		double relprice;
		/*if(SM.position[i] > 0)*/ relprice = SM.marketData[i].market[BID];
		if(SM.position[i] < 0) relprice = SM.marketData[i].market[ASK];
		double diff;
		if(SM.position[i]==0) diff = 0.0;
		else if(SM.position[i]>0) diff = relprice-vwap;
		else diff = vwap-relprice;
		if(fabs(vwap)>TINY) IM.master_ostr << relprice << "(" << diff << "),";
		else IM.master_ostr << midpt << ",";
	}
#endif
	for(int i=0; i<assetId; i++) IM.master_ostr << ",";
	IM.master_ostr << des;
	for(int i=assetId; i<IM.BasketSize; i++) IM.master_ostr << ",";
	IM.master_ostr << lmt << "," << existSize << "\n";
}




#define MAX_HOSTNAME_LENGTH 255
void CLog::setupSocket()
{
    m_LogSockfd = SocketUtils::establishSocket(3000);
    //m_PLSockfd = SocketUtils::establishSocket(3001);
}

void CLog::flushLogs()
{
	IM.ostr << "flushLogs: starting\n";
#if 0
    if(gethostname(hostName, MAX_HOSTNAME_LENGTH) < 0)
    	 error("ERROR getting host name");

    server = gethostbyname(hostName);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    printf("Please enter the message: ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0)
         error("ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0)
         error("ERROR reading from socket");
#endif

	if(IM.ClientServer)
	{
		//transmit the log
		const std::string buffer_str = "0123456789" + IM.ostr.str();
		const char *buffer = buffer_str.c_str();
		int n = write(m_LogSockfd,(void *)buffer,strlen(buffer));
		if (n < 0) error("ERROR writing to Log socket");

		//transmit P&L
		std::stringstream pandl_ostr(IM.pandl_ostr.str()); // << IM.cnt_ostr;
		pandl_ostr << IM.cnt_ostr.str();

		const std::string buffer2_str = "9876543210" + pandl_ostr.str();
		buffer = buffer2_str.c_str();
		//n = write(m_PLSockfd,(void *)buffer,strlen(buffer));
		n = write(m_LogSockfd,(void *)buffer,strlen(buffer));
		if (n < 0) error("ERROR writing P&L to socket");
	}

	IM.Logger->writeToFile(m_log_fname, IM.ostr.str(), true);
	cout << IM.ostr.str();
	IM.ostr.clear(); IM.ostr.str("");
	cout << "flushLogs: wrote to file\n";

	IM.Logger->writeToFile(m_master_file, IM.master_ostr.str(), true); IM.master_ostr.clear(); IM.master_ostr.str("");
	IM.Logger->writeToFile(m_pandl_file, IM.pandl_ostr.str(), true); IM.pandl_ostr.clear(); IM.pandl_ostr.str("");
	if(IM.cnt_ostr.str().length() != 0)
		IM.Logger->writeToFile(m_count_file, IM.cnt_ostr.str(), false); IM.cnt_ostr.clear(); IM.cnt_ostr.str("");

	if(enableMiscLogs)
	{
		IM.Logger->writeToFile(m_ugorth_file, IM.ugorth_ostr.str(), true); IM.ugorth_ostr.str("");
		IM.Logger->writeToFile(m_devsize_file, IM.devSize_ostr.str(), true); IM.devSize_ostr.str("");
		IM.Logger->writeToFile(m_lastorth_file, IM.lastOrth_ostr.str(), true); IM.lastOrth_ostr.str("");
		IM.Logger->writeToFile(m_lastpp_file, IM.lastPP_ostr.str(), true); IM.lastPP_ostr.str("");
		IM.Logger->writeToFile(m_slippage_file, IM.slippage_ostr.str(), true); IM.slippage_ostr.str("");
		IM.Logger->writeToFile(m_slippage2_file, IM.slippage2_ostr.str(), true); IM.slippage2_ostr.str("");
	}
	cout << "flushLogs: done with f-n\n";
}

void CLog::writeToFile(IBString fname, IBString line, bool append)
{
	ofstream myfile;

	if(append) myfile.open(fname.c_str(), ios::out | ios::app);
	else myfile.open(fname.c_str(), ios::out);

	if(myfile.is_open())
	{
		myfile << line;
		myfile.close();
	}
}
