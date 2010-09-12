//! \file us_data_model.cpp

#include "us_data_model.h"
#include "us_data_process.h"
#include "us_data_tree.h"
#include "us_util.h"
#include "us_settings.h"
#include "us_model.h"
#include "us_noise.h"
#include "us_editor.h"
#include <uuid/uuid.h>

// scan the database and local disk for R/E/M/N data sets
US_DataModel::US_DataModel( QWidget* parwidg /*=0*/ )
{
   parentw    = parwidg;   // parent (main manage_data) widget

   ddescs.clear();         // db descriptions
   ldescs.clear();         // local descriptions
   adescs.clear();         // all descriptions

   details    = false;     // flag whether to get content details
}

// set database related pointers
void US_DataModel::setDatabase( US_DB2* a_db, QString a_invtxt )
{
   db         = a_db;      // pointer to opened db connection
   investig   = a_invtxt;  // investigator text
   invID      = investig.section( ":", 0, 0 ).simplified();
}

// set progress bar related pointers
void US_DataModel::setProgress( QProgressBar* a_progr, QLabel* a_lbstat )
{
   progress   = a_progr;   // pointer to progress bar
   lb_status  = a_lbstat;  // pointer to status label
}

// set sibling classes pointers
void US_DataModel::setSiblings( QObject* a_proc, QObject* a_tree )
{
   ob_process = a_proc;    // pointer to sister DataProcess object
   ob_tree    = a_tree;    // pointer to sister DataTree object
}

// get database pointer
US_DB2* US_DataModel::dbase()
{
   return db;
}

// get investigator text
QString US_DataModel::invtext()
{
   return investig;
}

// get progress bar pointer
QProgressBar* US_DataModel::progrBar()
{
   return progress;
}

// get status label pointer
QLabel* US_DataModel::statlab()
{
   return lb_status;
}

// get us_data_process object pointer
QObject* US_DataModel::procobj()
{
   return ob_process;
}

// get us_data_tree object pointer
QObject* US_DataModel::treeobj()
{
   return ob_tree;
}

// scan the database and local disk for R/E/M/N data sets
void US_DataModel::scan_data( bool content_details )
{
   ddescs.clear();         // db descriptions
   ldescs.clear();         // local descriptions
   adescs.clear();         // all descriptions

   details     = content_details;

   scan_dbase( );          // read db to build db descriptions

   sort_descs( ddescs  );  // sort db descriptions

   scan_local( );          // read files to build local descriptions

   sort_descs( ldescs  );  // sort local descriptions

   merge_dblocal();        // merge database and local descriptions
}

// get pointer to data description object at specified row
US_DataModel::DataDesc US_DataModel::row_datadesc( int irow )
{
   return adescs.at( irow );
}

// get pointer to current data description object
US_DataModel::DataDesc US_DataModel::current_datadesc( )
{
   return cdesc;
}

// set pointer to current data description object
void US_DataModel::setCurrent( int irow )
{
   cdesc   = adescs.at( irow );
}

// get count of total data records
int US_DataModel::recCount()
{
   return adescs.size();
}

// get count of DB data records
int US_DataModel::recCountDB()
{
   return ddescs.size();
}

// get count of local data records
int US_DataModel::recCountLoc()
{
   return ldescs.size();
}

