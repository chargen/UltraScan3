//! \file us_analyte_gui.cpp
#include "us_analyte_gui.h"
#include "us_passwd.h"
#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_investigator.h"
#include "us_editor_gui.h"
#include "us_table.h"
#include "us_util.h"

US_AnalyteGui::US_AnalyteGui( bool            signal, 
                              const QString&  GUID,
                              int             access,
                              double          temp )
   : US_WidgetsDialog( 0, 0 ), 
     signal_wanted( signal ),
     guid         ( GUID ), 
     temperature  ( temp )
{
   setWindowTitle( tr( "Analyte Management" ) );
   setPalette( US_GuiSettings::frameColor() );
   setAttribute( Qt::WA_DeleteOnClose );

   personID      = US_Settings::us_inv_ID();
   analyte       = US_Analyte();
   saved_analyte = analyte;

   QPalette normal = US_GuiSettings::editColor();

   QGridLayout* main = new QGridLayout( this );
   main->setSpacing         ( 2 );
   main->setContentsMargins ( 2, 2, 2, 2 );

   int row = 0;

   QStringList DB = US_Settings::defaultDB();
   QString     db_name;

   if ( DB.size() < 5 )
      db_name = "No Default Set";
   else
      db_name = DB.at( 0 );

   QLabel* lb_DB = us_banner( tr( "Database: " ) + db_name, -1 );
   lb_DB->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_DB, row++, 0, 1, 3 );

   QGridLayout* protein = us_radiobutton( tr( "Protein"            ),
         rb_protein, true );
   QGridLayout* dna     = us_radiobutton( tr( "DNA"                ), rb_dna );
   QGridLayout* rna     = us_radiobutton( tr( "RNA"                ), rb_rna );
   QGridLayout* carb    = us_radiobutton( tr( "Carbohydrate/Other" ), rb_carb );

   QHBoxLayout* radios = new QHBoxLayout;
   radios->addLayout( protein );
   radios->addLayout( dna     );
   radios->addLayout( rna     );
   radios->addLayout( carb    );

   main->addLayout( radios, row++, 0, 1, 3 );

   QButtonGroup* typeButtons = new QButtonGroup( this );
   typeButtons->addButton( rb_protein, US_Analyte::PROTEIN );
   typeButtons->addButton( rb_dna    , US_Analyte::DNA );
   typeButtons->addButton( rb_rna    , US_Analyte::RNA );
   typeButtons->addButton( rb_carb   , US_Analyte::CARBOHYDRATE );
   connect( typeButtons, SIGNAL( buttonClicked    ( int ) ),
                         SLOT  ( set_analyte_type ( int ) ) );

   QPushButton* pb_investigator = us_pushbutton( tr( "Select Investigator" ) );
   connect( pb_investigator, SIGNAL( clicked() ), SLOT( sel_investigator() ) );
   main->addWidget( pb_investigator, row, 0 );

   QString investigator = QString::number( US_Settings::us_inv_ID() ) + ": " +
                          US_Settings::us_inv_name();

   if ( US_Settings::us_inv_level() < 1 ) 
      pb_investigator->setEnabled( false );

   le_investigator = us_lineedit( investigator, 0, true );
   main->addWidget( le_investigator, row++, 1, 1, 2 );

   QBoxLayout* search = new QHBoxLayout;

   // Search
   QLabel* lb_search = us_label( tr( "Search:" ) );
   search->addWidget( lb_search );

   le_search = us_lineedit();
   le_search->setReadOnly( true );
   connect( le_search, SIGNAL( textChanged( const QString& ) ),
                       SLOT  ( search     ( const QString& ) ) );
   search->addWidget( le_search );
   main->addLayout( search, row++, 0 );

   // Analyte descriptions from DB
   QLabel* lb_banner1 = us_banner( tr( "Doubleclick on analyte data to select" ), -1 );
   lb_banner1->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_banner1, row++, 0 );

   lw_analytes = us_listwidget();
   connect( lw_analytes, SIGNAL( itemDoubleClicked( QListWidgetItem* ) ),
                         SLOT  ( select_analyte   ( QListWidgetItem* ) ) );

   main->addWidget( lw_analytes, row, 0, 6, 1 );
   row += 6;

   // Labels
   QGridLayout* description = new QGridLayout;
   QLabel* lb_description = us_label( tr( "Analyte Description:" ) );
   description->addWidget( lb_description, 0, 0 );

   le_description = us_lineedit( "" ); 
   connect( le_description, SIGNAL( editingFinished   () ), 
                            SLOT  ( change_description() ) );
   description->addWidget( le_description, 0, 1 );
 
   QLabel* lb_guid = us_label( tr( "Global Identifier:" ) );
   description->addWidget( lb_guid, 1, 0 );

   le_guid = us_lineedit( "", 0, true ); 
   description->addWidget( le_guid, 1, 1 );
   main->addLayout( description, row, 0, 2, 3 );
 
   if ( US_Settings::us_debug() == 0 )
   {
      lb_guid->setVisible( false );
      le_guid->setVisible( false );
   }

   // Go back to top of 2nd column
   row = 3;
   disk_controls = new US_Disk_DB_Controls( access );
   connect( disk_controls, SIGNAL( changed       ( bool ) ),
                           SLOT  ( source_changed( bool ) ) );
   main->addLayout( disk_controls, row++, 1, 1, 2 );

   QLabel* lb_banner3 = us_banner( tr( "Database/Disk Functions" ), -2 );
   lb_banner3->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_banner3, row++, 1, 1, 2 );

   QPushButton* pb_list = us_pushbutton( 
         tr( "List Descriptions" ) );
   connect( pb_list, SIGNAL( clicked() ), SLOT( list() ) );
   main->addWidget( pb_list, row, 1 );

   QPushButton* pb_new = us_pushbutton( tr( "New Analyte" ) );
   connect( pb_new, SIGNAL( clicked() ), SLOT( new_analyte() ) );
   main->addWidget( pb_new, row++, 2 );

   pb_save = us_pushbutton( tr( "Save Analyte" ), false );
   connect( pb_save, SIGNAL( clicked() ), SLOT( save() ) );
   main->addWidget( pb_save, row, 1 );

   pb_delete = us_pushbutton( tr( "Delete Analyte" ), false );
   connect( pb_delete, SIGNAL( clicked() ), SLOT( delete_analyte() ) );
   main->addWidget( pb_delete, row++, 2 );

   QLabel* lb_banner4 = us_banner( tr( "Other Functions" ), -2 );
   lb_banner4->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
   main->addWidget( lb_banner4, row++, 1, 1, 2 );

   pb_sequence = us_pushbutton( tr( "Manage Sequence" ) );
   connect( pb_sequence, SIGNAL( clicked() ), SLOT( manage_sequence() ) );
   main->addWidget( pb_sequence, row, 1 );

   pb_spectrum = us_pushbutton( tr( "Manage Spectrum" ) );
   connect( pb_spectrum, SIGNAL( clicked() ), SLOT( spectrum() ) );
   main->addWidget( pb_spectrum, row++, 2 );

   pb_more = us_pushbutton( tr( "More Info" ), false );
   connect( pb_more, SIGNAL( clicked() ), SLOT( more_info() ) );
   main->addWidget( pb_more, row, 1 );

   cmb_optics = us_comboBox();
   cmb_optics->addItem( tr( "Absorbance"   ) );
   cmb_optics->addItem( tr( "Interference" ) );
   cmb_optics->addItem( tr( "Fluorescence" ) );
   main->addWidget( cmb_optics, row++, 2 );
   QSpacerItem* spacer0 = new QSpacerItem( 20, 9 );
   main->addItem  ( spacer0,    row++, 2 );

   row +=2; 

   // Lower half -- protein operations and data
   protein_widget = new QWidget( this );
   
   QGridLayout* protein_info   = new QGridLayout( protein_widget );
   protein_info->setSpacing        ( 2 );
   protein_info->setContentsMargins( 2, 2, 2, 2 );

   int prow = 0;

   QLabel* lb_protein_mw = us_label( tr( "MW <small>(Daltons)</small>:" ) );
   protein_info->addWidget( lb_protein_mw, prow, 0 );

   le_protein_mw = us_lineedit( "", 0, false );
   protein_info->addWidget( le_protein_mw, prow, 1 );

   QLabel* lb_protein_vbar20 = us_label( 
         tr( "VBar <small>(cm<sup>3</sup>/g at 20" ) + DEGC + ")</small>:" );
   protein_info->addWidget( lb_protein_vbar20, prow, 2 );

   le_protein_vbar20 = us_lineedit( "" );
   connect( le_protein_vbar20, SIGNAL( textChanged  ( const QString& ) ),
                               SLOT  ( value_changed( const QString& ) ) );
   protein_info->addWidget( le_protein_vbar20, prow++, 3 );

   QLabel* lb_protein_temp = us_label( 
         tr( "Temperature <small>(" ) + DEGC + ")</small>:" );
   protein_info->addWidget( lb_protein_temp, prow, 0 );

   le_protein_temp = us_lineedit( QString::number( temperature, 'f', 1 ) );
   
   if ( signal )
   {
      us_setReadOnly( le_protein_temp, true );
   }
   else
   {
      connect( le_protein_temp, SIGNAL( textChanged ( const QString& ) ), 
                                SLOT  ( temp_changed( const QString& ) ) );
   }

   protein_info->addWidget( le_protein_temp, prow, 1 );

   QLabel* lb_protein_vbar = us_label( 
         tr( "VBar <small>(cm<sup>3</sup>/g at T " ) + DEGC + ")</small>:" );

   protein_info->addWidget( lb_protein_vbar, prow, 2 );

   le_protein_vbar = us_lineedit( "", 0, true );
   protein_info->addWidget( le_protein_vbar, prow++, 3 );

   QLabel* lb_protein_residues = us_label( tr( "Residue count:" ) );
   protein_info->addWidget( lb_protein_residues, prow, 0 );

   le_protein_residues = us_lineedit( "", 0, true );
   protein_info->addWidget( le_protein_residues, prow, 1 );
   main->addWidget( protein_widget, row, 0, 1, 3 ); 
   
   QLabel* lb_protein_e280     = us_label(
         tr( "E280 <small>(OD/(mol*cm))</small>:" ) );
   protein_info->addWidget( lb_protein_e280, prow, 2 );
   le_protein_e280 = us_lineedit( "", 0, true );
   protein_info->addWidget( le_protein_e280, prow++, 3 );
   QSpacerItem* spacer1 = new QSpacerItem( 20, 0 );
   protein_info->addItem( spacer1, prow, 0, 1, 4 );
   protein_info->setRowStretch( prow, 100 );
   
   protein_widget->setVisible( true );
  
   // Lower half -- DNA/RNA operations and data
   dna_widget              = new QWidget( this );
   QGridLayout* dna_layout = new QGridLayout( dna_widget );
   dna_layout->setSpacing        ( 2 );
   dna_layout->setContentsMargins( 2, 2, 2, 2 );

   QPalette p = US_GuiSettings::labelColor();

   QGroupBox*    gb_double = new QGroupBox( tr( "Calculate MW" ) );
   gb_double->setPalette( p );
   QGridLayout*  grid1     = new QGridLayout;   
   grid1->setSpacing        ( 2 );
   grid1->setContentsMargins( 2, 2, 2, 2 );
   
   QGridLayout* box1 = us_checkbox( tr( "Double Stranded" ), ck_stranded, true );
   QGridLayout* box2 = us_checkbox( tr( "Complement Only" ), ck_mw_only );
   grid1->addLayout( box1, 0, 0 );
   grid1->addLayout( box2, 1, 0 );
   connect( ck_stranded, SIGNAL( toggled        ( bool ) ), 
                         SLOT  ( update_stranded( bool ) ) );
   connect( ck_mw_only , SIGNAL( toggled        ( bool ) ), 
                         SLOT  ( update_mw_only ( bool ) ) );
   
   QVBoxLayout* stretch1 = new QVBoxLayout;
   stretch1->addStretch();
   grid1->addLayout( stretch1, 2, 0 );

   gb_double->setLayout( grid1 ); 
   dna_layout->addWidget( gb_double, 0, 0 );


   QGroupBox*    gb_three_prime = new QGroupBox( tr( "Three prime" ) );
   gb_three_prime->setPalette( p );
   QGridLayout*  grid2          = new QGridLayout;   
   grid2->setSpacing        ( 2 );
   grid2->setContentsMargins( 2, 2, 2, 2 );

   QGridLayout* box3 = us_radiobutton( tr( "Hydroxyl" ), rb_3_hydroxyl, true );
   QGridLayout* box4 = us_radiobutton( tr( "Phosphate" ), rb_3_phosphate );

   grid2->addLayout( box3, 0, 0 );
   grid2->addLayout( box4, 0, 1 );
   gb_three_prime->setLayout( grid2 ); 
   connect( rb_3_hydroxyl, SIGNAL( toggled          ( bool ) ), 
                           SLOT  ( update_nucleotide( bool ) ) );

   dna_layout->addWidget( gb_three_prime, 1, 0 );

   QGroupBox*    gb_five_prime = new QGroupBox( tr( "Five prime" ) );
   gb_five_prime->setPalette( p );
   QGridLayout*  grid3         = new QGridLayout;   
   grid3->setSpacing        ( 2 );
   grid3->setContentsMargins( 2, 2, 2, 2 );

   QGridLayout* box5 = us_radiobutton( tr( "Hydroxyl" ), rb_5_hydroxyl, true );
   QGridLayout* box6 = us_radiobutton( tr( "Phosphate" ), rb_5_phosphate );

   grid3->addLayout( box5, 0, 0 );
   grid3->addLayout( box6, 0, 1 );
  
   gb_five_prime->setLayout( grid3 ); 
   connect( rb_5_hydroxyl, SIGNAL( toggled          ( bool ) ), 
                           SLOT  ( update_nucleotide( bool ) ) );

   dna_layout->addWidget( gb_five_prime, 2, 0 );

   QGridLayout* ratios = new QGridLayout;

   QLabel* lb_ratios = us_banner( tr( "Counterion molar ratio/nucletide" ) );
   ratios->addWidget( lb_ratios, 0, 0, 1, 3 );

   QLabel* lb_sodium = us_label( tr( "Sodium, Na+" ) );
   ratios->addWidget( lb_sodium, 1, 0 );

   ct_sodium = us_counter( 2, 0.0, 1.0, 0.0 );
   ct_sodium->setSingleStep( 0.01 );
   connect( ct_sodium, SIGNAL( valueChanged     ( double ) ),
                       SLOT  ( update_nucleotide( double ) ) );
   ratios->addWidget( ct_sodium, 1, 1, 1, 2 );

   QLabel* lb_potassium = us_label( tr( "Potassium, K+" ) );
   ratios->addWidget( lb_potassium, 2, 0 );

   ct_potassium = us_counter( 2, 0.0, 1.0, 0.0 );
   ct_potassium->setSingleStep( 0.01 );
   connect( ct_potassium, SIGNAL( valueChanged     ( double ) ),
                          SLOT  ( update_nucleotide( double ) ) );
   ratios->addWidget( ct_potassium, 2, 1, 1, 2 );

   QLabel* lb_lithium = us_label( tr( "Lithium, Li+" ) );
   ratios->addWidget( lb_lithium, 3, 0 );

   ct_lithium = us_counter( 2, 0.0, 1.0, 0.0 );
   ct_lithium->setSingleStep( 0.01 );
   connect( ct_lithium, SIGNAL( valueChanged     ( double ) ),
                        SLOT  ( update_nucleotide( double ) ) );
   ratios->addWidget( ct_lithium, 3, 1, 1, 2 );

   QLabel* lb_magnesium = us_label( tr( "Magnesium, Mg+" ) );
   ratios->addWidget( lb_magnesium, 4, 0 );

   ct_magnesium = us_counter( 2, 0.0, 1.0, 0.0 );
   ct_magnesium->setSingleStep( 0.01 );
   connect( ct_magnesium, SIGNAL( valueChanged     ( double ) ),
                          SLOT  ( update_nucleotide( double ) ) );
   ratios->addWidget( ct_magnesium, 4, 1, 1, 2 );

   QLabel* lb_calcium = us_label( tr( "Calcium, Ca+" ) );
   ratios->addWidget( lb_calcium, 5, 0 );

   ct_calcium = us_counter( 2, 0.0, 1.0, 0.0 );
   ct_calcium->setSingleStep( 0.01 );
   connect( ct_calcium, SIGNAL( valueChanged     ( double ) ),
                        SLOT  ( update_nucleotide( double ) ) );
   ratios->addWidget( ct_calcium, 5, 1 );

   dna_layout->addLayout( ratios, 0, 1, 4, 2 );

   QGridLayout* nucle_data = new QGridLayout;
   QLabel* lb_nucle_mw = us_label( tr( "MW <small>(Daltons)</small>:" ) );
   nucle_data->addWidget( lb_nucle_mw, 0, 0 );

   le_nucle_mw = us_lineedit( "", -2, true );
   nucle_data->addWidget( le_nucle_mw, 0, 1, 1, 3 );

   QLabel* lb_nucle_vbar = us_label( 
         tr( "VBar<small>(cm<sup>3</sup>/g)</small>:" ) );
   nucle_data->addWidget( lb_nucle_vbar, 1, 0 );

   le_nucle_vbar = us_lineedit( "" );
   connect( le_nucle_vbar, SIGNAL( textChanged  ( const QString& ) ),
                           SLOT  ( value_changed( const QString& ) ) );
   nucle_data->addWidget( le_nucle_vbar, 1, 1, 1, 3 );

   dna_layout->addLayout( nucle_data, 4, 0, 2, 3 );
   main->addWidget( dna_widget, row, 0, 2, 3 ); 
   dna_widget->setVisible( false );

   // Lower half -- carbohydrate operations and data
   carbs_widget = new QWidget( this );
   
   QGridLayout* carbs_info   = new QGridLayout( carbs_widget );
   carbs_info->setSpacing        ( 2 );
   carbs_info->setContentsMargins( 2, 2, 2, 2 );

   QLabel* lb_carbs_mw = us_label( tr( "MW <small>(Daltons)</small>:" ) );
   carbs_info->addWidget( lb_carbs_mw,   0, 0 );

   le_carbs_mw = us_lineedit( "" );
   carbs_info->addWidget( le_carbs_mw,   0, 1 );

   QLabel* lb_carbs_vbar = us_label( 
         tr( "VBar<small>(cm<sup>3</sup>/g)</small>:" ) );
   carbs_info->addWidget( lb_carbs_vbar, 0, 2 );

   le_carbs_vbar = us_lineedit( "" );
   carbs_info->addWidget( le_carbs_vbar, 0, 3 );

   QGridLayout* box7 = us_checkbox( tr( "Gradient-Forming" ),
                                    ck_grad_form, false );
   carbs_info->addLayout( box7,          1, 0, 1, 2 );

   main->addWidget( carbs_widget, row, 0, 2, 3 ); 
   carbs_widget->setVisible( false );

   row += 4;

   // Standard Buttons
   QBoxLayout* buttons = new QHBoxLayout;

   QPushButton* pb_reset = us_pushbutton( tr( "Reset" ) );
   connect( pb_reset, SIGNAL( clicked() ), SLOT( reset() ) );
   buttons->addWidget( pb_reset );

   QPushButton* pb_help = us_pushbutton( tr( "Help" ) );
   connect( pb_help, SIGNAL( clicked() ), SLOT( help() ) );
   buttons->addWidget( pb_help );

   QPushButton* pb_accept;
   
   if ( signal_wanted )
   {
      QPushButton* pb_cancel = us_pushbutton( tr( "Cancel" ) );
      connect( pb_cancel, SIGNAL( clicked() ), SLOT( reject() ) );
      buttons->addWidget( pb_cancel );

      pb_accept = us_pushbutton( tr( "Accept" ) );
   }
   else
      pb_accept = us_pushbutton( tr( "Close" ) );

   connect( pb_accept, SIGNAL( clicked() ), SLOT( close() ) );
   buttons->addWidget( pb_accept );

   main->addLayout( buttons, row, 0, 1, 3 );

   if ( guid.size() == 0 )
   {
      reset();
   }

   else
   {
      int result;
      analyte.analyteGUID = guid;

      if ( disk_controls->db() )
      {
         US_Passwd pw;
         US_DB2    db( pw.getPasswd() );

         if ( db.lastErrno() != US_DB2::OK )
         {
            QMessageBox::information( this,
               tr( "Database Error" ),
               tr( "The following error was returned:\n" ) + db.lastError() );
            return; 
         }

         result = analyte.load( true, guid, &db );
      }
      else
         result = analyte.load( false, guid );

      if ( result == US_DB2::OK )
         populate();
      else
      {
         QMessageBox::warning( this,
            tr( "Analyte Missing" ),
            tr( "No Analyte was found " )
            + ( disk_controls->db()
               ? tr( "in the database" )
               : tr( "on local disk" ) )
            + tr( ", with a GUID of\n" ) + guid );
      }

      QString strVBar = QString::number( analyte.vbar20, 'f', 4 );
      le_protein_vbar20->setText( strVBar );
      le_nucle_vbar    ->setText( strVBar );
      le_carbs_vbar    ->setText( strVBar );
      le_carbs_mw      ->setText( QString::number( (int) analyte.mw ) );
   }

   list();

   switch ( analyte.type )
   {
      case US_Analyte::PROTEIN:
         dna_widget    ->setVisible( false );
         carbs_widget  ->setVisible( false );
         protein_widget->setVisible( true );
         resize( 0, 0 ); // Resize to minimum dimensions
         break;

      case US_Analyte::DNA:
      case US_Analyte::RNA:
         update_nucleotide();
         protein_widget->setVisible( false ); 
         carbs_widget  ->setVisible( false );
         dna_widget    ->setVisible( true );
         break;

      case US_Analyte::CARBOHYDRATE:
         protein_widget->setVisible( false ); 
         dna_widget    ->setVisible( false );
         carbs_widget  ->setVisible( true );
         resize( 0, 0 ); // Resize to minimum dimensions
         break;
   }
}

