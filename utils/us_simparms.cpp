//! \file us_simparms.cpp

#include "us_simparms.h"
#include "us_astfem_math.h"
#include "us_hardware.h"
#include "us_settings.h"
#include "us_constants.h"

//!< level-conditioned debug print
#define DbgLv(a) if(dbg_level>=a)qDebug()

US_SimulationParameters::US_SimulationParameters()
{
   mesh_radius.clear(); 
   speed_step.clear();

   speed_step << SpeedProfile();

   simpoints         = 200;  
   meshType          = ASTFEM;
   gridType          = MOVING;
   radial_resolution = 0.001;
   meniscus          = 5.8;
   bottom            = 7.2;
   rnoise            = 0.0;  
   tinoise           = 0.0;      
   rinoise           = 0.0;     
   temperature       = NORMAL_TEMP;
   rotorCalID        = "0";      
   band_forming      = false;   
   band_volume       = 0.015;
   bottom_position   = 7.2;
   rotorcoeffs[ 0 ]  = 0.0;
   rotorcoeffs[ 1 ]  = 0.0;
   cp_sector         = 0;
   cp_pathlen        = 1.2;
   cp_angle          = 2.5;
   cp_width          = 0.0;

   firstScanIsConcentration = false;
}

US_SimulationParameters::SpeedProfile::SpeedProfile()
{
   duration_hours    = 0;
   duration_minutes  = 0;
   delay_hours       = 0;
   delay_minutes     = 0;
   scans             = 0;
   acceleration      = 400;
   rotorspeed        = 0;
   acceleration_flag = true;
}