// scan the database for R/E/M/N data sets
void US_DataModel::scan_dbase( )
{
   QStringList rawIDs;
   QStringList rawGUIDs;
   QStringList edtIDs;
   QStringList modIDs;
   QStringList noiIDs;
   QStringList query;
   QString     dmyGUID  = "00000000-0000-0000-0000-000000000000";
   QString     recID;
   QString     rawGUID;
   QString     contents;
   int         irecID;
   int         nraws = 0;
   int         nedts = 0;
   int         nmods = 0;
   int         nnois = 0;
   int         nstep = 20;
   int         istep = 0;

   lb_status->setText( tr( "Reading DataBase Data..." ) );
   progress->setMaximum( nstep );
   ddescs.clear();

QList<int> times;
QStringList tevent;
QTime t; t.start();
   // get raw data IDs
   query.clear();
   query << "all_rawDataIDs" << invID;
   db->query( query );

   while ( db->next() )
   {
      rawIDs << db->value( 0 ).toString();
   }
   progress->setValue( ++istep );
tevent << "all_raw"; times << t.elapsed();

   // get edited data IDs
   query.clear();
   query << "all_editedDataIDs" << invID;
   db->query( query );

   while ( db->next() )
   {
      edtIDs << db->value( 0 ).toString();
   }
   progress->setValue( ++istep );
tevent << "all_edt"; times << t.elapsed();
qDebug() << "TIMING ED step: " << istep;

   // get model IDs
   query.clear();
   query << "get_model_desc" << invID;
   db->query( query );

   while ( db->next() )
   {
      modIDs << db->value( 0 ).toString();
   }
   progress->setValue( ++istep );
tevent << "all_mod"; times << t.elapsed();

   // get noise IDs
   query.clear();
   query << "get_noise_desc" << invID;
   db->query( query );

   while ( db->next() )
   {
      noiIDs << db->value( 0 ).toString();
   }
   progress->setValue( ++istep );
tevent << "all_noi"; times << t.elapsed();
qDebug() << "TIMING NO step: " << istep;
   nraws = rawIDs.size();
   nedts = edtIDs.size();
   nmods = modIDs.size();
   nnois = noiIDs.size();
   nstep = istep + ( nraws * 5 ) + ( nedts * 5 ) + nmods + nnois;
   progress->setMaximum( nstep );
qDebug() << "BrDb: kr ke km kn"
 << rawIDs.size() << edtIDs.size() << modIDs.size() << noiIDs.size();

   for ( int ii = 0; ii < nraws; ii++ )
   {  // get raw data information from DB
      recID             = rawIDs.at( ii );
      irecID            = recID.toInt();

      query.clear();
      query << "get_rawData" << recID;
      db->query( query );
      db->next();
if ( ii<2 ) tevent << "raw 02 "; times << t.elapsed();

      //db.readBlobFromDB( fname, "download_aucData", irecID );
              rawGUID   = db->value( 0 ).toString();
      QString label     = db->value( 1 ).toString();
      QString filename  = db->value( 2 ).toString();
      QString comment   = db->value( 3 ).toString();
      QString experID   = db->value( 4 ).toString();
      QString date      = US_Util::toUTCDatetimeText( db->value( 6 )
                          .toDateTime().toUTC().toString( Qt::ISODate ),
                                                      true );
      QString runID     = filename.section( ".", 0, 0 );
      QString subType   = filename.section( ".", 1, 1 );
              contents  = "";
      QString filebase  = filename.section( "/", -1, -1 );
      rawGUIDs << rawGUID;

      query.clear();
      query << "get_experiment_info" << experID;
      db->query( query );
      db->next();

      QString expGUID   = db->value( 0 ).toString();
//qDebug() << "BrDb:     raw expGid" << expGUID;

      if ( details )
      {
         QString filetemp = US_Settings::tmpDir() + "/" + filebase;

         db->readBlobFromDB( filetemp, "download_aucData", irecID );

         contents         = US_Util::md5sum_file( filetemp );
//qDebug() << "BrDb:       (R)contents filetemp" << contents << filetemp;
      }

      cdesc.recordID    = irecID;
      cdesc.recType     = 1;
      cdesc.subType     = subType;
      cdesc.recState    = REC_DB;
      cdesc.dataGUID    = rawGUID.simplified();
      cdesc.parentGUID  = expGUID.simplified();
      cdesc.filename    = filename;
      cdesc.contents    = contents;
      cdesc.label       = label;
      cdesc.description = ( comment.isEmpty() ) ?
                          filename.section( ".", 0, 2 ) :
                          comment;
      cdesc.lastmodDate = date;

      if ( cdesc.dataGUID.length() != 36  ||  cdesc.dataGUID == dmyGUID )
         cdesc.dataGUID    = US_Util::new_guid();

      cdesc.parentGUID  = cdesc.parentGUID.length() == 36 ?
                          cdesc.parentGUID : dmyGUID;

      ddescs << cdesc;
      progress->setValue( ( istep += 5 ) );
   }
tevent << "nraws  "; times << t.elapsed();
qDebug() << "TIMING NR step: " << istep;

   for ( int ii = 0; ii < nedts; ii++ )
   {  // get edited data information from DB
      recID             = edtIDs.at( ii );
      irecID            = recID.toInt();

      query.clear();
      query << "get_editedData" << recID;
      db->query( query );
      db->next();

      QString rawID     = db->value( 0 ).toString();
      QString editGUID  = db->value( 1 ).toString();
      QString label     = db->value( 2 ).toString();
      QString filename  = db->value( 3 ).toString();
      QString comment   = db->value( 4 ).toString();
      QString date      = US_Util::toUTCDatetimeText( db->value( 5 )
                          .toDateTime().toUTC().toString( Qt::ISODate ),
                                                      true );
      QString subType   = filename.section( ".", 2, 2 );
              contents  = "";
      QString filebase  = filename.section( "/", -1, -1 );

              rawGUID   = rawGUIDs.at( rawIDs.indexOf( rawID ) );

qDebug() << "BrDb:     edt ii id eGID rGID label date"
 << ii << irecID << editGUID << rawGUID << label << date;

      if ( details )
      {
         QString filetemp = US_Settings::tmpDir() + "/" + filebase;

         db->readBlobFromDB( filetemp, "download_editData", irecID );

         contents         = US_Util::md5sum_file( filetemp );
//qDebug() << "BrDb:       (E)contents filetemp" << contents << filetemp;
      }

      cdesc.recordID    = irecID;
      cdesc.recType     = 2;
      cdesc.subType     = subType;
      cdesc.recState    = REC_DB;
      cdesc.dataGUID    = editGUID.simplified();
      cdesc.parentGUID  = rawGUID.simplified();
      //cdesc.filename    = filename;
      cdesc.filename    = "";
      cdesc.contents    = contents;
      cdesc.label       = label;
      cdesc.description = ( comment.isEmpty() ) ?
                          filename.section( ".", 0, 2 ) :
                          comment;
      cdesc.lastmodDate = date;

      if ( cdesc.dataGUID.length() != 36  ||  cdesc.dataGUID == dmyGUID )
         cdesc.dataGUID    = US_Util::new_guid();

      cdesc.parentGUID  = cdesc.parentGUID.simplified().length() == 36 ?
                          cdesc.parentGUID.simplified() : dmyGUID;

      ddescs << cdesc;
      progress->setValue( ( istep += 5 ) );
   }
tevent << "nedts  "; times << t.elapsed();
qDebug() << "TIMING NE step: " << istep;

   for ( int ii = 0; ii < nmods; ii++ )
   {  // get model information from DB
      recID             = modIDs.at( ii );
      irecID            = recID.toInt();

      query.clear();
      query << "get_model_info" << recID;
      db->query( query );
      db->next();
if ( ii<2 ) tevent << "mod 02 "; times << t.elapsed();

      QString modelGUID = db->value( 0 ).toString();
      QString descript  = db->value( 1 ).toString();
              contents  = db->value( 2 ).toString();
      QString label     = descript;
if ( ii<2 ) tevent << "mod 03 "; times << t.elapsed();

      if ( label.length() > 40 )
         label = descript.left( 18 ) + "..." + descript.right( 19 );

      QString subType   = model_type( contents );
      int     jj        = contents.indexOf( "editGUID=" );
      QString editGUID  = ( jj < 1 ) ? "" :
                          contents.mid( jj ).section( QChar( '"' ), 1, 1 );
if ( ii<2 ) tevent << "mod 04 "; times << t.elapsed();
//qDebug() << "BrDb:       mod ii id mGID dsc"
//   << ii << irecID << modelGUID << descript;

      if ( details )
      {
         QTemporaryFile temporary;
         temporary.open();
         temporary.write( contents.toAscii() );
         temporary.close();

         contents     = US_Util::md5sum_file( temporary.fileName() );
//qDebug() << "BrDb:         det: cont" << contents;
      }

      else
         contents     = "";
if ( ii<2 ) tevent << "mod 05 "; times << t.elapsed();

      cdesc.recordID    = irecID;
      cdesc.recType     = 3;
      cdesc.subType     = subType;
      cdesc.recState    = REC_DB;
      cdesc.dataGUID    = modelGUID.simplified();
      cdesc.parentGUID  = editGUID.simplified();
      cdesc.filename    = "";
      cdesc.contents    = contents;
      cdesc.label       = label;
      cdesc.description = descript;
      //cdesc.lastmodDate = QFileInfo( aucfile ).lastModified().toString();
      cdesc.lastmodDate = "";

      if ( cdesc.dataGUID.length() != 36  ||  cdesc.dataGUID == dmyGUID )
         cdesc.dataGUID    = US_Util::new_guid();

      cdesc.parentGUID  = cdesc.parentGUID.simplified().length() == 36 ?
                          cdesc.parentGUID.simplified() : dmyGUID;

      ddescs << cdesc;
      progress->setValue( ++istep );
if ( ii<2 ) tevent << "mod 09 "; times << t.elapsed();
   }
tevent << "nmods  "; times << t.elapsed();
qDebug() << "TIMING NM step: " << istep;

   for ( int ii = 0; ii < nnois; ii++ )
   {  // get noise information from DB
      recID             = noiIDs.at( ii );
      irecID            = recID.toInt();

      query.clear();
      query << "get_noise_info" << recID;
      db->query( query );
      db->next();

      QString noiseGUID = db->value( 0 ).toString();
      QString descript  = db->value( 1 ).toString();
      QString contents  = db->value( 2 ).toString();
      QString filename  = db->value( 3 ).toString();
      QString comment   = db->value( 4 ).toString();
      QString date      = db->value( 5 ).toString();
      QString modelGUID = db->value( 6 ).toString();
      QString label     = descript;

      if ( details )
      {
         QTemporaryFile temporary;
         temporary.open();
         temporary.write( contents.toAscii() );
         temporary.close();

         contents     = US_Util::md5sum_file( temporary.fileName() );
      }

      else
         contents     = "";

      if ( label.length() > 40 )
         label = descript.left( 18 ) + "..." + descript.right( 19 );

      cdesc.recordID    = irecID;
      cdesc.recType     = 4;
      cdesc.subType     = "";
      cdesc.recState    = REC_DB;
      cdesc.dataGUID    = noiseGUID.simplified();
      cdesc.parentGUID  = modelGUID.simplified();
      cdesc.filename    = filename;
      cdesc.contents    = contents;
      cdesc.label       = label;
      cdesc.description = descript;
      //cdesc.lastmodDate = QFileInfo( aucfile ).lastModified().toString();

      if ( cdesc.dataGUID.length() != 36  ||  cdesc.dataGUID == dmyGUID )
         cdesc.dataGUID    = US_Util::new_guid();

      cdesc.parentGUID  = cdesc.parentGUID.simplified().length() == 36 ?
                          cdesc.parentGUID.simplified() : dmyGUID;

      ddescs << cdesc;
      progress->setValue( ++istep );
   }
tevent << "nnois  "; times << t.elapsed();
for (int jj=0; jj<tevent.size(); jj++ )
 qDebug() << "TIMINGS: " << tevent[jj] << times[jj];
qDebug() << "TIMING DN step: " << istep;

   progress->setValue( nstep );
   lb_status->setText( tr( "Database Review Complete" ) );
}

