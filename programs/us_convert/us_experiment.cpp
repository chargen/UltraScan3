//! \file us_experiment.cpp

#include <QtCore>

#include "us_settings.h"
#include "us_db2.h"
#include "us_passwd.h"
#include "us_investigator.h"
#include "us_util.h"
#include "us_experiment.h"

US_Experiment::US_Experiment( void )
{
   // Experiment types
   experimentTypes.clear();
   experimentTypes << "Velocity"
                   << "Equilibrium"
                   << "Diffusion"
                   << "other";
}

// Function to see if the current runID already exists in the database
int US_Experiment::checkRunID( US_DB2* db )
{
   // Let's see if we can find the run ID
   expID = 0;
   QStringList q( "get_experiment_info_by_runID" );
   q << runID
     << QString::number( US_Settings::us_inv_ID() );
   db->query( q );
   if ( db->lastErrno() == US_DB2::NOROWS )
      return US_DB2::NOROWS;
   
   // Ok, let's update the experiment ID
   db->next();
   expID = db->value( 1 ).toString().toInt();
   return US_DB2::OK;
}

int US_Experiment::saveToDB( bool update, US_DB2* db )
{
   // Let's see if the project is in the db already
   int status = project.saveToDB( db );
   if ( status == US_DB2::NO_PROJECT )
      return status;

   else if ( status != US_DB2::OK )
      return status;

   // Get the RI information, if appropriate
   QByteArray RIxml;
   QByteArray RIxmlEscaped;
   createRIXml( RIxml );
   unsigned long length = db->mysqlEscapeString( RIxmlEscaped, RIxml, RIxml.size() );

   // Check for experiment runID in database
   int saveStatus = 0;
   QStringList q;
   status = checkRunID( db );
   if ( status == US_DB2::OK && ! update )
   {
      // Then the runID exists already, and we're not updating
      return US_DB2::DUPFIELD;
   }

   if ( status == US_DB2::OK )
   {
      // It's ok to update the existing experiment entry
      q.clear();
      q  << "update_experiment"
         << QString::number( expID )
         << QString( expGUID )
         << QString::number( project.projectID )
         << runID
         << QString::number( labID )
         << QString::number( instrumentID )
         << QString::number( operatorID )
         << QString::number( rotorID )
         << QString::number( calibrationID )
         << expType
         << opticalSystem
         << RIxmlEscaped
         << runTemp
         << label
         << comments
         << centrifugeProtocol;

      saveStatus = db->statusQuery( q );
   }

   else if ( status == US_DB2::NOROWS )
   {
      // Create new experiment entry
      q.clear();
      q  << "new_experiment"
         << expGUID
         << QString::number( project.projectID )
         << runID
         << QString::number( labID )
         << QString::number( instrumentID )
         << QString::number( operatorID )
         << QString::number( rotorID )
         << QString::number( calibrationID )
         << expType
         << opticalSystem
         << RIxml
         << runTemp
         << label
         << comments
         << centrifugeProtocol
         << QString::number( US_Settings::us_inv_ID() );

      saveStatus = db->statusQuery( q );
      expID = db->lastInsertID();
   }

   if ( expID == 0 )      // double check
   {
      qDebug() << "Error saving experiment: " << saveStatus
               << " " << db->lastError();
      return saveStatus;
   }

   // Let's get some info after db update
   q.clear();
   q << "get_experiment_info"
     << QString::number( expID );
   db->query( q );
   db->next();

   date = db->value( 12 ).toString();

   return US_DB2::OK;
}

