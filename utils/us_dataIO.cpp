//! \file us_dataIO.cpp
#include <uuid/uuid.h>

#include <QDomDocument>

#include "us_dataIO.h"
#include "us_crc.h"
#include "us_math.h"

bool US_DataIO::readLegacyFile( const QString& file, 
                                beckmanRaw&    data )
{
   // Open the file for reading
   QFile f( file );
   if ( ! f.open( QIODevice::ReadOnly | QIODevice::Text ) ) return false;
   QTextStream ts( &f );

   // Read the description
   data.description = ts.readLine();

   // Read scan parameters
   QString     s = ts.readLine();
   QStringList p = s.split( " ", QString::SkipEmptyParts );

   if ( p.size() < 8 ) return false;

   data.type          = p[ 0 ].toAscii()[ 0 ];  // I P R W F
   data.cell          = p[ 1 ].toInt();
   data.temperature   = p[ 2 ].toDouble();
   data.rpm           = p[ 3 ].toDouble();
   data.seconds       = p[ 4 ].toDouble();
   data.omega2t       = p[ 5 ].toDouble();
   data.t.wavelength  = p[ 6 ].toDouble();
   data.count         = p[ 7 ].toInt();


   // Read radius, data, and standard deviation
   data.readings.clear();
   bool interference_data = ( data.type == 'P' );

   while ( ! ts.atEnd() )
   {
      s = ts.readLine();

      p = s.split( " ", QString::SkipEmptyParts );

      reading r;

      r.d.radius = p[ 0 ].toFloat();
      r.value    = p[ 1 ].toFloat();
      
      if ( ! interference_data & p.size() > 2 ) 
         r.stdDev  = p[ 2 ].toFloat();
      else
         r.stdDev  = 0.0;
      
      data.readings << r;
   }

   f.close();
   return true;
}

int US_DataIO::writeRawData( const QString& file, rawData& data )
{
   // Open the file for writing
   QFile f( file );
   if ( ! f.open( QIODevice::WriteOnly ) ) return CANTOPEN;
   QDataStream ds( &f );

   unsigned long crc = 0xffffffffUL;
   
   // Write magic number
   char magic[ 5 ] = "UCDA";
   write( ds, magic, 4, crc );
   
   // Write data type
   write( ds, data.type, 2, crc );

   // Create and write a guid
   uuid_t uuid;
   uuid_generate( uuid );
   write( ds, (char*) &uuid, 16, crc );

   // Write description
   char desc[ 240 ];
   bzero( desc, sizeof desc );

   QByteArray d = data.description.toLatin1();
   strncpy( desc, d.data(), sizeof desc );
   write( ds, desc, sizeof desc, crc );

   // Find min and max radius, data, and std deviation
   parameters p;

   p.min_data1       =  1.0e99;
   p.max_data1       = -1.0e99;
   p.min_data2       =  1.0e99;
   p.max_data2       = -1.0e99;

   double min_radius =  1.0e99;
   double max_radius = -1.0e99;

   scan    s;
   reading r;

   foreach( s, data.scanData )
   {
      foreach( r, s.values )
      {
         min_radius = min( min_radius, r.d.radius );
         max_radius = max( max_radius, r.d.radius );

         p.min_data1 = min( p.min_data1, r.value );
         p.max_data1 = max( p.max_data1, r.value );

         p.min_data2 = min( p.min_data2, r.stdDev );
         p.max_data2 = max( p.max_data2, r.stdDev );
      }
   }

   // Write all integer types as little endian
   uchar c[ 4 ];
   qToLittleEndian( (ushort)( min_radius * 1000.0 ), c );
   write( ds, (char*)c, 2, crc );

   qToLittleEndian( (ushort)( max_radius * 1000.0 ), c );
   write( ds, (char*)c, 2, crc );

   // Distance between radius entries
   double r1    = data.scanData[ 0 ].values[ 0 ].d.radius;
   double r2    = data.scanData[ 0 ].values[ 1 ].d.radius;
   float  delta = (float) ( r2 - r1 );
   write( ds, (char*) &delta, 4, crc );

   float v = (float) p.min_data1;   
   write( ds, (char*) &v, 4, crc );
   
   v = (float) p.max_data1;
   write( ds, (char*) &v, 4, crc );

   v = (float) p.min_data2;
   write( ds, (char*) &v, 4, crc );
   
   v = (float) p.max_data2;
   write( ds, (char*) &v, 4, crc );

   // Write out scan count
   qToLittleEndian( (ushort)data.scanData.size(), c );
   write( ds, (char*)c, 2, crc );

   // Loop for each scan
   foreach ( s, data.scanData )
      writeScan( ds, s, crc, p );

   qToLittleEndian( crc, c );
   ds.writeRawData( (char*)c, 4 );

   f.close();

   return OK;
}

