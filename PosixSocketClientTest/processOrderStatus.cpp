#include "memory.h"

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;


void OrderBook::processOrderStatus(OrderId orderId, const IBString &status, long filledCumulative, double avgFillPrice)
{
	long filled, remaining;
	bool success;

	if((status == "Submitted")||(status == "Filled")||(status == "PendingCancel"))
	{
		BookElement *bookElement = getBookElement(orderId, &success);
		if(!success)
		{
			IM.ostr << "Book: Error: not confirmed in the book.\n";
			return;
		}

		if((bookElement->orderStatus == SUBMITTED_UNCONFIRMED)&&(status == "Submitted"))
			bookElement->orderStatus = SUBMITTED_CONFIRMED;

		IM.ostr << "Book: order " << orderId << " status : " << status << " and is in the book\n";
		if(filledCumulative > 0)
		{
			filled = filledCumulative - bookElement->m_order->cumulativeFill;
			remaining = MAX(bookElement->m_order->totalQuantity-filled, 0);
			if(filled < 0)
			{
				IM.ostr << "Book: Error: filledCumulative (" << filledCumulative
						<< ") < bookElement->m_order->cumulativeFill (" << bookElement->m_order->cumulativeFill << ").\n";
				return;
			}

			if(filled==0)
			{
				return;
			}

			SM.allowTrading[m_assetId] = true;

			IBString action = (bookElement->m_order->action) ? "SLD" : "BOT";
			IM.ostr << "Book: " << bookElement->m_order->stateDesc << " fill " << SM.marketData[m_assetId].contract->symbol << " : " << orderId << ": " << action << " " <<
					filled << " at " << avgFillPrice << ", " << remaining << " remaining" <<
					", t=" << SM.globalState->getTime() << "\n";

			//IM.Logger->writeFill(SM.globalState->getTime(),
			//		m_assetId, orderId, bookElement->m_order->action, filled,
			//		avgFillPrice);


			//compute how long it took to execute
			/*double time_in_seconds = SM.globalState->getTimeSinceStarting(bookElement->m_order->timePlaced);
			IM.ostr << "Book: duration=" << time_in_seconds << "\n";;*/

			bool bidAsk = bookElement->m_order->action;
			long filled_s = (1-2*bidAsk)*filled;
			double delta = avgFillPrice*filled;
			double delta_s = delta*(1-2*bidAsk);

			//record P&L per order type
			double vwap = SM.VWAP[m_assetId].getVWAP();
			if(fabs(vwap) > TINY)
			{
				//double pl_delta = MIN(filled, abs(SM.position[m_assetId]))*(avgFillPrice-vwap)*(2*bidAsk-1);
				double pl_delta = filled*(avgFillPrice-vwap)*(2*bidAsk-1);
				IM.ostr << "pl_delta=" << pl_delta << "\n";
				SM.globalState->calcPandLcontributions(m_assetId, pl_delta, bookElement->m_order);
			}

			double avgFillPriceNoSlippage, deltaNoSlippage_s;
			if(bookElement->m_order->action==BID_ORDER)
				avgFillPriceNoSlippage = MAX(MIN(avgFillPrice, bookElement->m_order->nbboWhenPlaced+0.01), bookElement->m_order->nbboWhenPlaced-0.01);
			else
				avgFillPriceNoSlippage = MAX(MIN(avgFillPrice, bookElement->m_order->nbboWhenPlaced+0.01), bookElement->m_order->nbboWhenPlaced-0.01);
			deltaNoSlippage_s = avgFillPriceNoSlippage*filled*(1-2*bidAsk);

			SM.globalState->updateExposure(bidAsk, delta);
			SM.VWAP[m_assetId].newTrade(filled_s, avgFillPrice);

			double commission_per_share = SM.globalState->calcCommission(bookElement->m_order->auxPrice);

			double commission = commission_per_share*filled;
			SM.globalState->updateCash(m_assetId, delta_s+commission, deltaNoSlippage_s+commission);
			SM.globalState->incrTrades(m_assetId, filled, bookElement->m_order);
			IM.ostr << "starting updatePortfolioStats\n";

			//SM.cpa->updateAccounting(delta_s, m_assetId, commission);
			SM.position[m_assetId] += filled_s;
			SM.algo->updatePortfolioStats(m_assetId, filled_s, bookElement->m_order->neutralizable);
			IM.ostr << "starting calcNLV\n";

			SM.globalState->calcNLV(m_assetId);
			IM.ostr << "GlobalState: P&L=" << SM.globalState->getPandL() << ", NoSlippageP&L=" << SM.globalState->getNoSlippagePandL() << "\n";

			/*IM.Logger->writeAccting(SM.globalState->getTime(),
					m_assetId, bookElement->m_order->action, filled,
					avgFillPrice, commission);*/


			if(status != "Filled")
			{
				bookElement->m_order->cumulativeFill = filledCumulative;
				bookElement->m_order->totalQuantity -= filled;
			}
			else
			{
				if(bookElement->m_order->action==BID_ORDER)
				{
					IM.slippage_ostr << SM.globalState->getDTime() << "," << 88 << "," << m_assetId << "," << avgFillPrice-bookElement->m_order->nbboWhenPlaced << "\n";
					IM.ostr << SM.marketData[m_assetId].contract->symbol  << " walk (" << bookElement->m_order->orderId << ") : " << avgFillPrice-bookElement->m_order->nbboWhenPlaced << "\n";
					IM.slippage2_ostr << SM.globalState->getDTime() << "," << 87 << "," << m_assetId << "," << avgFillPrice-SM.marketData[m_assetId].market[BID] << "\n";
				}
				else
				{
					IM.slippage_ostr << SM.globalState->getDTime() << "," << 88 << "," << m_assetId << "," << bookElement->m_order->nbboWhenPlaced-avgFillPrice << "\n";
					IM.ostr << SM.marketData[m_assetId].contract->symbol  << " walk (" << bookElement->m_order->orderId << ") : " << bookElement->m_order->nbboWhenPlaced-avgFillPrice << "\n";
					IM.slippage2_ostr << SM.globalState->getDTime() << "," << 87 << "," << m_assetId << "," << SM.marketData[m_assetId].market[ASK]-avgFillPrice << "\n";
				}

				remove(orderId);
				/*OSTR_RESET_HiPrec;
				ostr << SM.globalState->getDTime() << "," << m_assetId << "," << 98+bidAsk << "," << avgFillPrice << "\n";
				IM.Logger->writeTickData(ostr.str());*/

				//IM.client->reqAllOpenOrders();
			}

			//fill: activate marketMaker
			IM.ostr << "Book: calling afterFill, t=" << SM.globalState->getTime() << "\n";

			SM.algo->afterFill(m_assetId);
			return;
		}
	}
	else if((status == "Cancelled")||(status == "Inactive"))
	{
		int assetId = SM.ids->getAssetId(orderId);
		IM.ostr << "Book: canceling order " << orderId << ", status=" << status << ", assetId=" << assetId << "\n";

		bool success;
		BookElement *bookElement;
		bookElement = getBookElement(orderId, &success);
		if(success)
		{
			IM.Logger->writeCancel(SM.globalState->getTime(), m_assetId, orderId,
					bookElement->m_order->totalQuantity, bookElement->m_order->lmtPrice,
					bookElement->m_order->action);
		}
		remove(orderId);
	}
	else
	{
		IM.ostr << "Book: Error: order " << orderId << " status : " << status << "\n";
	}
}

