#include "Isis.h"

#include <cmath>

#include "Camera.h"
#include "Cube.h"
#include "Distance.h"
#include "FileList.h"
#include "Process.h"
#include "Pvl.h"
#include "Statistics.h"
#include "Target.h"

using namespace std;
using namespace Isis;

template <typename T> inline T MIN(const T &A, const T &B) {
  if(A < B) {
    return (A);
  }
  else         {
    return (B);
  }
}

template <typename T> inline T MAX(const T &A, const T &B) {
  if(A > B) {
    return (A);
  }
  else         {
    return (B);
  }
}

inline double SetFloor(double value, const int precision) {
  double scale = pow(10.0, precision);
  value = floor(value * scale) / scale;
  return (value);
}

inline double SetRound(double value, const int precision) {
  double scale = pow(10.0, precision);
  value = rint(value * scale) / scale;
  return (value);
}

inline double SetCeil(double value, const int precision) {
  double scale = pow(10.0, precision);
  value = ceil(value * scale) / scale;
  return (value);
}

inline double Scale(const double pixres, const double polarRadius,
                    const double equiRadius, const double trueLat = 0.0) {
  double lat = trueLat * Isis::PI / 180.0;
  double a = polarRadius * cos(lat);
  double b = equiRadius * sin(lat);
  double localRadius = equiRadius * polarRadius / sqrt(a * a + b * b);
  return (localRadius / pixres * pi_c() / 180.0);
}