// Function to read an experiment from DB
int US_Experiment::readFromDB( QString runID, 
                               US_DB2* db )
{
   QStringList q( "get_experiment_info_by_runID" );
   q << runID
     << QString::number( US_Settings::us_inv_ID() );
   db->query( q );

   QByteArray xmlFile;

   if ( db->next() )
   {
      this->runID        = runID;
      project.projectID  = db->value( 0 ).toInt();
      expID              = db->value( 1 ).toInt();
      expGUID            = db->value( 2 ).toString();
      labID              = db->value( 3 ).toInt();
      instrumentID       = db->value( 4 ).toInt();
      operatorID         = db->value( 5 ).toInt();
      rotorID            = db->value( 6 ).toInt();
      calibrationID      = db->value( 7 ).toInt();
      expType            = db->value( 8 ).toString();
      runTemp            = db->value( 9 ).toString();
      label              = db->value( 10 ).toString();
      comments           = db->value( 11 ).toString();
      centrifugeProtocol = db->value( 12 ).toString();
      date               = db->value( 13 ).toString();
      invID              = db->value( 14 ).toInt();
      opticalSystem      = db->value( 15 ).toString().toAscii();
      xmlFile            = db->value( 16 ).toString().toAscii();
   }

   else if ( db->lastErrno() == US_DB2::NOROWS )
      return US_DB2::NO_EXPERIMENT;

   else
      return( db->lastErrno() );

   // Get the rest of the info we need
   q.clear();
   q  << QString( "get_person_info" )
      << QString::number( invID );
   db->query( q );
   if ( db->next() )
      invGUID = db->value( 9 ).toString();

   project.readFromDB( project.projectID, db );

   // Hardware info
   operatorGUID = QString( "" );
   q.clear();
   q  << QString( "get_person_info" )
      << QString::number( operatorID );
   db->query( q );
   if ( db->next() )
      operatorGUID   = db->value( 9 ).toString();

   instrumentSerial = QString( "" );
   q.clear();
   q << QString( "get_instrument_info" )
     << QString::number( instrumentID );
   db->query( q );
   if ( db->next() )
      instrumentSerial = db->value( 1 ).toString();

   rotorGUID = QString( "" );
   q.clear();
   q << QString( "get_rotor_info" )
     << QString::number( rotorID );
   db->query( q );
   if ( db->next() )
   {
      rotorGUID   = db->value( 0 ).toString();
      rotorName   = db->value( 1 ).toString();
      rotorSerial = db->value( 2 ).toString();
   }

   if ( calibrationID == 0 )     // In this case, get the first one
   {
      q.clear();
      q << QString( "get_rotor_calibration_profiles" )
        << QString::number( rotorID );
      db->query( q );
      if ( db->next() )
         calibrationID = db->value( 0 ).toInt();
   }

   // Now get more calibration info
   q.clear();
   q << QString( "get_rotor_calibration_info" )
     << QString::number( calibrationID );
   db->query( q );
   if ( db->next() )
   {
      rotorCoeff1  = db->value( 4 ).toString().toFloat();
      rotorCoeff2  = db->value( 5 ).toString().toFloat();
      QStringList dateParts = db->value( 7 ).toString().split( " " );
      rotorUpdated = QDate::fromString( dateParts[ 0 ], "yyyy-MM-dd" );
   }

   // If this is RI data, get the intensity profile
   RIProfile.clear();
   if ( opticalSystem == "RI" )
   {
      int status = importRIxml( xmlFile );

      if ( status != US_Convert::OK )
      {
         // We don't really have a good error code in US_DB2 for this
         RIProfile.clear();
         return US_DB2::ERROR;
      }
   }
      
   return US_DB2::OK;
}

