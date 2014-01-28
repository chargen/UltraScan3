#include "us_mpi_analysis.h"
#include "us_math2.h"
#include "us_settings.h"
#include "mpi.h"

//  Parse XML routines
void US_MPI_Analysis::parse( const QString& xmlfile )
{
   QFile file ( xmlfile );

   if ( ! file.open( QIODevice::ReadOnly | QIODevice::Text) )
   {
      // Fail quietly - can't use udp yet
      if ( my_rank == 0 ) DbgLv(0) << "Cannot open file " << xmlfile;
      printf( "Cannot open file %s\n", xmlfile.toAscii().data() );
      printf( "Aborted" );
      MPI_Finalize();
      qApp->exit();
   }

   QXmlStreamReader xml( &file );

   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isStartElement() )
      {
         if ( xml.name() == "US_JobSubmit" )
         {
            QXmlStreamAttributes a = xml.attributes();
            analysis_type = a.value( "method" ).toString();
         }

         if ( xml.name() == "job" )
            parse_job( xml );

         if ( xml.name() == "dataset" )
         {
            US_SolveSim::DataSet* d = new US_SolveSim::DataSet;
            parse_dataset( xml, d );
            if ( parameters.contains( "CG_model" ) )
            {
               d->model_file  = parameters[ "CG_model" ];
               d->solute_type = 2;
            }
            else if ( parameters[ "ytype" ] == "vbar" )
            {
               d->solute_type = 1;
            }
            else
            {
               d->solute_type = 0;
            }
            data_sets << d;
         }
      }
   }

   file.close();
}

void US_MPI_Analysis::parse_job( QXmlStreamReader& xml )
{
   QXmlStreamAttributes a;

   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isEndElement()  &&  xml.name() == "job" ) break;

      if ( xml.isStartElement()  &&  xml.name() == "cluster" )
      {
         a       = xml.attributes();
         cluster = a.value( "shortname" ).toString();
      }

      if ( xml.isStartElement()  &&  xml.name() == "name" )
      {
         a       = xml.attributes();
         db_name = a.value( "value" ).toString();
      }

      if ( xml.isStartElement()  &&  xml.name() == "udp" )
      {
         a      = xml.attributes();
         server = QHostAddress( a.value( "server" ).toString() );
         port   = (quint16)a.value( "port" ).toString().toInt();
      }

      if ( xml.isStartElement()  &&  xml.name() == "request" )
      {
         a       = xml.attributes();
         QString s;
         requestID   = s.sprintf( "%06d", a.value( "id" ).toString().toInt() );
         requestGUID = a.value( "guid" ).toString();
      }

      if ( xml.isStartElement()  &&  xml.name() == "jobParameters" )
      {
         while ( ! xml.atEnd() )
         {
            xml.readNext();
            QString name = xml.name().toString();

            if ( xml.isEndElement()  &&  name == "jobParameters" ) break;

            if ( xml.isStartElement() )
            {
               a       = xml.attributes();

               if ( name == "bucket" )
               { // Get bucket coordinates; try to forestall round-off problems
                  QString ytype = QString( "ff0" );
                  Bucket b;
                  double smin   = a.value( "s_min"   ).toString().toDouble();
                  double smax   = a.value( "s_max"   ).toString().toDouble();
                  double fmin   = a.value( "ff0_min" ).toString().toDouble();
                  double fmax   = a.value( "ff0_max" ).toString().toDouble();

                  if ( fmax == 0.0 )
                  {
                     fmin       = a.value( "vbar_min" ).toString().toDouble();
                     fmax       = a.value( "vbar_max" ).toString().toDouble();
                     ytype      = QString( "vbar" );
                  }

                  if ( buckets.size() == 0 )
                     parameters[ "ytype" ]     = ytype;

                  b.s_min       = (long)( smin * 1e+6 + 0.5 ) * 1e-6;
                  b.s_max       = (long)( smax * 1e+6 + 0.5 ) * 1e-6;
                  b.ff0_min     = (long)( fmin * 1e+6 + 0.5 ) * 1e-6;
                  b.ff0_max     = (long)( fmax * 1e+6 + 0.5 ) * 1e-6;

                  buckets << b;
               }
               else if ( name == "CG_model" )
               {
                  parameters[ name ]        = a.value( "filename" ).toString();
               }
               else
               {
                  parameters[ name ]        = a.value( "value" ).toString();
               }
            }
         }
      }
   }

   if ( parameters.contains( "debug_level" ) )
      dbg_level  = parameters[ "debug_level" ].toInt();

   else
      dbg_level  = 0;

   US_Settings::set_us_debug( dbg_level );
   dbg_timing = ( parameters.contains( "debug_timings" )
              &&  parameters[ "debug_timings" ].toInt() != 0 );
}

