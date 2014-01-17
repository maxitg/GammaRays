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

#define TIME_EXTENTION_FACTOR 1.3
#define PHOTONS_QUALITY       0.95
#define LENGTHENING_MIN       0.1
#define LENGTHENING_MAX       10.
#define LENGTHENING_STEP      1.001

#define BACKGROUND_DURATION   86400

double GRBurst::middleTime() {
    return time + (startOffset + endOffset) / 2.;
}

double GRBurst::duration() {
    return (endOffset - startOffset) * TIME_EXTENTION_FACTOR;
}

double GRBurst::startTime() {
    return middleTime() - duration()/2.;
}

double GRBurst::endTime() {
    return middleTime() + duration()/2.;
}

double GRBurst::backgroundStartTime() {
    return startTime() - BACKGROUND_DURATION;
}

double GRBurst::backgroundEndTime() {
    return startTime();
}

void GRBurst::init() {
    
    query.startTime = startTime();
    query.endTime = endTime();
    query.location = location;
        
    query.init();
    
    backgroundQuery.startTime = backgroundStartTime();
    backgroundQuery.endTime = backgroundEndTime();
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
        
    mevDistribution.estimatedLinearComponent = mevExpectationValue;
    mevDistribution.start = query.startTime - (time + startOffset);
    mevDistribution.end = query.endTime - (time + startOffset);
    
    gevDistribution.estimatedLinearComponent = gevExpectationValue;
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
    
    cout << "event counts: " << query.events.size() << " " << backgroundQuery.events.size() << endl;
    
    for (int i = 0; i < query.events.size(); i++) {
        GRFermiLATPhoton photon = query.events[i];
        photon.location.error = query.psfs[photon.eventClass][photon.conversionType].spread(photon.energy, PHOTONS_QUALITY);
        //if (photon.eventClass == GRFermiEventClassTransient) continue;
        if (photon.time < startTime() || photon.time > endTime()) continue;
        if (location.isSeparated(photon.location)) continue;
        
        if (photon.energy < 1000.) mevPhotons.push_back(photon);
        else gevPhotons.push_back(photon);
    }
    sort(mevPhotons.begin(), mevPhotons.end());
    sort(mevBackgroundPhotons.begin(), mevBackgroundPhotons.end());
    
    for (int i = 0; i < backgroundQuery.events.size(); i++) {
        GRFermiLATPhoton photon = backgroundQuery.events[i];
        photon.location.error = backgroundQuery.psfs[photon.eventClass][photon.conversionType].spread(photon.energy, PHOTONS_QUALITY);
        //if (photon.eventClass == GRFermiEventClassTransient) continue;
        if (photon.time < backgroundStartTime() || photon.time > backgroundEndTime()) continue;
        if (location.isSeparated(photon.location)) continue;
        
        if (photon.energy < 1000.) mevBackgroundPhotons.push_back(photon);
        else gevBackgroundPhotons.push_back(photon);
    }
    sort(gevPhotons.begin(), gevPhotons.end());
    sort(gevBackgroundPhotons.begin(), gevBackgroundPhotons.end());
    
    calculateBackground();
    
    for (int i = 0; i < mevPhotons.size(); i++) {
        mevDistribution.values.push_back(mevPhotons[i].time - (time + startOffset));
    }
    
    for (int i = 0; i < gevPhotons.size(); i++) {
        gevDistribution.values.push_back(gevPhotons[i].time - (time + startOffset));
    }
        
    error = GRBurstErrorOk;
}

double GRBurst::flux(double minEnergy, double maxEnergy) {
    double result = 0.;
    for (int i = 0; i < mevPhotons.size(); i++) if (mevPhotons[i].energy >= minEnergy && mevPhotons[i].energy <= maxEnergy) {
        result += duration() / query.exposureMaps[mevPhotons[i].eventClass][mevPhotons[i].conversionType].exposure(mevPhotons[i].energy, mevPhotons[i].location);
    }
    
    for (int i = 0; i < gevPhotons.size(); i++) if (gevPhotons[i].energy >= minEnergy && gevPhotons[i].energy <= maxEnergy) {
        result += duration() / query.exposureMaps[gevPhotons[i].eventClass][gevPhotons[i].conversionType].exposure(gevPhotons[i].energy, gevPhotons[i].location);
    }
    
    return result;
}