void US_AnalyteGui::source_changed( bool db )
{
   emit use_db( db );
   list();
   qApp->processEvents();
}

void US_AnalyteGui::new_analyte( void )
{
   if ( ! discard_changes() ) return;

   analyte = US_Analyte();  // Set to default
   
   // Protein is default
   if ( rb_rna  ->isChecked() ) analyte.type = US_Analyte::RNA;
   if ( rb_dna  ->isChecked() ) analyte.type = US_Analyte::DNA;
   if ( rb_carb ->isChecked() ) analyte.type = US_Analyte::CARBOHYDRATE;

   if ( ! rb_protein->isChecked() ) analyte.vbar20 = 0.55;  // Empirical value

   saved_analyte = analyte;
   populate();
   if ( analyte.type == US_Analyte::RNA ||
        analyte.type == US_Analyte::DNA ) update_nucleotide();
}

void US_AnalyteGui::check_db( void )
{
   QStringList DB = US_Settings::defaultDB();

   if ( DB.size() < 5 )
   {
      QMessageBox::warning( this,
         tr( "Attention" ),
         tr( "There is no default database set." ) );
   }
   else
   {
      personID = US_Settings::us_inv_ID();

      if ( personID > 0 )
         le_investigator->setText( US_Settings::us_inv_name() );
   }
}