void US_DataIO::writeScan( QDataStream&    ds, const scan&       data, 
                           unsigned long& crc, const parameters& p )
{
   uchar c[ 4 ];
   char  d[ 5 ] = "DATA";
   write( ds, d, 4, crc );

   float t = (float) data.temperature;
   write( ds, (char*) &t, 4, crc );

   qToLittleEndian( (uint)data.rpm, c );
   write( ds, (char*)c, 4, crc );

   qToLittleEndian( (uint)data.seconds, c );
   write( ds, (char*)c, 4, crc );

   float o = (float) data.omega2t;
   write( ds, (char*) &o, 4, crc );

   qToLittleEndian( (ushort)( ( data.wavelength - 180.0 ) * 100.0 ), c );
   write( ds, (char*)c, 2, crc );

   int valueCount = data.values.size();
   qToLittleEndian( (uint)valueCount, c );
   write( ds, (char*)c, 4, crc );

   // Write reading
   double             delta  = ( p.max_data1 - p.min_data1 ) / 65535;
   double             delta2 = ( p.max_data2 - p.min_data2 ) / 65535;
   unsigned short int si;  // short int

   bool    stdDev = ( p.min_data2 != 0.0 || p.max_data2 != 0.0 );
   reading r;

   foreach ( r, data.values )
   {
      si = (unsigned short int) ( ( r.value - p.min_data1 ) / delta );

      qToLittleEndian( si, c );
      write( ds, (char*)c, 2, crc );

      // If applicable, write std deviation
      if ( stdDev )
      {
         si = (unsigned short int) ( ( r.stdDev - p.min_data2 ) / delta2 );
         qToLittleEndian( si, c );
         write( ds, (char*)c, 2, crc );
      }
   }

   // Write interpolated flags
   int flagSize = ( valueCount + 7 ) / 8;
   write( ds, data.interpolated.data(), flagSize, crc );
}

void US_DataIO::write( QDataStream& ds, const char* c, int len, ulong& crc )
{
   ds.writeRawData( c, len );
   crc = US_Crc::crc32( crc, (unsigned char*) c, len );
}

