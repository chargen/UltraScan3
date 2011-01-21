//! \file us_solution_gui.cpp

#include <QtGui>

#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_passwd.h"
#include "us_db2.h"
#include "us_investigator.h"
#include "us_buffer_gui.h"
#include "us_analyte_gui.h"
#include "us_solution.h"
#include "us_solution_gui.h"

US_SolutionGui::US_SolutionGui( 
      int   expID,
      int   chID,
      bool  signal_wanted,
      int   select_db_disk,
      const US_Solution& dataIn 
      ) : US_WidgetsDialog( 0, 0 ), experimentID( expID ), channelID( chID ),
        signal( signal_wanted ), solution( dataIn )
{
   investigatorID = US_Settings::us_inv_ID();

   setWindowTitle( tr( "Solution Management" ) );
   setPalette( US_GuiSettings::frameColor() );

   // Very light gray, for read-only line edits
   QPalette gray = US_GuiSettings::editColor();
   gray.setColor( QPalette::Base, QColor( 0xe0, 0xe0, 0xe0 ) );

   QGridLayout* main      = new QGridLayout( this );
   main->setSpacing         ( 2 );
   main->setContentsMargins ( 2, 2, 2, 2 );

   QFontMetrics fm( QFont( US_GuiSettings::fontFamily(),
                           US_GuiSettings::fontSize() ) );

   int row = 0;

   QStringList DB = US_Settings::defaultDB();
   if ( DB.isEmpty() ) DB << "Undefined";
   QLabel* lb_DB = us_banner( tr( "Database: " ) + DB.at( 0 ) );
   lb_DB->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_DB, row++, 0, 1, 3 );

   // First column
   QPushButton* pb_investigator = us_pushbutton( tr( "Select Investigator" ) );
   connect( pb_investigator, SIGNAL( clicked() ), SLOT( sel_investigator() ) );
   main->addWidget( pb_investigator, row++, 0 );

   if ( US_Settings::us_inv_level() < 1 )
      pb_investigator->setEnabled( false );

   // Available solutions
   QLabel* lb_banner2 = us_banner( tr( "Click on solution to select" ), -2  );
   lb_banner2->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   lb_banner2->setMinimumWidth( 400 );
   main->addWidget( lb_banner2, row++, 0 );

   lw_solutions = us_listwidget();
   lw_solutions-> setSortingEnabled( true );
   connect( lw_solutions, SIGNAL( itemClicked    ( QListWidgetItem* ) ),
                          SLOT  ( selectSolution ( QListWidgetItem* ) ) );
   main->addWidget( lw_solutions, row, 0, 7, 1 );

   row += 7;

   QHBoxLayout* lo_amount = new QHBoxLayout();

   lb_amount = us_label( tr( "Analyte Molar Ratio:" ) );
   lo_amount->addWidget( lb_amount );

   ct_amount = us_counter ( 2, 0, 100, 1 ); // #buttons, low, high, start_value
   ct_amount->setStep( 1 );
   ct_amount->setFont( QFont( US_GuiSettings::fontFamily(),
                              US_GuiSettings::fontSize() ) );
   lo_amount->addWidget( ct_amount );
   main->addLayout( lo_amount, row++, 0 );

   QLabel* lb_banner3 = us_banner( tr( "Current solution contents" ), -2  );
   lb_banner3->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_banner3, row++, 0 );

   lw_analytes = us_listwidget();
   lw_analytes-> setSortingEnabled( true );
   connect( lw_analytes, SIGNAL( itemClicked  ( QListWidgetItem* ) ),
                         SLOT  ( selectAnalyte( QListWidgetItem* ) ) );

   int add_rows = ( US_Settings::us_debug() == 0 ) ? 6 : 8;

   main->addWidget( lw_analytes, row, 0, add_rows, 1 );

   row += add_rows;

   // Second column
   row = 1;

   le_investigator = us_lineedit( tr( "Not Selected" ) );
   le_investigator->setReadOnly( true );
   main->addWidget( le_investigator, row++, 1, 1, 2 );

   disk_controls = new US_Disk_DB_Controls( select_db_disk );
   connect( disk_controls, SIGNAL( changed       ( bool ) ),
                           SLOT  ( source_changed( bool ) ) );
   main->addLayout( disk_controls, row++, 1, 1, 2 );

   pb_query = us_pushbutton( tr( "Query Solutions" ), true );
   connect( pb_query, SIGNAL( clicked() ), SLOT( load() ) );
   main->addWidget( pb_query, row, 1 );

   pb_save = us_pushbutton( tr( "Save Solution" ), false );
   connect( pb_save, SIGNAL( clicked() ), SLOT( save() ) );
   main->addWidget( pb_save, row++, 2 );

   pb_addAnalyte = us_pushbutton( tr( "Add Analyte" ), true );
   connect( pb_addAnalyte, SIGNAL( clicked() ), SLOT( addAnalyte() ) );
   main->addWidget( pb_addAnalyte, row, 1 );

   pb_del = us_pushbutton( tr( "Delete Solution" ), false );
   connect( pb_del, SIGNAL( clicked() ), SLOT( delete_solution() ) );
   main->addWidget( pb_del, row++, 2 );

   pb_removeAnalyte = us_pushbutton( tr( "Remove Analyte" ), false );
   connect( pb_removeAnalyte, SIGNAL( clicked() ), SLOT( removeAnalyte() ) );
   main->addWidget( pb_removeAnalyte, row, 1 );

   pb_buffer = us_pushbutton( tr( "Select Buffer" ), true );
   connect( pb_buffer, SIGNAL( clicked() ), SLOT( selectBuffer() ) );
   main->addWidget( pb_buffer, row++, 2 );

   le_bufferInfo = us_lineedit( "", 1 );
   le_bufferInfo ->setPalette ( gray );
   le_bufferInfo ->setReadOnly( true );
   main->addWidget( le_bufferInfo, row++, 1, 1, 2 );

   QLabel* lb_banner4 = us_banner( tr( "Edit solution properties" ), -2  );
   lb_banner4->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_banner4, row++, 1, 1, 2 );

   QLabel* lb_solutionDesc = us_label( tr( "Solution Name:" ) );
   main->addWidget( lb_solutionDesc, row++, 1, 1, 2 );

   le_solutionDesc = us_lineedit( "", 1 );
   connect( le_solutionDesc, SIGNAL( textEdited      ( const QString&   ) ),
                             SLOT  ( saveDescription ( const QString&   ) ) );
   main->addWidget( le_solutionDesc, row++, 1, 1, 2 );

   QLabel* lb_commonVbar20 = us_label( tr( "Common VBar (20C):" ) );
   main->addWidget( lb_commonVbar20, row, 1 );

   le_commonVbar20 = us_lineedit( "", 1 );
   connect( le_commonVbar20, SIGNAL( textEdited       ( const QString&   ) ),
                             SLOT  ( saveCommonVbar20 ( const QString&   ) ) );
   main->addWidget( le_commonVbar20, row++, 2 );

   QLabel* lb_storageTemp = us_label( tr( "Storage Temperature:" ) );
   main->addWidget( lb_storageTemp, row, 1 );

   le_storageTemp = us_lineedit( "", 1 );
   connect( le_storageTemp, SIGNAL( textEdited      ( const QString&   ) ),
                            SLOT  ( saveTemperature ( const QString&   ) ) );
   main->addWidget( le_storageTemp, row++, 2 );

   QLabel* lb_notes = us_label( tr( "Solution notes:" ) );
   main->addWidget( lb_notes, row++, 1, 1, 2 );

   te_notes = us_textedit();
   connect( te_notes, SIGNAL( textChanged( void ) ),
                      SLOT  ( saveNotes  ( void ) ) );
   main->addWidget( te_notes, row, 1, 5, 2 );
   te_notes->setMinimumHeight( 200 );
   te_notes->setReadOnly( false );
   row += 5;

   QLabel* lb_guid = us_label( tr( "Global Identifier:" ) );
   main->addWidget( lb_guid, row++, 1, 1, 2 );

   le_guid = us_lineedit( "" ); 
   le_guid->setPalette ( gray );
   le_guid->setReadOnly( true );
   main->addWidget( le_guid, row++, 1, 1, 2 );
 
   if ( US_Settings::us_debug() == 0 )
   {
      lb_guid->setVisible( false );
      le_guid->setVisible( false );
   }

   // Some pushbuttons
   QHBoxLayout* buttons = new QHBoxLayout;

   QPushButton* pb_reset = us_pushbutton( tr( "Reset" ) );
   connect( pb_reset, SIGNAL( clicked() ), SLOT( newSolution() ) );
   buttons->addWidget( pb_reset );

   QPushButton* pb_help = us_pushbutton( tr( "Help" ) );
   connect( pb_help, SIGNAL( clicked() ), SLOT( help() ) );
   buttons->addWidget( pb_help );

   pb_accept = us_pushbutton( tr( "Close" ) );

   if ( signal )
   {
      QPushButton* pb_cancel = us_pushbutton( tr( "Cancel" ) );
      connect( pb_cancel, SIGNAL( clicked() ), SLOT( cancel() ) );
      buttons->addWidget( pb_cancel );

      pb_accept -> setText( tr( "Accept" ) );
   }

   connect( pb_accept, SIGNAL( clicked() ), SLOT( accept() ) );
   buttons->addWidget( pb_accept );
   
   // Now let's assemble the page
   
   main->addLayout( buttons, row, 0, 1, 3 );
   
   reset();

   // Load the solution descriptions
   load();

   // Select the current one if we know what it is
   if ( solution.solutionID > 0 )
   {
      QList< QListWidgetItem* > items 
        = lw_solutions->findItems( solution.solutionDesc, Qt::MatchExactly );

      // should be exactly 1, but let's make sure
      if ( items.size() == 1 )
      {
         selectSolution( items[ 0 ] );
         lw_solutions->setCurrentItem( items[ 0 ] );
      }
   }
}

