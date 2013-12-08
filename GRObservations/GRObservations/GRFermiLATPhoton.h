//
//  GRFermiLATPhoton.h
//  GRObservations
//
//  Created by Maxim Piskunov on 24.03.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#ifndef __Gamma_Rays__GRFermiLATPhoton__
#define __Gamma_Rays__GRFermiLATPhoton__

#include <iostream>
#include <string>

#include "GRLocation.h"

enum GRFermiConversionType {
    GRFermiConversionTypeFront = 0,
    GRFermiConversionTypeBack = 1
    };

enum GRFermiEventClass {
    GRFermiEventClassTransient = 0,
    GRFermiEventClassSource = 1,
    GRFermiEventClassClean = 2,
    GRFermiEventClassUltraclean = 3
    };

const int GRFermiConversionTypesCount = 2;
const int GRFermiEventClassesCount = 4;

const GRFermiConversionType GRFermiConversionTypes[] = {GRFermiConversionTypeFront, GRFermiConversionTypeBack};
const GRFermiEventClass GRFermiEventClasses[] = {GRFermiEventClassTransient, GRFermiEventClassSource, GRFermiEventClassClean, GRFermiEventClassUltraclean};

class GRFermiLATPhoton {
public:
    GRLocation location;
    float energy;
    double time;
    GRFermiConversionType conversionType;
    GRFermiEventClass eventClass;
    
private:
    string conversionTypeName();
    string eventClassName();
    string energyDescription();
    
public:
    GRFermiLATPhoton(double time, GRLocation location, float energy, GRFermiConversionType conversionType, GRFermiEventClass fermiEventClass) : time(time), location(location), energy(energy), conversionType(conversionType), eventClass(fermiEventClass) {};
    bool operator<(GRFermiLATPhoton right) const;
    string description();
};

#endif /* defined(__Gamma_Rays__GRFermiLATPhoton__) */