// Set simulation parameter values from an experimental EditedData set.
void US_SimulationParameters::initFromData( US_DB2* db,
   US_DataIO2::EditedData& editdata )
{
   SpeedProfile sp;

   int     dbg_level   = US_Settings::us_debug();
   int     scanCount   = editdata.scanData.size();
   double  time1       = editdata.scanData[ 0 ].seconds;
   double  time2       = editdata.scanData[ 0 ].seconds;
   double  rpm         = editdata.scanData[ 0 ].rpm;
   double  rpmnext     = rpm;
   int     jj          = 0;
   int     cp_id       = 0;
   QString channel     = editdata.channel;
   int     ch          = QString( "ABCDEFGH" ).indexOf( channel );
           ch          = qMax( 0, ch );
           ch         /= 2;
//DbgLv(2) << "SP:iFD: ch" << ch << editdata.channel << (ch+1);
   QString ecell       = editdata.cell;
   int     iecell      = ecell.toInt();

   rotorCalID          = "0";
   QString fn          = US_Settings::resultDir() + "/" + editdata.runID + "/"
                         + editdata.runID + "." + editdata.dataType + ".xml";
   QFile file( fn );

   if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
   {  // If experiment/run file exists, get calibration,centerpiece IDs from it
      QXmlStreamReader xml( &file );
      int dcp_id       = 0;
      int dcell        = 0;
      QString dchan    = "";

      while ( ! xml.atEnd() )
      {
         xml.readNext();

         if ( xml.isStartElement()  &&  xml.name() == "calibration" )
         {  // Pick up rotor calibration ID from  <calibration ... id=...
            QXmlStreamAttributes a = xml.attributes();
            rotorCalID       = a.value( "id"      ).toString();
            rotorcoeffs[ 0 ] = a.value( "coeff1"  ).toString().toDouble();
            rotorcoeffs[ 1 ] = a.value( "coeff2"  ).toString().toDouble();
         }

         if ( xml.isStartElement()  &&  xml.name() == "dataset" )
         {  // Pick up cell and channel for comparison
            QXmlStreamAttributes a = xml.attributes();
            dcell            = a.value( "cell"    ).toString().toInt();
            dchan            = a.value( "channel" ).toString();
         }

         if ( xml.isStartElement()  &&  xml.name() == "centerpiece" )
         {  // Pick up centerpiece ID from  <centerpiece ... id=...
            QXmlStreamAttributes a = xml.attributes();
            dcp_id           = a.value( "id"      ).toString().toInt();
            if ( dcell == iecell  &&  dchan == channel )
            { // If cell,channel match edit, pick up CpID
               cp_id            = dcp_id;
            }
         }
      }

      file.close();

      if ( cp_id == 0 )    // If no cell,chan match; use last CP ID
         cp_id            = dcp_id;
   }
//DbgLv(2) << "SP:iFD: cp_id ch" << cp_id << ch;

   bottom_position     = 7.2;
   meniscus            = editdata.meniscus;

   speed_step.clear();
DbgLv(1) << "SP:iFD: scan" << 1 << "rpm time omega2t"
 << rpm << time1 << editdata.scanData[0].omega2t;

   for ( int ii = 1; ii < scanCount; ii++ )
   {  // Loop to build speed steps where RPM changes
      rpmnext          = editdata.scanData[ ii ].rpm;
DbgLv(2) << "SP:iFD: scan" << (ii+1) << "rpm time omega2t"
 << rpmnext << editdata.scanData[ii].seconds << editdata.scanData[ii].omega2t;

      if ( rpm != rpmnext  &&  ( qAbs( rpm - rpmnext ) / rpm ) < 0.005 )
      {  // Ignore apparent speed-step if the rpm change is small & for 1 scan
         if ( ii == ( scanCount - 1 )  ||
              rpm == editdata.scanData[ ii + 1 ].rpm )
            rpmnext = rpm;
         if ( ii == 1 )
            rpm     = rpmnext;
      }

      if ( rpm != rpmnext )
      {  // RPM has changed, so need to create speed step for previous scans
         time2               = editdata.scanData[ ii - 1 ].seconds;
         sp.duration_hours   = (int)( time2 / 3600.0 );
         sp.duration_minutes = (int)( time2 / 60.0 )
                               - ( sp.duration_hours * 60 );
         sp.delay_hours      = (int)( time1 / 3600.0 );
         sp.delay_minutes    = ( time1 / 60.0 )
                               - ( (double)sp.delay_hours * 60.0 );
         sp.scans            = ii - jj;
         sp.rotorspeed       = (int)rpm;
         speed_step.append( sp );
DbgLv(1) << "SP:iFD:   speedstep" << speed_step.size() << "scans" << sp.scans
 << "duration hrs mins" << sp.duration_hours << sp.duration_minutes
 << "delay hrs mins" << sp.delay_hours << sp.delay_minutes << "rpm" << rpm;

         jj                  = ii;
         rpm                 = rpmnext;
         time1               = editdata.scanData[ ii     ].seconds;
      }
   }

   // Set final (only?) speed step
   time2               = editdata.scanData[ scanCount - 1 ].seconds;
   sp.duration_hours   = (int)( time2 / 3600.0 );
   sp.duration_minutes = (int)( time2 / 60.0 ) - ( sp.duration_hours * 60 );
   sp.delay_hours      = (int)( time1 / 3600.0 );
   sp.delay_minutes    = ( time1 / 60.0 ) - ( (double)sp.delay_hours * 60.0 );
   sp.scans            = scanCount - jj;
   sp.rotorspeed       = (int)rpm;
   speed_step.append( sp );
DbgLv(1) << "SP:iFD:   speedstep" << speed_step.size() << "scans" << sp.scans
 << "duration hrs mins" << sp.duration_hours << sp.duration_minutes
 << "delay hrs mins" << sp.delay_hours << sp.delay_minutes << "rpm" << rpm;

#ifndef NO_DB
   int     iechan      = ch + 1;
   if ( db != NULL )
   {  // If reading from the database, get rotor,centerpiece info from DB
      int         stat_db = 0;
      bool        ok_db;
      QString     expID;
      QStringList query;
      QString     rcalIDsv = rotorCalID;  // Save IDs gotten from local file
      int         cpIDsv   = cp_id;
//DbgLv(2) << "Sim parms:runID" << editdata.runID;
//DbgLv(2) << "Sim parms:invID" << US_Settings::us_inv_ID();
      query << "get_experiment_info_by_runID"
            << editdata.runID
            << QString::number( US_Settings::us_inv_ID() );
      db->query( query );
      stat_db = db->lastErrno();
//DbgLv(2) << "Sim parms:query() stat" << stat_db;
      if ( stat_db != US_DB2::NOROWS )
      {  // Info by runID:  experiment and calibration IDs
         ok_db      = db->next();
//DbgLv(2) << "Sim parms: next() ok_db" << ok_db;
         if ( ok_db )
         {
            expID      = db->value( 1 ).toString();
            rotorCalID = db->value( 7 ).toString();
//DbgLv(2) << "Sim parms: expID" << expID;
//DbgLv(2) << "Sim parms: rotorCalID" << rotorCalID << "sv" << rcalIDsv;
         }
         else
            rotorCalID = "";

         if ( rotorCalID.isEmpty()  &&  ! expID.isEmpty() )
         {  // If still no calibration ID, try it another way
            query.clear();
            query << "get_experiment_info" << expID;
            db->query( query );
            stat_db = db->lastErrno();
            if ( stat_db != US_DB2::NOROWS  &&  db->next() )
            {
               rotorCalID = db->value( 7 ).toString();
//DbgLv(2) << "Sim parms(2):     rotorCalID" << rotorCalID;
            }
         }

         // If unable to get calibration ID from DB, fall back to local info
         if ( rotorCalID.isEmpty() || rotorCalID == "0" )
            rotorCalID = rcalIDsv;
//DbgLv(2) << "Sim parms(3):     rotorCalID" << rotorCalID;

         if ( ! expID.isEmpty() )
         {  // Get centerpiece ID from cell records for this experiment
            query.clear();
            query << "all_cell_experiments" << expID;
            db->query( query );
            while ( db->next() )
            {
               int cell     = db->value( 2 ).toInt();
               int ichan    = db->value( 3 ).toInt();
               int cellCpId = db->value( 4 ).toInt();

               if ( cellCpId > 0  &&  cell == iecell  &&  ichan == iechan )
               { // Valid CpID with cell,channel matching edit
                  cp_id        = cellCpId;
                  break;
               }
            }
         }

DbgLv(1) << "Sim parms:        cp_id" << cp_id << "sv" << cpIDsv;
         // If no centerpiece ID from DB, fall back to local info
         if ( cp_id < 1 )
            cp_id     = cpIDsv;
//DbgLv(2) << "Sim parms(2):     cp_id" << cp_id;
      }
   }

   // Set rotor coefficients, channel bottom position from hardware files
   setHardware( db, rotorCalID, -cp_id, ch );

   // Calculate bottom using RPM, start bottom, and rotor coefficients
   bottom = US_AstfemMath::calc_bottom( rpm, bottom_position, rotorcoeffs );
#else
   // For NO_DB (back end) the bottom needs to be set after this function
   bottom = bottom_position;
   db     = NULL; // Stop compiler warning
#endif
DbgLv(2) << "SP:iFD: bottom" << bottom;
}