// scan the local disk for R/E/M/N data sets
void US_DataModel::scan_local( )
{
   // start with AUC (raw) and edit files in directories of resultDir
   QString     rdir     = US_Settings::resultDir();
   QString     ddir     = US_Settings::dataDir();
   QString     dirm     = ddir + "/models";
   QString     dirn     = ddir + "/noises";
   QString     contents = "";
   QString     dmyGUID  = "00000000-0000-0000-0000-000000000000";
   QStringList aucdirs  = QDir( rdir )
      .entryList( QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name );
   QStringList aucfilt;
   QStringList edtfilt;
   QStringList modfilt( "M*xml" );
   QStringList noifilt( "N*xml" );
   QStringList modfils = QDir( dirm )
      .entryList( modfilt, QDir::Files, QDir::Name );
   QStringList noifils = QDir( dirn )
      .entryList( noifilt, QDir::Files, QDir::Name );
   int         ktask   = 0;
   int         naucd   = aucdirs.size();
   int         nmodf   = modfils.size();
   int         nnoif   = noifils.size();
   int         nstep   = naucd * 4 + nmodf + nnoif;
qDebug() << "BrLoc:  nau nmo nno nst" << naucd << nmodf << nnoif << nstep;
   aucfilt << "*.auc";
   edtfilt << "*.xml";
   rdir    = rdir + "/";
   lb_status->setText( tr( "Reading Local-Disk Data..." ) );
   progress->setMaximum( nstep );

   for ( int ii = 0; ii < naucd; ii++ )
   {  // loop thru potential data directories
      QString     subdir   = rdir + aucdirs.at( ii );
      QStringList aucfiles = QDir( subdir )
         .entryList( aucfilt, QDir::Files, QDir::Name );
      int         naucf    = aucfiles.size();
      US_DataIO2::RawData    rdata;
      US_DataIO2::EditValues edval;

      for ( int jj = 0; jj < naucf; jj++ )
      {  // loop thru .auc files found in a directory
         QString fname    = aucfiles.at( jj );
         QString runid    = fname.section( ".", 0, 0 );
         QString tripl    = fname.section( ".", -5, -2 );
         QString aucfile  = subdir + "/" + fname;
         QString descr    = "";
         QString expGUID  = expGUIDauc( aucfile );
                 contents = "";
qDebug() << "BrLoc: ii jj file" << ii << jj << aucfile;

         // read in the raw data and build description record
         US_DataIO2::readRawData( aucfile, rdata );

         if ( details )
         {
            contents     = US_Util::md5sum_file( aucfile );
qDebug() << "BrLoc:      contents" << contents;
         }

         char uuid[ 37 ];
         uuid_unparse( (uchar*)rdata.rawGUID, uuid );
         QString rawGUID  = QString( uuid );

         cdesc.recordID    = -1;
         cdesc.recType     = 1;
         cdesc.subType     = fname.section( ".", 1, 1 );
         cdesc.recState    = REC_LO;
         cdesc.dataGUID    = rawGUID.simplified();
         cdesc.parentGUID  = expGUID.simplified();
         cdesc.filename    = aucfile;
         cdesc.contents    = contents;
         cdesc.label       = runid + "." + tripl;
         cdesc.description = rdata.description;
         cdesc.lastmodDate = QFileInfo( aucfile ).lastModified().toString();

         if ( cdesc.dataGUID.length() != 36  ||  cdesc.dataGUID == dmyGUID )
            cdesc.dataGUID    = US_Util::new_guid();

         cdesc.parentGUID  = cdesc.parentGUID.simplified().length() == 36 ?
                             cdesc.parentGUID.simplified() : dmyGUID;

         ldescs << cdesc;

         // now load edit files associated with this auc file
         edtfilt.clear();
         edtfilt << runid + ".*." + tripl + ".xml";
qDebug() << "BrLoc:  edtfilt" << edtfilt;

         QStringList edtfiles = QDir( subdir )
            .entryList( edtfilt, QDir::Files, QDir::Name );

         for ( int kk = 0; kk < edtfiles.size(); kk++ )
         {
            QString efname   = edtfiles.at( kk );
            QString editid   = efname.section( ".", 1, 3 );
            QString edtfile  = subdir + "/" + efname;
                    contents = "";
//qDebug() << "BrLoc:    kk file" << kk << edtfile;

            // read EditValues for the edit data and build description record
            US_DataIO2::readEdits( edtfile, edval );

            if ( details )
            {
               contents     = US_Util::md5sum_file( edtfile );
//qDebug() << "BrLoc:      (E)contents edtfile" << contents << edtfile;
            }

            cdesc.recordID    = -1;
            cdesc.recType     = 2;
            cdesc.subType     = efname.section( ".", 2, 2 );
            cdesc.recState    = REC_LO;
            cdesc.dataGUID    = edval.editGUID.simplified();
            cdesc.parentGUID  = edval.dataGUID.simplified();
            cdesc.filename    = edtfile;
            cdesc.contents    = contents;
            cdesc.label       = runid + "." + editid;
            cdesc.description = cdesc.label;
            cdesc.lastmodDate = QFileInfo( edtfile ).lastModified().toString();

            if ( cdesc.dataGUID.length() != 36  ||  cdesc.dataGUID == dmyGUID )
               cdesc.dataGUID    = US_Util::new_guid();

            cdesc.parentGUID  = cdesc.parentGUID.simplified().length() == 36 ?
                                cdesc.parentGUID.simplified() : dmyGUID;

            ldescs << cdesc;
         }
         if ( ii == ( naucd / 2 )  &&  jj == ( naucf / 2 ) )
            progress->setValue( ++ktask );
      }
      progress->setValue( ++ktask );
   }
   progress->setValue( ++ktask );

   for ( int ii = 0; ii < nmodf; ii++ )
   {  // loop thru potential model files
      US_Model    model;
      QString     modfil   = dirm + "/" + modfils.at( ii );
                  contents = "";

      model.load( modfil );

      if ( details )
      {
         contents     = US_Util::md5sum_file( modfil );
      }

      cdesc.recordID    = -1;
      cdesc.recType     = 3;
      cdesc.subType     = model_type( model );
      cdesc.recState    = REC_LO;
      cdesc.dataGUID    = model.modelGUID.simplified();
      cdesc.parentGUID  = model.editGUID.simplified();
      cdesc.filename    = modfil;
      cdesc.contents    = contents;
      cdesc.description = model.description;
      cdesc.lastmodDate = QFileInfo( modfil ).lastModified().toString();

      if ( cdesc.dataGUID.length() != 36  ||  cdesc.dataGUID == dmyGUID )
         cdesc.dataGUID    = US_Util::new_guid();

      cdesc.parentGUID  = cdesc.parentGUID.simplified().length() == 36 ?
                          cdesc.parentGUID.simplified() : dmyGUID;

      if ( model.description.length() < 41 )
         cdesc.label       = model.description;
      else
         cdesc.label       = model.description.left( 18 )  + "..."
                           + model.description.right( 19 );

      ldescs << cdesc;

      progress->setValue( ++ktask );
   }

   for ( int ii = 0; ii < nnoif; ii++ )
   {  // loop thru potential noise files
      US_Noise    noise;
      QString     noifil   = dirn + "/" + noifils.at( ii );
                  contents = "";

      noise.load( noifil );

      if ( details )
      {
         contents     = US_Util::md5sum_file( noifil );
      }

      cdesc.recordID    = -1;
      cdesc.recType     = 4;
      cdesc.subType     = ( noise.type == US_Noise::RI ) ? "RI" : "TI";
      cdesc.recState    = REC_LO;
      cdesc.dataGUID    = noise.noiseGUID.simplified();
      cdesc.parentGUID  = noise.modelGUID.simplified();
      cdesc.filename    = noifil;
      cdesc.contents    = contents;
      cdesc.description = noise.description;
      cdesc.lastmodDate = QFileInfo( noifil ).lastModified().toString();

      if ( cdesc.dataGUID.length() != 36  ||  cdesc.dataGUID == dmyGUID )
         cdesc.dataGUID    = US_Util::new_guid();

      cdesc.parentGUID  = cdesc.parentGUID.simplified().length() == 36 ?
                          cdesc.parentGUID.simplified() : dmyGUID;

      if ( noise.description.length() < 41 )
         cdesc.label       = noise.description;
      else
         cdesc.label       = noise.description.left( 18 )  + "..."
                           + noise.description.right( 19 );

      ldescs << cdesc;

      progress->setValue( ++ktask );
   }

   progress->setValue( nstep );
   lb_status->setText( tr( "Local Data Review Complete" ) );
}

