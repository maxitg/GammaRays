//
//  GRPsf.cpp
//  GRObservations
//
//  Created by Maxim Piskunov on 31.03.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#include <math.h>

#include <memory>
#include <sstream>

#include "fitsio.h"

#include "GRPsf.h"

using namespace std;

int GRPsf::findEnergy(float energy) {
    if (energy < energies[0]) return -1;
    else {
        int lowerEnergy = 0;
        int upperEnergy = (int)energies.size()-1;
        while (upperEnergy - lowerEnergy > 1) {
            if (energies[(upperEnergy + lowerEnergy)/2] > energy) {
                upperEnergy = (upperEnergy + lowerEnergy)/2;
            } else {
                lowerEnergy = (upperEnergy + lowerEnergy)/2;
            }
        }
        return lowerEnergy;
    }
}

float GRPsf::spread(int energyIndex, float probability) {
    double currentProbability = 0.;
    double previousProbability = 0.;
    int i;
    for (i = 0; i < angles.size()-1 && currentProbability < probability; i++) {
        previousProbability = currentProbability;
        currentProbability += (probabilityDensity[energyIndex][i] + probabilityDensity[energyIndex][i+1]) * M_PI * (cos(angles[i] * M_PI / 180.) - cos(angles[i+1] * M_PI / 180.));
    }
    if (i == angles.size()-1 && currentProbability < probability) {
        i++;
        previousProbability = currentProbability;
        currentProbability = 1.;
    }
    
    double lowerAngle = angles[i-1];
    double upperAngle = (i == angles.size() ? 180. : angles[i]);
    double lowerProbability = previousProbability;
    double upperProbability = currentProbability;
    
    if (lowerProbability == upperProbability) return lowerAngle;
    else return lowerAngle + (upperAngle - lowerAngle) / (upperProbability - lowerProbability) * (probability - lowerProbability);
}

float GRPsf::spread(float energy, float probability) {
    int lowerIndex = findEnergy(energy);
    int upperIndex;
    if (lowerIndex == -1) {
        lowerIndex = 0;
        upperIndex = 1;
    } else if (lowerIndex == (int)energies.size()-1) {
        lowerIndex = (int)energies.size()-2;
        upperIndex = (int)energies.size()-1;
    } else {
        upperIndex = lowerIndex+1;
    }
    
    double lowerSpread = spread(lowerIndex, probability);
    double upperSpread = spread(upperIndex, probability);
    double lowerEnergy = energies[lowerIndex];
    double upperEnergy = energies[upperIndex];
    
    if (lowerEnergy == upperEnergy) return lowerSpread;
    else return lowerSpread + (upperSpread - lowerSpread) / (upperEnergy - lowerEnergy) * (energy - lowerEnergy);
}

ostream& GRPsf::writeSpreads(float probability, ostream &stream) {
    for (int i = 0; i < energies.size(); i++) {
        stream << energies[i] << " " << spread(i, probability) << endl;
    }
    return stream;
}

string GRPsf::description() {
    ostringstream result;
    result << "angles: ";
    for (int i = 0; i < angles.size(); i++) {
        result << angles[i] << " ";
    }
    result << endl;
    
    result << "energies: ";
    for (int i = 0; i < energies.size(); i++) {
        result << energies[i] << " ";
    }
    result << endl;
    
    result << "probability densities: ";
    result << endl;
    for (int i = 0; i < probabilityDensity.size(); i++) {
        for (int j = 0; j < probabilityDensity[i].size(); j++) {
            result << probabilityDensity[i][j] << " ";
        }
        result << endl;
    }
    
    return result.str();
}