void US_AnalyteGui::value_changed( const QString& )
{
   // This only is activated by changes to vbar20
   // (either protein or dna/rna) but vbar is not saved.
   temp_changed( le_protein_temp->text() );
}

void US_AnalyteGui::change_description( void )
{
   analyte.description = le_description->text();

   int row = -1;

   for ( int ii = 0; ii < descriptions.size(); ii++ )
   {
      if ( analyte.description == descriptions.at( ii ) )
      {
         row = ii;
         break;
      }
   }

   pb_save->setEnabled( row < 0  );

   if ( row < 0 )
   {  // no match to description:  clear GUID, de-select any list item
      le_guid->clear();
      lw_analytes->setCurrentRow( -1 );
   }

   if ( row < 0 ) return;

   le_search->clear();
   search( "" );

   lw_analytes->setCurrentRow( row );
}

void US_AnalyteGui::populate( void )
{
   le_description->setText( analyte.description );
   le_guid       ->setText( analyte.analyteGUID );
qDebug() << "AnG: populate  desc" << analyte.description;
qDebug() << "AnG: populate  type" << analyte.type;

   if ( analyte.type == US_Analyte::PROTEIN )
   {
      rb_protein->setChecked( true );

      US_Math2::Peptide p;
      double temperature = le_protein_temp->text().toDouble();
      US_Math2::calc_vbar( p, analyte.sequence, temperature );

      analyte.mw         = ( analyte.mw == 0 ) ? p.mw : analyte.mw;
      le_protein_mw      ->setText( QString::number( (int) analyte.mw ) );
      le_protein_vbar20  ->setText( QString::number( p.vbar20, 'f', 4 ) );
      le_protein_vbar    ->setText( QString::number( p.vbar  , 'f', 4 ) );
      le_protein_residues->setText( QString::number( p.residues ) );
      le_protein_e280    ->setText( QString::number( p.e280     ) );

      // If spectrum is empty, set to 280.0/e280
      QString spectrum_type = cmb_optics->currentText();

      if ( p.e280 != 0.0 )
      {
         if ( spectrum_type == tr( "Absorbance" ) )
         {
            if ( analyte.extinction.count() == 0 )
               analyte.extinction  [ 280.0 ] = p.e280;
         }
         else if ( spectrum_type == tr( "Interference" ) )
         {
            if ( analyte.refraction.count() == 0 )
               analyte.refraction  [ 280.0 ] = p.e280;
         }
         else
         {
            if ( analyte.fluorescence.count() == 0 )
               analyte.fluorescence[ 280.0 ] = p.e280;
         }
      }
   }

   else if ( analyte.type == US_Analyte::DNA  ||  
             analyte.type == US_Analyte::RNA )
   {
      ( analyte.type == US_Analyte::RNA ) 
           ? rb_rna->setChecked( true ) 
           : rb_dna->setChecked( true );

      inReset = true;
      ck_stranded->setChecked( analyte.doubleStranded );
      ck_mw_only ->setChecked( analyte.complement );

      ( analyte._3prime ) ? rb_3_hydroxyl ->setChecked( true )
                          : rb_3_phosphate->setChecked( true );

      ( analyte._5prime ) ? rb_5_hydroxyl ->setChecked( true ) 
                          : rb_5_phosphate->setChecked( true );
      ct_sodium   ->setValue( analyte.sodium );
      ct_potassium->setValue( analyte.potassium );
      ct_lithium  ->setValue( analyte.lithium );
      ct_magnesium->setValue( analyte.magnesium );
      ct_calcium  ->setValue( analyte.calcium );

      le_nucle_vbar->setText( QString::number( analyte.vbar20, 'f', 4 ) );
      inReset = false;
      update_sequence ( analyte.sequence );
   }

   else if ( analyte.type == US_Analyte::CARBOHYDRATE )  
   {
qDebug() << "AnG: populate  grad_form" << analyte.grad_form;
      rb_carb     ->setChecked( true );
      ck_grad_form->setChecked( analyte.grad_form );
   }
}


