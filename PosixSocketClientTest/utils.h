#ifndef _UTILS_H_
#define _UTILS_H_


class Utils
{
public:
	Utils() {};
	~Utils() {};

	static double calcInnerProd(double *vec1,double *vec2,int size,int stride1,int stride2)
	{
		double prod = 0.0;
		int idx1=0, idx2=0;
		for(int i=0; i<size; i++)
		{
			prod += vec1[idx1]*vec2[idx2];
			idx1 += stride1;
			idx2 += stride2;
		}
		return prod;
	}

	static double calcInnerProd(double *vec1, double *vec2, int size)
	{
		double prod = 0.0;
		for(int i=0; i<size; i++)
		{
			prod += vec1[i]*vec2[i];
		}
		return prod;
	}

	static double calcInnerProd(double *vec1,long *vec2,int size,int stride1,int stride2)
	{
		double prod = 0.0;
		int idx1=0, idx2=0;
		for(int i=0; i<size; i++)
		{
			prod += vec1[idx1]*vec2[idx2];
			idx1 += stride1;
			idx2 += stride2;
		}
		return prod;
	}

	static double calcVectorNorm(double *v, int size)
	{
		double norm = 0.0;
		for(int i=0; i<size; i++)
		{
			norm += v[i]*v[i];
		}
		return pow(norm,0.5);
	}

	static double calcVectorNorm(long *v, int size)
	{
		double norm = 0.0;
		for(int i=0; i<size; i++)
		{
			norm += v[i]*v[i];
		}
		return pow(norm,0.5);
	}

	static void calcOrth(double *orth, double *eigV, int K, long *v)
	{
		double pp = Utils::calcInnerProd(&eigV[(K-1)*K],v,K,1,1);
		for(int i=0; i<K; i++) orth[i] = v[i] - pp*eigV[(K-1)*K+i];
	}

	static void calcOrth(double *orth, double *eigV, int K, double *v)
	{
		double pp = Utils::calcInnerProd(&eigV[(K-1)*K],v,K,1,1);
		for(int i=0; i<K; i++) orth[i] = v[i] - pp*eigV[(K-1)*K+i];
	}


private:
};


#endif //_UTILS_H_
