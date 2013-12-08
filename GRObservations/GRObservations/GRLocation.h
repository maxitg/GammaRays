//
//  GRCelestialSpherePoint.h
//  GRObservations
//
//  Created by Maxim Piskunov on 24.03.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#ifndef __Gamma_Rays__GRCelestialSpherePoint__
#define __Gamma_Rays__GRCelestialSpherePoint__

#include <iostream>
#include <string>
#include <vector>

using namespace std;

enum GRCoordinateSystem {
    GRCoordinateSystemJ2000 = 0,
    GRCoordinateSystemGalactic = 1
    };

class GRLocation {
public:
    float ra;
    float dec;
    float error;
    
public:
    GRLocation(GRCoordinateSystem system, float ra, float dec, float error);
    GRLocation(GRCoordinateSystem system, float ra, float dec);
    GRLocation();
    double operator==(GRLocation location);
    float separation(GRLocation location);
    bool isSeparated(GRLocation location);
    string description();
};

#endif /* defined(__Gamma_Rays__GRCelestialSpherePoint__) */
