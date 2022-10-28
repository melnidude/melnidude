#include <math.h>
#include "memory.h"
#include "utils.h"
#include <cmath>

extern "C" {
int Singular_Value_Decomposition(double* A, int nrows, int ncols, double* U,
                      double* singular_values, double* V, double* dummy_array);
void Jacobi_Cyclic_Method(double eigenvalues[], double *eigenvectors,
                                                              double *A, int n);
void Jacobi_Cyclic_Method_Transpose(double eigenvalues[], double *eigenvectors,
                                                              double *A, int n);
void Sort_Eigenvalues(double eigenvalues[], double *eigenvectors, int n,
                                                                int sort_order);
void Sort_Eigenvalues_Transpose(double eigenvalues[], double *eigenvectors,
                                                        int n, int sort_order);
}

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;

CPCA::CPCA()
{
	K = IM.BasketSize;
	pca_window = IM.pca_window;
	pca_window_init = IM.pca_window_init;
	L = mxCreateDoubleMatrix(pca_window,K,mxREAL);
	last = SM.last;
	lastVector = new double[K];
	eigV = SM.eigVectors;

	ActualPCAwindow = mxCreateDoubleMatrix(1,1,mxREAL);
}
CPCA::~CPCA()
{
	free(L);
	delete lastVector;
}