void US_AnalyteGui::set_analyte_type( int type )
{
   if ( inReset ) return;

   if ( ! discard_changes() )
   {
      inReset = true;

      switch ( analyte.type )
      {
         case US_Analyte::PROTEIN:
            rb_protein->setChecked( true );
            break;
         case US_Analyte::DNA:
            rb_dna    ->setChecked( true );
            break;
         case US_Analyte::RNA:
            rb_rna    ->setChecked( true );
            break;
         case US_Analyte::CARBOHYDRATE:
            rb_carb   ->setChecked( true );
            break;
         default:
            rb_protein->setChecked( true );
            break;
      }

      inReset = false;
      return;
   }

   switch ( type )
   {
      case US_Analyte::PROTEIN:
         dna_widget    ->setVisible( false );
         carbs_widget  ->setVisible( false );
         protein_widget->setVisible( true );
         resize( 0, 0 ); // Resize to minimum dimensions
         break;

      case US_Analyte::DNA:
      case US_Analyte::RNA:
         protein_widget->setVisible( false ); 
         carbs_widget  ->setVisible( false );
         dna_widget    ->setVisible( true );
         break;

      case US_Analyte::CARBOHYDRATE:
         protein_widget->setVisible( false ); 
         dna_widget    ->setVisible( false );
         carbs_widget  ->setVisible( true );
         resize( 0, 0 ); // Resize to minimum dimensions
         break;
   }

   US_Analyte a;  // Create a blank analyte
   analyte            = a;
   analyte.type       = (US_Analyte::analyte_t) type;
   saved_analyte.type = analyte.type;
   reset();
   list();
}

void US_AnalyteGui::close( void )
{
   bool changed = ! ( analyte == saved_analyte );

   if ( analyte.analyteGUID.size() != 36  ||  changed )
   {
      int response = QMessageBox::question( this,
            tr( "Analyte Changed" ),
            tr( "Are you sure?\n\n" 
                "The displayed analyte has changed.\n"
                "You will not be able to easily reproduce this analyte.\n\n"
                "Do you really want to continue without saving?" ), 
            QMessageBox::Yes | QMessageBox::No );

      if ( response == QMessageBox::No ) return;
   }

   if ( analyte.type == US_Analyte::PROTEIN )
      analyte.vbar20 = le_protein_vbar20->text().toDouble();

   // Give a warning if the vbar implies a negative buoyancy
   if ( ( 1.0 - analyte.vbar20 * DENS_20W ) <= 0.0 )
   {
      QMessageBox::warning( this, tr( "Negative Buoyancy Implied" ),
         tr( "The current vbar value implies a non-positive buoyancy.\n"
             "This will have to be changed with any 2DSA operations.\n\n"
             "Floating data is indicated there by negative sedimentation\n"
             "coefficient values, rather than negative buoyancy." ) );
   }

   // If a signal is not wanted, just close
   if ( ! signal_wanted ) 
   {
      accept();
      return;
   }

   if ( data_ok() )
   {
      emit valueChanged( analyte );
      accept();
   }

   // Just return if data is not OK
}

void US_AnalyteGui::reset( void )
{
   inReset = true;
   lw_analytes->clear();

   analyte.analyteID.clear();

   le_investigator->setText( tr( "Not Selected" ) );

   if ( US_Settings::us_inv_ID() > 0 )
   {
      QString inv = QString::number( US_Settings::us_inv_ID() ) + ": " +
                    US_Settings::us_inv_name();

      le_investigator->setText( inv );
   }

   le_search->clear();
   le_search->setReadOnly( true );

   US_Analyte a;
   analyte = a;

   if ( rb_protein->isChecked() ) analyte.type = US_Analyte::PROTEIN;
   if ( rb_dna    ->isChecked() ) analyte.type = US_Analyte::DNA;
   if ( rb_rna    ->isChecked() ) analyte.type = US_Analyte::RNA;
   if ( rb_carb   ->isChecked() ) analyte.type = US_Analyte::CARBOHYDRATE;

   saved_analyte = analyte;

   filenames   .clear();
   analyteIDs  .clear();
   GUIDs       .clear();
   descriptions.clear();

   le_guid            ->clear();
   le_description     ->clear();
   le_protein_mw      ->clear();
   le_protein_vbar20  ->clear();
   le_protein_vbar    ->clear();
   ck_grad_form       ->setChecked( false );
   
   if ( ! signal_wanted ) le_protein_temp->setText( "20.0" );

   le_protein_residues->clear();

   ct_sodium          ->setValue( 0.0 );
   ct_potassium       ->setValue( 0.0 );
   ct_lithium         ->setValue( 0.0 );
   ct_magnesium       ->setValue( 0.0 );
   ct_calcium         ->setValue( 0.0 );
                      
   ck_stranded        ->setChecked( true );
   ck_mw_only         ->setChecked( false );
                      
   rb_3_hydroxyl      ->setChecked( true );
   rb_5_hydroxyl      ->setChecked( true );
                      
   le_nucle_mw        ->clear();
   le_nucle_vbar      ->clear();
   le_carbs_mw        ->clear();
   le_carbs_vbar      ->clear();
                      
   pb_save            ->setEnabled( false );
   pb_more            ->setEnabled( false );
   pb_delete          ->setEnabled( false );

   qApp->processEvents();
   inReset = false;
}

void US_AnalyteGui::temp_changed( const QString& text )
{
   double temperature = text.toDouble();
   double vbar20      = le_protein_vbar20->text().toDouble();
   double vbar        = vbar20 + 4.25e-4 * ( temperature - 20.0 );
   le_protein_vbar->setText( QString::number( vbar, 'f', 4 ) );
}

void US_AnalyteGui::parse_dna( void )
{
   A = analyte.sequence.count( "a" );
   C = analyte.sequence.count( "c" );
   G = analyte.sequence.count( "g" );
   T = analyte.sequence.count( "t" );
   U = analyte.sequence.count( "u" );
}

void US_AnalyteGui::manage_sequence( void )
{
   US_SequenceEditor* edit = new US_SequenceEditor( analyte.sequence );
   connect( edit, SIGNAL( sequenceChanged( QString ) ), 
                  SLOT  ( update_sequence( QString ) ) );
   edit->exec();

}

void US_AnalyteGui::update_sequence( QString seq )
{
   seq = seq.toLower().remove( QRegExp( "[\\s0-9]" ) );
   QString check = seq;

   if ( seq == analyte.sequence ) return;

   switch ( analyte.type )
   {
      case US_Analyte::PROTEIN:
         seq.remove( QRegExp( "[^a-z\\+\\?\\@]" ) );
         break;

      case US_Analyte::DNA:
         seq.remove( QRegExp( "[^acgt]" ) );
         break;

      case US_Analyte::RNA:
         seq.remove( QRegExp( "[^acgu]" ) );
         break;

      case US_Analyte::CARBOHYDRATE:
         break;
   }

   if ( check != seq )
   {
      int response = QMessageBox::question( this,
         tr( "Attention" ), 
         tr( "There are invalid characters in the sequence!\n"
             "Invalid characters will be removed\n"
             "Do you want to continue?" ), 
         QMessageBox::Yes, QMessageBox::No );

      if ( response == QMessageBox::No ) return;
   }

   // Reformat the sequence
   const int  gsize = 10;
   const int  lsize = 6;

   // Groups of gsize nucleotides
   int     segments = ( seq.size() + gsize - 1 ) / gsize;
   int     p        = 0;
   QString s;

   for ( int i = 0; i < segments; i++ )
   {
      QString t;

      if ( i % lsize == 0 )
         s += t.sprintf( "%04i ", i * gsize + 1 );
     
      s += seq.mid( p, gsize );
      p += gsize;
      
      if ( i % lsize == lsize - 1 )
         s.append( "\n" );
      else
         s.append( " " );
   }

   analyte.sequence = s;

   switch ( analyte.type )
   {
      case US_Analyte::PROTEIN:
      {
         US_Math2::Peptide p;
         double temperature = le_protein_temp->text().toDouble();
         US_Math2::calc_vbar( p, analyte.sequence, temperature );

         le_protein_mw      ->setText( QString::number( (int) p.mw ) );
         le_protein_vbar20  ->setText( QString::number( p.vbar20, 'f', 4 ) );
         le_protein_vbar    ->setText( QString::number( p.vbar  , 'f', 4 ) );
         le_protein_residues->setText( QString::number( p.residues ) );
         le_protein_e280    ->setText( QString::number( p.e280     ) );

         analyte.mw     = p.mw;
         analyte.vbar20 = p.vbar20;

         // If spectrum is empty, set to 280.0/e280
         QString spectrum_type = cmb_optics->currentText();

         if ( spectrum_type == tr( "Absorbance" ) )
         {
            if ( analyte.extinction.count() == 0 )
               analyte.extinction  [ 280.0 ] = p.e280;
         }
         else if ( spectrum_type == tr( "Interference" ) )
         {
            if ( analyte.refraction.count() == 0 )
               analyte.refraction  [ 280.0 ] = p.e280;
         }
         else
         {
            if ( analyte.fluorescence.count() == 0 )
               analyte.fluorescence[ 280.0 ] = p.e280;
         }
         break;
      }

      case US_Analyte::DNA:
      case US_Analyte::RNA:
         update_nucleotide();
         break;

      case US_Analyte::CARBOHYDRATE:
         le_carbs_mw  ->setText( QString::number( (int) analyte.mw ) );
         le_carbs_vbar->setText( QString::number( analyte.vbar20, 'f', 4 ) );
         break;
   }

   pb_save->setEnabled( true );
   pb_more->setEnabled( true );
}

