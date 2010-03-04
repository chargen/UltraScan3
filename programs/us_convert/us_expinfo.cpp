//! \file us_expinfo.cpp

#include "us_expinfo.h"
#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_passwd.h"
#include "us_db2.h"
#include "us_investigator.h"

US_ExpInfo::US_ExpInfo( US_Convert::ExperimentInfo& eData ) : US_WidgetsDialog( 0, 0 )
{
   setWindowTitle( tr( "Experiment Information" ) );
   setPalette( US_GuiSettings::frameColor() );

   // Everything will be in the main layout
   QGridLayout* main = new QGridLayout( this );
   main->setSpacing         ( 2 );
   main->setContentsMargins ( 2, 2, 2, 2 );

   int row = 0;

   // Database choices
   QStringList DB = US_Settings::defaultDB();
   QLabel* lb_DB = us_banner( tr( "Database: " ) + DB.at( 0 ) );
   main->addWidget( lb_DB, row++, 0, 1, 2 );

   QPushButton* pb_investigator = us_pushbutton( tr( "Select Investigator" ) );
   connect( pb_investigator, SIGNAL( clicked() ), SLOT( selectInvestigator() ) );
   pb_investigator->setEnabled( true );
   main->addWidget( pb_investigator, row, 0 );

   le_investigator = us_lineedit( "", 1 );
   le_investigator->setReadOnly( true );
   main->addWidget( le_investigator, row++, 1 );

   // Hardware choices
   QLabel* lb_hardware = us_banner( tr( "Hardware: " ) );
   main->addWidget( lb_hardware, row++, 0, 1, 2 );

   QLabel* lb_experiment =  us_label( tr( "Experiment:" ) );
   main->addWidget( lb_experiment, row++, 0, 1, 2 );

   QLabel* lb_lab = us_label( tr( "Lab:" ) );
   main->addWidget( lb_lab, row++, 0, 1, 2 );

   QLabel* lb_instrument = us_label( tr( "Instrument:" ) );
   main->addWidget( lb_instrument, row++, 0, 1, 2 );

   QLabel* lb_operator = us_label( tr( "Operator:" ) );
   main->addWidget( lb_operator, row++, 0, 1, 2 );

   // Set up rotor drop-down
   QLabel* lb_rotor = us_label( tr( "Rotor:" ) );
   main->addWidget( lb_rotor, row, 0 );

   rotorInfo();
   cb_rotor = us_comboBox();
   cb_rotor-> addItems( rotorTypes );
   main->addWidget( cb_rotor, row++, 1 );

   QLabel* lb_experiment_banner = us_banner( tr( "Experiment: " ) );
   main->addWidget( lb_experiment_banner, row++, 0, 1, 2 );

   // Set up experiment type drop-down
   QLabel* lb_expType = us_label( tr( "Experiment Type:" ) );
   main->addWidget( lb_expType, row, 0 );

   cb_expType = us_comboBox();
   QStringList experimentTypes;
   experimentTypes << "Velocity"
                   << "Equilibrium"
                   << "other";

   cb_expType->addItems( experimentTypes );
   main->addWidget( cb_expType, row++, 1 );

   QLabel* lb_label = us_label( tr( "Label:" ) );
   main->addWidget( lb_label, row++, 0, 1, 2 );

   le_label = us_lineedit();
   main->addWidget( le_label, row++, 0, 1, 2 );

   QLabel* lb_comment = us_label( tr( "Comments:" ) );
   main->addWidget( lb_comment, row++, 0, 1, 2 );

   te_comment = us_textedit();
   main->addWidget( te_comment, row, 0, 4, 2 );
   te_comment->setMinimumWidth( 320 );
   te_comment->setReadOnly( false );
   row += 4;

   // Some pushbuttons
   QHBoxLayout* buttons = new QHBoxLayout;

   QPushButton* pb_help = us_pushbutton( tr( "Help" ) );
   connect( pb_help, SIGNAL( clicked() ), SLOT( help() ) );
   buttons->addWidget( pb_help );

   QPushButton* pb_accept = us_pushbutton( tr( "Accept" ) );
   connect( pb_accept, SIGNAL( clicked() ), SLOT( accept() ) );
   buttons->addWidget( pb_accept );

   QPushButton* pb_cancel = us_pushbutton( tr( "Cancel" ) );
   connect( pb_cancel, SIGNAL( clicked() ), SLOT( cancel() ) );
   buttons->addWidget( pb_cancel );

   // Show previous values, if any
   if ( eData.invID > 0 )
   {
      le_investigator->setText( "InvID (" + QString::number( eData.invID ) + "): " +
               eData.lastName + ", " + eData.firstName );
      for ( uint i = 0; i < sizeof( experimentTypes); i++ )
      {
         if ( experimentTypes[ i ] == eData.expType )
         {
            cb_expType->setCurrentIndex( i );
            break;
         }
      }
      cb_rotor       ->setCurrentIndex( eData.rotor      );
      le_label       ->setText        ( eData.label      );
      te_comment     ->setText        ( eData.comments   );
   }

   main->addLayout( buttons, row++, 0, 1, 2 );
}

bool US_ExpInfo::rotorInfo( void )
{
   // Connect to the db
   US_Passwd pw;
   QString masterPW = pw.getPasswd();
   US_DB2 db( masterPW );

   QStringList q( "get_rotor_names" );
   db.query( q );

   rotorTypes.clear();
   while ( db.next() )
      rotorTypes << db.value( 1 ).toString();

   return true;
}

void US_ExpInfo::reset( void )
{
}

void US_ExpInfo::accept( void )
{
   US_Convert::ExperimentInfo eData;

   // First get the invID
   QString invInfo = le_investigator->text();
   if ( invInfo.isEmpty() )
   {
      QMessageBox::information( this,
                tr( "Error" ),
                tr( "You must choose an investigator before accepting" ) );
      return;
   }

   QStringList components = invInfo.split( ")", QString::SkipEmptyParts );
   components = components[0].split( "(", QString::SkipEmptyParts );
   eData.invID = components.last().toInt();
 
   // Get the investigator name too
   components  = invInfo.split( ": ", QString::SkipEmptyParts );
   components  = components.last().split( ", ", QString::SkipEmptyParts );
   eData.lastName  = components.first().toAscii();
   eData.firstName = components.last().toAscii();

   eData.expType      = cb_expType       ->currentText();
   eData.rotor        = cb_rotor         ->currentIndex();

   QDateTime time     = QDateTime::currentDateTime();
   eData.date         = time.toString( "yyyy-MM-dd" );
   eData.label        = le_label         ->text();
   eData.comments     = te_comment       ->toPlainText();

   emit updateExpInfoSelection( eData );
   close();
}

void US_ExpInfo::cancel( void )
{
   reset();

   emit cancelExpInfoSelection();
   close();
}

void US_ExpInfo::selectInvestigator( void )
{
   US_Investigator* inv_dialog = new US_Investigator( true );
   connect( inv_dialog, 
      SIGNAL( investigator_accepted( int, const QString&, const QString& ) ),
      SLOT  ( assignInvestigator   ( int, const QString&, const QString& ) ) );
   inv_dialog->exec();
}

void US_ExpInfo::assignInvestigator( int invID,
      const QString& lname, const QString& fname)
{
   le_investigator->setText( "InvID (" + QString::number( invID ) + "): " +
         lname + ", " + fname );
}