// merge the database and local description vectors into a single combined
void US_DataModel::merge_dblocal( )
{
   int nddes = ddescs.size();
   int nldes = ldescs.size();
   int nstep = ( ( nddes + nldes ) * 5 ) / 8;

   int jdr   = 0;
   int jlr   = 0;
   int kar   = 1;

   DataDesc  descd = ddescs.at( 0 );
   DataDesc  descl = ldescs.at( 0 );
//qDebug() << "MERGE: nd nl dlab llab"
// << nddes << nldes << descd.label << descl.label;

   lb_status->setText( tr( "Merging Data ..." ) );
   progress->setMaximum( nstep );

   while ( jdr < nddes  &&  jlr < nldes )
   {  // main loop to merge records until one is exhausted

      progress->setValue( kar );           // report progress

      if ( kar > nstep )
      {  // if count beyond max, bump max by one eighth
         nstep = ( kar * 9 ) / 8;
         progress->setMaximum( nstep );
      }

      while ( descd.dataGUID == descl.dataGUID )
      {  // records match in GUID:  merge them into one
         descd.recState    |= descl.recState;     // OR states
         descd.filename     = descl.filename;     // filename from local
         descd.lastmodDate  = descl.lastmodDate;  // last mod date from local

         if ( details )
            descd.contents     = descd.contents + " " + descl.contents;

         adescs << descd;                  // output combo record
//qDebug() << "MERGE:  kar jdr jlr (1)GID" << kar << jdr << jlr << descd.dataGUID;
         kar++;

         if ( ++jdr < nddes )              // bump db count and test if done
            descd = ddescs.at( jdr );      // get next db record

         else
         {
            if ( ++jlr < nldes )
               descl = ldescs.at( jlr );   // get next local record
            break;
         }


         if ( ++jlr < nldes )              // bump local count and test if done
            descl = ldescs.at( jlr );      // get next local record
         else
            break;
      }

      if ( jdr >= nddes  ||  jlr >= nldes )
         break;

      while ( descd.recType > descl.recType )
      {  // output db records that are left-over children
         adescs << descd;
//qDebug() << "MERGE:  kar jdr jlr (2)GID" << kar << jdr << jlr << descd.dataGUID;
         kar++;

         if ( ++jdr < nddes )
            descd = ddescs.at( jdr );
         else
            break;
      }

      if ( jdr >= nddes  ||  jlr >= nldes )
         break;

      while ( descl.recType > descd.recType )
      {  // output local records that are left-over children
         adescs << descl;
//qDebug() << "MERGE:  kar jdr jlr (3)GID" << kar << jdr << jlr << descl.dataGUID;
         kar++;

         if ( ++jlr < nldes )
            descl = ldescs.at( jlr );
         else
            break;
      }

      if ( jdr >= nddes  ||  jlr >= nldes )
         break;

      // If we've reached another matching pair or if we are not at
      // the same level, go back up to the start of the main loop.
      if ( descd.dataGUID == descl.dataGUID  ||
           descd.recType  != descl.recType  )
         continue;

      // If we are here, we have records at the same level,
      // but with different GUIDs. Output one of them, based on
      // an alphanumeric comparison of label values.

      if ( descd.label < descl.label )
      {  // output db record first based on alphabetic label sort
         adescs << descd;
//qDebug() << "MERGE:  kar jdr jlr (4)GID" << kar << jdr << jlr << descd.dataGUID;
         kar++;

         if ( ++jdr < nddes )
            descd = ddescs.at( jdr );
         else
            break;
      }

      else
      {  // output local record first based on alphabetic label sort
         adescs << descl;
//qDebug() << "MERGE:  kar jdr jlr (5)GID" << kar << jdr << jlr << descl.dataGUID;
         kar++;

         if ( ++jlr < nldes )
            descl = ldescs.at( jlr );
         else
            break;
      }

   }  // end of main merge loop;

   // after breaking from main loop, output any records left from one
   // source (db/local) or the other.
   nstep += ( nddes - jdr + nldes - jlr );
   progress->setMaximum( nstep );

   while ( jdr < nddes )
   {
      adescs << ddescs.at( jdr++ );
//descd=ddescs.at(jlr-1);
//qDebug() << "MERGE:  kar jdr jlr (8)GID" << kar << jdr << jlr << descd.dataGUID;
      kar++;
      progress->setValue( kar );
   }

   while ( jlr < nldes )
   {
      adescs << ldescs.at( jlr++ );
//descl=ldescs.at(jlr-1);
//qDebug() << "MERGE:  kar jdr jlr (9)GID" << kar << jdr << jlr << descl.dataGUID;
      kar++;
      progress->setValue( kar );
   }

//qDebug() << "MERGE: nddes nldes kar" << nddes << nldes << --kar;
//qDebug() << " a/d/l sizes" << adescs.size() << ddescs.size() << ldescs.size();

   progress->setValue( nstep );
   lb_status->setText( tr( "Data Merge Complete" ) );
}