void IsisMain() {
  Process p;

  // Get the list of names of input CCD cubes to stitch together
  FileList flist;
  UserInterface &ui = Application::GetUserInterface();
  flist.read(ui.GetFileName("FROMLIST"));
  if(flist.size() < 1) {
    QString msg = "The list file[" + ui.GetFileName("FROMLIST") +
                  " does not contain any filenames";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  QString projection("Equirectangular");
  if(ui.WasEntered("MAP")) {
    Pvl mapfile(ui.GetFileName("MAP"));
    projection = (QString) mapfile.FindGroup("Mapping")["ProjectionName"];
  }

  if(ui.WasEntered("PROJECTION")) {
    projection = ui.GetString("PROJECTION");
  }

  // Gather other user inputs to projection
  QString lattype = ui.GetString("LATTYPE");
  QString londir  = ui.GetString("LONDIR");
  QString londom  = ui.GetString("LONDOM");
  int digits = ui.GetInteger("PRECISION");

  // Fix them for mapping group
  lattype = (lattype == "PLANETOCENTRIC") ? "Planetocentric" : "Planetographic";
  londir = (londir == "POSITIVEEAST") ? "PositiveEast" : "PositiveWest";

  Progress prog;
  prog.SetMaximumSteps(flist.size());
  prog.CheckStatus();

  Statistics scaleStat;
  Statistics longitudeStat;
  Statistics latitudeStat;
  Statistics equiRadStat;
  Statistics poleRadStat;
  PvlObject fileset("FileSet");

  // Save major equitorial and polar radii for last occuring
  double eqRad;
  double poleRad;

  QString target("Unknown");
  for(int i = 0 ; i < flist.size() ; i++) {
    try {
      // Set the input image, get the camera model, and a basic mapping
      // group
      Cube cube;
      cube.open(flist[i].toString());

      int lines = cube.getLineCount();
      int samples = cube.getSampleCount();


      PvlObject fmap("File");
      fmap += PvlKeyword("Name", flist[i].toString());
      fmap += PvlKeyword("Lines", toString(lines));
      fmap += PvlKeyword("Samples", toString(samples));

      Camera *cam = cube.getCamera();
      Pvl mapping;
      cam->BasicMapping(mapping);
      PvlGroup &mapgrp = mapping.FindGroup("Mapping");
      mapgrp.AddKeyword(PvlKeyword("ProjectionName", projection), Pvl::Replace);
      mapgrp.AddKeyword(PvlKeyword("LatitudeType", lattype), Pvl::Replace);
      mapgrp.AddKeyword(PvlKeyword("LongitudeDirection", londir), Pvl::Replace);
      mapgrp.AddKeyword(PvlKeyword("LongitudeDomain", londom), Pvl::Replace);

      // Get the radii
      Distance radii[3];
      cam->radii(radii);

      eqRad   = radii[0].meters();
      poleRad = radii[2].meters();

      target = cam->target()->name();
      equiRadStat.AddData(&eqRad, 1);
      poleRadStat.AddData(&poleRad, 1);

      // Get resolution
      double lowres = cam->LowestImageResolution();
      double hires = cam->HighestImageResolution();
      scaleStat.AddData(&lowres, 1);
      scaleStat.AddData(&hires, 1);

      double pixres = (lowres + hires) / 2.0;
      double scale = Scale(pixres, poleRad, eqRad);
      mapgrp.AddKeyword(PvlKeyword("PixelResolution", toString(pixres)), Pvl::Replace);
      mapgrp.AddKeyword(PvlKeyword("Scale", toString(scale), "pixels/degree"), Pvl::Replace);
      mapgrp += PvlKeyword("MinPixelResolution", toString(lowres), "meters");
      mapgrp += PvlKeyword("MaxPixelResolution", toString(hires), "meters");

      // Get the universal ground range
      double minlat, maxlat, minlon, maxlon;
      cam->GroundRange(minlat, maxlat, minlon, maxlon, mapping);
      mapgrp.AddKeyword(PvlKeyword("MinimumLatitude", toString(minlat)), Pvl::Replace);
      mapgrp.AddKeyword(PvlKeyword("MaximumLatitude", toString(maxlat)), Pvl::Replace);
      mapgrp.AddKeyword(PvlKeyword("MinimumLongitude", toString(minlon)), Pvl::Replace);
      mapgrp.AddKeyword(PvlKeyword("MaximumLongitude", toString(maxlon)), Pvl::Replace);

      fmap.AddGroup(mapgrp);
      fileset.AddObject(fmap);

      longitudeStat.AddData(&minlon, 1);
      longitudeStat.AddData(&maxlon, 1);
      latitudeStat.AddData(&minlat, 1);
      latitudeStat.AddData(&maxlat, 1);
    }
    catch(IException &ie) {
      QString mess = "Problems with file " + flist[i].toString() + "\n" +
                     ie.what();
      throw IException(IException::User, mess, _FILEINFO_);
    }

    p.ClearInputCubes();
    prog.CheckStatus();
  }

//  Construct the output mapping group with statistics
  PvlGroup mapping("Mapping");
  double avgPixRes((scaleStat.Minimum() + scaleStat.Maximum()) / 2.0);
  double avgLat((latitudeStat.Minimum() + latitudeStat.Maximum()) / 2.0);
  double avgLon((longitudeStat.Minimum() + longitudeStat.Maximum()) / 2.0);
  double avgEqRad((equiRadStat.Minimum() + equiRadStat.Maximum()) / 2.0);
  double avgPoleRad((poleRadStat.Minimum() + poleRadStat.Maximum()) / 2.0);
  double scale  = Scale(avgPixRes, avgPoleRad, avgEqRad);

  mapping += PvlKeyword("ProjectionName", projection);
  mapping += PvlKeyword("TargetName", target);
  mapping += PvlKeyword("EquatorialRadius", toString(eqRad), "meters");
  mapping += PvlKeyword("PolarRadius", toString(poleRad), "meters");
  mapping += PvlKeyword("LatitudeType", lattype);
  mapping += PvlKeyword("LongitudeDirection", londir);
  mapping += PvlKeyword("LongitudeDomain", londom);
  mapping += PvlKeyword("PixelResolution", toString(SetRound(avgPixRes, digits)), "meters/pixel");
  mapping += PvlKeyword("Scale", toString(SetRound(scale, digits)), "pixels/degree");
  mapping += PvlKeyword("MinPixelResolution", toString(scaleStat.Minimum()), "meters");
  mapping += PvlKeyword("MaxPixelResolution", toString(scaleStat.Maximum()), "meters");
  mapping += PvlKeyword("CenterLongitude", toString(SetRound(avgLon, digits)));
  mapping += PvlKeyword("CenterLatitude",  toString(SetRound(avgLat, digits)));
  mapping += PvlKeyword("MinimumLatitude", toString(MAX(SetFloor(latitudeStat.Minimum(), digits), -90.0)));
  mapping += PvlKeyword("MaximumLatitude", toString(MIN(SetCeil(latitudeStat.Maximum(), digits), 90.0)));
  mapping += PvlKeyword("MinimumLongitude", toString(MAX(SetFloor(longitudeStat.Minimum(), digits), -180.0)));
  mapping += PvlKeyword("MaximumLongitude", toString(MIN(SetCeil(longitudeStat.Maximum(), digits), 360.0)));

  PvlKeyword clat("PreciseCenterLongitude", toString(avgLon));
  clat.AddComment("Actual Parameters without precision applied");
  mapping += clat;
  mapping += PvlKeyword("PreciseCenterLatitude",  toString(avgLat));
  mapping += PvlKeyword("PreciseMinimumLatitude", toString(latitudeStat.Minimum()));
  mapping += PvlKeyword("PreciseMaximumLatitude", toString(latitudeStat.Maximum()));
  mapping += PvlKeyword("PreciseMinimumLongitude", toString(longitudeStat.Minimum()));
  mapping += PvlKeyword("PreciseMaximumLongitude", toString(longitudeStat.Maximum()));


  Application::Log(mapping);

  // Write the output file if requested
  if(ui.WasEntered("TO")) {
    Pvl temp;
    temp.AddGroup(mapping);
    temp.Write(ui.GetFileName("TO", "map"));
  }

  if(ui.WasEntered("LOG")) {
    Pvl temp;
    temp.AddObject(fileset);
    temp.Write(ui.GetFileName("LOG", "log"));
  }

  p.EndProcess();
}
