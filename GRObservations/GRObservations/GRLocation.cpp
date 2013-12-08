//
//  GRCelestialSpherePoint.cpp
//  GRObservations
//
//  Created by Maxim Piskunov on 24.03.2013.
//  Copyright (c) 2013 Maxim Piskunov. All rights reserved.
//

#include <math.h>

#include <sstream>

#include "GRLocation.h"

GRLocation::GRLocation(GRCoordinateSystem system, float angle0, float angle1, float error) : error (error){
    if (system == GRCoordinateSystemJ2000) {
        ra = angle0;
        dec = angle1;
    } else if (system ==GRCoordinateSystemGalactic) {
        double l = angle0 * M_PI / 180.;
        double b = angle1 * M_PI / 180.;
        
        double pole_ra = 192.859508 * M_PI / 180.;
        double pole_dec = 27.128336 * M_PI / 180.;
        double posangle = (122.932-90.0) * M_PI / 180.;
        
        double ra_rad = atan2( (cos(b)*cos(l-posangle)), (sin(b)*cos(pole_dec) - cos(b)*sin(pole_dec)*sin(l-posangle)) ) + pole_ra;
        double dec_rad = asin( cos(b)*cos(pole_dec)*sin(l-posangle) + sin(b)*sin(pole_dec) );
        
        ra = (float)(ra_rad * 180. / M_PI);
        dec = (float)(dec_rad * 180. / M_PI);
    }
}

GRLocation::GRLocation(GRCoordinateSystem system, float angle0, float angle1) {
    *this = GRLocation(system, angle0, angle1, error);
}

GRLocation::GRLocation() {
    ra = 0.;
    dec = 0.;
    error = 0.;
}

float GRLocation::separation(GRLocation location) {
    double ra1Rad = ra * M_PI / 180.;
    double dec1Rad = dec * M_PI / 180.;
    double ra2Rad = location.ra * M_PI / 180.;
    double dec2Rad = location.dec * M_PI / 180.;
    double raDiff = ra1Rad - ra2Rad;
    if (raDiff < 0) raDiff = -raDiff;
    double decDiff = dec1Rad - dec2Rad;
    if (decDiff < 0) decDiff = -decDiff;
    
    double sep = atan2(sqrt(pow(cos(dec2Rad)*sin(raDiff), 2.) + pow(cos(dec1Rad)*sin(dec2Rad) - sin(dec1Rad)*cos(dec2Rad)*cos(raDiff), 2.)), sin(dec1Rad)*sin(dec2Rad) + cos(dec1Rad)*cos(dec2Rad)*cos(raDiff));
    return sep * 180. / M_PI;
}

bool GRLocation::isSeparated(GRLocation location) {
    if (separation(location) > error + location.error) return true;
    else return false;
}

string GRLocation::description() {
    ostringstream result;
    result << "(" << ra << " ra, " << dec << " dec) ± " << error;
    return result.str();
}