// sort a data-set description vector
void US_DataModel::sort_descs( QVector< DataDesc >& descs )
{
   QVector< DataDesc > tdess;                 // temporary descr. vector
   DataDesc            desct;                 // temporary descr. entry
   QStringList         sortr;                 // sort string lists
   QStringList         sorte;
   QStringList         sortm;
   QStringList         sortn;
   int                 nrecs = descs.size();  // number of descr. records

   if ( nrecs == 0 )
      return;

   tdess.resize( nrecs );

   for ( int ii = 0; ii < nrecs; ii++ )
   {  // build sort strings for Raw,Edit,Model,Noise; copy unsorted vector
      desct        = descs[ ii ];

      if (      desct.recType == 1 )
         sortr << sort_string( desct, ii );

      else if ( desct.recType == 2 )
         sorte << sort_string( desct, ii );

      else if ( desct.recType == 3 )
         sortm << sort_string( desct, ii );

      else if ( desct.recType == 4 )
         sortn << sort_string( desct, ii );

      tdess[ ii ]  = desct;
   }

   // sort the string lists for each type
   sortr.sort();
   sorte.sort();
   sortm.sort();
   sortn.sort();

   // review each type for duplicate GUIDs
   if ( review_descs( sortr, tdess ) )
      return;
   if ( review_descs( sorte, tdess ) )
      return;
   if ( review_descs( sortm, tdess ) )
      return;
   if ( review_descs( sortn, tdess ) )
      return;

   // create list of noise,model,edit orphans
   QStringList orphn = list_orphans( sortn, sortm );
   QStringList orphm = list_orphans( sortm, sorte );
   QStringList orphe = list_orphans( sorte, sortr );

   QString dmyGUID = "00000000-0000-0000-0000-000000000000";
   QString dsorts;
   QString dlabel;
   QString dindex;
   QString ddGUID;
   QString dpGUID;
   QString ppGUID;
   int kndx = tdess.size();
   int jndx;
   int kk;
   int ndmy = 0;     // flag of duplicate dummies

   // create dummy records to parent each orphan

   for ( int ii = 0; ii < orphn.size(); ii++ )
   {  // for each orphan noise, create a dummy model
      dsorts = orphn.at( ii );
      dlabel = dsorts.section( ":", 0, 0 );
      dindex = dsorts.section( ":", 1, 1 );
      ddGUID = dsorts.section( ":", 2, 2 ).simplified();
      dpGUID = dsorts.section( ":", 3, 3 ).simplified();
      jndx   = dindex.toInt();
      cdesc  = tdess[ jndx ];

      if ( dpGUID.length() < 2 )
      { // handle case where there is no valid parentGUID
         if ( ndmy == 0 )      // first time:  create one
            dpGUID = dmyGUID;
         else
            dpGUID = ppGUID;   // afterwards:  re-use same parent

         kk     = sortn.indexOf( dsorts );  // find index in full list
         dsorts = dlabel + ":" + dindex + ":" + ddGUID + ":" + dpGUID;

         if ( kk >= 0 )
         {  // replace present record for new parentGUID
            sortn.replace( kk, dsorts );
            cdesc.parentGUID  = dpGUID;
            tdess[ jndx ]     = cdesc;
         }

         if ( ndmy > 0 )       // after 1st time, skip creating new parent
            continue;

         ndmy++;               // flag that we have a parent for invalid ones
         ppGUID = dpGUID;      // save the GUID for new dummy parent
      }

      // if this record is no longer an orphan, skip creating new parent
      if ( index_substring( dpGUID, 2, sortm ) >= 0 )
         continue;

      if ( dpGUID == dmyGUID )
         cdesc.label       = "Dummy-Model-for-Orphans";

      cdesc.parentID    = cdesc.recordID;
      cdesc.recordID    = -1;
      cdesc.recType     = 3;
      cdesc.subType     = "";
      cdesc.recState    = NOSTAT;
      cdesc.dataGUID    = dpGUID;
      cdesc.parentGUID  = dmyGUID;
      cdesc.filename    = "";
      cdesc.contents    = "";
      cdesc.label       = cdesc.label.section( ".", 0, 0 );
      cdesc.description = cdesc.label + "--ARTIFICIAL-RECORD";
      cdesc.lastmodDate = QDateTime::currentDateTime().toUTC().toString();

      dlabel = dlabel.section( ".", 0, 0 );
      dindex = QString().sprintf( "%4.4d", kndx++ );
      ddGUID = cdesc.dataGUID;
      dpGUID = cdesc.parentGUID;
      dsorts = dlabel + ":" + dindex + ":" + ddGUID + ":" + dpGUID;

      sortm << dsorts;
      orphm << dsorts;
      tdess.append( cdesc );
//qDebug() << "N orphan:" << orphn.at( ii );
//qDebug() << "  M dummy:" << dsorts;
   }

   ndmy   = 0;

   for ( int ii = 0; ii < orphm.size(); ii++ )
   {  // for each orphan model, create a dummy edit
      dsorts = orphm.at( ii );
      dlabel = dsorts.section( ":", 0, 0 );
      dindex = dsorts.section( ":", 1, 1 );
      ddGUID = dsorts.section( ":", 2, 2 ).simplified();
      dpGUID = dsorts.section( ":", 3, 3 ).simplified();
      jndx   = dindex.toInt();
      cdesc  = tdess[ jndx ];

      if ( dpGUID.length() < 16 )
      { // handle case where there is no valid parentGUID
         if ( ndmy == 0 )      // first time:  create one
            dpGUID = dmyGUID;
         else
            dpGUID = ppGUID;   // afterwards:  re-use same parent

         kk     = sortm.indexOf( dsorts );  // find index in full list
         dsorts = dlabel + ":" + dindex + ":" + ddGUID + ":" + dpGUID;

         if ( kk >= 0 )
         {  // replace present record for new parentGUID
            sortm.replace( kk, dsorts );
            cdesc.parentGUID  = dpGUID;
            tdess[ jndx ]     = cdesc;
         }

         if ( ndmy > 0 )       // after 1st time, skip creating new parent
            continue;

         ndmy++;               // flag that we have a parent for invalid ones
         ppGUID = dpGUID;      // save the GUID for new dummy parent
      }

      // if this record is no longer an orphan, skip creating new parent
      if ( index_substring( dpGUID, 2, sorte ) >= 0 )
         continue;

      if ( dpGUID == dmyGUID )
         cdesc.label       = "Dummy-Edit-for-Orphans";

      cdesc.parentID    = cdesc.recordID;
      cdesc.recordID    = -1;
      cdesc.recType     = 2;
      cdesc.subType     = "";
      cdesc.recState    = NOSTAT;
      cdesc.dataGUID    = dpGUID;
      cdesc.parentGUID  = dmyGUID;
      cdesc.filename    = "";
      cdesc.contents    = "";
      cdesc.label       = cdesc.label.section( ".", 0, 0 );
      cdesc.description = cdesc.label + "--ARTIFICIAL-RECORD";
      cdesc.lastmodDate = QDateTime::currentDateTime().toUTC().toString();

      dlabel = dlabel.section( ".", 0, 0 );
      dindex = QString().sprintf( "%4.4d", kndx++ );
      ddGUID = cdesc.dataGUID;
      dpGUID = cdesc.parentGUID;
      dsorts = dlabel + ":" + dindex + ":" + ddGUID + ":" + dpGUID;

      sorte << dsorts;
      orphe << dsorts;
      tdess.append( cdesc );
//qDebug() << "M orphan:" << orphm.at( ii );
//qDebug() << "  E dummy:" << dsorts;
   }

   ndmy   = 0;

   for ( int ii = 0; ii < orphe.size(); ii++ )
   {  // for each orphan edit, create a dummy raw
      dsorts = orphe.at( ii );
      dlabel = dsorts.section( ":", 0, 0 );
      dindex = dsorts.section( ":", 1, 1 );
      ddGUID = dsorts.section( ":", 2, 2 ).simplified();
      dpGUID = dsorts.section( ":", 3, 3 ).simplified();
      jndx   = dindex.toInt();
      cdesc  = tdess[ jndx ];

      if ( dpGUID.length() < 2 )
      { // handle case where there is no valid parentGUID
         if ( ndmy == 0 )      // first time:  create one
            dpGUID = dmyGUID;
         else
            dpGUID = ppGUID;   // afterwards:  re-use same parent

         kk     = sorte.indexOf( dsorts );  // find index in full list
         dsorts = dlabel + ":" + dindex + ":" + ddGUID + ":" + dpGUID;

         if ( kk >= 0 )
         {  // replace present record for new parentGUID
            sorte.replace( kk, dsorts );
            cdesc.parentGUID  = dpGUID;
            tdess[ jndx ]     = cdesc;
         }

         if ( ndmy > 0 )       // after 1st time, skip creating new parent
            continue;

         ndmy++;               // flag that we have a parent for invalid ones
         ppGUID = dpGUID;      // save the GUID for new dummy parent
      }

      // if this record is no longer an orphan, skip creating new parent
      if ( index_substring( dpGUID, 2, sortr ) >= 0 )
         continue;

      if ( dpGUID == dmyGUID )
         cdesc.label       = "Dummy-Raw-for-Orphans";

      cdesc.parentID    = cdesc.recordID;
      cdesc.recordID    = -1;
      cdesc.recType     = 1;
      cdesc.subType     = "";
      cdesc.recState    = NOSTAT;
      cdesc.dataGUID    = dpGUID;
      cdesc.parentGUID  = dmyGUID;
      cdesc.filename    = "";
      cdesc.contents    = "";
      cdesc.label       = cdesc.label.section( ".", 0, 0 );
      cdesc.description = cdesc.label + "--ARTIFICIAL-RECORD";
      cdesc.lastmodDate = QDateTime::currentDateTime().toUTC().toString();

      dlabel = dlabel.section( ".", 0, 0 );
      dindex = QString().sprintf( "%4.4d", kndx++ );
      ddGUID = cdesc.dataGUID;
      dpGUID = cdesc.parentGUID;
      dsorts = dlabel + ":" + dindex + ":" + ddGUID + ":" + dpGUID;

      sortr << dsorts;
      tdess.append( cdesc );
qDebug() << "E orphan:" << orphe.at( ii );
qDebug() << "  R dummy:" << dsorts;
   }

//for ( int ii = 0; ii < sortr.size(); ii++ )
// qDebug() << "R entry:" << sortr.at( ii );
   int countR = sortr.size();    // count of each kind in sorted lists
   int countE = sorte.size();
   int countM = sortm.size();
   int countN = sortn.size();

   sortr.sort();                 // re-sort for dummy additions
   sorte.sort();
   sortm.sort();
   sortn.sort();
qDebug() << "sort/dumy: count REMN" << countR << countE << countM << countN;

   int noutR  = 0;               // count of each kind in hierarchical output
   int noutE  = 0;
   int noutM  = 0;
   int noutN  = 0;
   int indx;
   int pstate = REC_LO | PAR_LO;

   descs.clear();                // reset input vector to become sorted output

   // rebuild the description vector with sorted trees

   for ( int ii = 0; ii < countR; ii++ )
   {  // loop to output sorted Raw records
      QString recr = sortr[ ii ];
      QString didr = recr.section( ":", 2, 2 );
      QString pidr = recr.section( ":", 3, 3 );
      indx         = recr.section( ":", 1, 1 ).toInt();
      cdesc        = tdess.at( indx );

      // set up a default parent state flag
      pstate = cdesc.recState;
      pstate = ( pstate & REC_DB ) != 0 ? ( pstate | PAR_DB ) : pstate;
      pstate = ( pstate & REC_LO ) != 0 ? ( pstate | PAR_LO ) : pstate;

      // new state is the default,  or NOSTAT if this is a dummy record
      cdesc.recState = record_state_flag( cdesc, pstate );

      descs << cdesc;                   // output Raw rec
      noutR++;

      // set up parent state for children to follow
      int rpstate    = cdesc.recState;

      for ( int jj = 0; jj < countE; jj++ )
      {  // loop to output sorted Edit records for the above Raw
         QString rece   = sorte[ jj ];
         QString pide   = rece.section( ":", 3, 3 );

         if ( pide != didr )            // skip if current Raw not parent
            continue;

         QString dide   = rece.section( ":", 2, 2 );
         indx           = rece.section( ":", 1, 1 ).toInt();
         cdesc          = tdess.at( indx );
         cdesc.recState = record_state_flag( cdesc, rpstate );

         descs << cdesc;                // output Edit rec
         noutE++;

         // set up parent state for children to follow
         int epstate    = cdesc.recState;

         for ( int mm = 0; mm < countM; mm++ )
         {  // loop to output sorted Model records for above Edit
            QString recm   = sortm[ mm ];
            QString pidm   = recm.section( ":", 3, 3 );

            if ( pidm != dide )         // skip if current Edit not parent
               continue;

            QString didm   = recm.section( ":", 2, 2 );
            indx           = recm.section( ":", 1, 1 ).toInt();
            cdesc          = tdess.at( indx );
            cdesc.recState = record_state_flag( cdesc, epstate );

            descs << cdesc;             // output Model rec

            noutM++;

            // set up parent state for children to follow
            int mpstate    = cdesc.recState;

            for ( int nn = 0; nn < countN; nn++ )
            {  // loop to output sorted Noise records for above Model
               QString recn   = sortn[ nn ];
               QString pidn   = recn.section( ":", 3, 3 );

               if ( pidn != didm )      // skip if current Model not parent
                  continue;

               indx           = recn.section( ":", 1, 1 ).toInt();
               cdesc          = tdess.at( indx );
               cdesc.recState = record_state_flag( cdesc, mpstate );

               descs << cdesc;          // output Noise rec

               noutN++;
            }
         }
      }
   }

   if ( noutR != countR  ||  noutE != countE  ||
        noutM != countM  ||  noutN != countN )
   {  // not all accounted for, so we will need some dummy parents
      qDebug() << "sort_desc: count REMN"
         << countR << countE << countM << countN;
      qDebug() << "sort_desc: nout REMN"
         << noutR << noutE << noutM << noutN;
   }
}

