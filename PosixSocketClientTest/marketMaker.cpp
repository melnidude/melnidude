#include "memory.h"
#include "stdint.h"

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;


void CAlgoBase::orderManager(int assetId)
{
	Order *orderB;
	Order *orderS;

	IM.ostr << "\nOrderMgr: top of Book (" << SM.marketData[assetId].contract->symbol << ")"
			<< " B " << SM.marketData[assetId].market[BID]
			<< " A " << SM.marketData[assetId].market[ASK]  << ", t=" << SM.globalState->getTime() << "\n";

	if((SM.marketData[assetId].market[BID] <= 0)||(SM.marketData[assetId].market[ASK] <= 0)||(!IM.pca_enable)||
			(SM.marketData[0].market[BID] <= 0)||(SM.marketData[0].market[ASK] <= 0))
	{
		return;
	}

	if(!SM.allowTrading[assetId])
	{
		IM.ostr << "Order not placed: position mismatch: trading in " <<
			SM.marketData[assetId].contract->symbol << " paused\n";
		return;
	}

	IM.ostr << "OrderMgr: local position(" << SM.marketData[assetId].contract->symbol << ")=" << SM.position[assetId] << ", desired=" << SM.DesiredPosition[assetId] << "\n";

	SM.marketData[assetId].orderB->parentId = 0;
	SM.marketData[assetId].orderS->parentId = 0;
	orderB = NULL;
	orderS = NULL;

	BracketHierarchy hierarchy = SM.algo->GenerateDesiredCumulativeOrders(assetId, &orderB, &orderS);

	//SM.orderBook[assetId].genDeltaOrders(orderB, orderS, hierarchy);
}