// Set parameters from hardware files, related to rotor and centerpiece
void US_SimulationParameters::setHardware( US_DB2* db, QString rCalID,
      int cp, int ch )
{
   int dbg_level    = US_Settings::us_debug();
   rotorCalID       = rCalID;
   bottom_position  = 7.2;
//DbgLv(2) << "sH: cp ch rCalID" << cp << ch << rCalID;

   QList< US_AbstractCenterpiece > cp_list;
   QMap < QString, QString       > rotor_map;
   rotor_map.clear();

   if ( US_AbstractCenterpiece::read_centerpieces( db, cp_list ) )
   {
      if ( cp < 0 )
      {  // If cp given is negative, this means look for a serial number
         int cp_id = -cp;
             cp    = 0;

         for ( int jj = 0; jj < cp_list.size(); jj++ )
         {
            if ( cp_id == cp_list[ jj ].serial_number )
            {  // Replace cp value (serial) with an index in the list
               cp   = jj;
               break;
            }
         }
DbgLv(1) << "sH: cp ch cp_id" << cp << ch << cp_id;
      }

      // Pick up centerpiece info by Centerpiece and Channel indecies
      QStringList shapes;
      shapes << "sector" << "standard" << "rectangular" << "band forming"
             << "meniscus matching" << "circular" << "synthetic";
      QString shape   = cp_list[ cp ].shape;
      bottom_position = cp_list[ cp ].bottom_position[ ch ];
      cp_pathlen      = cp_list[ cp ].path_length    [ ch ];
      cp_angle        = cp_list[ cp ].angle;
      cp_width        = cp_list[ cp ].width;
      cp_sector       = qMax( 0, shapes.indexOf( shape ) );
      band_forming    = ( shape == "band forming" );

   }

   if ( US_Hardware::readRotorMap( db, rotor_map ) )
   {  // Get rotor coefficients by matching calibration ID
      US_Hardware::rotorValues( rotorCalID, rotor_map, rotorcoeffs );
   }

   else
      qDebug() << "setHardware:readRotorMap *ERROR*";

   return;
}

