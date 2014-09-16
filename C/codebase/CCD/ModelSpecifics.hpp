/*
 * ModelSpecifics.hpp
 *
 *  Created on: Jul 13, 2012
 *      Author: msuchard
 */

#ifndef MODELSPECIFICS_HPP_
#define MODELSPECIFICS_HPP_

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <numeric>
#include <math.h>

#include "ModelSpecifics.h"
#include "Iterators.h"
// #include "Combinations.h"

//#define USE_BIGNUM
#define USE_LONG_DOUBLE


#ifdef USE_LONG_DOUBLE
	typedef long double DDouble;
#else
	typedef double DDouble;
#endif

namespace bsccs {

//template <class BaseModel,typename WeightType>
//ModelSpecifics<BaseModel,WeightType>::ModelSpecifics(
//		const std::vector<real>& y,
//		const std::vector<real>& z) : AbstractModelSpecifics(y, z), BaseModel() {
//	// TODO Memory allocation here
//}

template <class BaseModel,typename WeightType>
ModelSpecifics<BaseModel,WeightType>::ModelSpecifics(const ModelData& input)
	: AbstractModelSpecifics(input), BaseModel() {
	// TODO Memory allocation here
}

template <class BaseModel,typename WeightType>
ModelSpecifics<BaseModel,WeightType>::~ModelSpecifics() {
	// TODO Memory release here
}

template <class BaseModel,typename WeightType>
bool ModelSpecifics<BaseModel,WeightType>::allocateXjY(void) { return BaseModel::precomputeGradient; }

template <class BaseModel,typename WeightType>
bool ModelSpecifics<BaseModel,WeightType>::allocateXjX(void) { return BaseModel::precomputeHessian; }

template <class BaseModel,typename WeightType>
bool ModelSpecifics<BaseModel,WeightType>::sortPid(void) { return BaseModel::sortPid; }


template <class BaseModel,typename WeightType>
bool ModelSpecifics<BaseModel,WeightType>::allocateNtoKIndices(void) { return BaseModel::hasNtoKIndices; }

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::setWeights(real* inWeights, bool useCrossValidation) {
	// Set K weights
	if (hKWeight.size() != K) {
		hKWeight.resize(K);
	}
	if (useCrossValidation) {
		for (int k = 0; k < K; ++k) {
			hKWeight[k] = inWeights[k];
		}
	} else {
		std::fill(hKWeight.begin(), hKWeight.end(), static_cast<WeightType>(1));
	}
	// Set N weights (these are the same for independent data models
	if (hNWeight.size() != N) {
		hNWeight.resize(N);
	}
	std::fill(hNWeight.begin(), hNWeight.end(), static_cast<WeightType>(0));
	for (int k = 0; k < K; ++k) {
		WeightType event = BaseModel::observationCount(hY[k])*hKWeight[k];
		incrementByGroup(hNWeight.data(), hPid, k, event);
	}

//	for (int n = 0; n < N; ++n) {
//		std::cout << hNWeight[n] << ", ";
//	}
//	std::cout << std::endl;
}

template<class BaseModel, typename WeightType>
void ModelSpecifics<BaseModel, WeightType>::computeXjY(bool useCrossValidation) {
	for (int j = 0; j < J; ++j) {
		hXjY[j] = 0;
				
		GenericIterator it(*hXI, j);

		if (useCrossValidation) {
			for (; it; ++it) {
				const int k = it.index();
				hXjY[j] += it.value() * hY[k] * hKWeight[k];
			}
		} else {
			for (; it; ++it) {
				const int k = it.index();
				hXjY[j] += it.value() * hY[k];
			}
		}
#ifdef DEBUG_COX
		cerr << "j: " << j << " = " << hXjY[j]<< endl;
#endif
	}
}

template<class BaseModel, typename WeightType>
void ModelSpecifics<BaseModel, WeightType>::computeXjX(bool useCrossValidation) {
	for (int j = 0; j < J; ++j) {
		hXjX[j] = 0;
		GenericIterator it(*hXI, j);

		if (useCrossValidation) {
			for (; it; ++it) {
				const int k = it.index();
				hXjX[j] += it.value() * it.value() * hKWeight[k];
			}
		} else {
			for (; it; ++it) {
				const int k = it.index();
				hXjX[j] += it.value() * it.value();
			}
		}
	}
}

template<class BaseModel, typename WeightType>
void ModelSpecifics<BaseModel, WeightType>::computeNtoKIndices(bool useCrossValidation) {

	hNtoK.resize(N+1);
	int n = 0;
	for (int k = 0; k < K;) {
		hNtoK[n] = k;
		int currentPid = hPid[k];
		do {
			++k;
		} while (k < K && currentPid == hPid[k]);
		++n;
	}
	hNtoK[n] = K;

//	for (std::vector<int>::iterator it = hNtoK.begin(); it != hNtoK.end(); ++it) {
//		std::cerr << *it << ", ";
//	}
//	std::cerr << std::endl;
//	exit(-1);
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::computeFixedTermsInLogLikelihood(bool useCrossValidation) {
	if(BaseModel::likelihoodHasFixedTerms) {
		logLikelihoodFixedTerm = 0.0;
		if(useCrossValidation) {
			for(int i = 0; i < K; i++) {
				logLikelihoodFixedTerm += BaseModel::logLikeFixedTermsContrib(hY[i], hOffs[i]) * hKWeight[i];
			}
		} else {
			for(int i = 0; i < K; i++) {
				logLikelihoodFixedTerm += BaseModel::logLikeFixedTermsContrib(hY[i], hOffs[i]);
			}
		}
	}
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::computeFixedTermsInGradientAndHessian(bool useCrossValidation) {
	if (sortPid()) {
		doSortPid(useCrossValidation);
	}
	if (allocateXjY()) {
		computeXjY(useCrossValidation);
	}
	if (allocateXjX()) {
		computeXjX(useCrossValidation);
	}
	if (allocateNtoKIndices()) {
		computeNtoKIndices(useCrossValidation);
	}
}

template <class BaseModel,typename WeightType>
double ModelSpecifics<BaseModel,WeightType>::getLogLikelihood(bool useCrossValidation) {

	real logLikelihood = static_cast<real>(0.0);
	if (useCrossValidation) {
		for (int i = 0; i < K; i++) {
			logLikelihood += BaseModel::logLikeNumeratorContrib(hY[i], hXBeta[i]) * hKWeight[i];
		}
	} else {
		for (int i = 0; i < K; i++) {
			logLikelihood += BaseModel::logLikeNumeratorContrib(hY[i], hXBeta[i]);
		}
	}

	if (BaseModel::likelihoodHasDenominator) { // Compile-time switch
		if(BaseModel::cumulativeGradientAndHessian) {
			for (int i = 0; i < N; i++) {
				// Weights modified in computeNEvents()
				logLikelihood -= BaseModel::logLikeDenominatorContrib(hNWeight[i], accDenomPid[i]);
			}
		} else {  // TODO Unnecessary code duplication

			for (int i = 0; i < N; i++) {
				// Weights modified in computeNEvents()
				logLikelihood -= BaseModel::logLikeDenominatorContrib(hNWeight[i], denomPid[i]);
			}
			/*
			double fakegradient = 0;
			double fakehessian = 123456;
			//computeGradientAndHessianImpl<DenseIterator>(0, &fakegradient, &fakehessian, unweighted);
			logLikelihood -= fakegradient;
			*/
		}
	}

	if (BaseModel::likelihoodHasFixedTerms) {
		logLikelihood += logLikelihoodFixedTerm;
	}

	return static_cast<double>(logLikelihood);
}

template <class BaseModel,typename WeightType>
double ModelSpecifics<BaseModel,WeightType>::getPredictiveLogLikelihood(real* weights) {
	real logLikelihood = static_cast<real>(0.0);

	if(BaseModel::cumulativeGradientAndHessian)	{
		for (int k = 0; k < K; ++k) {
			logLikelihood += BaseModel::logPredLikeContrib(hY[k], weights[k], hXBeta[k], &accDenomPid[0], hPid, k);
		}
	} else { // TODO Unnecessary code duplication
		for (int k = 0; k < K; ++k) {
			logLikelihood += BaseModel::logPredLikeContrib(hY[k], weights[k], hXBeta[k], denomPid, hPid, k);
		}
	}
	return static_cast<double>(logLikelihood);
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::getPredictiveEstimates(real* y, real* weights){

	// TODO Check with SM: the following code appears to recompute hXBeta at large expense
//	std::vector<real> xBeta(K,0.0);
//	for(int j = 0; j < J; j++){
//		GenericIterator it(*hXI, j);
//		for(; it; ++it){
//			const int k = it.index();
//			xBeta[k] += it.value() * hBeta[j] * weights[k];
//		}
//	}
	if (weights) {
		for (int k = 0; k < K; ++k) {
			if (weights[k]) {
				BaseModel::predictEstimate(y[k], hXBeta[k]);
			}
		}
	} else {
		for (int k = 0; k < K; ++k) {
			BaseModel::predictEstimate(y[k], hXBeta[k]);
		}
	}
	// TODO How to remove code duplication above?
}

// TODO The following function is an example of a double-dispatch, rewrite without need for virtual function
template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::computeGradientAndHessian(int index, double *ogradient,
		double *ohessian, bool useWeights) {
	// Run-time dispatch, so virtual call should not effect speed
	if (useWeights) {
		switch (hXI->getFormatType(index)) {
			case INDICATOR :
				computeGradientAndHessianImpl<IndicatorIterator>(index, ogradient, ohessian, weighted);
				break;
			case SPARSE :
				computeGradientAndHessianImpl<SparseIterator>(index, ogradient, ohessian, weighted);
				break;
			case DENSE :
				computeGradientAndHessianImpl<DenseIterator>(index, ogradient, ohessian, weighted);
				break;
			case INTERCEPT :
				computeGradientAndHessianImpl<InterceptIterator>(index, ogradient, ohessian, weighted);
				break;
		}
	} else {
		switch (hXI->getFormatType(index)) {
			case INDICATOR :
				computeGradientAndHessianImpl<IndicatorIterator>(index, ogradient, ohessian, unweighted);
				break;
			case SPARSE :
				computeGradientAndHessianImpl<SparseIterator>(index, ogradient, ohessian, unweighted);
				break;
			case DENSE :
				computeGradientAndHessianImpl<DenseIterator>(index, ogradient, ohessian, unweighted);
				break;
			case INTERCEPT :
				computeGradientAndHessianImpl<InterceptIterator>(index, ogradient, ohessian, unweighted);
				break;
		}
	}
}

class bigNum {
public:
	int exp;
	double frac;
	union DoubleInt {
		unsigned i[2];
		double d;
	};
	bigNum (double a) {
		frac = frexp(a,&exp);
	}
	bigNum (double f, int e) {
		frac = f;
		exp = e;
	}
	static bigNum mul (bigNum a, bigNum b) {
		if (a.isZero() || b.isZero()) {
			return bigNum(0);
		}
		int e = 0;
		double f = a.frac * b.frac;
		if (f < 0.5) {
			e = -1;
			f = 2*f;
		}
		e = e + a.exp + b.exp;
		return bigNum(f,e);
	}
	static bigNum mul (double a, bigNum b) {
		if (a==0 || b.isZero()) {
			return bigNum(0);
		}
		int e;
		double f = frexp(a * b.frac, &e);
		e = e + b.exp;
		return bigNum(f,e);
	}
	static bigNum div (bigNum a, bigNum b) {
		if (a.isZero()) {
			return bigNum(0);
		}
		int e = 0;
		double f = a.frac/b.frac;
		if (f >= 1) {
			e = 1;
			f = f/2;
		}
		e = e + a.exp - b.exp;
		return bigNum(f,e);
	}
/*
	static bigNum add (bigNum a, bigNum b) {
		int n = b.exp - a.exp;
		if (n > 100) {
			return b;
		}
		if (n < -100) {
			return a;
		}
		int e;
		double f = frexp(a.frac * pow(2, a.exp-b.exp) + b.frac, &e);
		e = e + b.exp;

		printf("%f \n", bigNum(f,e).toDouble());
		printf("%f \n", bigNum::add1(a,b).toDouble());
		printf("%f %d %f %d \n", a.frac, a.exp, b.frac, b.exp);

		return bigNum(f,e);
	}
*/

	static bigNum add(bigNum a, bigNum b) {
		int n = b.exp - a.exp;
		if (n > 100 || a.isZero()) {
			return b;
		}
		if (n < -100 || b.isZero()) {
			return a;
		}
		DoubleInt x;
		x.d = b.frac;
		x.i[1] = (((x.i[1]>>20)+n)<<20)|((x.i[1]<<12)>>12);
		int e;
		double f = frexp(x.d + a.frac, &e);
		e = e + a.exp;
		return bigNum(f,e);
	}

	bool isZero() {
		return (frac==0 && exp==0);
	}


	double toDouble() {
		return frac*pow(2,exp);
	}
};


#ifdef USE_BIGNUM

template <typename UIteratorType, typename SparseIteratorType>
std::vector<bigNum> computeHowardRecursion(UIteratorType itExpXBeta, SparseIteratorType itX, int numSubjects, int numCases, bsccs::real* caseOrNo) {

	// Recursion by Susanne Howard in Gail, Lubin and Rubinstein (1981)
	// Rewritten as a loop since very deep recursion can be expensive


 // bigNum code

	double caseSum = 0;

	std::vector<bigNum> B[2];
	std::vector<bigNum> dB[2];
	std::vector<bigNum> ddB[2];

	int currentB = 0;

	B[0].push_back(bigNum(1));
	B[1].push_back(bigNum(1));
	dB[0].push_back(bigNum(0));
	dB[1].push_back(bigNum(0));
	ddB[0].push_back(bigNum(0));
	ddB[1].push_back(bigNum(0));

	for (int i=1; i<=numCases; i++) {
		B[0].push_back(bigNum(0));
		B[1].push_back(bigNum(0));
		dB[0].push_back(bigNum(0));
		dB[1].push_back(bigNum(0));
		ddB[0].push_back(bigNum(0));
		ddB[1].push_back(bigNum(0));
	}
	//double maxXi = 0.0;
	//double maxSorted = 0.0;
	bigNum temp(0);
	//std::vector<double> sortXi;
	for (int n=1; n<= numSubjects; n++) {
		for (int m=std::max(1,n+numCases-numSubjects); m<=std::min(n,numCases);m++) {
			  B[!currentB][m] = bigNum::add(B[currentB][m], bigNum::mul(*itExpXBeta, B[currentB][m-1]));
			//B[!currentB][m] =   B[currentB][m] + (*itExpXBeta) *   B[currentB][m-1];

			  	 temp = bigNum::add(dB[currentB][m], bigNum::mul(*itExpXBeta, dB[currentB][m-1]));
			 dB[!currentB][m] = bigNum::add(temp, bigNum::mul((*itX)*(*itExpXBeta), B[currentB][m-1]));
			//dB[!currentB][m] =  dB[currentB][m] + (*itExpXBeta) *  dB[currentB][m-1] + (*itX)*(*itExpXBeta)*B[currentB][m-1];

			 	 temp = bigNum::add(ddB[currentB][m], bigNum::mul(*itExpXBeta, ddB[currentB][m-1]));
			 	 temp = bigNum::add(temp, bigNum::mul((*itX)*(*itX)*(*itExpXBeta), B[currentB][m-1]));
			ddB[!currentB][m] = bigNum::add(temp, bigNum::mul(2*(*itX)*(*itExpXBeta), dB[currentB][m-1]));
			//ddB[!currentB][m] = ddB[currentB][m] + (*itExpXBeta) * ddB[currentB][m-1] + (*itX)*(*itX)*(*itExpXBeta)*B[currentB][m-1] + 2*(*itX)*(*itExpXBeta)*dB[currentB][m-1];
		}
		if (caseOrNo[n-1] == 1) {
			caseSum += (*itX);
		}

		//if (*itX > maxXi) {
		//	maxXi = *itX;
		//}
		//sortXi.push_back(*itX);

		currentB = !currentB;
		++itExpXBeta; ++itX;

	}

	//std::sort (sortXi.begin(), sortXi.end());
	//for (int i=1; i<=numCases; i++) {
	//	maxSorted += sortXi[numSubjects-i];
	//}
	//maxXi = maxXi * numCases;


	std::vector<bigNum> result;
	result.push_back(B[currentB][numCases]);
	result.push_back(dB[currentB][numCases]);
	result.push_back(ddB[currentB][numCases]);
	result.push_back(bigNum(caseSum));
	//result.push_back(bigNum(maxXi));
	//result.push_back(maxSorted);

	return result;
	
}

#else
template <typename UIteratorType, typename SparseIteratorType>
std::vector<DDouble> computeHowardRecursion(UIteratorType itExpXBeta, SparseIteratorType itX, int numSubjects, int numCases, bsccs::real* caseOrNo) {

	
 // Normal Code
	DDouble caseSum = 0;

	std::vector<DDouble> B[2];
	std::vector<DDouble> dB[2];
	std::vector<DDouble> ddB[2];

	int currentB = 0;

	B[0].push_back(1);
	B[1].push_back(1);
	dB[0].push_back(0);
	dB[1].push_back(0);
	ddB[0].push_back(0);
	ddB[1].push_back(0);

	for (int i=1; i<=numCases; i++) {
		B[0].push_back(0);
		B[1].push_back(0);
		dB[0].push_back(0);
		dB[1].push_back(0);
		ddB[0].push_back(0);
		ddB[1].push_back(0);
	}
	//double maxXi = 0.0;
	//double maxSorted = 0.0;
	std::vector<DDouble> sortXi;
	for (int n=1; n<= numSubjects; n++) {
		for (int m=std::max(1,n+numCases-numSubjects); m<=std::min(n,numCases);m++) {
			  B[!currentB][m] =   B[currentB][m] + (*itExpXBeta) *   B[currentB][m-1];
			 dB[!currentB][m] =  dB[currentB][m] + (*itExpXBeta) *  dB[currentB][m-1] + (*itX)*(*itExpXBeta)*B[currentB][m-1];
			ddB[!currentB][m] = ddB[currentB][m] + (*itExpXBeta) * ddB[currentB][m-1] + (*itX)*(*itX)*(*itExpXBeta)*B[currentB][m-1] + 2*(*itX)*(*itExpXBeta)*dB[currentB][m-1];
		}
		if (caseOrNo[n-1] == 1) {
			caseSum += (*itX);
		}

		//if (*itX > maxXi) {
		//	maxXi = *itX;
		//}
		//sortXi.push_back(*itX);

		currentB = !currentB;
		++itExpXBeta; ++itX;

	}

	//std::sort (sortXi.begin(), sortXi.end());
	//for (int i=1; i<=numCases; i++) {
	//	maxSorted += sortXi[numSubjects-i];
	//}
	//maxXi = maxXi * numCases;


	std::vector<DDouble> result;
	result.push_back(B[currentB][numCases]);
	result.push_back(dB[currentB][numCases]);
	result.push_back(ddB[currentB][numCases]);
	result.push_back(caseSum);
	//result.push_back(maxXi);
	//result.push_back(maxSorted);

	return result;

}
#endif

template <class BaseModel,typename WeightType> template <class IteratorType, class Weights>
void ModelSpecifics<BaseModel,WeightType>::computeGradientAndHessianImpl(int index, double *ogradient, double *ohessian, Weights w) {
	real gradient = static_cast<real>(0);
	real hessian = static_cast<real>(0);

	IteratorType it(*(*sparseIndices)[index], N); // TODO How to create with different constructor signatures?

	if (BaseModel::cumulativeGradientAndHessian) { // Compile-time switch
		
		real accNumerPid  = static_cast<real>(0);
		real accNumerPid2 = static_cast<real>(0);

		// This is an optimization point compared to iterating over a completely dense view:  
		// a) the view below starts at the first non-zero entry
		// b) we only access numerPid and numerPid2 for non-zero entries 
		// This may save time; should document speed-up in massive Cox manuscript
		
		for (; it; ) {
			int k = it.index();
			if(w.isWeighted){ //if useCrossValidation
				accNumerPid  += numerPid[BaseModel::getGroup(hPid, k)] * hKWeight[k]; // TODO Only works when X-rows are sorted as well
				accNumerPid2 += numerPid2[BaseModel::getGroup(hPid, k)] * hKWeight[k];
			} else { // TODO Unnecessary code duplication
				accNumerPid  += numerPid[BaseModel::getGroup(hPid, k)]; // TODO Only works when X-rows are sorted as well
				accNumerPid2 += numerPid2[BaseModel::getGroup(hPid, k)];
			}
#ifdef DEBUG_COX
			cerr << "w: " << k << " " << hNWeight[k] << " " << numerPid[BaseModel::getGroup(hPid, k)] << ":" <<
					accNumerPid << ":" << accNumerPid2 << ":" << accDenomPid[BaseModel::getGroup(hPid, k)];
#endif			
			// Compile-time delegation
			BaseModel::incrementGradientAndHessian(it,
					w, // Signature-only, for iterator-type specialization
					&gradient, &hessian, accNumerPid, accNumerPid2,
					accDenomPid[BaseModel::getGroup(hPid, k)], hNWeight[k], it.value(), hXBeta[k], hY[k]); // When function is in-lined, compiler will only use necessary arguments
#ifdef DEBUG_COX		
			cerr << " -> g:" << gradient << " h:" << hessian << endl;	
#endif
			++it;
			
			if (IteratorType::isSparse) {
				const int next = it ? it.index() : N;
				for (++k; k < next; ++k) {
#ifdef DEBUG_COX
			cerr << "q: " << k << " " << hNWeight[k] << " " << 0 << ":" <<
					accNumerPid << ":" << accNumerPid2 << ":" << accDenomPid[BaseModel::getGroup(hPid, k)];
#endif			
					
					BaseModel::incrementGradientAndHessian(it,
							w, // Signature-only, for iterator-type specialization
							&gradient, &hessian, accNumerPid, accNumerPid2,
							accDenomPid[BaseModel::getGroup(hPid, k)], hNWeight[k], static_cast<real>(0), hXBeta[k], hY[k]); // When function is in-lined, compiler will only use necessary arguments
#ifdef DEBUG_COX		
			cerr << " -> g:" << gradient << " h:" << hessian << endl;	
#endif
					
				}						
			}
		}
	} else {

		for (; it; ++it) {
			const int n = it.index();

			if (true && hNWeight[n] > 0) {
				int numSubjects = hNtoK[n+1] - hNtoK[n];
				int numCases = hNWeight[n];
#if 0
				real* x = hXI->getDataVector(index) + hNtoK[n];
#else
				DenseView<IteratorType> x(IteratorType(*hXI, index), hNtoK[n], hNtoK[n+1]);
#endif

#ifdef USE_BIGNUM
				std::vector<bigNum> value = computeHowardRecursion(offsExpXBeta + hNtoK[n], x, numSubjects, numCases, hY + hNtoK[n]);
#else
				std::vector<DDouble> value = computeHowardRecursion(offsExpXBeta + hNtoK[n], x, numSubjects, numCases, hY + hNtoK[n]);
#endif

				/*
				if (hessian == 123456) {
					gradient += log(value[0]);
					//gradient += log(value[0].toDouble());
				}
				else {
				*/


				//MM
				//real banana = (real)((value[1]*value[1])/(value[0]*value[0]) - value[4]*value[4]);
				//real banana = (real)(pow(bigNum::div(value[1],value[0]).toDouble(), 2) - pow(value[4].toDouble(),2));

#ifdef USE_BIGNUM
				//bigNum
				gradient += (real)(value[3].toDouble() - bigNum::div(value[1],value[0]).toDouble());
				hessian += (real)(pow(bigNum::div(value[1],value[0]).toDouble(), 2) - bigNum::div(value[2],value[0]).toDouble());
#else				
				//normal
				gradient += (real)(value[3] - value[1]/value[0]);
				hessian += (real)((value[1]*value[1])/(value[0]*value[0]) - value[2]/value[0]);				
#endif
			} else {

				// Compile-time delegation

				BaseModel::incrementGradientAndHessian(it,
						w, // Signature-only, for iterator-type specialization
						&gradient, &hessian, numerPid[n], numerPid2[n],
						denomPid[n], hNWeight[n], it.value(), hXBeta[n], hY[n]); // When function is in-lined, compiler will only use necessary arguments
			}
		}
	}
/*
	if (BaseModel::precomputeGradient) { // Compile-time switch
		gradient -= hXjY[index];
	}


	if (BaseModel::precomputeHessian) { // Compile-time switch
		hessian += static_cast<real>(2.0) * hXjX[index];
	}
	*/

	*ogradient = static_cast<double>(gradient);
	*ohessian = static_cast<double>(hessian);
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::computeFisherInformation(int indexOne, int indexTwo,
		double *oinfo, bool useWeights) {

	if (useWeights) {
		std::cerr << "Weights are not yet implemented in Fisher Information calculations" << std::endl;
		exit(-1);

	} else { // no weights
		switch (hXI->getFormatType(indexOne)) {
			case INDICATOR :
				dispatchFisherInformation<IndicatorIterator>(indexOne, indexTwo, oinfo, weighted);
				break;
			case SPARSE :
				dispatchFisherInformation<SparseIterator>(indexOne, indexTwo, oinfo, weighted);
				break;
			case DENSE :
				dispatchFisherInformation<DenseIterator>(indexOne, indexTwo, oinfo, weighted);
				break;
			case INTERCEPT :
				dispatchFisherInformation<InterceptIterator>(indexOne, indexTwo, oinfo, weighted);
				break;
		}
	}
}

template <class BaseModel, typename WeightType> template <typename IteratorTypeOne, class Weights>
void ModelSpecifics<BaseModel,WeightType>::dispatchFisherInformation(int indexOne, int indexTwo, double *oinfo, Weights w) {
	switch (hXI->getFormatType(indexTwo)) {
		case INDICATOR :
			computeFisherInformationImpl<IteratorTypeOne,IndicatorIterator>(indexOne, indexTwo, oinfo, w);
			break;
		case SPARSE :
			computeFisherInformationImpl<IteratorTypeOne,SparseIterator>(indexOne, indexTwo, oinfo, w);
			break;
		case DENSE :
			computeFisherInformationImpl<IteratorTypeOne,DenseIterator>(indexOne, indexTwo, oinfo, w);
			break;
		case INTERCEPT :
			computeFisherInformationImpl<IteratorTypeOne,InterceptIterator>(indexOne, indexTwo, oinfo, w);
			break;
	}
//	std::cerr << "End of dispatch" << std::endl;
}


template<class BaseModel, typename WeightType> template<class IteratorType>
SparseIterator ModelSpecifics<BaseModel, WeightType>::getSubjectSpecificHessianIterator(int index) {

	if (hessianSparseCrossTerms.find(index) == hessianSparseCrossTerms.end()) {
		// Make new
		std::vector<int>* indices = new std::vector<int>();
		std::vector<real>* values = new std::vector<real>();
		CompressedDataColumn* column = new CompressedDataColumn(indices, values,
				SPARSE);
		hessianSparseCrossTerms.insert(std::make_pair(index, column));

		IteratorType itCross(*hXI, index);
		for (; itCross;) {
			real value = 0.0;
			int currentPid = hPid[itCross.index()];
			do {
				const int k = itCross.index();
				value += BaseModel::gradientNumeratorContrib(itCross.value(),
						offsExpXBeta[k], hXBeta[k], hY[k]);
				++itCross;
			} while (itCross && currentPid == hPid[itCross.index()]);
			indices->push_back(currentPid);
			values->push_back(value);
		}
	}
	return SparseIterator(*hessianSparseCrossTerms[index]);

}


template <class BaseModel, typename WeightType> template <class IteratorTypeOne, class IteratorTypeTwo, class Weights>
void ModelSpecifics<BaseModel,WeightType>::computeFisherInformationImpl(int indexOne, int indexTwo, double *oinfo, Weights w) {

	IteratorTypeOne itOne(*hXI, indexOne);
	IteratorTypeTwo itTwo(*hXI, indexTwo);
	PairProductIterator<IteratorTypeOne,IteratorTypeTwo> it(itOne, itTwo);

	real information = static_cast<real>(0);
	for (; it.valid(); ++it) {
		const int k = it.index();
		// Compile-time delegation

		BaseModel::incrementFisherInformation(it,
				w, // Signature-only, for iterator-type specialization
				&information,
				offsExpXBeta[k],
				0.0, 0.0, // numerPid[k], numerPid2[k], // remove
				denomPid[BaseModel::getGroup(hPid, k)],
				hKWeight[k], it.value(), hXBeta[k], hY[k]); // When function is in-lined, compiler will only use necessary arguments
	}

	if (BaseModel::hasStrataCrossTerms) {

		// Check if index is pre-computed
//#define USE_DENSE
#ifdef USE_DENSE
		if (hessianCrossTerms.find(indexOne) == hessianCrossTerms.end()) {
			// Make new
			std::vector<real> crossOneTerms(N);
			IteratorTypeOne crossOne(*hXI, indexOne);
			for (; crossOne; ++crossOne) {
				const int k = crossOne.index();
				incrementByGroup(crossOneTerms.data(), hPid, k,
						BaseModel::gradientNumeratorContrib(crossOne.value(), offsExpXBeta[k], hXBeta[k], hY[k]));
			}
			hessianCrossTerms[indexOne];
//			std::cerr << std::accumulate(crossOneTerms.begin(), crossOneTerms.end(), 0.0) << std::endl;
			hessianCrossTerms[indexOne].swap(crossOneTerms);
		}
		std::vector<real>& crossOneTerms = hessianCrossTerms[indexOne];

		// TODO Remove code duplication
		if (hessianCrossTerms.find(indexTwo) == hessianCrossTerms.end()) {
			std::vector<real> crossTwoTerms(N);
			IteratorTypeTwo crossTwo(*hXI, indexTwo);
			for (; crossTwo; ++crossTwo) {
				const int k = crossTwo.index();
				incrementByGroup(crossTwoTerms.data(), hPid, k,
						BaseModel::gradientNumeratorContrib(crossTwo.value(), offsExpXBeta[k], hXBeta[k], hY[k]));
			}
			hessianCrossTerms[indexTwo];
//			std::cerr << std::accumulate(crossTwoTerms.begin(), crossTwoTerms.end(), 0.0) << std::endl;
			hessianCrossTerms[indexTwo].swap(crossTwoTerms);
		}
		std::vector<real>& crossTwoTerms = hessianCrossTerms[indexTwo];

		// TODO Sparse loop
		real cross = 0.0;
		for (int n = 0; n < N; ++n) {
			cross += crossOneTerms[n] * crossTwoTerms[n] / (denomPid[n] * denomPid[n]);
		}
//		std::cerr << cross << std::endl;
		information -= cross;
#else
		SparseIterator sparseCrossOneTerms = getSubjectSpecificHessianIterator<IteratorTypeOne>(indexOne);
		SparseIterator sparseCrossTwoTerms = getSubjectSpecificHessianIterator<IteratorTypeTwo>(indexTwo);
		PairProductIterator<SparseIterator,SparseIterator> itSparseCross(sparseCrossOneTerms, sparseCrossTwoTerms);

		real sparseCross = 0.0;
		for (; itSparseCross.valid(); ++itSparseCross) {
			const int n = itSparseCross.index();
			sparseCross += itSparseCross.value() / (denomPid[n] * denomPid[n]);
		}
		information -= sparseCross;
#endif
	}

	*oinfo = static_cast<double>(information);
}



template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::computeNumeratorForGradient(int index) {
	// Run-time delegation
	switch (hXI->getFormatType(index)) {
		case INDICATOR : {
			IndicatorIterator it(*(*sparseIndices)[index]);
			for (; it; ++it) { // Only affected entries
				numerPid[it.index()] = static_cast<real>(0.0);
			}
			incrementNumeratorForGradientImpl<IndicatorIterator>(index);
			}
			break;
		case SPARSE : {
			SparseIterator it(*(*sparseIndices)[index]);
			for (; it; ++it) { // Only affected entries
				numerPid[it.index()] = static_cast<real>(0.0);
				if (BaseModel::hasTwoNumeratorTerms) { // Compile-time switch
					numerPid2[it.index()] = static_cast<real>(0.0); // TODO Does this invalid the cache line too much?
				}
			}
			incrementNumeratorForGradientImpl<SparseIterator>(index); }
			break;
		case DENSE :
			zeroVector(numerPid, N);
			if (BaseModel::hasTwoNumeratorTerms) { // Compile-time switch
				zeroVector(numerPid2, N);
			}
			incrementNumeratorForGradientImpl<DenseIterator>(index);
			break;
		case INTERCEPT :
			zeroVector(numerPid, N);
			if (BaseModel::hasTwoNumeratorTerms) { // Compile-time switch
				zeroVector(numerPid2, N);
			}
			incrementNumeratorForGradientImpl<InterceptIterator>(index);
			break;
		default :
			// throw error
			exit(-1);
	}
}

template <class BaseModel,typename WeightType> template <class IteratorType>
void ModelSpecifics<BaseModel,WeightType>::incrementNumeratorForGradientImpl(int index) {
	IteratorType it(*hXI, index);
	for (; it; ++it) {
		const int k = it.index();
		incrementByGroup(numerPid, hPid, k,
				BaseModel::gradientNumeratorContrib(it.value(), offsExpXBeta[k], hXBeta[k], hY[k]));
		if (!IteratorType::isIndicator && BaseModel::hasTwoNumeratorTerms) {
			incrementByGroup(numerPid2, hPid, k,
					BaseModel::gradientNumerator2Contrib(it.value(), offsExpXBeta[k]));
		}
		
#ifdef DEBUG_COX			
//			if (numerPid[BaseModel::getGroup(hPid, k)] > 0 && numerPid[BaseModel::getGroup(hPid, k)] < 1e-40) {
				cerr << "Increment" << endl;
				cerr << "hPid = " << hPid << ", k = " << k << ", index = " << BaseModel::getGroup(hPid, k) << endl;
				cerr << BaseModel::gradientNumeratorContrib(it.value(), offsExpXBeta[k], hXBeta[k], hY[k]) <<  " "
				<< it.value() << " " << offsExpXBeta[k] << " " << hXBeta[k] << " " << hY[k] << endl;
//				exit(-1);
//			}
#endif		
		
		
		
	}
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::updateXBeta(real realDelta, int index, bool useWeights) {
	// Run-time dispatch to implementation depending on covariate FormatType
	switch(hXI->getFormatType(index)) {
		case INDICATOR :
			updateXBetaImpl<IndicatorIterator>(realDelta, index, useWeights);
			break;
		case SPARSE :
			updateXBetaImpl<SparseIterator>(realDelta, index, useWeights);
			break;
		case DENSE :
			updateXBetaImpl<DenseIterator>(realDelta, index, useWeights);
			break;
		case INTERCEPT :
			updateXBetaImpl<InterceptIterator>(realDelta, index, useWeights);
			break;
		default :
			// throw error
			exit(-1);
	}
}

template <class BaseModel,typename WeightType> template <class IteratorType>
inline void ModelSpecifics<BaseModel,WeightType>::updateXBetaImpl(real realDelta, int index, bool useWeights) {
	IteratorType it(*hXI, index);

	for (; it; ++it) {
		const int k = it.index();
		hXBeta[k] += realDelta * it.value(); // TODO Check optimization with indicator and intercept
		// Update denominators as well
		if (BaseModel::likelihoodHasDenominator) { // Compile-time switch
			real oldEntry = offsExpXBeta[k];
			real newEntry = offsExpXBeta[k] = BaseModel::getOffsExpXBeta(hOffs, hXBeta[k], hY[k], k);
			incrementByGroup(denomPid, hPid, k, (newEntry - oldEntry));
		}
	}
	computeAccumlatedNumerDenom(useWeights);
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::computeRemainingStatistics(bool useWeights) {
	if (BaseModel::likelihoodHasDenominator) {
		fillVector(denomPid, N, BaseModel::getDenomNullValue());
		if (true) {
			for (int k = 0; k < K; ++k) {
				offsExpXBeta[k] = BaseModel::getOffsExpXBeta(hOffs, hXBeta[k], hY[k], k);
				incrementByGroup(denomPid, hPid, k, offsExpXBeta[k]);
			}
			computeAccumlatedNumerDenom(useWeights);
		} else { // CLR
			std::cerr << "Got here!" << std::endl;

			typedef std::vector<int> IndicesType;

			IndicesType indices;
			for (int k = 0; k < K; ++k) {
				indices.push_back(k);
			}

			for (int k = 0; k < K; ) {
				real value = 0.0;
				int currentPid = hPid[k];
				int cases = hNWeight[currentPid];
				if (cases == 1) {
					do {
						offsExpXBeta[k] = BaseModel::getOffsExpXBeta(hOffs, hXBeta[k], hY[k], k);
						incrementByGroup(denomPid, hPid, k, offsExpXBeta[k]);
						++k;
					} while (k < K && currentPid == hPid[k]);
				} else { // multiple cases
					int count = 0;
					int kk = k;
					do { // count rows in stratum
						offsExpXBeta[kk] = BaseModel::getOffsExpXBeta(hOffs, hXBeta[kk], hY[kk], kk);
						++count;
						++kk;
					} while (kk < K && currentPid == hPid[kk]);
					std::cerr << cases << " " << currentPid << " " << count << std::endl;

					IndicesType::iterator itBegin = indices.begin() + k;
					IndicesType::iterator itEnd = itBegin + count;

					struct F {
						real value;
						real* offsExpXBeta;
						real* hXBeta;
						real* hY;
						F(real* ptr1, real* ptr2, real* ptr3) : value(0.0), offsExpXBeta(ptr1), hXBeta(ptr2), hY(ptr3) {}
						bool operator()(IndicesType::iterator begin, IndicesType::iterator end) {
//							for (; begin != end; ++begin) {
//								std::cout << *begin << ", ";
//							}
							real tmp = 0.0;
							for (; begin != end; ++begin) {
								tmp += hXBeta[*begin];
							}
//							std::cout << std::endl;
							value += std::exp(tmp);
							return false;
						}
					};

					F f(offsExpXBeta, hXBeta, hY);
// 					f = for_each_combination(itBegin, itBegin + /*cases*/ 1, itEnd, f);
					denomPid[currentPid] = f.value;
					std::cerr << "denom = " << f.value << std::endl;
					k = kk; // continue outer loop after stratum
				}
//				if (nEvents > 1) {
//					std::cerr << nEvents << " " << currentPid << std::endl;
//				}
//				do {
//					offsExpXBeta[k] = BaseModel::getOffsExpXBeta(hOffs, hXBeta[k], hY[k], k);
//					incrementByGroup(denomPid, hPid, k, offsExpXBeta[k]);
//					++k;
//				} while (k < K && currentPid == hPid[k]);
			}
		}
	}
#ifdef DEBUG_COX
	cerr << "Done with initial denominators" << endl;

	for (int k = 0; k < K; ++k) {
		cerr << denomPid[k] << " " << accDenomPid[k] << " " << numerPid[k] << endl;
	}
#endif
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::computeAccumlatedNumerDenom(bool useWeights) {

	if (BaseModel::likelihoodHasDenominator && //The two switches should ideally be separated
		BaseModel::cumulativeGradientAndHessian) { // Compile-time switch
			if (accDenomPid.size() != K) {
				accDenomPid.resize(K, static_cast<real>(0));
			}
			if (accNumerPid.size() != K) {
				accNumerPid.resize(K, static_cast<real>(0));
			}
			if (accNumerPid2.size() != K) {
				accNumerPid2.resize(K, static_cast<real>(0));
			}

			// prefix-scan
			if(useWeights) { 
				//accumulating separately over train and validation sets
				real totalDenomTrain = static_cast<real>(0);
				real totalNumerTrain = static_cast<real>(0);
				real totalNumer2Train = static_cast<real>(0);
				real totalDenomValid = static_cast<real>(0);
				real totalNumerValid = static_cast<real>(0);
				real totalNumer2Valid = static_cast<real>(0);
				for (int k = 0; k < K; ++k) {
					if(hKWeight[k] == 1.0){
						totalDenomTrain += denomPid[k];
						totalNumerTrain += numerPid[k];
						totalNumer2Train += numerPid2[k];
						accDenomPid[k] = totalDenomTrain;
						accNumerPid[k] = totalNumerTrain;
						accNumerPid2[k] = totalNumer2Train;
					} else {
						totalDenomValid += denomPid[k];
						totalNumerValid += numerPid[k];
						totalNumer2Valid += numerPid2[k];
						accDenomPid[k] = totalDenomValid;
						accNumerPid[k] = totalNumerValid;
						accNumerPid2[k] = totalNumer2Valid;
					}
				}
			} else {
				real totalDenom = static_cast<real>(0);
				real totalNumer = static_cast<real>(0);
				real totalNumer2 = static_cast<real>(0);
				for (int k = 0; k < K; ++k) {
					totalDenom += denomPid[k];
					totalNumer += numerPid[k];
					totalNumer2 += numerPid2[k];
					accDenomPid[k] = totalDenom;
					accNumerPid[k] = totalNumer;
					accNumerPid2[k] = totalNumer2;
#ifdef DEBUG_COX
					cerr << denomPid[k] << " " << accDenomPid[k] << " (beta)" << endl;
#endif
				}

			}
	}
}

template <class BaseModel,typename WeightType>
void ModelSpecifics<BaseModel,WeightType>::doSortPid(bool useCrossValidation) {
/* For Cox model:
 *
 * We currently assume that hZ[k] are sorted in decreasing order by k.
 *
 */

//	cerr << "Copying Y" << endl;
//	// Copy y; only necessary if non-unique values in oY
//	nY.reserve(oY.size());
//	std::copy(oY.begin(),oY.end(),back_inserter(nY));
//	hY = const_cast<real*>(nY.data());

//	cerr << "Sorting PIDs" << endl;
//
//	std::vector<int> inverse_ranks;
//	inverse_ranks.reserve(K);
//	for (int i = 0; i < K; ++i) {
//		inverse_ranks.push_back(i);
//	}
//
//	std::sort(inverse_ranks.begin(), inverse_ranks.end(),
//			CompareSurvivalTuples<WeightType>(useCrossValidation, hKWeight, oZ));
//
//	nPid.resize(K, 0);
//	for (int i = 0; i < K; ++i) {
//		nPid[inverse_ranks[i]] = i;
//	}
//	hPid = const_cast<int*>(nPid.data());

//	for (int i = 0; i < K; ++i) {
//		cerr << oZ[inverse_ranks[i]] << endl;
//	}
//
//	cerr << endl;
//
//	for (int i = 0; i < K; ++i) {
//		cerr << oZ[i] << "\t" << hPid[i] << endl;
//	}
//
//	cerr << endl;
//
//	for (int i = 0; i < K; ++i) {
//		cerr << i << " -> " << hPid[i] << endl;
//	}
//
}

} // namespace

#endif /* MODELSPECIFICS_HPP_ */