// review sorted string lists for duplicate GUIDs
bool US_DataModel::review_descs( QStringList& sorts,
      QVector< DataDesc >& descv )
{
   bool           abort = false;
   int            nrecs = sorts.size();
   int            nmult = 0;
   int            kmult = 0;
   int            ityp;
   QString        cGUID;
   QString        pGUID;
   QString        rtyp;
   QVector< int > multis;
   const char* rtyps[] = { "RawData", "EditedData", "Model", "Noise" };

   if ( nrecs < 1 )
      return abort;

   int ii = sorts[ 0 ].section( ":", 1, 1 ).toInt();
   ityp   = descv[ ii ].recType;
   rtyp   = QString( rtyps[ ityp - 1 ] );

   if ( descv[ ii ].recordID >= 0 )
      rtyp   = "DB " + rtyp;
   else
      rtyp   = "Local " + rtyp;
qDebug() << "RvwD: ii ityp rtyp nrecs" << ii << ityp << rtyp << nrecs;

   for ( int ii = 1; ii < nrecs; ii++ )
   {  // do a pass to determine if there are duplicate GUIDs
      cGUID    = sorts[ ii ].section( ":", 2, 2 );     // current rec GUID
      kmult    = 0;                                    // flag no multiples yet

      for ( int jj = 0; jj < ii; jj++ )
      {  // review all the records preceeding this one
         pGUID    = sorts[ jj ].section( ":", 2, 2 );  // a previous GUID

         if ( pGUID == cGUID )
         {  // found a duplicate
            kmult++;

            if ( ! multis.contains( jj ) )
            {  // not yet marked, so mark previous as multiple
               multis << jj;    // save index
               nmult++;         // bump count
            }

            else  // if it was marked, we can quit the inner loop
               break;
         }
      }

      if ( kmult > 0 )
      {  // this pass found a duplicate:  save the index and bump count
         multis << ii;
         nmult++;
      }
//qDebug() << "RvwD:   ii kmult nmult" << ii << kmult << nmult;
   }

qDebug() << "RvwD:      nmult" << nmult;
   if ( nmult > 0 )
   {  // there were multiple instances of the same GUID
      QMessageBox msgBox;
      QString     msg;

      // format a message for the warning pop-up
      msg  =
         tr( "There are %1 %2 records that have\n" ).arg( nmult ).arg( rtyp ) +
         tr( "the same GUID as another.\n" ) +
         tr( "You should correct the situation before proceeding.\n" ) +
         tr( "  Click \"Ok\" to see details, then abort.\n" ) +
         tr( "  Click \"Ignore\" to proceed to further review.\n" );
      msgBox.setWindowTitle( tr( "Duplicate %1 Records" ).arg( rtyp ) );
      msgBox.setText( msg );
      msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Ignore );
      msgBox.setDefaultButton( QMessageBox::Ok );

      if ( msgBox.exec() == QMessageBox::Ok )
      {  // user wants details, so display them
         QString fileexts = tr( "Text,Log files (*.txt *.log);;" )
            + tr( "All files (*)" );
         QString pGUID = "";
         QString cGUID;
         QString label;

         msg =
            tr( "Review the details below on duplicate records.\n" ) +
            tr( "Save or Print the contents of this message.\n" ) +
            tr( "Decide which of the duplicates should be removed.\n" ) +
            tr( "Close the main US_DataModel window after exiting here.\n" ) +
            tr( "\nSummary of Duplicates:\n\n" );

         for ( int ii = 0; ii < nmult; ii++ )
         {  // add summary lines on duplicates
            int jj = multis.at( ii );
            cGUID  = sorts.at( jj ).section( ":", 2, 2 );
            label  = sorts.at( jj ).section( ":", 0, 0 );

            if ( cGUID != pGUID )
            {  // first instance of this GUID:  show GUID
               msg  += tr( "GUID:  " ) + cGUID + "\n";
               pGUID = cGUID;
            }

            // one label line for each multiple
            msg  += tr( "  Label:  " ) + label + "\n";
         }

         msg += tr( "\nDetails of Duplicates:\n\n" );

         for ( int ii = 0; ii < nmult; ii++ )
         {  // add detail lines
            int jj = multis.at( ii );
            cGUID  = sorts.at( jj ).section( ":", 2, 2 );
            pGUID  = sorts.at( jj ).section( ":", 3, 3 );
            label  = sorts.at( jj ).section( ":", 0, 0 );
            int kk = sorts.at( jj ).section( ":", 1, 1 ).toInt();
            cdesc  = descv[ kk ];

            msg   += tr( "GUID:  " ) + cGUID + "\n" +
               tr( "  ParentGUID:  " ) + pGUID + "\n" +
               tr( "  Label:  " ) + label + "\n" +
               tr( "  Description:  " ) + cdesc.description + "\n" +
               tr( "  DB record ID:  %1" ).arg( cdesc.recordID ) + "\n" +
               tr( "  File Directory:  " ) +
               cdesc.filename.section( "/",  0, -2 ) + "\n" +
               tr( "  File Name:  " ) +
               cdesc.filename.section( "/", -1, -1 ) + "\n" +
               tr( "  Last Mod Date:  " ) +
               cdesc.lastmodDate + "\n";
         }

         // pop up text dialog
         US_Editor* editd = new US_Editor( US_Editor::LOAD, true, fileexts );
         editd->setWindowTitle( tr( "Data Set Duplicate GUID Details" ) );
         editd->move( QCursor::pos() + QPoint( 200, 200 ) );
         editd->resize( 600, 500 );
         editd->e->setFont( QFont( "monospace", US_GuiSettings::fontSize() ) );
         editd->e->setText( msg );
         editd->show();

         abort = true;      // tell caller to abort data tree build
      }

      else
      {
         abort = false;     // signal to proceed with data tree build
      }
   }