void US_AnalyteGui::update_stranded( bool checked )
{
   if ( inReset ) return;
   if ( checked ) ck_mw_only->setChecked( false );
   update_nucleotide();
}

void US_AnalyteGui::update_mw_only( bool checked )
{
   if ( inReset ) return;
   if ( checked ) ck_stranded->setChecked( false );
   update_nucleotide();
}

void US_AnalyteGui::update_nucleotide( bool /* value */ )
{
   if ( inReset ) return;
   update_nucleotide();
}

void US_AnalyteGui::update_nucleotide( double /* value */ )
{
   if ( inReset ) return;
   update_nucleotide();
}

void US_AnalyteGui::update_nucleotide( void )
{
   if ( inReset ) return;

   parse_dna();

   bool isDNA             = rb_dna       ->isChecked();
   analyte.doubleStranded = ck_stranded  ->isChecked();
   analyte.complement     = ck_mw_only   ->isChecked();
   analyte._3prime        = rb_3_hydroxyl->isChecked();
   analyte._5prime        = rb_5_hydroxyl->isChecked();

   analyte.sodium    = ct_sodium   ->value();
   analyte.potassium = ct_potassium->value();
   analyte.lithium   = ct_lithium  ->value();
   analyte.magnesium = ct_magnesium->value();
   analyte.calcium   = ct_calcium  ->value();

   double MW = 0;
   uint   total = A + G + C + T + U;
   
   if ( analyte.doubleStranded ) total *= 2;
   
   const double mw_A = 313.209;
   const double mw_C = 289.184;
   const double mw_G = 329.208;
   const double mw_T = 304.196;
   const double mw_U = 274.170;
   
   if ( isDNA )
   {
      if ( analyte.doubleStranded )
      {
         MW += A * mw_A;
         MW += G * mw_G;
         MW += C * mw_C;
         MW += T * mw_T;
         MW += A * mw_T;
         MW += G * mw_C;
         MW += C * mw_G;
         MW += T * mw_A;
      }

      if ( analyte.complement )
      {
         MW += A * mw_T;
         MW += G * mw_C;
         MW += C * mw_G;
         MW += T * mw_A;
      }

      if ( ! analyte.complement && ! analyte.doubleStranded )
      {
         MW += A * mw_A;
         MW += G * mw_G;
         MW += C * mw_C;
         MW += T * mw_T;
      }
   }
   else /* RNA */
   {
      if ( analyte.doubleStranded )
      {
         MW += A * ( mw_A + 15.999 );
         MW += G * ( mw_G + 15.999 );
         MW += C * ( mw_C + 15.999 );
         MW += U * ( mw_U + 15.999 );
         MW += A * ( mw_U + 15.999 );
         MW += G * ( mw_C + 15.999 );
         MW += C * ( mw_G + 15.999 );
         MW += U * ( mw_A + 15.999 );
      }

      if ( analyte.complement )
      {
         MW += A * ( mw_U + 15.999 );
         MW += G * ( mw_C + 15.999 );
         MW += C * ( mw_G + 15.999 );
         MW += U * ( mw_A + 15.999 );
      }

      if ( ! analyte.complement && ! analyte.doubleStranded )
      {
         MW += A * ( mw_A + 15.999 );
         MW += G * ( mw_G + 15.999 );
         MW += C * ( mw_C + 15.999 );
         MW += U * ( mw_U + 15.999 );
      }
   }
   
   MW += analyte.sodium    * total * 22.99;
   MW += analyte.potassium * total * 39.1;
   MW += analyte.lithium   * total * 6.94;
   MW += analyte.magnesium * total * 24.305;
   MW += analyte.calcium   * total * 40.08;
   
   if ( analyte._3prime )
   {
      MW += 17.01;
      if ( analyte.doubleStranded )  MW += 17.01; 
   }
   else // we have phosphate
   {
      MW += 94.87;
      if ( analyte.doubleStranded ) MW += 94.87;
   }

   if ( analyte._5prime )
   {
      MW -=  77.96;
      if ( analyte.doubleStranded )  MW -= 77.96; 
   }

   if ( analyte.sequence.isEmpty() ) MW = 0;

   QString s;

   if ( analyte.doubleStranded )
   {
      s.sprintf(" %2.5e kD (%d A, %d G, %d C, %d U, %d T, %d bp)",
            MW / 1000.0, A, G, C, U, T, total / 2);
   }
   else
   {
     s.sprintf(" %2.5e kD (%d A, %d G, %d C, %d U, %d T, %d bases)",
            MW / 1000.0, A, G, C, U, T, total );
   }
   
   le_nucle_mw->setText( s );

   if ( rb_dna->isChecked() )
   {

      if ( MW > 0 && T == 0 && U > 0)
         QMessageBox::question( this, 
            tr( "Attention:" ),
            tr( "Are you sure?\n"
                "There don't appear to be any thymine residues present,\n"
                "instead there are uracil residues in this sequence." ) );
   }
   else // DNA */
   {
      if ( MW > 0 && T > 0 && U == 0 )
         QMessageBox::question( this,
            tr( "Attention:" ),
            tr( "Are you sure?\n"
                "There don't appear to be any uracil residues present,\n"
                "instead there are thymine residues in this sequence." ) );
   }
}

