//
//  GRExposureMap.h
//  GRObservations
//
//  Created by Maxim Piskunov on 28.04.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#ifndef __Gamma_Rays__GRExposureMap__
#define __Gamma_Rays__GRExposureMap__

#include <iostream>
#include <vector>

#include "GRLocation.h"

using namespace std;

class GRExposureMap {
public:
    vector <float> energies;
    vector <vector <vector <float> > > exposures; // [ra][dec]
    
private:
    int findEnergy(float energy);
public:
    float exposure(int energyIndex, GRLocation location);
    float exposure(float energy, GRLocation location);
};

#endif /* defined(__Gamma_Rays__GRExposureMap__) */
