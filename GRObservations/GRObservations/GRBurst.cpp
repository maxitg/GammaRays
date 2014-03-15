//
//  GRBurst.cpp
//  GRObservations
//
//  Created by Maxim Piskunov on 24.03.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#include <sys/stat.h>
#include <sys/errno.h>

#include <math.h>

#include <fstream>
#include <sstream>

#include "GRBurst.h"
#define TIME_EXTENSION_FACTOR   2.0
#define PHOTONS_QUALITY         0.95
#define STRETCHING_MIN          0.01
#define STRETCHING_MAX          100.
#define STRETCHING_STEP         1.0001

#define BACKGROUND_DURATION     86400

void GRBurst::init() {
    
    double middle = time + (startOffset + endOffset) / 2.;
    double duration = (endOffset - startOffset) * TIME_EXTENSION_FACTOR;
    
    query.startTime = middle - duration/2.;
    query.endTime = middle + duration/2.;
    query.location = location;
    
    query.init();
    
    backgroundQuery.startTime = query.startTime - BACKGROUND_DURATION;
    backgroundQuery.endTime = query.startTime;
    backgroundQuery.location = location;
    
    backgroundQuery.init();
    
    error = GRBurstErrorNotDownloaded;
}

void GRBurst::download() {
    if (query.error == GRFermiLATDataServerQueryErrorNotDownloaded) query.download();
    if (query.error != GRFermiLATDataServerQueryErrorNotProcessed && query.error != GRFermiLATDataServerQueryErrorNotRead && query.error != GRFermiLATDataServerQueryErrorOk) {
        error = GRBurstErrorQueryError;
        errorDescription = query.errorDescription;
        return;
    }
    
    if (backgroundQuery.error == GRFermiLATDataServerQueryErrorNotDownloaded) backgroundQuery.download();
    if (backgroundQuery.error != GRFermiLATDataServerQueryErrorNotProcessed && backgroundQuery.error != GRFermiLATDataServerQueryErrorNotRead && backgroundQuery.error != GRFermiLATDataServerQueryErrorOk) {
        error = GRBurstErrorBackgroundQueryError;
        errorDescription = backgroundQuery.errorDescription;
        return;
    }

    error = GRBurstErrorNotProcessed;
    
}

void GRBurst::process() {
    if (query.error == GRFermiLATDataServerQueryErrorNotProcessed) query.process();
    if (query.error != GRFermiLATDataServerQueryErrorNotRead && query.error != GRFermiLATDataServerQueryErrorOk) {
        error = GRBurstErrorQueryError;
        errorDescription = query.errorDescription;
        return;
    }
    
    if (backgroundQuery.error == GRFermiLATDataServerQueryErrorNotProcessed) backgroundQuery.process();
    if (backgroundQuery.error != GRFermiLATDataServerQueryErrorNotRead && backgroundQuery.error != GRFermiLATDataServerQueryErrorOk) {
        error = GRBurstErrorBackgroundQueryError;
        errorDescription = backgroundQuery.errorDescription;
        return;
    }
    
    error = GRBurstErrorNotRead;

}

void GRBurst::calculateBackground() {
    double mevExpectationValue = 0.;
    double gevExpectationValue = 0.;
    
    vector <GRFermiLATPhoton> *backgroundPhotonLists[2] = {&mevBackgroundPhotons, &gevBackgroundPhotons};
    double *backgroundEstimationList[2] = {&mevExpectationValue, &gevExpectationValue};
    
    for (int i = 0; i < 2; i++) {
        *(backgroundEstimationList[i]) = 0.;
        for (int j = 0; j < (*(backgroundPhotonLists[i])).size(); j++) {
            GRFermiLATPhoton photon = (*(backgroundPhotonLists[i]))[j];
            double grbExposure = query.exposureMaps[photon.eventClass][photon.conversionType].exposure(photon.energy, photon.location);
            double backgroundExposure = backgroundQuery.exposureMaps[photon.eventClass][photon.conversionType].exposure(photon.energy, photon.location);
            *(backgroundEstimationList[i]) += grbExposure / backgroundExposure;
        }
    }
        
    mevDistribution.linearComponent = -mevExpectationValue;
    mevDistribution.start = query.startTime - (time + startOffset);
    mevDistribution.end = query.endTime - (time + startOffset);
    
    gevDistribution.linearComponent = -gevExpectationValue;
    gevDistribution.start = query.startTime - (time + startOffset);
    gevDistribution.end = query.endTime - (time + startOffset);
}