void US_AnalyteGui::more_info( void )
{
   US_Math2::Peptide p;
   double            temperature;

   if ( analyte.type == US_Analyte::PROTEIN )
   {
      temperature = le_protein_temp->text().toDouble();
      US_Math2::calc_vbar( p, analyte.sequence, temperature );
   }
   else
      temperature = 20.0;


   QString s;
   QString s1 =
             "***************************************************\n"     +
         tr( "*            Analyte Analysis Results             *\n" )   +
             "***************************************************\n\n\n" +
         tr( "Report for:           " ) + analyte.description + "\n\n" ;

   
   if ( analyte.type == US_Analyte::PROTEIN )
   {
      s1 += tr( "Number of Residues:   " ) + s.sprintf( "%i", p.residues ) + " AA\n";
      s1 += tr( "Molecular Weight:     " ) + s.sprintf( "%i", (int)p.mw )  +
            tr( " Dalton\n" ) +
         
            tr( "V-bar at 20.0 " ) + DEGC + ":     " + 
                 QString::number( p.vbar20, 'f', 4 )   + tr( " cm^3/g\n" );

      // Write overridden value if applicable
      bool   override = false;
      double vbar20   = le_protein_vbar20->text().toDouble();

      if ( fabs( vbar20 - p.vbar20 ) > 1e-4 ) override = true;

      if ( override )
         s1 += tr( "V-bar at 20.0 " ) + DEGC + ":     " +
               QString::number( vbar20, 'f', 4 )     + 
               tr( " cm^3/g (override)\n" );

      if ( temperature != 20.0 )
      {
         s1+=     tr( "V-bar at " ) + QString::number( temperature, 'f', 1 ) + " " + 
                      DEGC + ":     " + QString::number( p.vbar, 'f', 4 ) + 
                      tr( " cm^3/g\n" );

         if ( override )
            s1+=  tr( "V-bar at " ) + QString::number( temperature, 'f', 1 ) + " " + 
                      DEGC + ":     " + le_protein_vbar->text() + 
                      tr( " cm^3/g (override)\n" );
      }
   }
   else
   {
      s1 += tr( "Molecular Weight:    " ) + le_nucle_mw->text() + "\n" +
          
            tr( "V-bar:                " ) + le_nucle_vbar->text() + tr( " cm^3/g\n" );
         
   }

   s1 += tr( "\nExtinction coefficients for the denatured analyte:\n" 
             "  Wavelength (nm)   OD/(mol cm)\n" );

   QList< double > keys = analyte.extinction.keys();
   qSort( keys );

   for ( int i = 0; i < keys.size(); i++ )
   {
      QString s;
      s.sprintf( "  %4.1f          %9.4f\n", 
            keys[ i ], analyte.extinction[ keys[ i ] ] );
      s1 += s;
   }

   s1 += tr( "\nRefraction coefficients for the analyte:\n"
                      "  Wavelength (nm)   OD/(mol cm)\n" );
        
   keys = analyte.refraction.keys();
   qSort( keys );

   for ( int i = 0; i < keys.size(); i++ )
   {
      QString s;
      s.sprintf( "  %4.1f          %9.4f\n", 
            keys[ i ], analyte.refraction[ keys[ i ] ] );
      s1 += s;
   }

   s1 += tr( "\nFluorescence coefficients for the analyte:\n"
                      "  Wavelength (nm)   OD/(mol cm)\n" );
        
   keys = analyte.fluorescence.keys();
   qSort( keys );

   for ( int i = 0; i < keys.size(); i++ )
   {
      QString s;
      s.sprintf( "  %4.1f          %9.4f\n", 
            keys[ i ], analyte.fluorescence[ keys[ i ] ] );
      s1 += s;
   }

   s1 += tr( "\nComposition: \n\n" );

   if ( analyte.type == US_Analyte::PROTEIN )
   {

      s1 += tr( "Alanine:        " )  + s.sprintf( "%3i", p.a ) + "  ";
      s1 += tr( "Arginine:       " )  + s.sprintf( "%3i", p.r ) + "\n";
            
      s1 += tr( "Asparagine:     " )  + s.sprintf( "%3i", p.n ) + "  ";
      s1 += tr( "Aspartate:      " )  + s.sprintf( "%3i", p.d ) + "\n";
            
      s1 += tr( "Asparagine or \n" )  +
            tr( "Aspartate:      " )  + s.sprintf( "%3i", p.b ) + "\n";
            
      s1 += tr( "Cysteine:       " )  + s.sprintf( "%3i", p.c ) + "  ";
      s1 += tr( "Glutamate:      " )  + s.sprintf( "%3i", p.e ) + "\n";
            
      s1 += tr( "Glutamine:      " )  + s.sprintf( "%3i", p.q ) + "  ";
      s1 += tr( "Glycine:        " )  + s.sprintf( "%3i", p.g ) + "\n";
           
      s1 += tr( "Glutamine or  \n" )  +  
            tr( "Glutamate:      " )  + s.sprintf( "%3i", p.z ) + "\n";
            
      s1 += tr( "Histidine:      " )  + s.sprintf( "%3i", p.h ) + "  ";
      s1 += tr( "Isoleucine:     " )  + s.sprintf( "%3i", p.i ) + "\n";
            
      s1 += tr( "Leucine:        " )  + s.sprintf( "%3i", p.l ) + "  ";
      s1 += tr( "Lysine:         " )  + s.sprintf( "%3i", p.k ) + "\n";
            
      s1 += tr( "Methionine:     " )  + s.sprintf( "%3i", p.m ) + "  ";
      s1 += tr( "Phenylalanine:  " )  + s.sprintf( "%3i", p.f ) + "\n";
            
      s1 += tr( "Proline:        " )  + s.sprintf( "%3i", p.p ) + "  ";
      s1 += tr( "Serine:         " )  + s.sprintf( "%3i", p.s ) + "\n";
            
      s1 += tr( "Threonine:      " )  + s.sprintf( "%3i", p.t ) + "  ";
      s1 += tr( "Tryptophan:     " )  + s.sprintf( "%3i", p.w ) + "\n";
            
      s1 += tr( "Tyrosine:       " )  + s.sprintf( "%3i", p.y ) + "  ";
      s1 += tr( "Valine:         " )  + s.sprintf( "%3i", p.v ) + "\n";
            
      s1 += tr( "Unknown:        " )  + s.sprintf( "%3i", p.x ) + "  ";
      s1 += tr( "Hao:            " )  + s.sprintf( "%3i", p.j ) + "\n";
            
      s1 += tr( "Delta-linked Ornithine: " ) + QString::number( p.o ) + "\n";
   }
   else
   {
      int a = analyte.sequence.count( "a" );
      int c = analyte.sequence.count( "c" );
      int g = analyte.sequence.count( "g" );
      int t = analyte.sequence.count( "t" );
      int u = analyte.sequence.count( "u" );

      s1 += tr( "Adenine:        " )  + s.sprintf( "%3i", a ) + "  ";
      s1 += tr( "Cytosine:       " )  + s.sprintf( "%3i", c ) + "\n";
            
      s1 += tr( "Guanine:        " )  + s.sprintf( "%3i", g ) + "  ";

      if ( analyte.type == US_Analyte::DNA )
         s1 += tr( "Thymine:        " )  + s.sprintf( "%3i", t ) + "\n";
      
      else if  ( analyte.type == US_Analyte::RNA )   
         s1 += tr( "Uracil:         " )  + s.sprintf( "%3i", u ) + "\n";
   }

   US_EditorGui* dialog = new US_EditorGui();
   QFont font = QFont( "monospace", US_GuiSettings::fontSize() );
   dialog->editor->e->setFont( font );
   dialog->editor->e->setText( s1 );

   QFontMetrics fm( font );

   dialog->resize( fm.width( 'W' ) * 80, fm.height() * 20 );

   dialog->exec();
}

void US_AnalyteGui::search( const QString& text )
{
   lw_analytes->clear();
   info.clear();
   int crow = -1;

   for ( int i = 0; i < descriptions.size(); i++ )
   {
      if ( descriptions[ i ].contains( 
             QRegExp( ".*" + text + ".*", Qt::CaseInsensitive ) ) )
      {
         AnalyteInfo ai;
         ai.description = descriptions[ i ];
         ai.guid        = GUIDs       [ i ];
         ai.filename    = filenames   [ i ];
         ai.index       = i;
         info << ai;

         lw_analytes->addItem( 
               new QListWidgetItem( descriptions[ i ], lw_analytes ) );

         if ( ai.description == analyte.description )
            crow = i;
      }
   }

   if ( crow >= 0 )
      lw_analytes->setCurrentRow( crow );
}

void US_AnalyteGui::sel_investigator( void )
{
   reset();
   
   US_Investigator* inv_dialog = new US_Investigator( true );
   
   connect( inv_dialog,
      SIGNAL( investigator_accepted( int ) ),
      SLOT  ( assign_investigator  ( int ) ) );
   
   inv_dialog->exec();
}

void US_AnalyteGui::assign_investigator( int invID )
{
   personID = invID;

   QString number = ( personID > 0 )
            ? QString::number( invID ) + ": "
            : "";

   le_investigator->setText( number + US_Settings::us_inv_name() );

   lw_analytes->clear();
   le_search  ->clear();
   le_search  ->setEnabled( false );
   pb_save    ->setEnabled ( false );
   pb_delete  ->setEnabled ( false );
}

void US_AnalyteGui::spectrum( void )
{
   QString   spectrum_type = cmb_optics->currentText();
   QString   strExtinc     = tr( "Extinction:" );
   bool      changed       = false;
   bool      is_prot       = rb_protein->isChecked();
   double    e280val       = le_protein_e280->text().toDouble();
   US_Table* dialog;

   if ( spectrum_type == tr( "Absorbance" ) )
   {
      if ( is_prot  &&  analyte.extinction.count() == 0 )
         analyte.extinction[ 280.0 ] = e280val;

      dialog = new US_Table( analyte.extinction,   strExtinc, changed );
   }
   else if ( spectrum_type == tr( "Interference" ) )
   {
      if ( is_prot  &&  analyte.refraction.count() == 0 )
         analyte.refraction[ 280.0 ] = e280val;

      dialog = new US_Table( analyte.refraction,   strExtinc, changed );
   }
   else
   {
      if ( is_prot  &&  analyte.fluorescence.count() == 0 )
         analyte.fluorescence[ 280.0 ] = e280val;

      dialog = new US_Table( analyte.fluorescence, strExtinc, changed );
   }

   dialog->setWindowTitle( tr( "Manage %1 Values" ).arg( spectrum_type ) );

   dialog->exec();
}

void US_AnalyteGui::connect_error( const QString& error )
{
   QMessageBox::warning( this, tr( "Connection Problem" ),
      tr( "Could not connect to database\n" ) + error );
}

void US_AnalyteGui::list( void )
{
   if ( disk_controls->db() )
      list_from_db();
   else
      list_from_disk();

   le_search->setEnabled(  true  );
   le_search->setReadOnly( false );
}