int US_Experiment::saveToDisk(
    QList< US_Convert::TripleInfo >& triples,
    QString runType,
    QString runID,
    QString dirname )
{ 
   QRegExp rx( "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$" );


   if ( this->expGUID.isEmpty() || ! rx.exactMatch( this->expGUID ) )
      this->expGUID = US_Util::new_guid();

   if ( dirname.right( 1 ) != "/" ) dirname += "/"; // Ensure trailing /
   QString writeFile = runID      + "." 
                     + runType    + ".xml";
   QFile file( dirname + writeFile );
   if ( !file.open( QIODevice::WriteOnly | QIODevice::Text) )
      return( US_Convert::CANTOPEN );

   QXmlStreamWriter xml;
   xml.setDevice( &file );
   xml.setAutoFormatting( true );

   xml.writeStartDocument();
   xml.writeDTD("<!DOCTYPE US_Scandata>");
   xml.writeStartElement("US_Scandata");
   xml.writeAttribute("version", "1.0");

   // elements
   xml.writeStartElement( "experiment" );
   xml.writeAttribute   ( "id",   QString::number( this->expID ) );
   xml.writeAttribute   ( "guid", this->expGUID );
   xml.writeAttribute   ( "type", this->expType );
   xml.writeAttribute   ( "runID", this->runID );

      xml.writeStartElement( "investigator" );
      xml.writeAttribute   ( "id", QString::number( this->invID ) );
      xml.writeAttribute   ( "guid", this->invGUID );
      xml.writeEndElement  ();
      
      xml.writeStartElement( "name" );
      xml.writeAttribute   ( "value", this->name );
      xml.writeEndElement  ();
      
      xml.writeStartElement( "project" );
      xml.writeAttribute   ( "id",   QString::number( this->project.projectID ) );
      xml.writeAttribute   ( "guid", this->project.projectGUID );
      xml.writeAttribute   ( "desc", this->project.projectDesc );
      xml.writeEndElement  ();
      
      xml.writeStartElement( "lab" );
      xml.writeAttribute   ( "id",   QString::number( this->labID   ) );
      xml.writeEndElement  ();
      
      xml.writeStartElement( "instrument" );
      xml.writeAttribute   ( "id",     QString::number( this->instrumentID ) );
      xml.writeAttribute   ( "serial", this->instrumentSerial );
      xml.writeEndElement  ();
      
      xml.writeStartElement( "operator" );
      xml.writeAttribute   ( "id", QString::number( this->operatorID ) );
      xml.writeAttribute   ( "guid", this->operatorGUID );
      xml.writeEndElement  ();

      xml.writeStartElement( "rotor" );
      xml.writeAttribute   ( "id",     QString::number( this->rotorID   ) );
      xml.writeAttribute   ( "guid",   this->rotorGUID );
      xml.writeAttribute   ( "serial", this->rotorSerial );
      xml.writeAttribute   ( "name", this->rotorName );
      xml.writeEndElement  ();

      xml.writeStartElement( "calibration" );
      xml.writeAttribute   ( "id",     QString::number( this->calibrationID   ) );
      xml.writeAttribute   ( "coeff1", QString::number( this->rotorCoeff1     ) );
      xml.writeAttribute   ( "coeff2", QString::number( this->rotorCoeff2     ) );
      xml.writeAttribute   ( "date",   this->rotorUpdated.toString( "yyyy-MM-dd" ) );
      xml.writeEndElement  ();

      // loop through the following for c/c/w combinations
      for ( int i = 0; i < triples.size(); i++ )
      {
         US_Convert::TripleInfo t = triples[ i ];
         if ( t.excluded ) continue;

         QString triple         = t.tripleDesc;
         QStringList parts      = triple.split(" / ");

         QString     cell       = parts[ 0 ];
         QString     channel    = parts[ 1 ];
         QString     wl         = parts[ 2 ];

         QString uuidc = US_Util::uuid_unparse( (unsigned char*)t.tripleGUID );
         xml.writeStartElement( "dataset" );
         xml.writeAttribute   ( "id", QString::number( t.tripleID ) );
         xml.writeAttribute   ( "guid", uuidc );
         xml.writeAttribute   ( "cell", cell );
         xml.writeAttribute   ( "channel", channel );

         if ( runType == "WA" )
            xml.writeAttribute( "radius", wl );

         else
            xml.writeAttribute( "wavelength", wl );

            xml.writeStartElement( "centerpiece" );
            xml.writeAttribute   ( "id", QString::number( t.centerpiece ) );
            xml.writeEndElement  ();

            xml.writeStartElement( "solution" );
            xml.writeAttribute   ( "id",   QString::number( t.solution.solutionID ) );
            xml.writeAttribute   ( "guid", t.solution.solutionGUID );
            xml.writeAttribute   ( "desc", t.solution.solutionDesc );
            xml.writeEndElement  ();

         xml.writeEndElement   ();
      }

   xml.writeStartElement( "opticalSystem" );
   xml.writeAttribute   ( "value", this->opticalSystem );
   xml.writeEndElement  ();

   xml.writeStartElement( "date" );
   xml.writeAttribute   ( "value", this->date );
   xml.writeEndElement  ();

   xml.writeStartElement( "runTemp" );
   xml.writeAttribute   ( "value", this->runTemp );
   xml.writeEndElement  ();

   xml.writeTextElement ( "label", this->label );
   xml.writeTextElement ( "comments", this->comments );
   xml.writeTextElement ( "centrifugeProtocol", this->centrifugeProtocol );

   xml.writeEndElement(); // US_Scandata
   xml.writeEndDocument();

   return( US_Convert::OK );
}

