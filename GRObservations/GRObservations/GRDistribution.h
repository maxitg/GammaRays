//
//  GRDistribution.h
//  GRObservations
//
//  Created by Maxim Piskunov on 07.04.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#ifndef __Gamma_Rays__GRDistribution__
#define __Gamma_Rays__GRDistribution__

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

struct GRDistributionCDFPoint {
    double value;
    double probability;
};

class GRDistribution {
public:
    vector <double> values;
    double estimatedLinearComponent;
    double start;
    double end;
    
private:
    double kolmogorovSmirnovProbability(double distance, int n1, int n2);
    double kolmogorovSmirnovDistance(double probability, int n1, int n2);
public:
    vector <GRDistributionCDFPoint> cdf();
    float kolmogorovSmirnovTest(GRDistribution distribution, double lengthening = 1., bool plot = false, ostream &mev = cout, ostream &gev = cout);
    pair<double, double> lengtheningLimits(GRDistribution distribution, float probability, bool *success);
};

#endif /* defined(__Gamma_Rays__GRDistribution__) */
