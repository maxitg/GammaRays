//
//  GRDistribution.cpp
//  GRObservations
//
//  Created by Maxim Piskunov on 07.04.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#include <math.h>
#include <cmath>

#include <fstream>

#include <glpk.h>

#include "GRDistribution.h"

vector <GRDistributionCDFPoint> GRDistribution::cdf() {
    vector <GRDistributionCDFPoint> result;
    result.reserve(2*values.size());
    for (int i = 0; i < values.size(); i++) {
        GRDistributionCDFPoint point;
        point.value = values[i];
        point.probability = (double)i/values.size();
        result.push_back(point);
        point.value = values[i];
        point.probability = (double)(i+1)/values.size();
        result.push_back(point);
    }
    return result;
}

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

//  self is GeV distribution
//  distribution is MeV distribution
float GRDistribution::kolmogorovSmirnovTest(GRDistribution distribution, double lengthening, bool plot, ostream &mev, ostream &gev) {
    for (int i = 0; i < distribution.values.size(); i++) {
        distribution.values[i] *= lengthening;
    }
    distribution.start *= lengthening;
    distribution.end *= lengthening;
        
    double maxDistance = 0.;
    
    vector <int> selfCounts(values.size());
    vector <int> distributionCounts(distribution.values.size());
    int selfIndex = 0;
    int distributionIndex = 0;
    while (selfIndex != values.size() || distributionIndex != distribution.values.size()) {
        bool selfIncreased;
        if (selfIndex == values.size()) {
            distributionIndex++;
            selfIncreased = false;
        } else if (distributionIndex == distribution.values.size()) {
            selfIndex++;
            selfIncreased = true;
        } else if (values[selfIndex] > distribution.values[distributionIndex]) {
            distributionIndex++;
            selfIncreased = false;
        } else {
            selfIndex++;
            selfIncreased = true;
        }
        
        if (selfIncreased) {
            selfCounts[selfIndex-1] = distributionIndex;
        } else {
            distributionCounts[distributionIndex-1] = selfIndex;
        }
    }
        
    double descretePart = (double)distribution.values.size() / (distribution.values.size() + estimatedLinearComponent - distribution.estimatedLinearComponent);
    double selfPart = estimatedLinearComponent / (distribution.values.size() + estimatedLinearComponent - distribution.estimatedLinearComponent);
    double distributionPart = -distribution.estimatedLinearComponent / (distribution.values.size() + estimatedLinearComponent - distribution.estimatedLinearComponent);
    
    vector <pair<double, double> > mevPoints;
    vector <pair<double, double> > gevPoints;
    
    double bestSelfPoint, bestDistributionPoint, bestValue;
    for (int i = 0; i < values.size(); i++) {
        bool selfInProgress = (values[i] >= start) && (values[i] <= end);
        bool distributionInProgress = (values[i] >= distribution.start) && (values[i] <= distribution.end);
                
        double selfPoint = (double)i / values.size();
        double distributionPoint = descretePart * (double)selfCounts[i] / distribution.values.size();
        if (selfInProgress) distributionPoint += selfPart * (values[i] - start)/(end - start);
        if (distributionInProgress) distributionPoint += distributionPart * (values[i] - distribution.start) / (distribution.end - distribution.start);
        
        if (plot) gevPoints.push_back(make_pair(values[i], selfPoint));
        if (plot) mevPoints.push_back(make_pair(values[i], distributionPoint));
        
        maxDistance = max(maxDistance, abs(selfPoint - distributionPoint));
        
        selfPoint = (double)(i+1) / values.size();
        if (plot) gevPoints.push_back(make_pair(values[i], selfPoint));
        
        if (maxDistance < abs(selfPoint - distributionPoint)) {
            maxDistance = abs(selfPoint - distributionPoint);
            bestSelfPoint = selfPoint;
            bestDistributionPoint = distributionPoint;
            bestValue = values[i];
        }
    }
    
    for (int i = 0; i < distribution.values.size(); i++) {
        bool selfInProgress = (distribution.values[i] >= start) && (distribution.values[i] <= end);
        bool distributionInProgress = (distribution.values[i] >= distribution.start) && (distribution.values[i] <= distribution.end);
        
        double selfPoint = (double)distributionCounts[i] / values.size();
        double distributionPoint = descretePart * (double)i / distribution.values.size();
        if (selfInProgress) distributionPoint += selfPart * (distribution.values[i] - start)/(end - start);
        if (distributionInProgress) distributionPoint += distributionPart * (distribution.values[i] - distribution.start) / (distribution.end - distribution.start);
        
        if (plot) gevPoints.push_back(make_pair(distribution.values[i], selfPoint));
        if (plot) mevPoints.push_back(make_pair(distribution.values[i], distributionPoint));

        maxDistance = max(maxDistance, abs(selfPoint - distributionPoint));

        distributionPoint = descretePart * (double)(i+1) / distribution.values.size();
        if (selfInProgress) distributionPoint += selfPart * (distribution.values[i] - start)/(end - start);
        if (distributionInProgress) distributionPoint += distributionPart * (distribution.values[i] - distribution.start) / (distribution.end - distribution.start);
        
        if (plot) mevPoints.push_back(make_pair(distribution.values[i], distributionPoint));
        
        if (maxDistance < abs(selfPoint - distributionPoint)) {
            maxDistance = abs(selfPoint - distributionPoint);
            bestSelfPoint = selfPoint;
            bestDistributionPoint = distributionPoint;
            bestValue = distribution.values[i];
        }
    }
    
    if (plot) {
        cout << lengthening << ": " << bestValue << " " << bestSelfPoint << " " << bestDistributionPoint << endl;
        sort(mevPoints.begin(), mevPoints.end());
        sort(gevPoints.begin(), gevPoints.end());
        for (int i = 0; i < mevPoints.size(); i++) {
            mev << mevPoints[i].first << " " << mevPoints[i].second << endl;
        }
        for (int i = 0; i < gevPoints.size(); i++) {
            gev << gevPoints[i].first << " " << gevPoints[i].second << endl;
        }
    }
    
    return kolmogorovSmirnovProbability(maxDistance, (int)values.size(), (int)distribution.values.size());
}

pair<double, double> GRDistribution::lengtheningLimits(GRDistribution distribution, float probability, bool *success) {
    return make_pair(0., 0.);
}