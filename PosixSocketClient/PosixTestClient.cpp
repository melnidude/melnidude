#include "PosixTestClient.h"

#include "EPosixClientSocket.h"
#include "EPosixClientSocketPlatform.h"

#include "Contract.h"
#include "Order.h"
#include "OrderState.h"
#include "globalState.h"
#include "manager.h"

#include <iostream>
#include <fstream>

#include <sstream>
#include "memory.h"
#include <time.h>

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;

const int PING_DEADLINE = 2; // seconds
const int SLEEP_BETWEEN_PINGS = 30; // seconds

///////////////////////////////////////////////////////////
// member funcs
PosixTestClient::PosixTestClient()
	: m_pClient(new EPosixClientSocket(this))
	, m_state(ST_CONNECT)
	, m_sleepDeadline(0)
{
}

PosixTestClient::~PosixTestClient()
{
}

bool PosixTestClient::connect(const char *host, unsigned int port, int clientId)
{
	// trying to connect
	printf( "Connecting to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, clientId);

	bool bRes = m_pClient->eConnect( host, port, clientId);

	if (bRes) {
		printf( "Connected to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, clientId);
	}
	else
		printf( "Cannot connect to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, clientId);

	return bRes;
}

void PosixTestClient::disconnect() const
{
	IM.ostr << "Disconnecting...\n";
	IM.Logger->flushLogs();

	m_pClient->eDisconnect();

	printf ( "Disconnected\n");
}

bool PosixTestClient::isConnected() const
{
	return m_pClient->isConnected();
}

void PosixTestClient::processMessages()
{
	fd_set readSet, writeSet, errorSet;
	struct timeval tval;
	tval.tv_usec = 0;
	tval.tv_sec = 0;

	time_t now = time(NULL);

	switch (m_state) {
		case ST_PLACEORDER:
			//placeOrder();
			break;
		case ST_PLACEORDER_ACK:
			break;
		case ST_CANCELORDER:
			//cancelOrder();
			break;
		case ST_CANCELORDER_ACK:
			break;
		case ST_PING:
			reqCurrentTime();
			break;
		case ST_PING_ACK:
			if( m_sleepDeadline < now) {
				IM.ostr << "disconnect\n";
				IM.Logger->flushLogs();
				disconnect();
				return;
			}
			break;
		case ST_IDLE:
			if( m_sleepDeadline < now) {
				m_state = ST_PING;
				return;
			}
			break;
	}

	if( m_sleepDeadline > 0) {
		// initialize timeout with m_sleepDeadline - now
		tval.tv_sec = m_sleepDeadline - now;
	}

	if( m_pClient->fd() >= 0 ) {

		FD_ZERO( &readSet);
		errorSet = writeSet = readSet;

		FD_SET( m_pClient->fd(), &readSet);

		if( !m_pClient->isOutBufferEmpty())
			FD_SET( m_pClient->fd(), &writeSet);

		FD_CLR( m_pClient->fd(), &errorSet);

		int ret = select( m_pClient->fd() + 1, &readSet, &writeSet, &errorSet, &tval);

		if( ret == 0) { // timeout
			//OSTR_RESET
			//ostr << "timeout\n";
			//OSTR_LOG(LOG_MSG);
			return;
		}

		if( ret < 0) {	// error
			IM.ostr << "error (ret<0)\n";
			IM.Logger->flushLogs();
			disconnect();
			return;
		}

		if( m_pClient->fd() < 0)
		{
			return;
		}

		if( FD_ISSET( m_pClient->fd(), &errorSet)) {
			// error on socket
			printf("socket error\n");
			m_pClient->onError();
		}

		if( m_pClient->fd() < 0)
		{
			IM.ostr << "just return 2\n";
			return;
		}

		if( FD_ISSET( m_pClient->fd(), &writeSet)) {
			// socket is ready for writing
			m_pClient->onSend();
		}

		if( m_pClient->fd() < 0)
		{
			return;
		}

		if( FD_ISSET( m_pClient->fd(), &readSet)) {
			// socket is ready for reading
			m_pClient->onReceive();
		}
	}
}

//////////////////////////////////////////////////////////////////
// methods
void PosixTestClient::reqCurrentTime()
{
	printf( "Requesting Current Time\n");

	// set ping deadline to "now + n seconds"
	m_sleepDeadline = time( NULL) + PING_DEADLINE;

	m_state = ST_PING_ACK;

	m_pClient->reqCurrentTime();
}


void PosixTestClient::reqAllOpenOrders()
{
	m_pClient->reqAllOpenOrders();
}


void PosixTestClient::placeOrder(int assetId, Order &order, long orderId)
{
	m_pClient->placeOrder(orderId, *(SM.marketData[assetId].contract), order);

	IM.ostr << "***** Book: placed order(" << order.orderId << "): " << order.action
			<< " " << order.totalQuantity << " of " <<
			SM.marketData[assetId].contract->symbol <<
			" at " << order.lmtPrice << ", auxPrice=" << order.auxPrice << ", t=" <<
			SM.globalState->getTime() << "\n";
}

void PosixTestClient::cancelOrder(long orderId)
{
	IM.ostr << "PosixTestClient: canceling order " << orderId << ", t=" <<
			SM.globalState->getTime() << "\n";

	m_pClient->cancelOrder(orderId);
}

///////////////////////////////////////////////////////////////////
// events
void PosixTestClient::orderStatus( OrderId orderId, const IBString &status, int filled,
	   int remaining, double avgFillPrice, int permId, int parentId,
	   double lastFillPrice, int clientId, const IBString& whyHeld)

{
	if((!IM.algoInitDone)||(!IM.pca_enable))
	{
		IM.ostr << "PosixTestClient: closeout order " << orderId << " status : " << status << "\n";
		IM.Logger->flushLogs();
		return;
	}

	int assetId = SM.ids->getAssetId(orderId);

	IM.ostr << "PosixTestClient: " << SM.marketData[assetId].contract->symbol << " order " << orderId << " status : " << status << ", filled=" << filled <<
			", whyHeld=" << whyHeld << "\n";

	SM.orderBook[assetId].processOrderStatus(orderId,status,filled,avgFillPrice);

	IM.Logger->flushLogs();
}

void PosixTestClient::nextValidId(OrderId orderId)
{
	if(SM.ids->m_mktDataStartingId<0)
		SM.ids->m_mktDataStartingId = orderId;
}

void PosixTestClient::currentTime( long time)
{
	if ( m_state == ST_PING_ACK) {
		time_t t = ( time_t)time;
		struct tm * timeinfo = localtime ( &t);
		printf( "The current date/time is: %s", asctime( timeinfo));

		time_t now = ::time(NULL);
		m_sleepDeadline = now + SLEEP_BETWEEN_PINGS;

		m_state = ST_IDLE;
	}
}

void PosixTestClient::error(const int id, const int errorCode, const IBString errorString)
{
	IM.ostr << "Error id=" << id << ", errorCode=" << errorCode << ", msg=" << errorString.c_str() <<
			", t=" << SM.globalState->getTime() << "\n";

	if( id == -1 && errorCode == 1100) // if "Connectivity between IB and TWS has been lost"
	{
		IM.ostr << "Disconnecting\n";
		disconnect();
	}

	if(errorCode == 326)
	{
		disconnect();

		SM.clientId++;
		IM.ostr << "TestClient: connecting with clientId " << SM.clientId << "\n";
		IM.client->connect(IM.host, IM.port, SM.clientId);
		sleep(2);
	}

	if(id>=0)
	{
		int assetId = SM.ids->getAssetId(id);
		if(orderFailed(errorCode))
		{
			IM.ostr << "TestClient: error processing order " << id << ", assetId=" << assetId << ".\nLiquidating portfolio...\n";

			bool found = false;
			for(unsigned int i=0; i<IM.LiqOrders.size(); i++)
			{
				if(IM.LiqOrders[i]==id)
				{
					found = true;
					break;
				}
			}
			if(!found)
			{
				IM.LiqOrders.push_back(id);
				SM.orderBook[assetId].remove(id);
				LiquidateAll();
			}
		}
		else if(errorCode==201) //order rejected
		{
			IM.ostr << "TestClient: making SM.DesiredPosition[assetId]=SM.position[assetId]...\n";
			SM.DesiredPosition[assetId] = SM.position[assetId];
			m_pClient->cancelOrder(id);
		}
	}
	IM.Logger->flushLogs();
}

bool PosixTestClient::orderFailed(int errorCode)
{
	if(errorCode==100) return false;
	else if(errorCode==103) return false;
	else if(errorCode==104) return false;
	else if(errorCode==134) return false;
	else if(errorCode==135) return false;///
	else if(errorCode==136) return false;
	else if(errorCode==161) return false;
	else if(errorCode==201) return false;
	else if(errorCode==202) return false;
	else return true;
}


void PosixTestClient::tickPrice( TickerId tickerId, TickType field, double price, int canAutoExecute)
{
	if(IM.quoteSource=="IB")
	{
		int assetId = tickerId-SM.ids->recdId();
		SM.algo->tickPrice(assetId, field, price, 99, "IB");
	}
}


void PosixTestClient::tickSize( TickerId tickerId, TickType field, int size) {}
void PosixTestClient::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
											 double modelPrice, double pvDividend) {}

void PosixTestClient::tickGeneric(TickerId tickerId, TickType tickType, double value) {}
void PosixTestClient::tickString(TickerId tickerId, TickType tickType, const IBString& value) {}
void PosixTestClient::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const IBString& formattedBasisPoints,
							   double totalDividends, int holdDays, const IBString& futureExpiry, double dividendImpact, double dividendsToExpiry) {}
void PosixTestClient::openOrder( OrderId orderId, const Contract& c, const Order& o, const OrderState& ostate)
{
	IM.ostr << "\nTestClient: open order: " << c.symbol << ", " << orderId << ", " << o.action << " " << o.totalQuantity
			<< " at " << o.lmtPrice << ", " << ostate.status << "\n";
	IM.ostr << "TestClient: openState: status=" << ostate.status << ", initMargin=" << ostate.initMargin << ", maintMargin=" << ostate.maintMargin <<
			", equityWithLoan=" << ostate.equityWithLoan  << "\n";

	IBString status;
	IBString initMargin;
	IBString maintMargin;
	IBString equityWithLoan;

	if((!IM.algoInitDone)||(!IM.pca_enable))
	{
		cancelDimension(orderId);
	}

	//check if we need to close out this dimension
	int assetId = SM.ids->getAssetId(orderId);
	if(SM.cancelDimension==assetId)
	{
		IM.ostr << "TestClient: cancelDimension: assetId=" << assetId << "\n";
		cancelDimension(orderId);
	}
	IM.Logger->flushLogs();
}


void PosixTestClient::openOrderEnd() {}
void PosixTestClient::winError( const IBString &str, int lastError) {}

void PosixTestClient::connectionClosed()
{
	IM.ostr << "Connection closed\n";
	IM.Logger->flushLogs();
}

void PosixTestClient::updateAccountTime(const IBString& timeStamp) {}
void PosixTestClient::accountDownloadEnd(const IBString& accountName) {}

void PosixTestClient::contractDetails( int reqId, const ContractDetails& contractDetails)
{
#if COMBO
	int assetId = SM.ids->getAssetId(reqId);
	SM.marketData[assetId].contract->conId = contractDetails.underConId;
	SM.marketData[assetId].leg.conId = contractDetails.underConId;
#endif
}

void PosixTestClient::bondContractDetails( int reqId, const ContractDetails& contractDetails) {}
void PosixTestClient::contractDetailsEnd( int reqId) {}
void PosixTestClient::execDetails( int reqId, const Contract& contract, const Execution& execution) {}
void PosixTestClient::execDetailsEnd( int reqId) {}

void PosixTestClient::updateMktDepth(TickerId id, int position, int operation, int side,
									  double price, int size) {}
void PosixTestClient::updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
										int side, double price, int size) {}