void US_MPI_Analysis::parse_dataset( QXmlStreamReader& xml,
      US_SolveSim::DataSet* dataset )
{
   dataset->simparams.speed_step.clear();

   QXmlStreamAttributes a;

   while ( ! xml.atEnd() )
   {
      xml.readNext();
      QString              name = xml.name().toString();

      if ( xml.isEndElement()  &&  xml.name() == "dataset" ) return;

      if ( ! xml.isStartElement() )  continue;

      if ( xml.name() == "files" )
         parse_files( xml, dataset ); 

      if ( xml.name() == "solution" )
         parse_solution( xml, dataset ); 

      if ( xml.name() == "simpoints" )
      {
         a                  = xml.attributes();
         dataset->simparams.simpoints
                            = a.value( "value" ).toString().toInt();
      }

      if ( xml.name() == "band_volume" )
      {
         a                  = xml.attributes();
         dataset->simparams.band_volume
                            = a.value( "value" ).toString().toDouble();
      }

      if ( xml.name() == "radial_grid" )
      {
         a                  = xml.attributes();
         dataset->simparams.meshType = (US_SimulationParameters::MeshType)
                              a.value( "value" ).toString().toInt();
      }

      if ( xml.name() == "time_grid" )
      {
         a                  = xml.attributes();
         dataset->simparams.gridType = (US_SimulationParameters::GridType)
                              a.value( "value" ).toString().toInt();
      }

      if ( xml.name() == "density" )
      {
         a                    = xml.attributes();
         dataset->density     = a.value( "value" ).toString().toDouble();
      }

      if ( xml.name() == "viscosity" )
      {
         a                    = xml.attributes();
         dataset->viscosity   = a.value( "value" ).toString().toDouble();
      }

      if ( xml.name() == "rotor_stretch" )
      {
         a                    = xml.attributes();
         QStringList stretch  = 
            a.value( "value" ).toString().split( " ", QString::SkipEmptyParts );

         dataset->rotor_stretch[ 0 ] = stretch[ 0 ].toDouble();
         dataset->rotor_stretch[ 1 ] = stretch[ 1 ].toDouble();
      }

      if ( xml.name() == "centerpiece_bottom" )
      {
         a                      = xml.attributes();
         dataset->centerpiece_bottom
                                = a.value( "value" ).toString().toDouble();
      }

      if ( xml.name() == "centerpiece_shape" )
      {
         a                      = xml.attributes();
         QString shape          = a.value( "value" ).toString();
         QStringList shapes;
         shapes << "sector"
                << "standard"
                << "rectangular"
                << "band-forming"
                << "meniscus-matching"
                << "circular"
                << "synthetic";
         dataset->simparams.cp_sector
                                = qMax( 0, shapes.indexOf( shape ) );
      }

      if ( xml.name() == "centerpiece_angle" )
      {
         a                      = xml.attributes();
         dataset->simparams.cp_angle
                                = a.value( "value" ).toString().toDouble();
      }

      if ( xml.name() == "centerpiece_pathlength" )
      {
         a                      = xml.attributes();
         dataset->simparams.cp_pathlen
                                = a.value( "value" ).toString().toDouble();
      }

      if ( xml.name() == "centerpiece_width" )
      {
         a                      = xml.attributes();
         dataset->simparams.cp_width
                                = a.value( "value" ).toString().toDouble();
      }

      if ( xml.name() == "speedstep" )
      {
         US_SimulationParameters::SpeedProfile sp;

         US_SimulationParameters::speedstepFromXml( xml, sp );

         dataset->simparams.speed_step << sp;
      }
   }
}

void US_MPI_Analysis::parse_files( QXmlStreamReader& xml,
      US_SolveSim::DataSet* dataset )
{
   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isEndElement()  &&  xml.name() == "files" )  break;

      if ( xml.isStartElement() )
      {
         QXmlStreamAttributes a        = xml.attributes();
         QString              type     = xml.name().toString();
         QString              filename = a.value( "filename" ).toString();

         if ( type == "auc"   ) dataset->auc_file   = filename;
         if ( type == "edit"  ) dataset->edit_file  = filename;
         if ( type == "noise" ) dataset->noise_files << filename;
      }
   }

   QString clambda = dataset->edit_file.section( ".", -2, -2 );
   if ( clambda.contains( "-" ) )
   {  // Modify edit file name for MWL case
      clambda         = clambda + "@" + dataset->auc_file.section( ".", -2, -2 );
      QString fpart1  = dataset->edit_file.section( ".",  0, -3 );
      QString fpart2  = dataset->edit_file.section( ".", -1, -1 );
      dataset->edit_file = fpart1 + "." + clambda + "." + fpart2;
if (my_rank==0) DbgLv(0) << "PF: MWL edit_file" << dataset->edit_file;
   }
}

void US_MPI_Analysis::parse_solution( QXmlStreamReader& xml,
      US_SolveSim::DataSet* dataset )
{
   while ( ! xml.atEnd() )
   {
      xml.readNext();

      if ( xml.isEndElement()  &&  xml.name() == "solution" ) break;

      if ( xml.isStartElement() && xml.name() == "buffer" )
      {
         QXmlStreamAttributes a        = xml.attributes();
         dataset->density   = a.value( "density"   ).toString().toDouble();
         dataset->viscosity = a.value( "viscosity" ).toString().toDouble();
         dataset->manual    = a.value( "manual"    ).toString().toInt();
      }

      if ( xml.isStartElement() && xml.name() == "analyte" )
      {
         US_Solution::AnalyteInfo aninfo;
         QXmlStreamAttributes a        = xml.attributes();

         aninfo.analyte.mw     = a.value( "mw"     ).toString().toDouble();
         aninfo.analyte.vbar20 = a.value( "vbar20" ).toString().toDouble();
         aninfo.amount         = a.value( "amount" ).toString().toDouble();
         QString atype         = a.value( "type"   ).toString();
         aninfo.analyte.type   = US_Analyte::PROTEIN;
         if ( atype == "DNA" )
            aninfo.analyte.type   = US_Analyte::DNA;
         if ( atype == "RNA" )
            aninfo.analyte.type   = US_Analyte::RNA;
         if ( atype == "Other" )
            aninfo.analyte.type   = US_Analyte::CARBOHYDRATE;

         dataset->solution_rec.analyteInfo << aninfo;
      }
   }

   dataset->vbar20  = US_Math2::calcCommonVbar( dataset->solution_rec, 20.0 );
}

