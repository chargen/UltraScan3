//! \file us_model.cpp

#include "us_model.h"
#include "us_constants.h"
#include "us_settings.h"
#include "us_util.h"

#include <uuid/uuid.h>

US_Model::SimulationComponent::SimulationComponent()
{
   name                 = "New Component";
   bzero( analyteGUID, 16 );
   molar_concentration  = 0.0;
   signal_concentration = 1.0;
   vbar20               = TYPICAL_VBAR;
   mw                   = 50000.0;
   s                    = 0.0;
   D                    = 0.0;
   f                    = 1.0;
   f_f0                 = 1.25;
   extinction           = 0.0;
   sigma                = 0.0;
   delta                = 0.0;
   stoichiometry        = 1;
   shape                = SPHERE;
   axial_ratio          = 10.0;
   analyte_type         = 0;  // Protein
   c0.radius       .clear();
   c0.concentration.clear();
}

US_Model::Association::Association()
{
   k_eq  = 0.0;
   k_off = 0.0;
   reaction_components.clear();
   stoichiometry      .clear();
}

US_Model::US_Model()
{
   viscosity       = VISC_20W;
   compressibility = COMP_25W;
   temperature     = NORMAL_TEMP;
   optics          = ABSORBANCE;
   description     = "New Model";
   wavelength      = 0.0;
   coSedSolute     = -1;
   type            = MANUAL;
   //iterations      = 1;
   bufferGUID  .clear();
   guid        .clear();
   components  .clear();
   associations.clear();
}

int US_Model::load( bool db_access, const QString& guid, US_DB2* db )
{
   if ( db_access ) return load_db  ( guid, db );
   else             return load_disk( guid );
}

int US_Model::write( bool db_access, const QString& filename, US_DB2* db )
{
   if ( db_access ) return write_db  ( db );
   else             return write_disk( filename );
}

bool US_Model::model_path( QString& path )
{
   QDir dir;
   path = US_Settings::dataDir() + "/models";

   if ( ! dir.exists( path ) )
   {
      if ( ! dir.mkpath( path ) )
      {
         return false;
      }
   }

   return true;
}

int US_Model::load_disk( const QString& guid )
{
   int error = US_DB2::ERROR;  // Error by default

   QString path;

   if ( ! model_path( path ) )
   {
      //message = QObject::tr ( "Could not create analyte directory" );
      return error;
   }

   QDir        f( path );
   QStringList filter( "M*.xml" );
   QStringList names = f.entryList( filter, QDir::Files, QDir::Name );
   QString     filename;
   bool        found = false;

   for ( int i = 0; i < names.size(); i++ )
   {
      filename = path + "/" + names[ i ];
      QFile file( filename );

      if ( ! file.open( QIODevice::ReadOnly | QIODevice::Text) ) continue;

      QXmlStreamReader xml( &file );

      while ( ! xml.atEnd() )
      {
         xml.readNext();

         if ( xml.isStartElement() )
         {
            if ( xml.name() == "model" )
            {
               QXmlStreamAttributes a = xml.attributes();

               if ( a.value( "guid" ).toString() == guid ) found = true;
               break;
            }
         }
      }

      file.close();

      if ( found ) return read_model( filename );
   }

   //message =  QObject::tr ( "Could not find analyte guid" );
   return error;
}

