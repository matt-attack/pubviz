#ifndef LOCAL_XY
#define LOCAL_XY

#include <math.h>
#include <cmath>

#define M_PI 3.14159265358979323846


// Earth equatorial radius in meters according to WGS84.
const static double _earth_equator_radius = 6378137.0;

// Earth 'first' eccentricity according to WGS84.
const static double _earth_eccentricity = 0.08181919084261;

class LocalXYUtil
{
  double rho_lat_ = 0.0;
  double rho_lon_ = 0.0;
  double origin_lat_ = 0.0;// note this is in radians
  double origin_lon_ = 0.0;// note this is in radians

public:

  LocalXYUtil()
  {

  }

  bool Initialized()
  {
    return !(origin_lat_ == 0.0 && origin_lon_ == 0.0);
  }

  LocalXYUtil(double origin_lat, double origin_lon)
  {
    origin_lat_ = DegreesToRadians(origin_lat);
    origin_lon_ = DegreesToRadians(origin_lon);

    double altitude = 0;
    double depth = -altitude;

    double p = _earth_eccentricity * sin(origin_lat_);
    p = 1.0 - p * p;

    double rho_e = _earth_equator_radius * (1.0 - _earth_eccentricity * _earth_eccentricity) / (sqrt(p) * p);
    double rho_n = _earth_equator_radius / sqrt(p);

    rho_lat_ = rho_e - depth;
    rho_lon_ = (rho_n - depth) * cos(origin_lat_);
  }

  void ToLatLon(double x, double y, double& lat, double& lon)
  {
    double dLon = 1.0 * x - 0.0 * y;
    double dLat = 0.0 * x + 1.0 * y;
    double rlat = (dLat / rho_lat_) + origin_lat_;
    double rlon = (dLon / rho_lon_) + origin_lon_;

    lat = RadiansToDegrees(rlat);
    lon = RadiansToDegrees(rlon);
  }

  void FromLatLon(double lat, double lon, double& x, double& y)
  {
    double rLat = DegreesToRadians(lat);
    double rLon = DegreesToRadians(lon);
    double dLat = (rLat - origin_lat_) * rho_lat_;
    double dLon = (rLon - origin_lon_) * rho_lon_;

    x = 1.0 * dLon + 0.0 * dLat;
    y = -0.0 * dLon + 1.0 * dLat;
  }

  inline double OriginLatitude()
  {
    return RadiansToDegrees(origin_lat_);
  }

  inline double OriginLongitude()
  {
    return RadiansToDegrees(origin_lon_);
  }

  inline double DegreesToRadians(double angle)
  {
    return angle * M_PI / 180.0;
  }

  inline double RadiansToDegrees(double radians)
  {
    return (180.0 / M_PI) * radians;
  }
};

#endif