qDebug() << "review_descs   abort" << abort;

   return abort;
}

// find index of substring at given position in strings of string list
int US_DataModel::index_substring( QString ss, int ixs, QStringList& sl )
{
   QString sexp = "XXX";
   QRegExp rexp;

   if ( ixs == 0 )
      sexp = ss + ":*";        // label at beginning of strings in list

   else if ( ixs == 1  ||  ixs == 2 )
      sexp = "*:" + ss + ":*"; // RecIndex/recGUID in middle of list strings

   else if ( ixs == 3 )
      sexp = "*:" + ss;        // parentGUID at end of strings in list

   rexp = QRegExp( sexp, Qt::CaseSensitive, QRegExp::Wildcard );

   return sl.indexOf( rexp );
}

// get sublist from string list of substring matches at a given string position
QStringList US_DataModel::filter_substring( QString ss, int ixs,
   QStringList& sl )
{
   QStringList subl;

   if ( ixs == 0 )
      // match label at beginning of strings in list
      subl = sl.filter( QRegExp( "^" + ss + ":" ) );

   else if ( ixs == 1  ||  ixs == 2 )
      // match RecIndex or recGUID in middle of strings in list
      subl = sl.filter( ":" + ss + ":" );

   else if ( ixs == 3 )
      // match parentGUID at end of strings in list
      subl = sl.filter( QRegExp( ":" + ss + "$" ) );

   return subl;
}