int US_DataIO::readRawData( const QString& file, rawData& data )
{
   QFile f( file );
   if ( ! f.open( QIODevice::ReadOnly ) ) return CANTOPEN;
   QDataStream ds( &f );

   int           err = OK;
   unsigned long crc = 0xffffffffUL;

   try
   {
      // Read magic number
      char magic[ 4 ];
      read( ds, magic, 4, crc );
      if ( strncmp( magic, "UCDA", 4 ) != 0 ) throw NOT_USDATA;
    
      // Read and get the file type
      char type[ 3 ];
      read( ds, type, 2, crc );
      type[ 2 ] = '\0';
    
      QStringList types = QStringList() << "RA" << "IP" << "RI" << "FI" 
                                        << "WA" << "WI";
    
      if ( ! types.contains( QString( type ) ) ) throw BADTYPE;
      strncpy( data.type, type, 2 );
    
      // Get the guid
      read( ds, data.guid, 16, crc );
    
      // Get the description
      char desc[ 240 ];
      read( ds, desc, 240, crc );
      data.description = QString( desc );

      // Get the parameters to expand the values

      union
      {
         char   c[ 2 ];
         ushort I;
      } si;

      read( ds, si.c, 2, crc );
      double min_radius = qFromLittleEndian( si.I ) / 1000.0;

      read( ds, si.c, 2, crc );
      // Unused
      //double max_radius = qFromLittleEndian( si.I ) / 1000.0;

      union
      {
         char  c[ 4 ];
         int   I;
         float f;
      } v;

      read( ds, v.c, 4, crc );
      double delta_radius = v.f;

      read( ds, v.c, 4, crc );
      double min_data1 = v.f;

      read( ds, v.c, 4, crc );
      double max_data1 = v.f;

      read( ds, v.c, 4, crc );
      double min_data2 = v.f;

      read( ds, v.c, 4, crc );
      double max_data2 = v.f;

      read( ds, si.c, 2, crc );
      short int scan_count = qFromLittleEndian( si.I );

      // Read each scan
      for ( int i = 0 ; i < scan_count; i ++ )
      {
         read( ds, v.c, 4, crc );
         if ( strncmp( v.c, "DATA", 4 ) != 0 ) throw NOT_USDATA;

         scan s;
         
         // Temperature
         read( ds, v.c, 4, crc );
         s.temperature = v.f;

         // RPM
         read( ds, v.c, 4, crc );
         s.rpm = qFromLittleEndian( v.I );

         // Seconds
         read( ds, v.c, 4, crc );
         s.seconds = qFromLittleEndian( v.I );

         // Omega2t
         read( ds, v.c, 4, crc );
         s.omega2t = v.f;

         // Wavelength
         read( ds, si.c, 2, crc );
         s.wavelength = qFromLittleEndian( si.I ) / 100.0 + 180.0;

         // Reading count
         read( ds, v.c, 4, crc );
         int valueCount = qFromLittleEndian( v.I );

         // Get the readings
         double  radius  = min_radius;
         double  factor1 = ( max_data1 - min_data1 ) / 65535.0;
         double  factor2 = ( max_data2 - min_data2 ) / 65535.0;
         bool    stdDev  = ( min_data2 != 0.0 || max_data2 != 0.0 );

         for ( int j = 0; j < valueCount; j++ )
         {
            reading r;

            r.d.radius = radius;
            
            read( ds, si.c, 2, crc );
            r.value = qFromLittleEndian( si.I ) * factor1 + min_data1;

            if ( stdDev )
            {
               read( ds, si.c, 2, crc );
               r.stdDev = qFromLittleEndian( si.I ) * factor2 + min_data2;
            }
            else
               r.stdDev = 0.0;

            // Add the reading to the scan
            s.values << r;
            
            radius += delta_radius;
         } 

         // Get the interpolated bitmap;
         int bytes          = ( valueCount + 7 ) / 8;
         char* interpolated = new char[ bytes ];
         
         read( ds, interpolated, bytes, crc );

         s.interpolated = QByteArray( interpolated, bytes );

         delete [] interpolated;

         // Add the scan to the data
         data.scanData <<  s;
      }

      // Read the crc
      unsigned long read_crc;
      ds.readRawData( (char*) &read_crc , 4 );
      if ( crc != qFromLittleEndian( read_crc ) ) throw BADCRC;

   } catch( ioError error )
   {
      err = error;
   }

   f.close();
   return err;
}

void US_DataIO::read( QDataStream& ds, char* c, int len, ulong& crc )
{
   ds.readRawData( c, len );
   crc = US_Crc::crc32( crc, (uchar*) c, len );
}

int US_DataIO::readEdits( const QString& filename, editValues& parameters )
{
   QFile f( filename );
   if ( ! f.open( QIODevice::ReadOnly ) ) return CANTOPEN;
   QTextStream ds( &f );

   QXmlStreamReader xml( &f );

   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isStartElement() )
      {
         if ( xml.name() == "identification" ) 
            ident ( xml, parameters );
         
         else if ( xml.name() == "run" ) 
            run( xml, parameters );
      }
   }

   bool error = xml.hasError();
   f.close();
   
   if ( error ) return BADXML;

   return OK;
}

void US_DataIO::ident( QXmlStreamReader& xml, editValues& parameters )
{
   while ( ! xml.atEnd() )
   {
      if ( xml.isEndElement()  &&  xml.name() == "identification" ) return;
     
      if ( xml.isStartElement()  &&  xml.name() == "runid" )
      {
         QXmlStreamAttributes a = xml.attributes();
         parameters.runID = a.value( "value" ).toString();
      }

      if ( xml.isStartElement()  &&  xml.name() == "uuid" )
      {
         QXmlStreamAttributes a = xml.attributes();
         parameters.uuid = a.value( "value" ).toString();
      }

      xml.readNext();
   }
}