// Function to refresh the display with values from the solution structure,
//  and to enable/disable features
void US_SolutionGui::reset( void )
{
   QList< US_Solution::AnalyteInfo >&   analytes   = solution.analytes;
   QString                              bufferDesc = solution.bufferDesc;

   le_bufferInfo   -> setText( solution.bufferDesc   );
   le_solutionDesc -> setText( solution.solutionDesc );
   le_commonVbar20 -> setText( QString::number( solution.commonVbar20 ) );
   le_storageTemp  -> setText( QString::number( solution.storageTemp  ) );
   te_notes        -> setText( solution.notes        );
   le_guid         -> setText( solution.solutionGUID );
   ct_amount       -> disconnect();
   ct_amount       -> setEnabled( false );
   ct_amount       -> setValue( 1 );

   pb_buffer       -> setEnabled( true );

   pb_addAnalyte   -> setEnabled( true );
   pb_removeAnalyte-> setEnabled( false );

   // Let's calculate if we're eligible to save this solution
   pb_save         -> setEnabled( false );
   if ( analytes.size() > 0    &&
        ! bufferDesc.isEmpty() )
   {
      pb_save      -> setEnabled( true );
   }

   // We can always delete something, even if it's just what's in the dialog
   pb_del          -> setEnabled( false );
   if ( lw_solutions->currentRow() != -1 )
   {
      pb_del       -> setEnabled( true );
   }

   // Display analytes that have been selected
   lw_analytes->clear();
   analyteMap.clear();
   for ( int i = 0; i < analytes.size(); i++ )
   {
      // Create a map to account for sorting of the list
      QListWidgetItem* item = new QListWidgetItem( analytes[ i ].analyteDesc, lw_analytes );
      analyteMap[ item ] = i;

      lw_analytes->addItem( item );
   }

   // Turn the red label back
   QPalette p = lb_amount->palette();
   p.setColor( QPalette::WindowText, Qt::white );
   lb_amount->setPalette( p );

   // Figure out if the accept button should be enabled
   if ( ! signal )      // Then it's just a close button
      pb_accept->setEnabled( true );

   else if ( solution.saveStatus == US_Solution::BOTH )
      pb_accept->setEnabled( true );

   else if ( ( ! disk_controls->db() ) && solution.saveStatus == US_Solution::HD_ONLY )
      pb_accept->setEnabled( true );

   else if ( (   disk_controls->db() ) && solution.saveStatus == US_Solution::DB_ONLY )
      pb_accept->setEnabled( true );

   else
      pb_accept->setEnabled( false );

   // Display investigator
   if ( investigatorID == 0 )
      investigatorID = US_Settings::us_inv_ID();
   if ( investigatorID > 0 )
      le_investigator->setText( QString::number( investigatorID ) + ": " + US_Settings::us_inv_name() );
   else
      le_investigator->setText( "Not Selected" );
}

