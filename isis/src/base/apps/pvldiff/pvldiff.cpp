#include "Isis.h"

#include <cmath>
#include <float.h>

#include "PvlContainer.h"
#include "Pvl.h"
#include "IException.h"


using namespace std;
using namespace Isis;

bool filesMatch;
QString differenceReason;
PvlGroup tolerances;
PvlGroup ignorekeys;

void CompareKeywords(const PvlKeyword &pvl1, const PvlKeyword &pvl2);
void CompareObjects(const PvlObject &pvl1, const PvlObject &pvl2);
void CompareGroups(const PvlGroup &pvl1, const PvlGroup &pvl2);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  tolerances = PvlGroup();
  ignorekeys = PvlGroup();
  differenceReason = "";
  filesMatch = true;

  const Pvl file1(ui.GetFileName("FROM"));
  const Pvl file2(ui.GetFileName("FROM2"));

  if(ui.WasEntered("DIFF")) {
    Pvl diffFile(ui.GetFileName("DIFF"));

    if(diffFile.HasGroup("Tolerances")) {
      tolerances = diffFile.FindGroup("Tolerances");
    }

    if(diffFile.HasGroup("IgnoreKeys")) {
      ignorekeys = diffFile.FindGroup("IgnoreKeys");
    }
  }

  CompareObjects(file1, file2);

  PvlGroup differences("Results");
  if(filesMatch) {
    differences += PvlKeyword("Compare", "Identical");
  }
  else {
    differences += PvlKeyword("Compare", "Different");
    differences += PvlKeyword("Reason", differenceReason);
  }

  Application::Log(differences);

  if(ui.WasEntered("TO")) {
    Pvl out;
    out.AddGroup(differences);
    out.Write(ui.GetFileName("TO"));
  }

  differenceReason = "";
}

void CompareKeywords(const PvlKeyword &pvl1, const PvlKeyword &pvl2) {
  if(pvl1.Name().compare(pvl2.Name()) != 0) {
    filesMatch = false;
    differenceReason = "Keyword '" + pvl1.Name() + "' does not match keyword '" + pvl2.Name() + "'";
  }

  if(pvl1.Size() != pvl2.Size()) {
    filesMatch = false;
    differenceReason = "Keyword '" + pvl1.Name() + "' size does not match.";
    return;
  }

  if(tolerances.HasKeyword(pvl1.Name()) &&
      tolerances[pvl1.Name()].Size() > 1 &&
      pvl1.Size() != tolerances[pvl1.Name()].Size()) {
    QString msg = "Size of keyword '" + pvl1.Name() + "' does not match with ";
    msg += "its number of tolerances in the DIFF file.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if(ignorekeys.HasKeyword(pvl1.Name()) &&
      ignorekeys[pvl1.Name()].Size() > 1 &&
      pvl1.Size() != ignorekeys[pvl1.Name()].Size()) {
    QString msg = "Size of keyword '" + pvl1.Name() + "' does not match with ";
    msg += "its number of ignore keys in the DIFF file.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  for(int i = 0; i < pvl1.Size() && filesMatch; i++) {
    QString val1 = pvl1[i];
    QString val2 = pvl2[i];
    QString unit1 = pvl1.Unit(i);
    QString unit2 = pvl2.Unit(i);

    int ignoreIndex = 0;
    if(ignorekeys.HasKeyword(pvl1.Name()) && ignorekeys[pvl1.Name()].Size() > 1) {
      ignoreIndex = i;
    }

    try {
      if(!ignorekeys.HasKeyword(pvl1.Name()) ||
          ignorekeys[pvl1.Name()][ignoreIndex] == "false") {

        if(unit1.toLower() != unit2.toLower()) {
          filesMatch = false;
          differenceReason = "Keyword '" + pvl1.Name() + "': units do not match.";
          return;
        }

        double tolerance = 0.0;
        double difference = abs(toDouble(val1) - toDouble(val2));

        if(tolerances.HasKeyword(pvl1.Name())) {
          tolerance = toDouble(
              (tolerances[pvl1.Name()].Size() == 1) ?
                 tolerances[pvl1.Name()][0] : tolerances[pvl1.Name()][i]);
        }

        if(difference > tolerance) {
          filesMatch = false;
          if(pvl1.Size() == 1) {
            differenceReason = "Keyword '" + pvl1.Name() + "': difference is " +
                               toString(difference);
          }
          else {
            differenceReason = "Keyword '" + pvl1.Name() + "' at index " +
                               toString(i) + ": difference is " + toString(difference);
          }
          differenceReason += " (tolerance is " + toString(tolerance) + ")";
        }
      }
    }
    catch(IException &) {
      if(val1.toLower() != val2.toLower()) {
        filesMatch = false;
        differenceReason = "Keyword '" + pvl1.Name() + "': values do not "
            "match.";
      }
    }
  }
}

void CompareObjects(const PvlObject &pvl1, const PvlObject &pvl2) {
  if(pvl1.Name().compare(pvl2.Name()) != 0) {
    filesMatch = false;
    differenceReason = "Object " + pvl1.Name() + " does not match " +
        pvl2.Name();
  }

  if(pvl1.Keywords() != pvl2.Keywords()) {
    filesMatch = false;
    differenceReason = "Object " + pvl1.Name() + " has varying keyword counts.";
  }

  if(pvl1.Groups() != pvl2.Groups()) {
    filesMatch = false;
    differenceReason = "Object " + pvl1.Name() + " has varying group counts.";
  }

  if(pvl1.Objects() != pvl2.Objects()) {
    filesMatch = false;
    differenceReason = "Object " + pvl1.Name() + " has varying object counts.";
  }

  if(!filesMatch) {
    return;
  }

  for(int keyword = 0; keyword < pvl1.Keywords() && filesMatch; keyword++) {
    CompareKeywords(pvl1[keyword], pvl2[keyword]);
  }

  for(int object = 0; object < pvl1.Objects() && filesMatch; object++) {
    CompareObjects(pvl1.Object(object), pvl2.Object(object));
  }

  for(int group = 0; group < pvl1.Groups() && filesMatch; group++) {
    CompareGroups(pvl1.Group(group), pvl2.Group(group));
  }

  if(!filesMatch && pvl1.Name().compare("Root") != 0) {
    differenceReason = "Object " + pvl1.Name() + ": " + differenceReason;
  }
}

void CompareGroups(const PvlGroup &pvl1, const PvlGroup &pvl2) {
  if(pvl1.Keywords() != pvl2.Keywords()) {
    filesMatch = false;
    return;
  }

  for(int keyword = 0; keyword < pvl1.Keywords() && filesMatch; keyword++) {
    CompareKeywords(pvl1[keyword], pvl2[keyword]);
  }

  if(!filesMatch) {
    differenceReason = "Group " + pvl1.Name() + ": " + differenceReason;
  }
}