int US_Experiment::readFromDisk( 
    QList< US_Convert::TripleInfo >& triples,
    QString runType,
    QString runID,
    QString dirname )
{
   // First figure out the xml file name, and try to open it
   QString filename = runID      + "." 
                    + runType    + ".xml";

   QFile f( dirname + filename );
   if ( ! f.open( QIODevice::ReadOnly ) ) return US_Convert::CANTOPEN;
   QTextStream ds( &f );

   QXmlStreamReader xml( &f );

   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isStartElement() )
      {
         if ( xml.name() == "experiment" )
         {
            QXmlStreamAttributes a = xml.attributes();
            this->expID          = a.value("id").toString().toInt();
            this->expGUID        = a.value( "guid" ).toString();
            this->expType        = a.value( "type" ).toString();
            this->runID          = a.value( "runID" ).toString();
            readExperiment ( xml, triples, runType, runID );
         }
      }
   }

   bool error = xml.hasError();
   f.close();

   if ( error ) return US_Convert::BADXML;

   return US_Convert::OK;
}

void US_Experiment::readExperiment( 
     QXmlStreamReader& xml, 
     QList< US_Convert::TripleInfo >& triples,
     QString runType,
     QString runID )
{
   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isEndElement()  &&  xml.name() == "experiment" ) return;

      if ( xml.isStartElement() )
      {
         if ( xml.name() == "investigator" )
         {
            QXmlStreamAttributes a = xml.attributes();
            this->invID          = a.value( "id" ).toString().toInt();
            this->invGUID        = a.value( "guid" ).toString();
         }
 
         else if ( xml.name() == "name" )
         {
            QXmlStreamAttributes a = xml.attributes();
            this->name           = a.value( "value" ).toString();
         }

         else if ( xml.name() == "project" )
         {
            QXmlStreamAttributes a = xml.attributes();
            this->project.projectID      = a.value( "id"   ).toString().toInt();
            this->project.projectGUID    = a.value( "guid" ).toString();
            this->project.projectDesc    = a.value( "desc" ).toString();
         }
   
         else if ( xml.name() == "lab" )
         {
            QXmlStreamAttributes a = xml.attributes();
            this->labID    = a.value( "id" )  .toString().toInt();
         }
   
         else if ( xml.name() == "instrument" )
         {
            QXmlStreamAttributes a    = xml.attributes();
            this->instrumentID      = a.value( "id" )    .toString().toInt();
            this->instrumentSerial  = a.value( "serial" ).toString();
         }
   
         else if ( xml.name() == "operator" )
         {
            QXmlStreamAttributes a = xml.attributes();
            this->operatorID     = a.value( "id" ).toString().toInt();
            this->operatorGUID   = a.value( "guid" ).toString();
         }
   
         else if ( xml.name() == "rotor" )
         {
            QXmlStreamAttributes a = xml.attributes();
            this->rotorID        = a.value( "id"     ).toString().toInt();
            this->rotorGUID      = a.value( "guid"   ).toString();
            this->rotorSerial    = a.value( "serial" ).toString();
            this->rotorName      = a.value( "name" ).toString();
            this->calibrationID  = a.value( "calibrationID" ).toString().toInt();
         }
   
         else if ( xml.name() == "calibration" )
         {
            QXmlStreamAttributes a = xml.attributes();
            this->calibrationID      = a.value( "id"     ).toString().toInt();
            this->rotorCoeff1        = a.value( "coeff1" ).toString().toFloat();
            this->rotorCoeff2        = a.value( "coeff2" ).toString().toFloat();
            this->rotorUpdated       = QDate::fromString( a.value( "date" ).toString(), "yyyy-MM-dd" );
         }

         else if ( xml.name() == "dataset" )
         {
            QXmlStreamAttributes a = xml.attributes();
            QString cell           = a.value( "cell" ).toString();
            QString channel        = a.value( "channel" ).toString();

            QString wl;
            if ( runType == "WA" )
               wl                  = a.value( "radius" ).toString();

            else
               wl                  = a.value( "wavelength" ).toString();
   
            // Find the index of this triple
            QString triple         = cell + " / " + channel + " / " + wl;
            bool found             = false;
            int ndx                = 0;
            for ( int i = 0; i < triples.size(); i++ )
            {
               if ( triples[ i ].excluded ) continue;

               if ( triple == triples[ i ].tripleDesc )
               {
                  found = true;
                  ndx   = i;
                  break;
               }
            }

            if ( found )
            {
               triples[ ndx ].tripleID = a.value( "id" ).toString().toInt();
               QString uuidc = a.value( "guid" ).toString();
               US_Util::uuid_parse( uuidc, (unsigned char*) triples[ ndx ].tripleGUID );

               if ( runType == "WA" )
               {
                   double r       = wl.toDouble() * 1000.0;
                   QString radius = QString::number( (int) round( r ) );
                   triples[ ndx ].tripleFilename = runID      + "." 
                                                 + runType    + "." 
                                                 + cell       + "." 
                                                 + channel    + "." 
                                                 + radius     + ".auc";
               }
               
               else
               {
                   triples[ ndx ].tripleFilename = runID      + "." 
                                                 + runType    + "." 
                                                 + cell       + "." 
                                                 + channel    + "." 
                                                 + wl         + ".auc";
               }

               triples[ ndx ].excluded       = false;

               readDataset( xml, triples[ ndx ] );
            }
         }

         else if ( xml.name() == "opticalSystem" )
         {
            QXmlStreamAttributes a = xml.attributes();
            this->opticalSystem  = a.value( "value" ).toString().toAscii();
         }

         else if ( xml.name() == "date" )
         {
            QXmlStreamAttributes a = xml.attributes();
            this->date           = a.value( "value" ).toString();
         }

         else if ( xml.name() == "runTemp" )
         {
            QXmlStreamAttributes a = xml.attributes();
            this->runTemp        = a.value( "value" ).toString();
         }

         else if ( xml.name() == "label" )
         {
            xml.readNext();
            this->label = xml.text().toString();
         }

         else if ( xml.name() == "comments" )
         {
            xml.readNext();
            this->comments = xml.text().toString();
         }

         else if ( xml.name() == "centrifugeProtocol" )
         {
            xml.readNext();
            this->centrifugeProtocol = xml.text().toString();
         }

      }

   }
}