// Function to accept the current set of solutions and return
void US_SolutionGui::accept( void )
{
   if ( signal )
   {
      if ( le_solutionDesc->text().isEmpty() )
      {
         QMessageBox::information( this,
               tr( "Attention" ),
               tr( "Please enter a description for\n"
                   "your solution before accepting." ) );
         return;
      }
   
      if ( le_storageTemp->text().isEmpty() )
         solution.storageTemp = 0;
   
      emit updateSolutionGuiSelection( solution );
   }

   close();
}

// Function to cancel the current dialog and return
void US_SolutionGui::cancel( void )
{
   if ( signal )
      emit cancelSolutionGuiSelection();

   close();
}

// Function to select the current investigator
void US_SolutionGui::sel_investigator( void )
{
   US_Investigator* inv_dialog = new US_Investigator( true, investigatorID );

   connect( inv_dialog,
      SIGNAL( investigator_accepted( int, const QString&, const QString& ) ),
      SLOT  ( assign_investigator  ( int, const QString&, const QString& ) ) );

   inv_dialog->exec();
}

// Function to assign the selected investigator as current
void US_SolutionGui::assign_investigator( int invID,
      const QString& lname, const QString& fname)
{
   investigatorID = invID;
   le_investigator->setText( QString::number( invID ) + ": " +
         lname + ", " + fname );
}

