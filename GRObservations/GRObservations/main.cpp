//
//  main.cpp
//  GRObservations
//
//  Created by Maxim Piskunov on 15.04.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#include <math.h>
#include <string>
#include <iostream>
#include <sstream>

#include "GRBurst.h"

#define FERMI_DIR "/usr/local/ScienceTools/x86_64-apple-darwin14.4.0"

using namespace std;

double probability(double sigma) {
    return 1-erf((double)sigma/sqrt(2.));
}

double sigma(double prob) {
    if (prob == 0.) return INFINITY;
    if (prob == 1.) return 0.;
    
    double sigmaLow = 0.;
    double sigmaHigh = 1.;
    while (probability(sigmaHigh) > prob) sigmaHigh *= 2.;
    
    while (sigmaHigh - sigmaLow > 0.01) {
        if (probability((sigmaLow + sigmaHigh)/2.) > prob) sigmaLow = (sigmaLow + sigmaHigh)/2.;
        else sigmaHigh = (sigmaLow + sigmaHigh)/2.;
    }
    
    return (sigmaLow + sigmaHigh) / 2.;
}

int main(int argc, const char * argv[])
{
    ofstream log("log");
    
    vector <GRBurst> burstCatalog;
    
    if (argc != 2) {
        cout << "Usage: GRObservations burstsCatalogLocation" << endl;
        return 1;
    }
    
    ofstream launchScript("launchScript");
    launchScript << "#!/bin/bash" << endl;
    setenv("FERMI_DIR", FERMI_DIR, 0);
    launchScript << "export FERMI_DIR=" << getenv("FERMI_DIR") << ";" << endl;
    launchScript << "source $FERMI_DIR/fermi-init.sh;" << endl;
    launchScript << "\"$@\"" << endl;
    launchScript.close();
    system("chmod +x launchScript");
    
    ifstream burstsFile(argv[1]);
    while (!burstsFile.eof()) {
        string name;
        double time;
        float ra;
        float dec;
        float error;
        double startOffset;
        double endOffset;
        char isReadable;
        
        burstsFile >> isReadable >> name >> time >> ra >> dec >> error >> startOffset;
        
        if (burstsFile.eof()) continue;

        burstsFile >> endOffset;
        
        if (isReadable == '#') continue;
        
        if ((endOffset - startOffset) != 0) {
            GRBurst burst;
            burst.name = name;
            burst.time = time;
            burst.location.ra = ra;
            burst.location.dec = dec;
            burst.location.error = error;
            burst.startOffset = startOffset;
            burst.endOffset = endOffset;
            burstCatalog.push_back(burst);
            cout << name << " " << time << " " << GRLocation(GRCoordinateSystemJ2000, ra, dec, error).description() << endl;
        }
    }
    
    for (int i = 0; i < burstCatalog.size(); i++) {
        
        cout << "processing " << burstCatalog[i].name << "..." << endl;
        
        burstCatalog[i].init();
        burstCatalog[i].download();
        burstCatalog[i].process();
        burstCatalog[i].read();
        
        if (burstCatalog[i].error != GRBurstErrorOk) {
            cout << "error " << burstCatalog[i].error << " " << burstCatalog[i].query.error << " " << burstCatalog[i].backgroundQuery.error << " : " << burstCatalog[i].errorDescription << endl;
            if (burstCatalog[i].query.error == 19) {
                continue;
            }
            else return -1;
        }
        
        log << burstCatalog[i].name << ":" << endl;
        log << "\tLow energy count  = " << burstCatalog[i].mevDistribution.values.size() - burstCatalog[i].mevDistribution.linearComponent << endl;
        log << "\tHigh energy count = " << burstCatalog[i].gevDistribution.values.size() - burstCatalog[i].gevDistribution.linearComponent << endl;
        log << "\tPhoton Ratio       = " << burstCatalog[i].highEnergyPhotonFraction() << endl;
        
        if ((burstCatalog[i].gevDistribution.values.size() - burstCatalog[i].gevDistribution.linearComponent) < 10) {
            log << "\tnot enought high energy photons" << endl << endl;
            continue;
        }
        
        log << "\tBackground to signal ratio = " << - burstCatalog[i].mevDistribution.linearComponent / burstCatalog[i].mevPhotons.size() << endl;
        
        burstCatalog[i].evaluate();
        
        log << "\tProbability not to be stretched = " << sigma(burstCatalog[i].trivialProbability) << endl;
        for (int k = 0; k < 5; k++) {
            log << "\t" << k+1 << "σ -> stretching factor in ranges (" << burstCatalog[i].minStretching[k] << ", " << burstCatalog[i].maxStretching[k] << ")" << endl;
        }
        log << endl;
        
        ofstream probs((burstCatalog[i].name + "/probs").c_str());
        for (int j = 0; j < burstCatalog[i].stretchingValues.size(); j++) {
            probs << burstCatalog[i].stretchingValues[j] << " " << burstCatalog[i].stretchingProbabilities[j] << " " << burstCatalog[i].maxDistanceTimes[j] << " " << burstCatalog[i].maxDistanceGevValue[j] << " " << burstCatalog[i].maxDistanceMevValue[j] << endl;
        }
        probs.close();
    }
    
    log.close();
    
    return 0;
}