void US_DataIO::run( QXmlStreamReader& xml, editValues& parameters )
{
   QXmlStreamAttributes a = xml.attributes();
   parameters.cell       = a.value( "cell"       ).toString();
   parameters.channel    = a.value( "channel"    ).toString();
   parameters.wavelength = a.value( "wavelength" ).toString();

   while ( ! xml.atEnd() )
   {
      if ( xml.isEndElement()  &&  xml.name() == "run" ) return;

      if ( xml.isStartElement()  &&  xml.name() == "parameters" )
         params( xml, parameters );

      if ( xml.isStartElement()  &&  xml.name() == "operations" )
         operations( xml, parameters );

      xml.readNext();
   }
}

void US_DataIO::params( QXmlStreamReader& xml, editValues& parameters )
{
   while ( ! xml.atEnd() )
   {
      if ( xml.isEndElement()  &&  xml.name() == "parameters" ) return;

      if ( xml.isStartElement()  &&  xml.name() == "meniscus" )
      {
         QXmlStreamAttributes a = xml.attributes();
         parameters.meniscus = a.value( "radius" ).toString().toDouble();
      }

      if ( xml.isStartElement()  &&  xml.name() == "plateau" )
      {
         QXmlStreamAttributes a = xml.attributes();
         parameters.plateau = a.value( "radius" ).toString().toDouble();
      }

      if ( xml.isStartElement()  &&  xml.name() == "baseline" )
      {
         QXmlStreamAttributes a = xml.attributes();
         parameters.baseline = a.value( "radius" ).toString().toDouble();
      }

      if ( xml.isStartElement()  &&  xml.name() == "data_range" )
      {
         QXmlStreamAttributes a = xml.attributes();
         parameters.rangeLeft  = a.value( "left"  ).toString().toDouble();
         parameters.rangeRight = a.value( "right" ).toString().toDouble();
      }

      xml.readNext();
   }
}

void US_DataIO::operations( QXmlStreamReader& xml, editValues& parameters )
{
   while ( ! xml.atEnd() )
   {
      if ( xml.isEndElement()  &&  xml.name() == "operations" ) return;

      if ( xml.isStartElement()  &&  xml.name() == "invert" )
         parameters.invert = -1.0;

      if ( xml.isStartElement()  &&  xml.name() == "remove_spikes" )
         parameters.removeSpikes = true;

      if ( xml.isStartElement()  &&  xml.name() == "subtract_ri_noise" )
      {
         QXmlStreamAttributes a = xml.attributes();
         parameters.noiseOrder = a.value( "order" ).toString().toInt();
      }

      if ( xml.isStartElement()  &&  xml.name() == "edited" )
         do_edits( xml, parameters );

      if ( xml.isStartElement()  &&  xml.name() == "excludes" )
         excludes( xml, parameters );

      xml.readNext();
   }
}

void US_DataIO::excludes( QXmlStreamReader& xml, editValues& parameters )
{
   while ( ! xml.atEnd() )
   {
      if ( xml.isEndElement()  &&  xml.name() == "excludes" ) return;

      if ( xml.isStartElement()  &&  xml.name() == "exclude" )
      {
         QXmlStreamAttributes a = xml.attributes();
         parameters.excludes << a.value( "scan" ).toString().toInt();
      }

      xml.readNext();
   }
}

void US_DataIO::do_edits( QXmlStreamReader& xml, editValues& parameters )
{
   while ( ! xml.atEnd() )
   {
      if ( xml.isEndElement()  &&  xml.name() == "edited" ) return;

      if ( xml.isStartElement()  &&  xml.name() == "edit" )
      {
         edits e;
         QXmlStreamAttributes a = xml.attributes();
         e.scan   = a.value( "scan"   ).toString().toInt();
         e.radius = a.value( "radius" ).toString().toDouble();
         e.value  = a.value( "value"  ).toString().toDouble();

         parameters.editedPoints << e;
      }

      xml.readNext();
   }
}

QString US_DataIO::errorString( int code )
{
   switch ( code )
   {
      case OK        : return QObject::tr( "The operation completed successully" );
      case CANTOPEN  : return QObject::tr( "The file cannot be opened" );
      case BADCRC    : return QObject::tr( "The file was corrupted" );
      case NOT_USDATA: return QObject::tr( "The file was not valid scan data" );
      case BADTYPE   : return QObject::tr( "The filetype was not recognized" );
      case BADXML    : return QObject::tr( "The XML file was invalid" );
      case NODATA    : return QObject::tr( "No legacy data files were found" );
   }

   return QObject::tr( "Unknown error code" );
}