// Function to load solutions into solutions list widget
void US_SolutionGui::load( void )
{
   if ( disk_controls->db() )
      loadDB();

   else
      loadDisk();

}

// Function to load solutions from disk
void US_SolutionGui::loadDisk( void )
{
   QString path;
   if ( ! solution.diskPath( path ) ) return;

   IDs.clear();
   descriptions.clear();
   GUIDs.clear();
   filenames.clear();

   QDir dir( path );
   QStringList filter( "S*.xml" );
   QStringList names = dir.entryList( filter, QDir::Files, QDir::Name );

   QFile a_file;

   for ( int i = 0; i < names.size(); i++ )
   {
      a_file.setFileName( path + "/" + names[ i ] );

      if ( ! a_file.open( QIODevice::ReadOnly | QIODevice::Text) ) continue;

      QXmlStreamReader xml( &a_file );

      while ( ! xml.atEnd() )
      {
         xml.readNext();

         if ( xml.isStartElement() )
         {
            if ( xml.name() == "solution" )
            {
               QXmlStreamAttributes a = xml.attributes();

               IDs          << a.value( "id" ).toString();
               GUIDs        << a.value( "guid" ).toString();
               filenames    << path + "/" + names[ i ];

            }

            else if ( xml.name() == "description" )
            {
               xml.readNext();
               descriptions << xml.text().toString();
            }
         }
      }

      a_file.close();
   }

   loadSolutions();
}

// Function to load solutions from db
void US_SolutionGui::loadDB( void )
{
   US_Passwd pw;
   QString masterPW = pw.getPasswd();
   US_DB2 db( masterPW );

   if ( db.lastErrno() != US_DB2::OK )
   {
      db_error( db.lastError() );
      return;
   }

   if ( investigatorID < 1 ) investigatorID = US_Settings::us_inv_ID();

   QStringList q( "all_solutionIDs" );
   q << QString::number( investigatorID );
   db.query( q );

   if ( db.lastErrno() != US_DB2::OK ) return;

   IDs.clear();
   descriptions.clear();
   GUIDs.clear();
   filenames.clear();

   while ( db.next() )
   {
      QString newID = db.value( 0 ).toString();
      IDs          << newID;
      descriptions << db.value( 1 ).toString();
      GUIDs        << QString( "" );
      filenames    << QString( "" );
   }

   loadSolutions();
}