int GRBurst::photonCount(double minEnergy, double maxEnergy) {
    int result = 0;
    for (int i = 0; i < mevPhotons.size(); i++) if (mevPhotons[i].energy >= minEnergy && mevPhotons[i].energy <= maxEnergy) result++;
    for (int i = 0; i < gevPhotons.size(); i++) if (gevPhotons[i].energy >= minEnergy && gevPhotons[i].energy <= maxEnergy) result++;
    return result;
}

double GRBurst::backgroundFlux(double minEnergy, double maxEnergy) {
    double result = 0.;
    for (int i = 0; i < mevBackgroundPhotons.size(); i++) if (mevBackgroundPhotons[i].energy >= minEnergy && mevBackgroundPhotons[i].energy <= maxEnergy) {
        result += duration() / backgroundQuery.exposureMaps[mevBackgroundPhotons[i].eventClass][mevBackgroundPhotons[i].conversionType].exposure(mevBackgroundPhotons[i].energy, mevBackgroundPhotons[i].location);
    }
    
    for (int i = 0; i < gevBackgroundPhotons.size(); i++) if (gevBackgroundPhotons[i].energy >= minEnergy && gevBackgroundPhotons[i].energy <= maxEnergy) {
        result += duration() / backgroundQuery.exposureMaps[gevBackgroundPhotons[i].eventClass][gevBackgroundPhotons[i].conversionType].exposure(gevBackgroundPhotons[i].energy, gevBackgroundPhotons[i].location);
    }
    
    return result;
}

void GRBurst::evaluate() {
    if (mkdir(name.c_str(), S_IRWXU ^ S_IRWXG ^ S_IRWXO) == -1) {
        if (errno != EEXIST) {
            error = GRBurstErrorMkdirFailed;
        }
    }
    
    double probabilityValues[5];
    for (int i = 0; i < 5; i++) {
        probabilityValues[i] = 1-erf((double)(i+1)/sqrt(2.));
        minLengthening[i] = INFINITY;
        maxLengthening[i] = 0.;
    }
    
    double bestValue = 0., bestProbability = 0.;
    for (double value = LENGTHENING_MIN; value <= LENGTHENING_MAX; value *= LENGTHENING_STEP) {
        double probability = gevDistribution.kolmogorovSmirnovTest(mevDistribution, value);
        lengtheningValues.push_back(value);
        lengtheningProbabilities.push_back(probability);
        if (probability > bestProbability) {
            bestProbability = probability;
            bestValue = value;
        }
        
        for (int i = 0; i < 5; i++) {
            if (lengtheningProbabilities[lengtheningProbabilities.size()-1] < probabilityValues[i]) continue;
            minLengthening[i] = min(value, minLengthening[i]);
            maxLengthening[i] = max(value, maxLengthening[i]);
        }
    }
    
    ofstream mev((name + "/mev").c_str());
    ofstream gev((name + "/gev").c_str());
    trivialProbability = gevDistribution.kolmogorovSmirnovTest(mevDistribution, 1., true, mev, gev);
    mev.close();
    gev.close();
    
    mev.open((name + "/stretchedMeV").c_str());
    gevDistribution.kolmogorovSmirnovTest(mevDistribution, bestValue, true, mev, gev);
    mev.close();
    
    cout << endl;
}

void GRBurst::clear() {
    error = GRBurstErrorNotDownloaded;
    errorDescription = "";
    name = "";
    time = 0.;
    location = GRLocation(GRCoordinateSystemJ2000, 0., 0., 0.);
    startOffset = 0.;
    endOffset = 0.;
    redshift = -1.;
    
    backgroundQuery = GRFermiLATDataServerQuery();
    query = GRFermiLATDataServerQuery();
    
    mevPhotons.clear();
    mevBackgroundPhotons.clear();
    gevPhotons.clear();
    gevBackgroundPhotons.clear();
    
    mevDistribution = GRDistribution();
    gevDistribution = GRDistribution();
    
    for (int i = 0; i < 5; i++) {
        minLengthening[i] = 0.;
        maxLengthening[i] = 0.;
    }
    
    lengtheningValues.clear();
    lengtheningProbabilities.clear();
}

bool energyCompare(GRFermiLATPhoton left, GRFermiLATPhoton right) {
    return left.energy < right.energy;
}

string GRBurst::topEnergyDescription(int n) {
    vector <GRFermiLATPhoton> photons;
    if (gevPhotons.size() < n) {
        n -= gevPhotons.size();
        photons = mevPhotons;
        sort(photons.begin(), photons.end(), energyCompare);
        return photons[n-1].energyDescription();
    } else {
        photons = gevPhotons;
        sort(photons.begin(), photons.end(), energyCompare);
        return photons[n-1].energyDescription();
    }
}