void US_Experiment::readDataset( QXmlStreamReader& xml, US_Convert::TripleInfo& triple )
{
   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isEndElement()  &&  xml.name() == "dataset" ) return;

      if ( xml.isStartElement() )
      {
         if ( xml.name() == "centerpiece" )
         {
            QXmlStreamAttributes a = xml.attributes();
            triple.centerpiece     = a.value( "id" ).toString().toInt();
         }
 
         else if ( xml.name() == "solution" )
         {
            QXmlStreamAttributes a = xml.attributes();
            triple.solution.solutionID   = a.value( "id"   ).toString().toInt();
            triple.solution.solutionGUID = a.value( "guid" ).toString();
            triple.solution.solutionDesc = a.value( "desc" ).toString();
         }

      }
   }
}

int US_Experiment::saveRIDisk( 
    QString runID,
    QString dirname )
{ 
   if ( dirname.right( 1 ) != "/" ) dirname += "/"; // Ensure trailing /
   QString writeFile = runID      + "." 
                     + "RIProfile.xml";
   QFile file( dirname + writeFile );
   if ( !file.open( QIODevice::WriteOnly | QIODevice::Text) )
      return( US_Convert::CANTOPEN );

   // Get the RI information
   QByteArray RIxml;
   createRIXml( RIxml );

   QTextStream out( &file );
   out << RIxml;

   return( US_Convert::OK );
}

int US_Experiment::readRIDisk( 
    QString runID,
    QString dirname )
{
   // First figure out the xml file name, and try to open it
   QString filename = runID      + "." 
                    + "RIProfile.xml";

   QFile f( dirname + filename );
   if ( ! f.open( QIODevice::ReadOnly ) ) return US_Convert::CANTOPEN;
   QTextStream ds( &f );

   QXmlStreamReader xml( &f );

   this->RIProfile.clear();
   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isStartElement() )
      {
         if ( xml.name() == "RI" )
         {
            QXmlStreamAttributes a = xml.attributes();
            double value           = a.value("value").toString().toDouble();
            this->RIProfile << value;
         }
      }
   }

   bool error = xml.hasError();
   f.close();

   if ( error ) return US_Convert::BADXML;

   return US_Convert::OK;
}

void US_Experiment::createRIXml( QByteArray& str )
{
   if ( this->opticalSystem != "RI" )
   {
      str = QByteArray( "" );
      return ;
   }

   if ( this->RIProfile.size() == 0 )
   {
      str = QByteArray( "" );
      return ;
   }

   // Create the RI xml directly, and write it to a buffer
   // QByteArray xmlFile;
   // QBuffer buffer( &xmlFile );
   QBuffer buffer( &str );
   buffer.open( QIODevice::WriteOnly );

   QXmlStreamWriter xml;
   xml.setDevice( &buffer );
   xml.setAutoFormatting( true );

   xml.writeStartDocument();
   xml.writeDTD("<!DOCTYPE US_RIProfile>");
   xml.writeStartElement("US_RIProfile");
   xml.writeAttribute("version", "1.0");

   // Loop through the intensity profile values
   foreach ( double value, this->RIProfile )
   {
      xml.writeStartElement( "RI" );
      xml.writeAttribute   ( "value", QString::number( value ) );
      xml.writeEndElement  ();

   }

   xml.writeEndElement(); // US_RIProfile
   xml.writeEndDocument();

   buffer.close();
}

