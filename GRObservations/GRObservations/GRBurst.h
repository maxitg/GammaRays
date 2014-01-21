//
//  GRBurst.h
//  GRObservations
//
//  Created by Maxim Piskunov on 24.03.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#ifndef __Gamma_Rays__GRBurst__
#define __Gamma_Rays__GRBurst__

#include <stdio.h>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "GRFermiLATDataServerQuery.h"
#include "GRDistribution.h"

using namespace std;

enum GRBurstError {
    GRBurstErrorOk,
    GRBurstErrorNotDownloaded,
    GRBurstErrorNotProcessed,
    GRBurstErrorNotRead,
    
    GRBurstErrorQueryError,
    GRBurstErrorBackgroundQueryError,
    
    GRBurstErrorMkdirFailed
    };

class GRBurst {
public:
    GRBurstError error;
    string errorDescription;
    
    string name;
    double time;
    GRLocation location;
    
    double startOffset;
    double endOffset;
    
    double redshift;
        
public:
    GRFermiLATDataServerQuery backgroundQuery;
    GRFermiLATDataServerQuery query;
    
    vector <GRFermiLATPhoton> mevPhotons;
    vector <GRFermiLATPhoton> mevBackgroundPhotons;
    
    vector <GRFermiLATPhoton> gevPhotons;
    vector <GRFermiLATPhoton> gevBackgroundPhotons;
    
public:
    GRDistribution mevDistribution;
    GRDistribution gevDistribution;
    
public:
    double minLengthening[5];
    double maxLengthening[5];
    vector <float> lengtheningValues;
    vector <float> lengtheningProbabilities;
    
    double trivialProbability;
    
private:
    void calculateBackground();
    double middleTime();
    double duration();
    double startTime();
    double endTime();
    double backgroundStartTime();
    double backgroundEndTime();
            
public:
    void init();
    void download();
    void process();
    void read();
    void evaluate();
};

#endif /* defined(__Gamma_Rays__GRBurst__) */