void PosixTestClient::updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch) {}
void PosixTestClient::managedAccounts( const IBString& accountsList) {}
void PosixTestClient::receiveFA(faDataType pFaDataType, const IBString& cxml) {}
void PosixTestClient::historicalData(TickerId reqId, const IBString& date, double open, double high,
									  double low, double close, int volume, int barCount, double WAP, int hasGaps) {}
void PosixTestClient::scannerParameters(const IBString &xml) {}
void PosixTestClient::scannerData(int reqId, int rank, const ContractDetails &contractDetails,
	   const IBString &distance, const IBString &benchmark, const IBString &projection,
	   const IBString &legsStr) {}
void PosixTestClient::scannerDataEnd(int reqId) {}
void PosixTestClient::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
								   long volume, double wap, int count) {}
void PosixTestClient::fundamentalData(TickerId reqId, const IBString& data) {}
void PosixTestClient::deltaNeutralValidation(int reqId, const UnderComp& underComp) {}
void PosixTestClient::tickSnapshotEnd(int reqId) {}

void PosixTestClient::updateAccountValue(const IBString& key, const IBString& val,
										  const IBString& currency, const IBString& accountName)
{
	if(!key.compare("CashBalance"))
	{
		double cash = atof(val.c_str());
		IM.ostr << "TestClient: acct update: CashBalance " << cash << ", local " << SM.globalState->getCash() <<
				", t=" << SM.globalState->getTime() << "\n";
		SM.globalState->setCash(cash);
	}
	else if(!key.compare("NetLiquidation"))
	{
		double NLV = atof(val.c_str());
		SM.globalState->setNLV(NLV);
		IM.ostr << "TestClient: acct update: NLV " << NLV <<
				", t=" << SM.globalState->getTime() << "\n";
	}
	else if(!key.compare("BuyingPower"))
	{
		double BP = atof(val.c_str());
		SM.cpa->setIB_BP(BP);
		IM.ostr << "TestClient: acct update: IB BP " << BP <<
				", t=" << SM.globalState->getTime() << "\n";
	}
	else
	{
		IM.ostr << "TestClient: acct update: key=" << key << ", val=" << val <<
			", t=" << SM.globalState->getTime() << "\n";
	}
	IM.Logger->flushLogs();
}

