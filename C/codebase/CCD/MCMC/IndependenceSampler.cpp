/*
 * IndependenceSampler.cpp
 *
 *  Created on: Aug 6, 2012
 *      Author: trevorshaddox
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <ctime>
#include <set>

#include <math.h>

#include "IndependenceSampler.h"
#include <Eigen/Dense>
#include <Eigen/Cholesky>
#include <Eigen/Core>



#define PI	3.14159265358979323851280895940618620443274267017841339111328125

//#define Debug_TRS

namespace bsccs {

IndependenceSampler::IndependenceSampler(CyclicCoordinateDescent & ccd) {
	MHstep.initialize(ccd);
}


IndependenceSampler::~IndependenceSampler() {

}

double IndependenceSampler::getTransformedTuningValue(double tuningParameter){
	return exp(-tuningParameter);
}


void IndependenceSampler::sample(MCMCModel& model, double tuningParameter) {
	//cout << "IndependenceSampler::sample" << endl;

	model.BetaStore();
	BetaParameter & Beta = model.getBeta();
	BetaParameter & Beta_Hat = model.getBeta_Hat();
	//Eigen::LLT<Eigen::MatrixXf> choleskyEigen = model.getCholeskyLLT();

	int sizeOfSample = Beta.getSize();

	//Sampled independent normal values

	Eigen::VectorXf independentNormal = Eigen::VectorXf::Random(sizeOfSample);
	for (int i = 0; i < sizeOfSample; i++) {
		bsccs::real normalValue = generateGaussian();
		// NB: tuningParameter scales the VARIANCE
		independentNormal[i] = normalValue * std::sqrt(getTransformedTuningValue(tuningParameter)); // multiply by stdev
	}

#ifdef Debug_TRS
	cout << "Cholesky in Sampler " << endl;
	Eigen::MatrixXf CholeskyDecompL(sizeOfSample, sizeOfSample);
	CholeskyDecompL = choleskyEigen.matrixU();
	cout << CholeskyDecompL << endl;
#endif

	((model.getCholeskyLLT()).matrixU()).solveInPlace(independentNormal);

	// TODO Check marginal variance on b[i]


	for (int i = 0; i < sizeOfSample; i++) {
		Beta.set(i,independentNormal[i] + Beta_Hat.get(i));
	}

}

bool IndependenceSampler::evaluateSample(MCMCModel& model, double tuningParameter, CyclicCoordinateDescent & ccd){
	//cout << "IndependenceSampler::evaluateSample" << endl;

	bool accept = MHstep.evaluate(model);


	return(accept);
}

double IndependenceSampler::evaluateLogMHRatio(MCMCModel& model){
	//cout << "IndependenceSampler::evaluateSample" << endl;

	double logRatio = MHstep.getLogMetropolisRatio(model)*MHstep.getLogHastingsRatio(model);


	return(logRatio);
}




}