// Function to load the solutions list widget from the solutions data structure
void US_SolutionGui::loadSolutions( void )
{
   lw_solutions->clear();
   info.clear();
   solutionMap.clear();

   for ( int i = 0; i < descriptions.size(); i++ )
   {
      SolutionInfo si;
      si.solutionID  = IDs         [ i ].toInt();
      si.description = descriptions[ i ];
      si.GUID        = GUIDs       [ i ];
      si.filename    = filenames   [ i ];
      si.index       = i;
      info << si;

      // Create a map to account for automatic sorting of the list
      QListWidgetItem* item = new QListWidgetItem( descriptions[ i ], lw_solutions );
      solutionMap[ item ] = i;

      lw_solutions->addItem( item );
   }

}

// Function to handle when analyte listwidget item is selected
void US_SolutionGui::selectSolution( QListWidgetItem* item )
{
   // Account for the fact that the list has been sorted
   int     ndx          = solutionMap[ item ];
   int     solutionID   = info[ ndx ].solutionID;
   QString solutionGUID = info[ ndx ].GUID;

   solution.clear();

   if ( disk_controls->db() )
   {
      US_Passwd pw;
      QString masterPW = pw.getPasswd();
      US_DB2 db( masterPW );
   
      if ( db.lastErrno() != US_DB2::OK )
      {
         db_error( db.lastError() );
         return;
      }

      solution.readFromDB  ( solutionID, &db );
      solution.saveStatus = US_Solution::DB_ONLY;
   }

   else
   {
      solution.readFromDisk( solutionGUID );
      solution.saveStatus = US_Solution::HD_ONLY;
   }

   reset();
}

// Function to add analyte to solution
void US_SolutionGui::addAnalyte( void )
{
   int dbdisk = ( disk_controls->db() ) ? US_Disk_DB_Controls::DB
                                        : US_Disk_DB_Controls::Disk;

   US_AnalyteGui* analyte_dialog = new US_AnalyteGui( true, QString(), dbdisk );

   connect( analyte_dialog, SIGNAL( valueChanged  ( US_Analyte ) ),
            this,           SLOT  ( assignAnalyte ( US_Analyte ) ) );

   connect( analyte_dialog, SIGNAL( use_db        ( bool ) ), 
                            SLOT  ( update_disk_db( bool ) ) );

   analyte_dialog->exec();
   qApp->processEvents();

}

// Get information about selected analyte
void US_SolutionGui::assignAnalyte( US_Analyte data )
{
   US_Solution::AnalyteInfo currentAnalyte;
   currentAnalyte.analyteGUID = data.analyteGUID;
   currentAnalyte.analyteDesc = data.description;
   currentAnalyte.analyteID   = data.analyteID.toInt();   // May not be accurate
   currentAnalyte.vbar20      = data.vbar20;
   currentAnalyte.mw          = data.mw;

   // Now get analyteID from db if we can
   US_Passwd pw;
   QString masterPW = pw.getPasswd();
   US_DB2 db( masterPW );

   if ( db.lastErrno() == US_DB2::OK )
   {
      QStringList q( "get_analyteID" );
      q << currentAnalyte.analyteGUID;
      db.query( q );
   
      if ( db.next() )
         currentAnalyte.analyteID = db.value( 0 ).toInt();
   }

   // Make sure item has not been added already
   if ( solution.analytes.contains( currentAnalyte ) )
   {
      QMessageBox::information( this,
            tr( "Attention" ),
            tr( "Your solution already contains this analyte\n"
                "If you wish to change the amount, remove it and "
                "add it again.\n" ) );
      return;
   }

   solution.analytes << currentAnalyte;

   calcCommonVbar20();

   // We're maintaining a map to account for automatic sorting of the list
   QListWidgetItem* item = new QListWidgetItem( currentAnalyte.analyteDesc, lw_analytes );
   analyteMap[ item ] = solution.analytes.size() - 1;      // The one we just added

   solution.saveStatus = US_Solution::NOT_SAVED;
   reset();
}