void US_AnalyteGui::list_from_db( void )
{
   if ( personID < 0 )
   {
      QMessageBox::information( this,
            tr( "Investigator not set" ),
            tr( "Please select an investigator first." ) );
      return;
   }

   US_Passwd pw;
   US_DB2    db( pw.getPasswd() );

   if ( db.lastErrno() != US_DB2::OK )
   {
      connect_error( db.lastError() );
      return;
   }

   QStringList q( "get_analyte_desc" );
   q << QString::number( personID );

   db.query( q );

   if ( db.lastErrno() != US_DB2::OK )
   {
      QMessageBox::information( this,
            tr( "Database Error" ),
            tr( "The following error was returned:\n" ) + db.lastError() );
      return; 
   }

   descriptions.clear();
   analyteIDs  .clear();
   filenames   .clear();
   GUIDs       .clear();

   while ( db.next() )
   {
      QString               a_type = db.value( 2 ).toString();
      US_Analyte::analyte_t current = US_Analyte::PROTEIN;


           if ( a_type == "Protein" ) current = US_Analyte::PROTEIN;
      else if ( a_type == "RNA"     ) current = US_Analyte::RNA;
      else if ( a_type == "DNA"     ) current = US_Analyte::DNA;
      else if ( a_type == "Other"   ) current = US_Analyte::CARBOHYDRATE;
      
      if ( current != analyte.type ) continue;

      analyteIDs   << db.value( 0 ).toString();
      descriptions << db.value( 1 ).toString();
      filenames    << "";
      GUIDs        << "";
   }

   search();
}

void US_AnalyteGui::list_from_disk( void )
{
   QString path;
   if ( ! US_Analyte::analyte_path( path ) ) return;

   filenames   .clear();
   descriptions.clear();
   GUIDs       .clear();

   QDir f( path );
   QStringList filter( "A*.xml" );
   QStringList f_names = f.entryList( filter, QDir::Files, QDir::Name );

   QString type = "*Unknown*";
   if      ( rb_protein->isChecked() ) type = "PROTEIN";
   else if ( rb_dna    ->isChecked() ) type = "DNA";
   else if ( rb_rna    ->isChecked() ) type = "RNA";
   else if ( rb_carb   ->isChecked() ) type = "CARBOHYDRATE";

   QFile a_file;

   for ( int i = 0; i < f_names.size(); i++ )
   {
      a_file.setFileName( path + "/" + f_names[ i ] );

      if ( ! a_file.open( QIODevice::ReadOnly | QIODevice::Text) ) continue;

      QXmlStreamReader xml( &a_file );

      while ( ! xml.atEnd() )
      {
         xml.readNext();

         if ( xml.isStartElement() )
         {
            if ( xml.name() == "analyte" )
            {
               QXmlStreamAttributes a = xml.attributes();

               if ( a.value( "type" ).toString() == type )
               {
                  descriptions << a.value( "description" ).toString();
                  GUIDs        << a.value( "analyteGUID" ).toString();
                  filenames    << path + "/" + f_names[ i ];
               }
               break;
            }
         }
      }

      a_file.close();
   }

   search();
}

bool US_AnalyteGui::database_ok( US_DB2& db )
{
   if ( db.lastErrno() == 0 ) return true;

   QMessageBox::information( this,
      tr( "Database Error" ),
      tr( "The following error was returned:\n" ) + db.lastError() );

   return false; 
}

bool US_AnalyteGui::data_ok( void )
{
   // Check to see if a sequence is entered
   if ( analyte.sequence.isEmpty() )
   {
      QMessageBox question( QMessageBox::Question,
            tr( "Attention" ),
            tr( "There is no sequence defined.\n\n" 
                "Continue?" ), 
            QMessageBox::No,
            this );

      question.addButton( tr( "Continue" ), QMessageBox::YesRole );

      if ( question.exec() == QMessageBox::No )
         return false;
   }
   
   analyte.description = le_description->text().remove( '|' );

   if ( analyte.description.isEmpty() )
   {
      QMessageBox question( QMessageBox::Question,
            tr( "Attention" ),
            tr( "There is no description for this analyte.\n\n" 
                "Continue?" ), 
            QMessageBox::No,
            this );

      question.addButton( tr( "Continue" ), QMessageBox::YesRole );

      if ( question.exec() == QMessageBox::No )
         return false;

   }

   double vbar = le_protein_vbar->text().toDouble();

   if ( analyte.type == US_Analyte::PROTEIN && ( vbar <= 0.0  || vbar > 2.0 ) )
   {
      QMessageBox::information( this,
         tr( "Attention" ), 
         tr( "The vbar entry (%1) is not a reasonable value." )
         .arg( vbar ) );
      return false;
   }

   double mwvl   = le_protein_mw->text().toDouble();
   if ( analyte.type == US_Analyte::DNA ||
        analyte.type == US_Analyte::RNA  )
      mwvl   = le_nucle_mw->text().section( " ", 1, 1 ).toDouble();
   else if ( analyte.type == US_Analyte::CARBOHYDRATE )
      mwvl   = le_carbs_mw->text().toDouble();

   if ( mwvl <= 0.0 )
   {
      QMessageBox::information( this,
         tr( "Attention" ), 
         tr( "The Molecular Weight entry (%1) is not a reasonable value." )
         .arg( mwvl ) );
      return false;
   }

   return true;
}

int US_AnalyteGui::status_query( const QStringList& q )
{
   US_Passwd pw;
   US_DB2    db( pw.getPasswd() );

   if ( db.lastErrno() != US_DB2::OK )
   {
      connect_error( db.lastError() );
      return db.lastErrno();
   }

   db.statusQuery( q );

   int status = db.lastErrno();

   if ( status == US_DB2::ANALY_IN_USE )  return status;

   if ( database_ok( db ) ) return US_DB2::ERROR;

   if ( status != US_DB2::OK )
   {
      QMessageBox::information( this,
            tr( "Database Error" ),
            tr( "The following errro was returned:\n" ) + db.lastError() );
      return status;
   }

   return US_DB2::OK;
}

bool US_AnalyteGui::discard_changes( void )
{
   // See if there are changes to discard
   if ( saved_analyte == analyte ) return true;

   int response = QMessageBox::question( this,
      tr( "Analyte Changed" ),
      tr( "The analyte has changed.\n"
          "Do you want to continue without saving?" ),
      QMessageBox::Cancel, QMessageBox::Yes );

   if ( response == QMessageBox::Yes ) return true;

   return false;
}

void US_AnalyteGui::select_analyte( QListWidgetItem* /* item */ )
{
   if ( ! discard_changes() ) return;

   analyte.extinction  .clear();
   analyte.refraction  .clear();
   analyte.fluorescence.clear();

   pb_save  ->setEnabled( false );
   pb_delete->setEnabled( false );
   pb_more  ->setEnabled( false );

   try
   {
      if ( disk_controls->db() )
         select_from_db();
      else
         select_from_disk();
   }
   catch ( int )
   {
      return;
   }

qDebug() << "AnG: select_analyte  desc" << analyte.description;
   populate();
   le_protein_vbar20->setText( QString::number( analyte.vbar20, 'f', 4 ) );
   le_nucle_vbar    ->setText( QString::number( analyte.vbar20, 'f', 4 ) );
   le_carbs_vbar    ->setText( QString::number( analyte.vbar20, 'f', 4 ) );
   if ( analyte.type == US_Analyte::DNA ||  analyte.type == US_Analyte::RNA ) 
      update_nucleotide();
   //le_nucle_mw      ->setText( QString::number( (int) analyte.mw ) );
   le_carbs_mw      ->setText( QString::number( (int) analyte.mw ) );
   saved_analyte = analyte;

   pb_delete->setEnabled( true );
   pb_save  ->setEnabled( true );
   pb_more  ->setEnabled( true );
}

void US_AnalyteGui::select_from_disk( void )
{
   int index = lw_analytes->currentRow();
   if ( index < 0 ) throw -1;
   if ( info[ index ].filename.isEmpty() ) 
   {
      throw -1;
   }

   int result = analyte.load( false, info[ index ].guid );

   if ( result != US_DB2::OK )
   {
      QMessageBox::warning( this,
            tr( "Analyte Load Rrror" ),
            tr( "Load error when reading the disk:\n\n" ) + analyte.message );
      throw result;
   }

   analyte.message = tr( "disk" );

}

void US_AnalyteGui::select_from_db( void )
{
   int index = lw_analytes->currentRow();
   if ( index < 0 ) throw -1;

   US_Passwd pw;
   US_DB2    db( pw.getPasswd() );

   if ( db.lastErrno() != US_DB2::OK )
   {
      connect_error( db.lastError() );
      throw db.lastErrno();
   }

   // TODO: Fix analyte to fetch from analyteID
   // Here we are getting the guid when we know what the IS is, but
   // US_Analyte searches for the guid...  Extra work for no benefit.

   QStringList q;
   int         i = info[ index ].index;
   q << "get_analyte_info" << analyteIDs[ i ];
   db.query( q );
   db.next();

   QString guid = db.value( 0 ).toString();

   int result = analyte.load( true, guid, &db );

   if ( result != US_DB2::OK )
   {
      QMessageBox::warning( this,
         tr( "Analyte Load Error" ),
         tr( "Load error when reading the database:\n\n" ) + analyte.message );
      throw result;
   }

   analyte.message = tr( "database" );
}

