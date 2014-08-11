//
//  GRExposureMap.cpp
//  GRObservations
//
//  Created by Maxim Piskunov on 28.04.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#include "GRExposureMap.h"

int GRExposureMap::findEnergy(float energy) {
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

float GRExposureMap::exposure(float energy, GRLocation location) {
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
    
    double lowerSpread = exposure(lowerIndex, location);
    double upperSpread = exposure(upperIndex, location);
    double lowerEnergy = energies[lowerIndex];
    double upperEnergy = energies[upperIndex];
    
    if (lowerEnergy == upperEnergy) return lowerSpread;
    else return lowerSpread + (upperSpread - lowerSpread) / (upperEnergy - lowerEnergy) * (energy - lowerEnergy);
}

float GRExposureMap::exposure(int energyIndex, GRLocation location) {
    int raLow = location.ra;
    int raHigh = raLow+1;
    if (raHigh == 360) {
        raLow = 358;
        raHigh = 359;
    }
    
    int decLow = location.dec + 90.;
    int decHigh = decLow+1;
    if (decHigh == 180) {
        decLow = 178;
        decHigh = 179;
    }
    
    double rl = raLow;
    double rh = raHigh;
    double dl = decLow - 90.;
    double dh = decHigh - 90.;
    
    double ll = exposures[energyIndex][raLow][decLow];
    double lh = exposures[energyIndex][raLow][decHigh];
    double hl = exposures[energyIndex][raHigh][decLow];
    double hh = exposures[energyIndex][raHigh][decHigh];
    
    return ll*(rh - location.ra)*(dh - location.dec) + hl*(location.ra - rl)*(dh - location.dec) + lh*(rh - location.ra)*(location.dec - dl) + hh*(location.ra - rl)*(location.dec - dl);
}