// Function to handle when solution listwidget item is selected
void US_SolutionGui::selectAnalyte( QListWidgetItem* item )
{
   // Get the right index in the sorted list, and load the amount
   int ndx = analyteMap[ item ];
   ct_amount ->setValue( solution.analytes[ ndx ].amount );

   // Now turn the label red to catch attention
   QPalette p = lb_amount->palette();
   p.setColor( QPalette::WindowText, Qt::red );
   lb_amount->setPalette( p );

   pb_removeAnalyte ->setEnabled( true );
   ct_amount        ->setEnabled( true );
   connect( ct_amount, SIGNAL( valueChanged ( double ) ),      // if the user has changed it
                       SLOT  ( saveAmount   ( double ) ) );
}

// Function to add analyte to solution
void US_SolutionGui::removeAnalyte( void )
{
   // Allow for the fact that this listwidget is sorted
   QListWidgetItem* item = lw_analytes->currentItem();
   int ndx = analyteMap[ item ];

   solution.analytes.removeAt( ndx );
   lw_analytes ->removeItemWidget( item );

   calcCommonVbar20();

   solution.saveStatus = US_Solution::NOT_SAVED;
   reset();
}

// Function to calculate the default commonVbar20 value
void US_SolutionGui::calcCommonVbar20( void )
{
   solution.commonVbar20 = 0.0;

   if ( solution.analytes.size() == 1 )
      solution.commonVbar20 = solution.analytes[ 0 ].vbar20;

   else     // multiple analytes
   {
      double numerator   = 0.0;
      double denominator = 0.0;
      foreach ( US_Solution::AnalyteInfo analyte, solution.analytes )
      {
         numerator   += analyte.vbar20 * analyte.mw * analyte.amount;
         denominator += analyte.mw * analyte.amount;
      }

      solution.commonVbar20 = ( denominator == 0 ) ? 0.0 : ( numerator / denominator );

   }

}

// Create a dialog to request a buffer selection
void US_SolutionGui::selectBuffer( void )
{
   int dbdisk = ( disk_controls->db() ) ? US_Disk_DB_Controls::DB
                                        : US_Disk_DB_Controls::Disk;

   US_BufferGui* buffer_dialog = new US_BufferGui( true, US_Buffer(), dbdisk );

   connect( buffer_dialog, SIGNAL( valueChanged ( US_Buffer ) ),
            this,          SLOT  ( assignBuffer ( US_Buffer ) ) );

   connect( buffer_dialog, SIGNAL( use_db        ( bool ) ), 
                           SLOT  ( update_disk_db( bool ) ) );

   buffer_dialog->exec();
   qApp->processEvents();
}

// Get information about selected buffer
void US_SolutionGui::assignBuffer( US_Buffer buffer )
{
   solution.bufferID = buffer.bufferID.toInt();
   solution.bufferGUID = buffer.GUID;
   solution.bufferDesc = buffer.description;

   // Now get the corresponding bufferID, if we can
   US_Passwd pw;
   QString masterPW = pw.getPasswd();
   US_DB2 db( masterPW );

   if ( db.lastErrno() == US_DB2::OK )
   {
      QStringList q( "get_bufferID" );
      q << solution.bufferGUID;
      db.query( q );
   
      if ( db.next() )
         solution.bufferID = db.value( 0 ).toInt();

   }

   solution.saveStatus = US_Solution::NOT_SAVED;
   reset();
}

// Function to update the amount that is associated with an individual analyte
void US_SolutionGui::saveAmount( double amount )
{
   // Get the right index in the sorted list of analytes
   QListWidgetItem* item = lw_analytes->currentItem();

   // if item not selected return

   int ndx = analyteMap[ item ];
   solution.analytes[ ndx ].amount = amount;
   solution.saveStatus = US_Solution::NOT_SAVED;

   calcCommonVbar20();

   // Update commonVbar20 value in GUI
   le_commonVbar20 -> setText( QString::number( solution.commonVbar20 ) );
}