void PosixTestClient::updatePortfolio(const Contract& contract, int position,
		double marketPrice, double marketValue, double averageCost,
		double unrealizedPNL, double realizedPNL, const IBString& accountName)
{
	int assetId = SM.ids->getAssetId(contract.localSymbol);

	if(IM.CloseInitPos)
	{
		if((!IM.pca_enable)&&(position!=0))
		{
			liquidateDimension(contract, assetId, position);
		}
	}

	//check if positions are in agreement
	if(assetId<0)
	{
		IM.ostr << "TestClient: can't find id of " << contract.localSymbol <<
				", t=" << SM.globalState->getTime() << "\n";
		IM.Logger->flushLogs();
		return;
	}

	if(IM.pca_enable)
	{
		SM.allowTrading[assetId] = true;
		if(SM.position[assetId]!=position)
		{
			IM.ostr << "TestClient: positions differ:  " << contract.localSymbol <<
					", (LocalPos,ServerPos)=(" << SM.position[assetId] << "," << position <<
					") : t=" << SM.globalState->getTime() << "\n";
			if((SM.position_prev[assetId]==SM.position[assetId])&&(position==SM.position_srv_prev[assetId]))
				SM.positionDiscrepancyCnt[assetId]++;
			else
				SM.positionDiscrepancyCnt[assetId] = 0;
			SM.position_prev[assetId] = SM.position[assetId];
			SM.position_srv_prev[assetId] = position;
			if(SM.positionDiscrepancyCnt[assetId] > 5)
			{
				SM.allowTrading[assetId] = false;
				SM.cancelDimension = assetId;
				if(position != 0)
					liquidateDimension(contract, assetId, position);

				double pl_delta = SM.position[assetId]*0.5*(SM.marketData[assetId].market[BID]+SM.marketData[assetId].market[ASK]);
				SM.globalState->updateCash(assetId, -pl_delta, -pl_delta);
				IM.ostr << "TestClient: reconciled positions, cash adjusted by :  " << pl_delta << "\n";

				SM.position[assetId] = 0;
				sleep(2);
			}
		}
		else
		{
			IM.ostr << "TestClient: positions same:  " << contract.localSymbol <<
					", (LocalPos,ServerPos)=(" << SM.position[assetId] << "," << position <<
					") : t=" << SM.globalState->getTime() << "\n";
			SM.cancelDimension = -1;
			SM.positionDiscrepancyCnt[assetId] = 0;
		}
	}
	IM.Logger->flushLogs();
}