// list orphans of a record type (in rec list, no tie to parent list)
QStringList US_DataModel::list_orphans( QStringList& rlist,
   QStringList& plist )
{
   QStringList olist;

   for ( int ii = 0; ii < rlist.size(); ii++ )
   {  // examine parentGUID for each record in the list
      QString pGUID = rlist.at( ii ).section( ":", 3, 3 );

      // see if it is the recordGUID of any in the potential parent list
      if ( index_substring( pGUID, 2, plist ) < 0 )
         olist << rlist.at( ii ); // no parent found, so add to the orphan list
   }

   return olist;
}

// return a record state flag with parent state ORed in
int US_DataModel::record_state_flag( DataDesc descr, int pstate )
{
   int state = descr.recState;

   if ( descr.recState == NOSTAT  ||
        descr.description.contains( "-ARTIFICIAL" ) )
      state = NOSTAT;                    // mark a dummy record

   else
   {  // detect and mark parentage of non-dummy
      if ( ( pstate & REC_DB ) != 0 )
         state = state | PAR_DB;         // mark a record with db parent

      if ( ( pstate & REC_LO ) != 0 )
         state = state | PAR_LO;         // mark a record with local parent
   }

   return state;
}

// compose concatenation on which to sort (label:index:dataGUID:parentGUID)
QString US_DataModel::sort_string( DataDesc ddesc, int indx )
{  // create string for ascending sort on label
   QString ostr = ddesc.label                        // label to sort on
      + ":"     + QString().sprintf( "%4.4d", indx ) // index in desc. vector
      + ":"     + ddesc.dataGUID                     // data GUID
      + ":"     + ddesc.parentGUID;                  // parent GUID
   return ostr;
}

// compose string describing model type
QString US_DataModel::model_type( int imtype, int nassoc, int niters )
{
   QString mtype;

   // format the base model type string
   switch ( imtype )
   {
      default:
      case (int)US_Model::TWODSA:
      case (int)US_Model::MANUAL:
         mtype = "2DSA";
         break;
      case (int)US_Model::TWODSA_MW:
         mtype = "2DSA-MW";
         break;
      case (int)US_Model::GA:
      case (int)US_Model::GA_RA:
         mtype = "GA";
         break;
      case (int)US_Model::GA_MW:
         mtype = "GA-MW";
         break;
      case (int)US_Model::COFS:
         mtype = "COFS";
         break;
      case (int)US_Model::FE:
         mtype = "FE";
         break;
      case (int)US_Model::GLOBAL:
         mtype = "GLOBAL";
         break;
      case (int)US_Model::ONEDSA:
         mtype = "1DSA";
         break;
   }

   // add RA for Reversible Associations (if associations count > 1)
   if ( nassoc > 1 )
      mtype = mtype + "-RA";

   // add MC for Monte Carlo (if iterations count > 1)
   if ( niters > 1 )
      mtype = mtype + "-MC";

   return mtype;
}

// compose string describing model type
QString US_DataModel::model_type( US_Model model )
{
   // return model type string based on integer flags in the model object
   return model_type( (int)model.type,
                      model.associations.size(),
                      model.iterations );
}

// compose string describing model type
QString US_DataModel::model_type( QString modxml )
{
   QChar quo( '"' );
   int   jj;
   int   imtype;
   int   nassoc;
   int   niters;

   // model type number from type attribute
   jj       = modxml.indexOf( " type=" );
   imtype   = ( jj < 1 ) ? 0 : modxml.mid( jj ).section( quo, 1, 1 ).toInt();

   // count of associations is count of k_eq attributes present
   nassoc   = modxml.count( "k_eq=" );

   // number of iterations from iterations attribute value
   jj       = modxml.indexOf( " iterations=" );
   niters   = ( jj < 1 ) ? 0 : modxml.mid( jj ).section( quo, 1, 1 ).toInt();

   // return model type string based on integer flags
   return model_type( imtype, nassoc, niters );
}

void US_DataModel::dummy_data()
{
   adescs.clear();
   ddescs.clear();
   ldescs.clear();
   details    = true;

   cdesc.recType        = 1;
   cdesc.recState       = REC_DB | PAR_DB;
   cdesc.subType        = "";
   cdesc.label          = "item_1_2";
   cdesc.description    = "demo1_veloc";
   cdesc.dataGUID       = "demo1_veloc";
   cdesc.recordID       = 1;
   cdesc.filename       = "";
   adescs<<cdesc;
   ddescs<<cdesc;

   cdesc.recType        = 2;
   cdesc.recState       = REC_DB | REC_LO | PAR_DB | PAR_LO;
   cdesc.subType        = "RA";
   cdesc.label          = "item_2_2";
   cdesc.description    = "demo1_veloc";
   cdesc.contents       = "AA 12 AA 12";
   cdesc.recordID       = 2;
   cdesc.filename       = "demo1_veloc_edit.xml";
   adescs<<cdesc;
   ddescs<<cdesc;
   ldescs<<cdesc;

   cdesc.recType        = 3;
   cdesc.recState       = REC_LO | PAR_LO;
   cdesc.subType        = "2DSA";
   cdesc.label          = "item_3_2";
   cdesc.description    = "demo1_veloc.sa2d.model.11";
   cdesc.recordID       = -1;
   cdesc.filename       = "demo1_veloc_model.xml";
   adescs<<cdesc;
   ldescs<<cdesc;

   cdesc.recType        = 4;
   cdesc.recState       = REC_DB | REC_LO | PAR_DB | PAR_LO;
   cdesc.subType        = "TI";
   cdesc.label          = "item_4_2";
   cdesc.description    = "demo1_veloc.ti_noise";
   cdesc.contents       = "BB 12 AA 13";
   cdesc.recordID       = 3;
   cdesc.filename       = "demo1_veloc_model.xml";
   adescs<<cdesc;
   ddescs<<cdesc;
   ldescs<<cdesc;

   cdesc.recType        = 2;
   cdesc.recState       = NOSTAT;
   cdesc.subType        = "RA";
   cdesc.label          = "item_5_2";
   cdesc.description    = "demo1_veloc";
   cdesc.recordID       = -1;
   cdesc.filename       = "";
   adescs<<cdesc;
}

QString US_DataModel::expGUIDauc( QString aucfile )
{
   QString expGUID = "00000000-0000-0000-0000-000000000000";
   QString expfnam = aucfile.section( "/", -1, -1 )
                            .section( ".", 0, 1 ) + ".xml";
   QString expfile = aucfile.section( "/", 0, -2 ) + "/" + expfnam;

   QFile file( expfile );

   if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
   {
      QXmlStreamReader xml( &file );

      while( ! xml.atEnd() )
      {
         xml.readNext();

         if ( xml.isStartElement()  &&  xml.name() == "experiment" )
         {
            QXmlStreamAttributes a = xml.attributes();
            expGUID  = a.value( "guid" ).toString();
            break;
         }
      }

      file.close();
   }

   return expGUID;
}