// Set parameters from hardware files, related to rotor and centerpiece (Local)
void US_SimulationParameters::setHardware( QString rCalID, int cp, int ch )
{
   return setHardware( NULL, rCalID, cp, ch );
}

// Load simulation parameters from file
int US_SimulationParameters::load_simparms( QString fname )
{
   int stat = 0;
   int kr   = 0;
   QStringList meshlist;
   QStringList gridlist;
   meshlist << "ASTFEM" << "Claverie" << "MovingHat" << "User" << "ASTFVM";
   gridlist << "Fixed" <<  "Moving";
   US_SimulationParameters::SpeedProfile sp;

   QFile xfile( fname );

   if ( xfile.open( QIODevice::ReadOnly | QIODevice::Text ) )
   {
      QXmlStreamReader xml( &xfile );
      QXmlStreamAttributes a;
      QString astr;
      int     kk;
      speed_step.clear();

      while ( ! xml.atEnd() )
      {
         xml.readNext();

         if ( xml.isStartElement()  &&  xml.name() == "params" )
         {
            a     = xml.attributes();

            astr  = a.value( "meshType" ).toString();
            if ( !astr.isEmpty() )
            {
               kk        = meshlist.indexOf( astr );
               meshType  = (US_SimulationParameters::MeshType)kk;
            }
            astr      = a.value( "gridType" ).toString();
            if ( !astr.isEmpty() )
            {
               kk        = gridlist.indexOf( astr );
               gridType  = (US_SimulationParameters::GridType)kk;
            }
            astr  = a.value( "simpoints"   ).toString();
            if ( !astr.isEmpty() )
               simpoints    = astr.toInt();
            astr  = a.value( "radialres"   ).toString();
            if ( !astr.isEmpty() )
               radial_resolution = astr.toDouble();
            astr  = a.value( "meniscus"    ).toString();
            if ( !astr.isEmpty() )
               meniscus     = astr.toDouble();
            astr  = a.value( "bottom"      ).toString();
            if ( !astr.isEmpty() )
               bottom       = astr.toDouble();
            astr  = a.value( "rnoise"      ).toString();
            if ( !astr.isEmpty() )
               rnoise       = astr.toDouble();
            astr  = a.value( "tinoise"     ).toString();
            if ( !astr.isEmpty() )
               tinoise      = astr.toDouble();
            astr  = a.value( "rinoise"     ).toString();
            if ( !astr.isEmpty() )
               rinoise      = astr.toDouble();
            astr  = a.value( "temperature" ).toString();
            if ( !astr.isEmpty() )
               temperature  = astr.toDouble();
            astr  = a.value( "bandform"    ).toString();
            if ( !astr.isEmpty() )
               band_forming = ( astr == "yes" || astr == "1" );
            else
               band_forming = false;
            if ( band_forming )
               band_volume  = a.value( "bandvolume" ).toString().toDouble();
            else
               band_volume  = 0.0;
            astr  = a.value( "rotorCalID" ).toString();
            if ( !astr.isEmpty() )
               rotorCalID   = astr;
            astr  = a.value( "rotorcoeffs" ).toString().simplified();
            if ( !astr.isEmpty() )
            {
               rotorcoeffs[ 0 ] = astr.section( " ", 0, 0 ).toDouble();
               rotorcoeffs[ 1 ] = astr.section( " ", 1, 1 ).toDouble();
            }
            astr  = a.value( "sector"      ).toString();
            if ( !astr.isEmpty() )
               cp_sector    = astr.toInt();
            astr  = a.value( "pathlength"  ).toString();
            if ( !astr.isEmpty() )
               cp_pathlen   = astr.toDouble();
            astr  = a.value( "angle"       ).toString();
            if ( !astr.isEmpty() )
               cp_angle     = astr.toDouble();
            astr  = a.value( "width"       ).toString();
            if ( !astr.isEmpty() )
               cp_width     = astr.toDouble();
         }

         else if ( xml.isStartElement()  &&  xml.name() == "speedstep" )
         {
            a     = xml.attributes();

            astr  = a.value( "duration_hrs"  ).toString();
            if ( !astr.isEmpty() )
               sp.duration_hours    = astr.toInt();
            astr  = a.value( "duration_mins" ).toString();
            if ( !astr.isEmpty() )
               sp.duration_minutes  = astr.toInt();
            astr  = a.value( "delay_hrs"     ).toString();
            if ( !astr.isEmpty() )
               sp.delay_hours       = astr.toInt();
            astr  = a.value( "delay_mins"    ).toString();
            if ( !astr.isEmpty() )
               sp.delay_minutes     = astr.toDouble();
            astr  = a.value( "rotorspeed"    ).toString();
            if ( !astr.isEmpty() )
               sp.rotorspeed        = astr.toInt();
            astr  = a.value( "acceleration"  ).toString();
            if ( !astr.isEmpty() )
               sp.acceleration      = astr.toInt();
            astr  = a.value( "accelerflag"   ).toString();
            if ( !astr.isEmpty() )
               sp.acceleration_flag = ( astr == "yes" || astr == "1" );
            astr  = a.value( "scans"         ).toString();
            if ( !astr.isEmpty() )
               sp.scans             = astr.toInt();

            speed_step.append( sp );
         }

         else if ( xml.isStartElement()  &&  xml.name() == "usermesh" )
         {
            a     = xml.attributes();

            if ( kr == 0 )
               mesh_radius.clear();

            kr++;
            mesh_radius << a.value( "radius" ).toString().toDouble();
         }
      }
   }

   else
   {
      stat = -1;
   }

   return stat;
}

