/**
 * @file
 * $Revision: 1.1.1.1 $
 * $Date: 2006/10/31 23:18:08 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

using namespace std;

#include "Message.h"

QString Isis::Message::KeywordValueNotInList(const QString &key, const QString &value,
                                             const vector<QString> &list) {
  QString message;
  message = "Keyword [" + key + "=" + value + "] must be one of [";
  for(unsigned int i = 0; i < list.size(); i++) {
    message += list[i];
    if(i < (list.size() - 1)) message += ",";
  }
  message += "]";

  return message;
}