int US_Model::read_model( const QString& filename )
{
   QFile file( filename );

   if ( ! file.open( QIODevice::ReadOnly | QIODevice::Text) )
   {
      qDebug() << "Cannot open file for reading: " << filename;
      return false;
   }

   QXmlStreamReader     xml( &file );
   QXmlStreamAttributes a;
   bool                 read_next = true;

   while ( ! xml.atEnd() )
   {
      if ( read_next ) xml.readNext();
      read_next = true;

      if ( xml.isStartElement() )
      {
         if ( xml.name() == "model" )
         {
            a = xml.attributes();

            description     = a.value( "description"     ).toString();
            bufferGUID      = a.value( "bufferGuid"      ).toString();
            bufferDesc      = a.value( "bufferDesc"      ).toString();
            guid            = a.value( "guid"            ).toString();
            density         = a.value( "density"         ).toString().toDouble();
            viscosity       = a.value( "viscosity"       ).toString().toDouble();
            compressibility = a.value( "compressibility" ).toString().toDouble();
            wavelength      = a.value( "wavelength"      ).toString().toDouble();
            temperature     = a.value( "temperature"     ).toString().toDouble();
            coSedSolute     = a.value( "coSedSolute"     ).toString().toInt();
            type            =
               (ModelType)a.value( "type"   ).toString().toInt();
            //iterations      = a.value( "iterations"      ).toString().toInt();
         }

         else if ( xml.name() == "analyte" )
         {
            SimulationComponent sc;
            a = xml.attributes();

            QString guid = a.value( "guid" ).toString();
            uuid_parse( guid.toAscii().data(), sc.analyteGUID );

            sc.name        = a.value( "name"       ).toString();
            sc.vbar20      = a.value( "vbar20"     ).toString().toDouble();
            sc.mw          = a.value( "mw"         ).toString().toDouble();
            sc.s           = a.value( "s"          ).toString().toDouble();
            sc.D           = a.value( "D"          ).toString().toDouble();
            sc.f           = a.value( "f"          ).toString().toDouble();
            sc.f_f0        = a.value( "f_f0"       ).toString().toDouble();
            //sc.wavelength  = a.value( "wavelength" ).toString().toDouble();
            sc.extinction  = a.value( "extinction" ).toString().toDouble();
            sc.axial_ratio = a.value( "axial"      ).toString().toDouble();
            sc.sigma       = a.value( "sigma"      ).toString().toDouble();
            sc.delta       = a.value( "delta"      ).toString().toDouble();

            sc.molar_concentration  = a.value( "molar"  ).toString().toDouble();
            sc.signal_concentration = a.value( "signal" ).toString().toDouble();
            sc.stoichiometry        = a.value( "stoich" ).toString().toInt();
            sc.shape                =
               (ShapeType)a.value( "shape"  ).toString().toInt();
            sc.analyte_type         = a.value( "type"   ).toString().toInt();

            mfem_scans( xml, sc );
            read_next = false; // Skip the next read

            components << sc;
         }
      }
   }

   return US_DB2::OK;
}

void US_Model::mfem_scans( QXmlStreamReader& xml, SimulationComponent& sc )
{
   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isStartElement() )
      {
         if ( xml.name() == "mfem_scan" )
         {
            QXmlStreamAttributes a = xml.attributes();

            sc.c0.radius        << a.value( "radius" ).toString().toDouble();
            sc.c0.concentration << a.value( "conc"   ).toString().toDouble();
         }
         else 
            break;
      }
   }
}

void US_Model::get_associations( QXmlStreamReader& xml, Association& as )
{
   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isStartElement() )
      {
         if ( xml.name() == "component" )
         {
            QXmlStreamAttributes a = xml.attributes();

            as.reaction_components << a.value( "index"  ).toString().toInt();
            as.stoichiometry       << a.value( "stoich" ).toString().toInt();
         }
         else 
            break;
      }
   }
}

int US_Model::load_db( const QString& guid, US_DB2* db )
{
   QStringList q;

   q << "get_modelID" << guid;
   
   db->query( q );
   if ( db->lastErrno() != US_DB2::OK ) return db->lastErrno();
   
   db->next();
   QString id = db->value( 0 ).toString();

   q << "get_model_info" << id;
   
   db->query( q );
   if ( db->lastErrno() != US_DB2::OK ) return db->lastErrno();

   db->next();
   QByteArray contents = db->value( 2 ).toString().toAscii();

   // Write the model file to a temporary file
   QTemporaryFile temporary;
   temporary.open();
   temporary.write( contents );
   temporary.close();

   QString file = temporary.fileName();
   return read_model( file );
}

int US_Model::write_db( US_DB2* db )
{
      // Create the model xml file in a string
      QTemporaryFile temporary;
      write_temp( temporary );
      temporary.open();
      QByteArray contents = temporary.readAll();

      QStringList q;

      // Generate a guid if necessary
      // The guid may be valid from a disk read, but is not in the DB
      if ( ! guid.size() != 36 ) guid = US_Util::new_guid();

      q << "get_modelID" << guid;
      
      db->query( q );
      
      q.clear();
     
      if ( db->lastErrno() != US_DB2::OK )
         q << "new_model" << guid << description << contents;
      else
      {
         QString id = db->value( 0 ).toString();
         q << "update_model" << id << description << contents;
      }

      return db->statusQuery( q );
}