// Save simulation parameters to file
int US_SimulationParameters::save_simparms( QString fname )
{
   int stat = 0;
   const char* mesh[] = { "ASTFEM", "Claverie", "MovingHat", "User", "ASTFVM" };
   const char* grid[] = { "Fixed",  "Moving" };
   US_SimulationParameters::SpeedProfile* spi;

   QFile xfile( fname );

   if ( xfile.open( QIODevice::WriteOnly | QIODevice::Text ) )
   {
      QXmlStreamWriter xml( &xfile );
      xml.setAutoFormatting( true );

      xml.writeStartDocument();
      xml.writeDTD         ( "<!DOCTYPE US_SimParams>" );
      xml.writeStartElement( "SimParams" );
      xml.writeAttribute   ( "version", "1.0" );

      xml.writeStartElement( "params" );
      xml.writeAttribute   ( "meshType",    QString( mesh[ (int)meshType ] ) );
      xml.writeAttribute   ( "gridType",    QString( grid[ (int)gridType ] ) );
      xml.writeAttribute   ( "simpoints",   QString::number( simpoints ) );
      xml.writeAttribute   ( "radialres", QString::number( radial_resolution ));
      xml.writeAttribute   ( "meniscus",    QString::number( meniscus ) );
      xml.writeAttribute   ( "bottom",      QString::number( bottom ) );
      xml.writeAttribute   ( "rnoise",      QString::number( rnoise ) );
      xml.writeAttribute   ( "tinoise",     QString::number( tinoise ) );
      xml.writeAttribute   ( "rinoise",     QString::number( rinoise ) );
      xml.writeAttribute   ( "temperature", QString::number( temperature ) );

      if ( ! rotorCalID.isEmpty() )
         xml.writeAttribute   ( "rotorCalID", rotorCalID );

      if ( rotorcoeffs[ 0 ] != 0.0 )
      {
         xml.writeAttribute   ( "rotorcoeffs", QString().sprintf( "%.3e %.3e",
            rotorcoeffs[ 0 ], rotorcoeffs[ 1 ] ) );
      }

      xml.writeAttribute   ( "bandform",  band_forming ? "1" : "0" );

      if ( band_forming )
         xml.writeAttribute   ( "bandvolume",  QString::number( band_volume ) );

      xml.writeAttribute   ( "sector",      QString::number( cp_sector  ) );
      xml.writeAttribute   ( "pathlength",  QString::number( cp_pathlen ) );
      xml.writeAttribute   ( "angle",       QString::number( cp_angle   ) );
      xml.writeAttribute   ( "width",       QString::number( cp_width   ) );

      if ( meshType == US_SimulationParameters::USER )
      {
         for ( int ii = 0; ii < mesh_radius.size(); ii++ )
         {
            xml.writeStartElement( "usermesh" );
            xml.writeAttribute( "radius",
               QString().sprintf( "%11.5e", mesh_radius[ ii ] ).simplified() );
            xml.writeEndElement();
         }
      }

      for ( int ii = 0; ii < speed_step.size(); ii++ )
      {
         spi = &speed_step[ ii ];

         xml.writeStartElement( "speedstep" );
         xml.writeAttribute   ( "duration_hrs",
            QString::number( spi->duration_hours   ) );
         xml.writeAttribute   ( "duration_mins",
            QString::number( spi->duration_minutes ) );
         xml.writeAttribute   ( "delay_hrs",
            QString::number( spi->delay_hours      ) );
         xml.writeAttribute   ( "delay_mins",
            QString::number( spi->delay_minutes    ) );
         xml.writeAttribute   ( "rotorspeed",
            QString::number( spi->rotorspeed       ) );
         xml.writeAttribute   ( "acceleration",
            QString::number( spi->acceleration     ) );
         xml.writeAttribute   ( "accelerflag",
            spi->acceleration_flag ? "1" : "0" );
         xml.writeAttribute   ( "scans",
            QString::number( spi->scans            ) );
         xml.writeEndElement  ();  // speedstep
      }

      xml.writeEndElement  ();   // params
      xml.writeEndElement  ();   // SimParams

      xml.writeEndDocument ();
      xfile.close();
   }

   else
   {
      stat  = -1;
   }

   return stat;
}


