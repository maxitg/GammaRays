//
//  GRDistribution.cpp
//  GRObservations
//
//  Created by Maxim Piskunov on 07.04.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#include <cmath>
#include <algorithm>

#include <fstream>

#include "GRDistribution.h"

double GRDistribution::kolmogorovSmirnovProbability(double distance, int n1, int n2) {
    double n = (double)n1*n2/(n1+n2);
    double z = (sqrt(n) + 0.12 + 0.11/sqrt(n)) * distance;
    
    if (z == 0.) return 1.;
    if (z < 1.18) {
        double y = exp(-1.23370055013616983/pow(z,2.));
        return 1. - 2.25675833419102515*sqrt(-log(y))*(y + pow(y,9) + pow(y,25) + pow(y,49));
    } else {
        double x = exp(-2.*pow(z,2.));
        return 2.*(x - pow(x,4) + pow(x,9));
    }
}

double GRDistribution::kolmogorovSmirnovDistance(double probability, int n1, int n2) {
    double y,logy,yp,x,xp,f,ff,u,t;
    double z;
    if (probability == 1.) z = 0.;
    else if (probability > 0.3) {
        f = -0.392699081698724155*pow(1.-probability, 2.);
        
        const double ooe = 0.367879441171442322;
        double tp,up,to=0.;
        if (f < -0.2) up = log(ooe-sqrt(2*ooe*(f+ooe)));
        else up = -10.;
        do {
            up += (tp=(log(f/up)-up)*(up/(1.+up)));
            if (tp < 1.e-8 && abs(tp+to)<0.01*abs(tp)) break;
            to = tp;
        } while (abs(tp/up) > 1.e-15);
        y = exp(up);
        
        do {
            yp = y;
            logy = log(y);
            ff = f/pow(1.+ pow(y,4)+ pow(y,12), 2.);
            u = (y*logy-ff)/(1.+logy);
            y = y - (t=u/max(0.5,1.-0.5*u/(y*(1.+logy))));
        } while (abs(t/y)>1.e-15);
        z = 1.57079632679489662/sqrt(-log(y));
    } else {
        x = 0.03;
        do {
            xp = x;
            x = 0.5*probability+pow(x,4)-pow(x,9);
            if (x > 0.06) x += pow(x,16)-pow(x,25);
        } while (abs((xp-x)/x)>1.e-15);
        z = sqrt(-0.5*log(x));
    }
    double n = (double)n1*n2/(n1+n2);
    return z / (sqrt(n) + 0.12 + 0.11/sqrt(n));
}

pair<double, double> GRDistribution::cdfValueRange(double time) {
    if (time < start) return make_pair(0, 0);
    if (time > end) return make_pair(1, 1);
    
    vector<double>::iterator low = lower_bound(values.begin(), values.end(), time);
    vector<double>::iterator high = upper_bound(values.begin(), values.end(), time);
    
    pair<double, double> result = make_pair(low-values.begin(), high-values.begin());
    
    result.first += linearComponent * (time - start) / (end - start);
    result.second += linearComponent * (time - start) / (end - start);
    
    double total = values.size() + linearComponent;
    
    result.first /= total;
    result.second /= total;
    
    return result;
}

vector <struct GRDistributionCDFPoint> GRDistribution::cdf() {
    vector <double> times;
    times.reserve(values.size() + 2);
    times.push_back(start);
    for (int i = 0; i != values.size(); ++i) {
        times.push_back(values[i]);
    }
    times.push_back(end);
    
    vector <struct GRDistributionCDFPoint> result;
    result.reserve(2*values.size() + 2);
    
    for (int i = 0; i != times.size(); ++i) {
        pair<double, double> values = cdfValueRange(times[i]);
        
        GRDistributionCDFPoint lower;
        lower.value = times[i];
        lower.probability = values.first;
        result.push_back(lower);
        
        if (values.first != values.second) {
            GRDistributionCDFPoint upper;
            upper.value = times[i];
            upper.probability = values.second;
            result.push_back(upper);
        }
    }
    
    return result;
}

// this is supposed to be GeV distribution
// distribution is MeV distribution

float GRDistribution::kolmogorovSmirnovTest(GRDistribution distribution, double stretching, double *time, double *thisValue, double *distributionValue) {
    vector<GRDistributionCDFPoint> thisCDF = this->cdf();
    vector<GRDistributionCDFPoint> distributionCDF = distribution.cdf();
    
    double maxDistance = 0.;
    double maxDistanceTime = 0.;
    pair<double, double> maxDistanceRange = make_pair(0, 0);
    
    for (int i = 0; i != thisCDF.size(); ++i) {
        double time = thisCDF[i].value;
        double thisProbability = thisCDF[i].probability;
        
        pair<double, double> distributionProbabilityRange = distribution.cdfValueRange(time / stretching);
        
        if (abs(thisProbability - distributionProbabilityRange.first) > maxDistance) {
            maxDistance = abs(thisProbability - distributionProbabilityRange.first);
            maxDistanceTime = time;
            maxDistanceRange = make_pair(thisProbability, distributionProbabilityRange.first);
        }
        
        if (abs(thisProbability - distributionProbabilityRange.second) > maxDistance) {
            maxDistance = abs(thisProbability - distributionProbabilityRange.second);
            maxDistanceTime = time;
            maxDistanceRange = make_pair(thisProbability, distributionProbabilityRange.second);
        }
    }
    
    for (int i = 0; i != distributionCDF.size(); ++i) {
        double time = distributionCDF[i].value;
        double distributionProbability = distributionCDF[i].probability;
        
        pair<double, double> thisProbabilityRange = this->cdfValueRange(time * stretching);
        
        if (abs(distributionProbability - thisProbabilityRange.first) > maxDistance) {
            maxDistance = abs(distributionProbability - thisProbabilityRange.first);
            maxDistanceTime = time;
            maxDistanceRange = make_pair(thisProbabilityRange.first, distributionProbability);
        }
        
        if (abs(distributionProbability - thisProbabilityRange.second) > maxDistance) {
            maxDistance = abs(distributionProbability - thisProbabilityRange.second);
            maxDistanceTime = time;
            maxDistanceRange = make_pair(thisProbabilityRange.second, distributionProbability);
        }
    }
    
    if (time != NULL) *time = maxDistanceTime;
    if (thisValue != NULL) *thisValue = maxDistanceRange.first;
    if (distributionValue != NULL) *distributionValue = maxDistanceRange.second;
    
    return kolmogorovSmirnovProbability(maxDistance, (int)values.size(), (int)distribution.values.size());
}