void US_Model::write_temp( QTemporaryFile& file )
{
   file.open();// QIODevice::WriteOnly | QIODevice::Text );
   QXmlStreamWriter xml( &file );
   xml.setAutoFormatting( true );

   xml.writeStartDocument();
   xml.writeDTD         ( "<!DOCTYPE US_Model>" );
   xml.writeStartElement( "ModelData" );
   xml.writeAttribute   ( "version", "1.0" );

   xml.writeStartElement( "model" );
   xml.writeAttribute   ( "description",     description );
   xml.writeAttribute   ( "guid",            guid        );
   xml.writeAttribute   ( "bufferGuid",      bufferGUID  );
   xml.writeAttribute   ( "bufferDesc",      bufferDesc  );
   xml.writeAttribute   ( "density",         QString::number( density        ));
   xml.writeAttribute   ( "viscosity",       QString::number( viscosity      ));
   xml.writeAttribute   ( "compressibility", QString::number( compressibility));
   xml.writeAttribute   ( "wavelength",      QString::number( wavelength     ));
   xml.writeAttribute   ( "temperature",     QString::number( temperature    ));
   xml.writeAttribute   ( "coSedSolute",     QString::number( coSedSolute    ));
   xml.writeAttribute   ( "optics",          QString::number( optics         ));
   xml.writeAttribute   ( "type",            QString::number( type           ));
   //xml.writeAttribute   ( "iterations",      QString::number( iterations ) );

   char uuid[ 37 ];
   uuid[ 36 ] = 0;

   // Write components
   for ( int i = 0; i < components.size(); i++ )
   {
      SimulationComponent* sc = &components[ i ];
      xml.writeStartElement( "analyte" );

      if ( uuid_is_null( sc->analyteGUID ) )
         xml.writeAttribute( "guid",    "" );
      else
      {
         uuid_unparse( sc->analyteGUID, uuid );
         xml.writeAttribute( "guid",    QString( uuid                      ) );
      }

      xml.writeAttribute( "name",       sc->name                             );
      xml.writeAttribute( "vbar20",     QString::number( sc->vbar20        ) );
      xml.writeAttribute( "mw",         QString::number( sc->mw            ) );
      xml.writeAttribute( "s",          QString::number( sc->s             ) );
      xml.writeAttribute( "D",          QString::number( sc->D             ) );
      xml.writeAttribute( "f",          QString::number( sc->f             ) );
      xml.writeAttribute( "f_f0",       QString::number( sc->f_f0          ) );
      xml.writeAttribute( "extinction", QString::number( sc->extinction    ) );
      xml.writeAttribute( "axial",      QString::number( sc->axial_ratio   ) );
      xml.writeAttribute( "sigma",      QString::number( sc->sigma         ) );
      xml.writeAttribute( "delta",      QString::number( sc->delta         ) );
      xml.writeAttribute( "stoich",     QString::number( sc->stoichiometry ) );
      xml.writeAttribute( "shape",      QString::number( sc->shape         ) );
      xml.writeAttribute( "type",       QString::number( sc->analyte_type  ) );

      xml.writeAttribute( "molar",      QString::number( sc->molar_concentration  ) );
      xml.writeAttribute( "signal",     QString::number( sc->signal_concentration ) );

      for ( int j = 0; j < sc->c0.radius.size(); j++ )
      {
         xml.writeStartElement( "mfem_scan" );

         MfemInitial* scan = &sc->c0;
         xml.writeAttribute( "radius", QString::number( scan->radius       [ j ] ) );
         xml.writeAttribute( "conc",   QString::number( scan->concentration[ j ] ) );
         xml.writeEndElement();  // mfem_scan
      }

      xml.writeEndElement(); // analyte (SimulationComponent)
   }

   // Write associations
   for ( int i = 0; i < associations.size(); i++ )
   {
      Association* as = &associations[ i ];
      xml.writeStartElement( "association" );
      xml.writeAttribute( "k_eq", QString::number( as->k_eq ) );
      xml.writeAttribute( "k_off", QString::number( as->k_off ) );

      for ( int j = 0; j < as->reaction_components.size(); j++ )
      {
         xml.writeStartElement( "component" );

         QString index  = QString::number( as->reaction_components[ j ] );
         QString stoich = QString::number( as->stoichiometry      [ j ] );

         xml.writeAttribute( "index",         index  );
         xml.writeAttribute( "stoichiometry", stoich );
         xml.writeEndElement(); // component
      }

      xml.writeEndElement(); // association
   }

   xml.writeEndElement(); // model
   xml.writeEndElement(); // ModelData
   xml.writeEndDocument();
   file.close();
}

int US_Model::write_disk( const QString& filename )
{
   QTemporaryFile temporary;
   write_temp( temporary );

   temporary.open();   // Open for reading

   QFile file( filename );

   if ( ! file.open( QIODevice::WriteOnly | QIODevice::Text) )
      return US_DB2::ERROR;
   
   file.write( temporary.readAll() );
   file.close();
   temporary.close();

   return US_DB2::OK;
}
