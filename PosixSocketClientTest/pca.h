using namespace std;

class CPCA
{
public:
	CPCA();
	~CPCA();

	void run();

private:
	int K;
	int pca_window;
	int pca_window_init;
	mxArray *L;
	mxArray *V, *SigmaValue, *lastOrth;
	double *last;
	double *lastVector;
	double *eigV;
	mxArray *ActualPCAwindow;
};