void US_AnalyteGui::save( void )
{
   if ( analyte.analyteGUID.size() != 36 )
      analyte.analyteGUID = US_Util::new_guid();

   if ( ! data_ok() ) return;

   le_guid->setText( analyte.analyteGUID );

   if ( analyte.type == US_Analyte::DNA ||
        analyte.type == US_Analyte::RNA  )
   {
      // Strip trailing items from the mw text box.
      QStringList mw = le_nucle_mw->text().split( " ", QString::SkipEmptyParts );

      if ( mw.empty() )
      {
         QMessageBox::warning( this,
            tr( "Analyte Error" ),
            tr( "Molecular weight is empty.  Define a sequence." ) );
         return;
      }

      analyte.mw     = mw[ 0 ].toDouble() * 1000.0;
      analyte.vbar20 = le_nucle_vbar->text().toDouble();
   }

   else if ( analyte.type == US_Analyte::CARBOHYDRATE )
   {
      analyte.mw        = le_carbs_mw  ->text().toDouble();
      analyte.vbar20    = le_carbs_vbar->text().toDouble();
      analyte.grad_form = ck_grad_form ->isChecked();
   }

   else
   {
      analyte.mw     = le_protein_mw    ->text().toDouble();
      analyte.vbar20 = le_protein_vbar20->text().toDouble();
   }

   verify_vbar();

   int result;

   if ( disk_controls->db() )
   {
      US_Passwd pw;
      US_DB2    db( pw.getPasswd() );
      
      if ( db.lastErrno() != US_DB2::OK )
      {
         QMessageBox::information( this,
            tr( "Database Error" ),
            tr( "The following error was returned:\n" ) + db.lastError() );
         return; 
      }

      result = analyte.write( true, "", &db );
   }
   else
   {
      QString path;
      if ( ! US_Analyte::analyte_path( path ) ) return;

      QString filename = US_Analyte::get_filename( path, analyte.analyteGUID );
      result = analyte.write( false, filename );
   }

   saved_analyte = analyte;

   if ( result == US_DB2::OK )
      QMessageBox::information( this,
            tr( "Save successful" ),
            tr( "The analyte has been successfully saved." ) );
   else
      QMessageBox::warning( this,
            tr( "Save Error" ),
            tr( "The analyte could not be saved.\n\n" ) + analyte.message );

   list();
}

void US_AnalyteGui::delete_analyte( void )
{
   //TODO  What if we select new and then want to delete that?
   if ( analyte.analyteGUID.size() != 36 ) return;

   if ( disk_controls->db() )
      delete_from_db();
   else
      delete_from_disk();


   list();
}

void US_AnalyteGui::delete_from_disk( void )
{
   // Find the file 
   QString path;
   if ( ! US_Analyte::analyte_path( path ) ) return;

   QString fn = US_Analyte::get_filename( path, analyte.analyteGUID );


   if ( analyte_in_use( analyte.analyteGUID ) )
   {
      QMessageBox::warning( this,
            tr( "Not Deleted" ),
            tr( "The analyte could not be deleted,\n"
                "since it is in use in one or more solutions." ) );
      return;
   }

   // Delete it
   QFile file( fn );
   if ( file.exists() ) 
   {
      file.remove();
      reset();

      QMessageBox::information( this,
         tr( "Analyte Deleted" ),
         tr( "The analyte has been deleted from the disk." ) );
   }
}

void US_AnalyteGui::delete_from_db( void )
{
   QStringList q;
   q << "delete_analyte" << analyte.analyteID;

   int status = status_query( q );

   if ( status == US_DB2::ANALY_IN_USE )
   {
      QMessageBox::warning( this,
            tr( "Analyte Not Deleted" ),
            tr( "The analyte could not be deleted,\n"
                "since it is in use in one or more solutions." ) );
      return;
   }

   if ( status != US_DB2::OK ) return;

   reset();

   QMessageBox::information( this,
      tr( "Analyte Deleted" ),
      tr( "The analyte has been deleted from the database." ) );
}

void US_AnalyteGui::verify_vbar()
{
   if ( analyte.type == US_Analyte::PROTEIN )
   {
      US_Math2::Peptide p;
      double temperature = le_protein_temp->text().toDouble();
      US_Math2::calc_vbar( p, analyte.sequence, temperature );

      double mwval    = le_protein_mw->text().toDouble();

      if ( p.mw == 0.0 )
         p.mw            = mwval;

      else if ( mwval != 0.0  &&  qAbs( mwval - p.mw ) > 1.0 )
      {
         QString msg  = tr(
               "There is a difference between<br/>"
               "the Molecular Weight value that you specified and<br/>"
               "the one calculated from the protein sequence.<br/> <br/>"
               "Do you wish to accept the specified value?<ul>"
               "<li><b>Yes</b> to use %1 (the specified);</li>"
               "<li><b>No </b> to use %2 (the calculated).</li></ul>" )
                            .arg( mwval ).arg( p.mw );

         QMessageBox msgBox     ( this );
         msgBox.setWindowTitle  ( tr( "Analyte MW Difference" ) );
         msgBox.setTextFormat   ( Qt::RichText );
         msgBox.setText         ( msg );
         msgBox.addButton       ( QMessageBox::No  );
         msgBox.addButton       ( QMessageBox::Yes );
         msgBox.setDefaultButton( QMessageBox::Yes );

         if ( msgBox.exec() == QMessageBox::No )
            mwval        = p.mw;
      }

      else if ( mwval == 0.0 )
         mwval        = p.mw;

      le_protein_mw->setText( QString::number( (int) mwval ) );
      analyte.mw      = mwval;
      double pvbar    = p.vbar20;
      double vbar20   = le_protein_vbar20->text().toDouble();

      if ( qAbs( vbar20 - pvbar ) > 1e-4 )
      {
         QString msg  = tr(
               "There is a difference between<br/>"
               "the vbar20 value that you specified and<br/>"
               "the one calculated from the protein sequence.<br/> <br/>"
               "Do you wish to accept the specified value?<ul>"
               "<li><b>Yes</b> to use %1 (the specified);</li>"
               "<li><b>No </b> to use %2 (the calculated).</li></ul>" )
                            .arg( vbar20 ).arg( pvbar );

         QMessageBox msgBox( this );
         msgBox.setWindowTitle( tr( "Analyte Vbar Difference" ) );
         msgBox.setTextFormat ( Qt::RichText );
         msgBox.setText       ( msg );
         msgBox.addButton     ( QMessageBox::No  );
         msgBox.addButton     ( QMessageBox::Yes );
         msgBox.setDefaultButton( QMessageBox::Yes );

         if ( msgBox.exec() == QMessageBox::No )
            vbar20       = pvbar;

         analyte.vbar20  = vbar20;
         le_protein_vbar20->setText( QString::number( vbar20 ) );
      }
   }
}

// Determine by GUID whether an analyte is in use in any solution on disk
bool US_AnalyteGui::analyte_in_use( QString& analyteGUID )
{
   bool in_use = false;
   QString soldir = US_Settings::dataDir() + "/solutions/";
   QStringList sfilt( "S*.xml" );
   QStringList snames = QDir( soldir )
      .entryList( sfilt, QDir::Files, QDir::Name );

   for ( int ii = 0;  ii < snames.size(); ii++ )
   {
      QString sfname = soldir + snames.at( ii );
      QFile sfile( sfname );

      if ( ! sfile.open( QIODevice::ReadOnly | QIODevice::Text ) ) continue;

      QXmlStreamReader xml( &sfile );

      while ( ! xml.atEnd() )
      {
         xml.readNext();

         if ( xml.isStartElement()  &&  xml.name() == "analyte" )
         {
            QXmlStreamAttributes atts = xml.attributes();

            if ( atts.value( "guid" ).toString() == analyteGUID )
            {
               in_use = true;
               break;
            }
         }
      }

      sfile.close();

      if ( in_use )  break;
   }

   return in_use;
}


/*  Class US_SequenceEditor */
US_SequenceEditor::US_SequenceEditor( const QString& sequence ) 
   : US_WidgetsDialog( 0, 0 )
{
   setWindowTitle( tr( "Sequence Management" ) );
   setPalette( US_GuiSettings::frameColor() );
   setAttribute( Qt::WA_DeleteOnClose );

   QGridLayout* main = new QGridLayout( this );
   main->setSpacing         ( 2 );
   main->setContentsMargins ( 2, 2, 2, 2 );

   edit = new US_Editor( US_Editor::LOAD, false );
   edit->e->setAcceptRichText( false );
   edit->e->setText( sequence );
   main->addWidget( edit, 0, 0, 5, 2 );

   QPushButton* pb_cancel = us_pushbutton( tr( "Cancel" ) );
   connect( pb_cancel, SIGNAL( clicked() ), SLOT( close() ) );
   main->addWidget( pb_cancel, 5, 0 );

   QPushButton* pb_accept = us_pushbutton( tr( "Accept" ) );
   connect( pb_accept, SIGNAL( clicked() ), SLOT( accept() ) );
   main->addWidget( pb_accept, 5, 1 );

   QFont font = QFont( "monospace", US_GuiSettings::fontSize() );
   edit->e->setFont( font );

   QFontMetrics fm( font );
   resize( fm.width( 'W' ) * 80, fm.height() * 20 );

}

void US_SequenceEditor::accept( void ) 
{
   emit sequenceChanged( edit->e->toPlainText() );
   close();
}