int US_Experiment::importRIxml( QByteArray& str )
{
   QBuffer buffer( &str );
   buffer.open( QIODevice::ReadOnly );

   QXmlStreamReader xml( &buffer );

   this->RIProfile.clear();
   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isStartElement() )
      {
         if ( xml.name() == "RI" )
         {
            QXmlStreamAttributes a = xml.attributes();
            double value           = a.value("value").toString().toDouble();
            this->RIProfile << value;
         }
      }
   }

   bool error = xml.hasError();
   buffer.close();

   if ( error ) return US_Convert::BADXML;

   return US_Convert::OK ;
}

// Zero out all data structures
void US_Experiment::clear( void )
{
   invID              = US_Settings::us_inv_ID();
   invGUID            = QString( "" );
   name               = US_Settings::us_inv_name();
   expID              = 0;
   expGUID            = QString( "" );
   project.clear();
   runID              = QString( "" );
   labID              = 0;
   instrumentID       = 0;
   instrumentSerial   = QString( "" );
   operatorID         = 0;
   operatorGUID       = QString( "" );
   rotorID            = 0;
   calibrationID      = 0;
   rotorCoeff1        = 0.0;
   rotorCoeff2        = 0.0;
   rotorGUID          = QString( "" );
   rotorSerial        = QString( "" );
   rotorName          = QString( "" );
   expType            = QString( "" );
   opticalSystem      = QByteArray( "  " );
   rpms.clear();
   runTemp            = QString( "" );
   label              = QString( "" );
   comments           = QString( "" );
   centrifugeProtocol = QString( "" );
   date               = QString( "" );
   syncOK             = false;
   RIProfile.clear();

   rpms.clear();
}

void US_Experiment::show( void )
{
   QString syncOK_text = ( syncOK ) ? "true" : "false";

   qDebug() << "invID        = " << invID << '\n'
            << "invGUID      = " << invGUID << '\n'
            << "name         = " << name << '\n'
            << "expID        = " << expID << '\n'
            << "expGUID      = " << expGUID << '\n'
            << "projectID    = " << project.projectID << '\n'
            << "projectGUID  = " << project.projectGUID << '\n'
            << "projectDesc  = " << project.projectDesc << '\n'
            << "runID        = " << runID << '\n'
            << "labID        = " << labID << '\n'
            << "instrumentID = " << instrumentID << '\n'
            << "instrumentSerial = " << instrumentSerial << '\n'
            << "operatorID   = " << operatorID << '\n'
            << "operatorGUID = " << operatorGUID << '\n'
            << "rotorID      = " << rotorID << '\n'
            << "rotorGUID    = " << rotorGUID << '\n'
            << "rotorSerial  = " << rotorSerial << '\n'
            << "rotorName    = " << rotorName << '\n'
            << "calibrationID = " << calibrationID << '\n'
            << "rotorCoeff1  = " << rotorCoeff1 << '\n'
            << "rotorCoeff2  = " << rotorCoeff2 << '\n'
            << "rotorUpdated = " << rotorUpdated.toString( "yyyy-MM-dd" ) << '\n'
            << "expType      = " << expType << '\n'
            << "opticalSystem = " << opticalSystem << '\n'
            << "runTemp      = " << runTemp << '\n'
            << "label        = " << label << '\n'
            << "comments     = " << comments << '\n'
            << "centrifugeProtocol = " << centrifugeProtocol << '\n'
            << "date         = " << date << '\n'
            << "syncOK       = " << syncOK_text << '\n';

   for ( int i = 0; i < rpms.size(); i++ )
   {
      qDebug() << "i = " << i ;
      qDebug() << "rpm = " << rpms[ i ];
   }

   qDebug() << "RI Profile data";
   for ( int i = 0; i < RIProfile.size(); i++ )
      qDebug() << RIProfile[ i ];
}