// Function to update the description associated with the current solution
void US_SolutionGui::saveDescription( const QString& )
{
   solution.solutionDesc = le_solutionDesc ->text();
   solution.saveStatus   = US_Solution::NOT_SAVED;
}

// Function to update the common vbar associated with the current solution
void US_SolutionGui::saveCommonVbar20( const QString& )
{
   solution.commonVbar20 = le_commonVbar20 ->text().toDouble();
   solution.saveStatus  = US_Solution::NOT_SAVED;
}

// Function to update the storage temperature associated with the current solution
void US_SolutionGui::saveTemperature( const QString& )
{
   solution.storageTemp = le_storageTemp ->text().toDouble();
   solution.saveStatus  = US_Solution::NOT_SAVED;
}

// Function to update the notes associated with the current solution
void US_SolutionGui::saveNotes( void )
{
   // Let's see if the notes have actually changed
   if ( solution.notes != te_notes->toPlainText() )
   {
      solution.notes        = te_notes        ->toPlainText();
      solution.saveStatus   = US_Solution::NOT_SAVED;
   }
}

// Function to create a new solution
void US_SolutionGui::newSolution( void )
{
   IDs.clear();
   descriptions.clear();
   GUIDs.clear();
   filenames.clear();

   analyteMap.clear();
   solutionMap.clear();

   info.clear();
   solution.clear();

   lw_solutions->clear();
   reset();
}

// Function to save solution information to disk or db
void US_SolutionGui::save( void )
{
   if ( le_solutionDesc->text().isEmpty() )
   {
      QMessageBox::information( this,
            tr( "Attention" ),
            tr( "Please enter a description for\n"
                "your solution before saving it!" ) );
      return;
   }

   if ( le_storageTemp->text().isEmpty() )
      solution.storageTemp = 0;

   if ( disk_controls->db() )
   {
      US_Passwd pw;
      QString masterPW = pw.getPasswd();
      US_DB2 db( masterPW );
   
      if ( db.lastErrno() != US_DB2::OK )
      {
         db_error( db.lastError() );
         return;
      }

      solution.saveToDB( experimentID, channelID, &db );

      solution.saveStatus = ( solution.saveStatus == US_Solution::HD_ONLY ) 
                          ? US_Solution::BOTH : US_Solution::DB_ONLY;
   }

   else
   {
      solution.saveToDisk();

      solution.saveStatus = ( solution.saveStatus == US_Solution::DB_ONLY ) 
                          ? US_Solution::BOTH : US_Solution::HD_ONLY;
   }

   QMessageBox::information( this,
         tr( "Save results" ),
         tr( "Solution saved" ) );
}

// Function to delete a solution from disk, db, or in the current form
void US_SolutionGui::delete_solution( void )
{
   if ( disk_controls->db() )
   {
      US_Passwd pw;
      QString masterPW = pw.getPasswd();
      US_DB2 db( masterPW );
   
      if ( db.lastErrno() != US_DB2::OK )
      {
         db_error( db.lastError() );
         return;
      }

      solution.deleteFromDB( &db );
   }

   else
      solution.deleteFromDisk();

   solution.clear();
   analyteMap.clear();
   load();
   solution.saveStatus = US_Solution::NOT_SAVED;
   reset();

   QMessageBox::information( this,
         tr( "Delete results" ),
         tr( "Solution Deleted" ) );
}

void US_SolutionGui::source_changed( bool db )
{
   QStringList DB = US_Settings::defaultDB();

   if ( db && ( DB.size() < 5 ) )
   {
      QMessageBox::warning( this,
         tr( "Attention" ),
         tr( "There is no default database set." ) );
   }

   emit use_db( db );
   qApp->processEvents();

   // Clear out solution list
   solutionMap.clear();
   lw_solutions->clear();

   load();
   reset();
}

void US_SolutionGui::update_disk_db( bool db )
{
   ( db ) ? disk_controls->set_db() : disk_controls->set_disk();
}

// Function to display an error returned from the database
void US_SolutionGui::db_error( const QString& error )
{
   QMessageBox::warning( this, tr( "Database Problem" ),
         tr( "Database returned the following error: \n" ) + error );
}
