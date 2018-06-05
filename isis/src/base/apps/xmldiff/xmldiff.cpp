#include "Isis.h"

#include <cmath>
#include <float.h>
#include <fstream>
#include <string>

#include <QProcess>
#include <QRegularExpression>

#include "IException.h"
#include "FileName.h"
#include "OriginalXmlLabel.h"
#include "PvlContainer.h"
#include "Pvl.h"

using namespace std;
using namespace Isis;

PvlGroup compare(FileName file1, FileName file2);
QString format(FileName inputFile, QString outputFileName);
void cleanupTempFiles(QString file1, QString file2);
void readFile(FileName &file, QString &content);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  FileName file1(ui.GetFileName("FROM"));
  FileName file2(ui.GetFileName("FROM2"));

  PvlGroup differences = compare(file1, file2);

  Application::Log(differences);

  if ( ui.WasEntered("TO") ) {
    Pvl pvl;
    pvl.addGroup(differences);
    pvl.write(ui.GetFileName("TO"));
  }
}


/**
*  @author 2018-05-13 Adam Goins
*
*  This method is designed to receive two xml files to compare. It runs xmllint to format the files
*  and then sorts them with respect to their tag heiarchy so that tags can appear in different
*  order without affecting equivelance. This comparison is done ignoring spacing by default,
*  however the users can optionally toggle the ability to turn off ignoreSpacing, meaning
*  comparison happens with respect to any spacing that occurs in the documents.
*
*  @param file1 The first file to compare against.
*  @param file2 The second file to compare against.
*  @param ignoreSpaces If comparison should be done with respect to any spacing in the two files.
*/
PvlGroup compare(FileName file1, FileName file2) {

  QString outputFile1 = format( file1, QString("temporaryOutput1.xml") );
  QString outputFile2 = format( file2, QString("temporaryOutput2.xml") );

  PvlGroup differences("Results");

  QProcess exec;
  QString command("bash -c \"diff " + outputFile1 + " " + outputFile2 + " \"");
  exec.start(command);
  exec.waitForFinished();

  cleanupTempFiles(outputFile1, outputFile2);

  QString exitCode = QString(exec.exitCode());
  QString output(exec.readAllStandardOutput());
  QString error(exec.readAllStandardError());

  QStringList list = output.split("\n");

  if ( output.count() == 0 && error.count() == 0 ) {
    differences += PvlKeyword("Compare", "Identical");
    return differences;
  }

  if ( error.count() > 0 ) {
    differences += PvlKeyword("Compare", "Error");
    differences += PvlKeyword("Exit_Code", exitCode);
    differences += PvlKeyword("Details", error);
    return differences;
  }

  differences += PvlKeyword("Compare", "Different");
  return differences;
}


/**
*  @author 2018-05-13 Adam Goins
*
*  This method is designed receive an input xml file to be compared against, format it in a manner
*  that allows for easy comparison, and output the formatted file to the temporary outputFileName.
*
*  If spaces are to be ignored, we use the trCommand to strip newline and return characters
*  and use the sedCommand to strip all whitespacing from the file.
*
*  The lintCommand uses xmllint to format the xml file and the sortCommand sorts the output xml
*  file with respect to it's tag heirarchy to allow order-invariant comparison.
*
*  @param inputFile The input file that we will be formatting to a comparison-consistent format.
*  @param outputFileName The file name we will write the temporary formatted xml file to.
*/
QString format(FileName inputFile, QString outputFileName) {

  QProcess format;
  QString formatCommand("bash -c \"");
  QString trCommand("tr '\\n\\r' ' ' < " + inputFile.toString() + " > " + outputFileName);
  QString sedCommand("sed -i -z 's/ //g' " + outputFileName);
  QString sortCommand("sort -o " + outputFileName + " " + outputFileName);
  QString lintCommand;

  lintCommand += "xmllint --format --noblanks " + inputFile.toString() + " > " + outputFileName;
  formatCommand += lintCommand + " && " + sortCommand;
  formatCommand += "\"";

  format.start(formatCommand);
  format.waitForFinished();
  return outputFileName;
}

/**
*  @author 2018-05-13 Adam Goins
*
*  This method is designed to remove the temporary formatted files that were created for comparison.
*
*  @param file1 The first temporary file to remove.
*  @param file2 The second temporary file to remove.
*/
void cleanupTempFiles(QString file1, QString file2) {
  QProcess exec;
  QString command("bash -c \"rm " + file1 + " " + file2 + "\"");
  exec.start(command);
  exec.waitForFinished();
}