void PosixTestClient::reqAccountUpdates(bool subscribe)
{
	m_pClient->reqAccountUpdates(subscribe, "Gerry");
}

void PosixTestClient::reqTickData()
{
	for(int i=0; i<IM.BasketSize; i++)
	{
		m_pClient->reqMktData(SM.marketData[i].mktDataId, *(SM.marketData[i].contract), "", false);
	}
}

void PosixTestClient::cancelTickData()
{
	for(int i=0; i<IM.BasketSize; i++)
	{
		m_pClient->cancelMktData(SM.marketData[i].mktDataId);
	}
}

void PosixTestClient::reserveIds(int numIds)
{
	m_pClient->reqIds(IM.BasketSize*numIds);
}

void PosixTestClient::reqContractDetails(int reqId, const Contract &contract)
{
#if COMBO
	m_pClient->reqContractDetails(reqId, contract);
#endif
}


void PosixTestClient::cancelDimension(OrderId orderId)
{
	bool found = false;
    for(unsigned int i=0; i<IM.LiqOrders.size(); i++)
    {
        if(IM.LiqOrders[i]==orderId)
        {
        	found = true;
        	break;
        }
    }
    if(!found)
    {
		IM.ostr << "TestClient: liquidating portfolio order " << orderId << ", t=" <<
				SM.globalState->getTime() << "\n";

    	IM.LiqOrders.push_back(orderId);
		m_pClient->cancelOrder(orderId);
    }
}

void PosixTestClient::liquidateDimension(const Contract& contract, int assetId, int position)
{
	bool found = false;
    for(unsigned int i=0; i<IM.LiqPositions.size(); i++)
    {
        if(!IM.LiqPositions[i].localSymbol.compare(contract.localSymbol))
        {
        	found = true;
        	break;
        }
    }
    if(!found)
    {
    	Holding holding = {contract.localSymbol, position};
		IM.LiqPositions.push_back(holding);

		Order order;
		order.orderId = SM.ids->getNextId(0);
		order.action = (position>0) ? "SELL" : "BUY";
		order.orderType = "MKT";
		order.totalQuantity = abs(position);

		IM.LiqOrders.push_back(order.orderId);
		m_pClient->placeOrder(order.orderId, contract, order);

		IM.ostr << "TestClient: position liquidation  " << contract.localSymbol <<
				", position=" << position << ", t=" << SM.globalState->getTime() << "\n";

		IM.Logger->writePlaced(SM.globalState->getTime(), assetId, order.orderId, (position>0), order.totalQuantity, 0.0, "Closeout");
    }
}
