//
//  GRPsf.h
//  GRObservations
//
//  Created by Maxim Piskunov on 31.03.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#ifndef __Gamma_Rays__GRPsf__
#define __Gamma_Rays__GRPsf__

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

class GRPsf {
public:
    vector <float> energies;
    vector <float> angles;
    vector <vector <float> > probabilityDensity;
    
private:
    int findEnergy(float energy);
public:
    float spread(float energy, float probability);
    float spread(int energyIndex, float probability);
    ostream& writeSpreads(float probability, ostream &stream);
    string description();
};

#endif /* defined(__Gamma_Rays__GRPsf__) */