void CPCA::run()
{
	int minCnt;
	double minCnt_d;

	minCnt = IM.pca_window;
	for(int i=0; i<K; i++) minCnt = MIN(minCnt, SM.last_cnt[i]);
	minCnt_d = minCnt;
	if(!IM.pca_enable) //accumulate enough data to start
	{
		bool enough = true;
		for(int i=0; i<K; i++)
		{
			if(SM.last_cnt[i] < pca_window_init)
			{
				IM.ostr << "PCA: not enough data for PCA: " << SM.marketData[i].contract->symbol
						<< " has " << SM.last_cnt[i] << " trades.\n";
				enough = false;
				break;
			}
		}
		if(!enough) return;
	}

	if(IM.debugPCA)
	{
		IBString timeOfDay = SM.globalState->getTime();

		IM.ostr << "*****Time (before autocorr): " << timeOfDay << "\n\n";
	}

	//////////////////////////////////
	for(int n1=0; n1<K; n1++)
	{
		for(int n2=0; n2<K; n2++)
		{
			SM.pca_A[n1*K+n2] = 0.0;
			for(int m=0; m<IM.pca_window; m++)
			{
				SM.pca_A[n1*K+n2] += last[m+n1*IM.pca_window]*last[m+n2*IM.pca_window];    //P[m][n1]*P[m][n2];
			}
		}
	}

	if(IM.debugPCA)
	{
		IBString timeOfDay = SM.globalState->getTime();

		IM.ostr << "*****Time (after autocorr): " << timeOfDay << "\n\n";
		IM.ostr << "PCA: Autocorrelation matrix:\n";
		for(int i=0; i<K; i++)
		{
			for(int j=0; j<K; j++) IM.ostr << SM.pca_A[i*K+j] << " ";
			IM.ostr << "\n";
		}
	}

	/*
    Jacobi_Cyclic_Method_Transpose(SM.singular_values, SM.pca_UU, SM.pca_A, K);

	Sort_Eigenvalues_Transpose(SM.singular_values, SM.pca_UU, K, 1);  // only need to find principle?

	//flip all-non-positive eigenvectors -- Is this really necessary?
	for(int r=0; r<K; r++)
	{
		int sum = 0;
		for(int c=0; c<K; c++) sum += (SM.pca_UU[r*K+c]<=0);
		if(sum==K) for(int c=0; c<K; c++) SM.pca_UU[r*K+c] = -SM.pca_UU[r*K+c];
	}
	*/

	if(IM.debugPCA)
	{
		IBString timeOfDay = SM.globalState->getPreciseTime();

		IM.ostr << "\n*****Time (before Eigenvectors): " << timeOfDay << "\n\n";
	}
	//////////////////////////////////////

	//if(IM.enableMatlab)
	if(0)
	{
		memcpy(mxGetPr(ActualPCAwindow), &minCnt_d, 1*sizeof(double));
		engPutVariable(SM.ep, "ActualPCAwindow", ActualPCAwindow);
		memcpy(mxGetPr(L), last, pca_window*K*sizeof(double));
		engPutVariable(SM.ep, "L", L);
		engEvalString(SM.ep, "L = L(1:ActualPCAwindow,:);");
		engEvalString(SM.ep, "[V E]=eig(L'*L); [SigmaValue SigmaIndex] = sort(abs(diag(E)));");
		engEvalString(SM.ep, "SigmaValue = sqrt(SigmaValue/(size(L,1)-1));");
		engEvalString(SM.ep, "Vorth = V(:,1:end-1);"); //orthogonal subspace
		engEvalString(SM.ep, "fNot = L(1,:)*Vorth;"); //project market onto the orthogonal subspace
		engEvalString(SM.ep, "Lorth = fNot*Vorth';"); //projection back to the price domain

		V = engGetVariable(SM.ep,"V"); SigmaValue = engGetVariable(SM.ep,"SigmaValue");
		lastOrth = engGetVariable(SM.ep,"Lorth");

		memcpy(eigV, mxGetPr(V),K*K*sizeof(double));
		memcpy(SM.SigmaValue, mxGetPr(SigmaValue),K*sizeof(double));
		memcpy(SM.lastOrth, mxGetPr(lastOrth),K*sizeof(double));

		mxDestroyArray(V);
		mxDestroyArray(SigmaValue);
		mxDestroyArray(lastOrth);
	}
	else
	{
	    Jacobi_Cyclic_Method_Transpose(SM.SigmaValue, eigV, SM.pca_A, K);

		Sort_Eigenvalues_Transpose(SM.SigmaValue, eigV, K, 1);  // only need to find principle?

		for(int i=0; i<K; i++)
		{
			SM.SigmaValue[i] = sqrt(SM.SigmaValue[i]/(IM.pca_window-1));
		}

		Utils::calcOrth(SM.lastOrth,eigV,K,lastVector);
	}

	for(int i=0; i<K; i++) lastVector[i] = last[i*IM.pca_window];

	/*OSTR_RESET
	ostr << "\nPCA: lastVector:\n";
	for(int i=0; i<K; i++) ostr << lastVector[i] << " ";
	ostr << "\n";
	ostr << "\nPCA: ***lastOrth array:\n";
	for(int i=0; i<K; i++) ostr << SM.lastOrth[i] << " ";
	ostr << "\n";
	IM.Logger->write_pca_log(ostr.str());
	OSTR_LOG(LOG_MSG);*/

#if 0
	if(IM.debugPCA)
	{
		//OSTR_RESET
		IBString timeOfDay = SM.globalState->getTime();

		IM.ostr << "\n*****Time: " << timeOfDay << "\n\n";
		IM.ostr << "PCA: last matrix (10 elements):\n";
		for(int i=0; i<K; i++)
		{
			for(int j=0; j<10/*IM.pca_window*/; j++) IM.ostr << last[j+i*IM.pca_window] << " ";
			IM.ostr << "\n";
		}
		//IM.Logger->write_pca_log(ostr.str());
	}
#endif

	//flip all-non-positive eigenvectors -- Is this necessary?
	for(int c=0; c<K; c++)
	{
		int sum = 0;
		for(int r=0; r<K; r++) sum += (eigV[r*K+c]<=0);
		if(sum==K) for(int r=0; r<K; r++) eigV[r*K+c] = -eigV[r*K+c];
	}

	//Utils::calcOrth(SM.pca_VV,eigV,K,lastVector);

	//////////////////////////////////////
	if(IM.debugPCA)
	{
		IBString timeOfDay = SM.globalState->getPreciseTime();

		IM.ostr << "\n*****Time (after Eigenvectors): " << timeOfDay << "\n\n";
	}
	//////////////////////////////////////

	SM.algo->afterPCA(SM.position, lastVector, eigV, SM.SigmaValue);
	IM.ostr << "after afterPCA\n";
}