// Load simulation parameters from file
int US_SimulationParameters::get_simparms( US_SimulationParameters& sparms,
      QString fname )
{
   return sparms.load_simparms( fname );
}

// Save simulation parameters to file
int US_SimulationParameters::put_simparms( US_SimulationParameters& sparms,
      QString fname )
{
   return sparms.save_simparms( fname );
}

void US_SimulationParameters::debug( void )
{
   qDebug() << "Sim parms Dump";
   qDebug() << "Mesh radius size:" << mesh_radius.size();
   qDebug() << "Simpoints       :" << simpoints;
   qDebug() << "Mesh Type       :" << meshType;
   qDebug() << "Grid Type       :" << gridType;
   qDebug() << "Radial Res      :" << radial_resolution;
   qDebug() << "Meniscus        :" << meniscus;
   qDebug() << "Bottom Pos      :" << bottom_position;
   qDebug() << "Bottom          :" << bottom;
   qDebug() << "Temperature     :" << temperature;
   qDebug() << "Random noise    :" << rnoise;
   qDebug() << "Time Inv Noise  :" << tinoise;
   qDebug() << "Radial Inv Noise:" << rinoise;
   qDebug() << "Band Forming    :" << band_forming;
   qDebug() << "Band Volume     :" << band_volume;
   qDebug() << "Rotor Calibr.ID :" << rotorCalID;
   qDebug() << "Rotor Coeffs    :" << rotorcoeffs[ 0 ] << rotorcoeffs[ 1 ];
   qDebug() << "CP Sector       :" << cp_sector;
   qDebug() << "CP Pathlength   :" << cp_pathlen;
   qDebug() << "CP Angle        :" << cp_angle;
   qDebug() << "CP Width        :" << cp_width;

   for ( int i = 0; i < speed_step.size(); i++ )
   {
      qDebug() << "  Step" << i;
      qDebug() << "   Duration Hours" << speed_step[ i ].duration_hours;
      qDebug() << "   Duration Mins " << speed_step[ i ].duration_minutes;
      qDebug() << "   Delay Hours   " << speed_step[ i ].delay_hours;
      qDebug() << "   Delay Mins    " << speed_step[ i ].delay_minutes;
      qDebug() << "   Scans         " << speed_step[ i ].scans;
      qDebug() << "   Acceleration  " << speed_step[ i ].acceleration;
      qDebug() << "   Rotor Speed   " << speed_step[ i ].rotorspeed;
      qDebug() << "   Accel Flag    " << speed_step[ i ].acceleration_flag;
   }
}
