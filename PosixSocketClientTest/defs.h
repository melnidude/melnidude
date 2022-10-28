#ifndef _DEFS_H_
#define _DEFS_H_

#define BILLION  1000000000L;
const double INFINITY_D = 1E16;
const double INFINITY16 = 0x7FFFFFFF;
const double VERY_LARGE = 1000000;
const double TINY = 1E-10;
const int MAX_REQ_ID = 200;
const int MAX_ORDERS_IN_BOOK = 10;

#define LOCAL_NEUTRALIZATION 1
#define NEUT_STOP 0
#define ALGO2 0
#define COST_AVE_MEDIUM 0
#define NEUTR 0
#define GREEDY 1
#define _DEBUG 0
#define COMBO 0
#define MIDPOINT_PCA 1
#define MIN_NORM_FACTOR 0.8
#define OLD_MM_ALGO 1

#define SIGN(x) ((x) >= 0 ? 1 : -1)
#define MIN(x,y) ((x) <= (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

enum BracketHierarchy {
	B_PARENT_OF_S,
	S_PARENT_OF_B,
	LEGS_EQUAL
};

//logging
#define LOG_MSG 0
#define LOG_ERR 1
#define ENABLE_LOGGING 1

#if ENABLE_LOGGING
#define OSTR_DEF \
	std::stringstream ostr; \
	ostr.setf(std::ios::fixed); \
	ostr.precision(2);
#define OSTR_RESET \
	ostr.str("");
#define OSTR_RESET_HiPrec \
	ostr.precision(7); \
	ostr.str("");
#define OSTR_LOG(err) \
	IM.Logger->writeStr(ostr.str()); \
	cout << ostr.str();
#define OSTR_LOG2(err) \
	IM.Logger->writeStr(ostr.str()); \
	if(err) cout << ostr.str();
#else
#define OSTR_DEF
#define OSTR_RESET if(0) { std::stringstream ostr;
#define OSTR_LOG(err) }
#define OSTR_LOG2(err)
#endif
#endif //_DEFS_H_