void GRBurst::read() {
    if (query.error == GRFermiLATDataServerQueryErrorNotRead) query.read();
    if (query.error != GRFermiLATDataServerQueryErrorOk) {
        error = GRBurstErrorQueryError;
        errorDescription = query.errorDescription;
        return;
    }
    
    if (backgroundQuery.error == GRFermiLATDataServerQueryErrorNotRead) backgroundQuery.read();
    if (backgroundQuery.error != GRFermiLATDataServerQueryErrorOk) {
        error = GRBurstErrorBackgroundQueryError;
        errorDescription = backgroundQuery.errorDescription;
        return;
    }
    
    for (int i = 0; i < query.events.size(); i++) {
        GRFermiLATPhoton photon = query.events[i];
        photon.location.error = query.psfs[photon.eventClass][photon.conversionType].spread(photon.energy, PHOTONS_QUALITY);
        if (photon.eventClass == GRFermiEventClassTransient) continue;
        if (location.isSeparated(photon.location)) continue;
        
        if (photon.energy < 1000.) mevPhotons.push_back(photon);
        else gevPhotons.push_back(photon);
    }
    sort(mevPhotons.begin(), mevPhotons.end());
    sort(gevPhotons.begin(), gevPhotons.end());
    
    for (int i = 0; i < backgroundQuery.events.size(); i++) {
        GRFermiLATPhoton photon = backgroundQuery.events[i];
        photon.location.error = backgroundQuery.psfs[photon.eventClass][photon.conversionType].spread(photon.energy, PHOTONS_QUALITY);
        if (photon.eventClass == GRFermiEventClassTransient) continue;
        if (location.isSeparated(photon.location)) continue;
        
        if (photon.energy < 1000.) mevBackgroundPhotons.push_back(photon);
        else gevBackgroundPhotons.push_back(photon);
    }
    sort(mevBackgroundPhotons.begin(), mevBackgroundPhotons.end());
    sort(gevBackgroundPhotons.begin(), gevBackgroundPhotons.end());
    
    cout << "event counts: signal {" << mevPhotons.size() << ", " << gevPhotons.size() << "} background {" << mevBackgroundPhotons.size() << ", " << gevBackgroundPhotons.size() << "}" << endl;
    
    calculateBackground();
    
    for (int i = 0; i < mevPhotons.size(); i++) {
        mevDistribution.values.push_back(mevPhotons[i].time - (time + startOffset));
    }
    
    for (int i = 0; i < gevPhotons.size(); i++) {
        gevDistribution.values.push_back(gevPhotons[i].time - (time + startOffset));
    }
        
    error = GRBurstErrorOk;
}

void GRBurst::evaluate() {
    if (mkdir(name.c_str(), S_IRWXU ^ S_IRWXG ^ S_IRWXO) == -1) {
        if (errno != EEXIST) {
            error = GRBurstErrorMkdirFailed;
        }
    }
    
    ofstream mev((name + "/mev").c_str());
    vector <struct GRDistributionCDFPoint> mevCdf = mevDistribution.cdf();
    for (int i = 0; i != mevCdf.size(); ++i) {
        mev << mevCdf[i].value << " " << mevCdf[i].probability << endl;
    }
    mev.close();
    
    ofstream gev((name + "/gev").c_str());
    vector <struct GRDistributionCDFPoint> gevCdf = gevDistribution.cdf();
    for (int i = 0; i != gevCdf.size(); ++i) {
        gev << gevCdf[i].value << " " << gevCdf[i].probability << endl;
    }
    gev.close();
    
    double probabilityValues[5];
    for (int i = 0; i < 5; i++) {
        probabilityValues[i] = 1-erf((double)(i+1)/sqrt(2.));
        minStretching[i] = INFINITY;
        maxStretching[i] = 0.;
    }
    
    double bestValue = 0., bestProbability = 0.;
    for (double value = STRETCHING_MIN; value <= STRETCHING_MAX; value *= STRETCHING_STEP) {
        double time, gevValue, mevValue;
        double probability = gevDistribution.kolmogorovSmirnovTest(mevDistribution, value, &time, &gevValue, &mevValue);
        stretchingValues.push_back(value);
        stretchingProbabilities.push_back(probability);
        maxDistanceTimes.push_back(time);
        maxDistanceGevValue.push_back(gevValue);
        maxDistanceMevValue.push_back(mevValue);
        
        if (probability > bestProbability) {
            bestProbability = probability;
            bestValue = value;
        }
        
        for (int i = 0; i < 5; i++) {
            if (stretchingProbabilities[stretchingProbabilities.size()-1] < probabilityValues[i]) continue;
            minStretching[i] = min(value, minStretching[i]);
            maxStretching[i] = max(value, maxStretching[i]);
        }
    }
    
    trivialProbability = gevDistribution.kolmogorovSmirnovTest(mevDistribution, 1.);
    
    cout << endl;
}