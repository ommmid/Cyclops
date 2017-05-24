/*
 * NewCovariatePrior.h
 *
 *  Created on: May 22, 2017
 *      Author: Marc A. Suchard
 */

#ifndef NEWCOVARIATEPRIOR_H_
#define NEWCOVARIATEPRIOR_H_

#include <memory>
#include <string>
#include <cmath>
#include <sstream>
#include <limits>

#include <iostream> // TODO Remove

#ifndef PI
#define PI	3.14159265358979323851280895940618620443274267017841339111328125
#endif

#include "CovariatePrior.h"
#include "PriorFunction.h"

namespace bsccs {
namespace priors {

class NewLaplacePrior : public CovariatePrior {
public:

    // typedef std::unique_ptr<PriorFunction> InternalPriorFunctionPtr;

    NewLaplacePrior(PriorFunctionPtr priorFunction) : CovariatePrior(),
        priorFunction(std::move(priorFunction)) {
        // Do nothing
    }

	virtual ~NewLaplacePrior() {
		// Do nothing
	}

	const std::string getDescription() const {
	    const auto locationLambda = getLocationLambda();
		std::stringstream info;
		info << "Laplace(" << locationLambda.first << ", " << locationLambda.second << ")";
		return info.str();
	}
    double logDensity(const DoubleVector& beta, const int index) const {
        const auto x = beta[index];
        const auto locationLambda = getLocationLambda();
        const auto location = locationLambda.first;
        const auto lambda = locationLambda.second;
        return std::log(0.5 * lambda) - lambda * std::abs(x - location);
    }

	bool getIsRegularized() const {
	    return true;
	}

	bool getSupportsKktSwindle() const {
		return true;
	}

	double getKktBoundary() const {
	    const auto locationLambda = getLocationLambda();
	    const auto lambda = locationLambda.second;
		return lambda;
	}

	double getDelta(GradientHessian gh, const DoubleVector& betaVector, const int index) const {

	    const auto locationLambda = getLocationLambda();
	    const double location = locationLambda.first;
		const double lambda = locationLambda.second;

		const double beta = betaVector[index] - location; // Adjust for location-shift

		double delta = 0.0;

		double neg_update = - (gh.first - lambda) / gh.second;
		double pos_update = - (gh.first + lambda) / gh.second;

		int signBetaIndex = sign(beta);

		if (signBetaIndex == 0) {

			if (neg_update < 0) {
				delta = neg_update;
			} else if (pos_update > 0) {
				delta = pos_update;
			} else {
				delta = 0;
			}
		} else { // non-zero entry

			if (signBetaIndex < 0) {
				delta = neg_update;
			} else {
				delta = pos_update;
			}

			if ( sign(beta + delta) != signBetaIndex ) {
				delta = - beta;
			}
		}
		return delta;
	}

	std::vector<VariancePtr> getVarianceParameters() const {
	    auto tmp = std::vector<VariancePtr>();
	    // tmp.push_back(variance);
		return tmp;
	}

protected:
	double convertVarianceToHyperparameter(double value) const {
		return std::sqrt(2.0 / value);
	}

	double convertHyperparameterToVariance(double value) const {
		return 2.0 / (value * value);
	}

    PriorFunction::LocationScale getLocationLambda() const {
        const auto locationScale = (*priorFunction)();
        return PriorFunction::LocationScale(locationScale.first,
                             convertHyperparameterToVariance(locationScale.second));
    }

	// double getLambda() const {
	// 	return convertVarianceToHyperparameter(*variance);
	// }

private:

    PriorFunctionPtr priorFunction;

	template <typename Vector>
	typename Vector::value_type logIndependentDensity(const Vector& vector) const {

	    const auto locationLambda = getLocationLambda();
	    const double location = locationLambda.first;
	    const double lambda = locationLambda.second;

		return vector.size() * std::log(0.5 * lambda) - lambda * oneNorm(vector, location);
	}

	template <typename Vector>
	typename Vector::value_type oneNorm(const Vector& vector, const double location) const {
		typename Vector::value_type result = 0.0;
		for (typename Vector::const_iterator it; it != vector.end(); ++it) {
			result += std::abs(*it - location);
		}
		return result;
	}

	int sign(double x) const {
		if (x == 0) {
			return 0;
		}
		if (x < 0) {
			return -1;
		}
		return 1;
	}

	// VariancePtr variance;
};

static PriorPtr makePrior(PriorType priorType, PriorFunctionPtr&& priorFunction) {
    PriorPtr prior;
    switch (priorType) {
    case NONE :
        prior = bsccs::make_shared<NoPrior>();
        break;
    case LAPLACE :
        prior = bsccs::make_shared<NewLaplacePrior>(std::move(priorFunction));
        break;
    case NORMAL :
        prior = bsccs::make_shared<NormalPrior>(1.0);
        break;
    default : break;
    }
    return prior;
}

} /* namespace priors */
} /* namespace bsccs */
#endif /* NEWCOVARIATEPRIOR_H_ */