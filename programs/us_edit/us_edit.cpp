//! \file us_edit.cpp

#include <QApplication>
#include <QDomDocument>

#include "us_edit.h"
#include "us_license_t.h"
#include "us_license.h"
#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_investigator.h"
#include "us_run_details2.h"
#include "us_exclude_profile.h"
#include "us_select_lambdas.h"
#include "us_ri_noise.h"
#include "us_edit_scan.h"
#include "us_math2.h"
#include "us_util.h"
#include "us_load_auc.h"
#include "us_passwd.h"
#include "us_get_edit.h"
#include "us_constants.h"
#include "us_images.h"

#ifndef DbgLv
#define DbgLv(a) if(dbg_level>=a)qDebug()
#endif

//! \brief Main program for US_Edit. Loads translators and starts
//         the class US_FitMeniscus.

int main( int argc, char* argv[] )
{
   QApplication application( argc, argv );

   #include "main1.inc"

   // License is OK.  Start up.
   
   US_Edit w;
   w.show();                   //!< \memberof QWidget
   return application.exec();  //!< \memberof QApplication
}

// Constructor
US_Edit::US_Edit() : US_Widgets()
{
   check        = QIcon( US_Settings::appBaseDir() + "/etc/check.png" );
   invert       = 1.0;
   all_edits    = false;
   men_1click   = US_Settings::debug_match( "men2click" ) ? false : true;
   total_speeds = 0;
   total_edits  = 0;
   v_line       = NULL;
   dbg_level    = US_Settings::us_debug();
   dbP          = NULL;

   setWindowTitle( tr( "Edit UltraScan Data" ) );
   setPalette( US_GuiSettings::frameColor() );

   QVBoxLayout* top = new QVBoxLayout( this );
   top->setSpacing         ( 2 );
   top->setContentsMargins ( 2, 2, 2, 2 );

   // Put the Run Info across the entire window
   QHBoxLayout* runInfo = new QHBoxLayout();
   QLabel* lb_info = us_label( tr( "Run Info:" ), -1 );
   runInfo->addWidget( lb_info );

   le_info = us_lineedit( "", 1, true );
   runInfo->addWidget( le_info );

   top->addLayout( runInfo );

   QHBoxLayout* main = new QHBoxLayout();
   QVBoxLayout* left = new QVBoxLayout;

   // Start of Grid Layout
   QGridLayout* specs = new QGridLayout;

   // Investigator
   QPushButton* pb_investigator = us_pushbutton( tr( "Select Investigator" ) );

   if ( US_Settings::us_inv_level() < 1 )
      pb_investigator->setEnabled( false );

   int     id      = US_Settings::us_inv_ID();
   QString number  = ( id > 0 ) ? 
      QString::number( US_Settings::us_inv_ID() ) + ": " 
      : "";
   le_investigator = us_lineedit( number + US_Settings::us_inv_name(),
                                  1, true );

   // Disk/DB control
   disk_controls   = new US_Disk_DB_Controls;

   // Load
   QPushButton*
      pb_load      = us_pushbutton( tr( "Load Data" ) );
   pb_details      = us_pushbutton( tr( "Run Details" ), false );

   // Triple and Speed Step
   lb_triple       = us_label( tr( "Cell / Channel / Wavelength" ), -1 );
   cb_triple       = us_comboBox();
   lb_rpms         = us_label( tr( "Speed Step (RPM) of triple" ), -1 );
   cb_rpms         = us_comboBox();
   lb_rpms->setVisible( false );
   cb_rpms->setVisible( false );

   // Scan Gaps
   QFont font( US_GuiSettings::fontFamily(), US_GuiSettings::fontSize() - 1 );
   lb_gaps         = us_label( tr( "Threshold for Scan Gaps" ), -1 );
   ct_gaps         = us_counter( 1, 10.0, 100.0 );
   ct_gaps->setStep ( 10.0 );
   ct_gaps->setValue( 50.0 );

   // MWL Control
   QFontMetrics fmet( font );
   int fwid = fmet.maxWidth();
   int rhgt = ct_gaps->height();
   int lwid = fwid * 4;
   int swid = lwid + fwid;
   const QChar chlamb( 955 );
   lb_mwlctl       = us_banner( tr( "Wavelength Controls" ) );
   QButtonGroup* r_group = new QButtonGroup( this );
   QButtonGroup* x_group = new QButtonGroup( this );
   lo_lrange       = us_radiobutton( tr( "Lambda Range" ),       rb_lrange,
                                     true  );
   lo_custom       = us_radiobutton( tr( "Custom Lambda List" ), rb_custom,
                                     false );
   rb_lrange->setFont( font );
   rb_custom->setFont( font );
   r_group->addButton( rb_lrange, 0 );
   r_group->addButton( rb_custom, 1 );
   lb_ldelta       = us_label( tr( "%1 Index Increment:" ).arg( chlamb ), -1 );
   ct_ldelta       = us_counter( 1, 1, 100, 1 );
   ct_ldelta->setFont( font );
   ct_ldelta->setStep( 1 );
   ct_ldelta->setMinimumWidth( lwid );
   ct_ldelta->resize( rhgt, swid );
   lb_lstart       = us_label( tr( "%1 Start:" ).arg( chlamb ), -1 );
   lb_lend         = us_label( tr( "%1 End:"   ).arg( chlamb ), -1 );
   lb_lplot        = us_label( tr( "Plot (W nm):" ), -1 );

   int     nlmbd   = 224;
   int     lmbdlo  = 251;
   int     lmbdhi  = 650;
   int     lmbddl  = 1;
   QString lrsmry  = tr( "%1 raw: %2 %3 to %4" )
      .arg( nlmbd ).arg( chlamb ).arg( lmbdlo ).arg( lmbdhi );
   le_ltrng  = us_lineedit( lrsmry, -1, true );
   QString lxsmry = tr( "%1 MWL exports: %2 %3 to %4, raw index increment %5" )
      .arg( nlmbd ).arg( chlamb ).arg( lmbdlo ).arg( lmbdhi ).arg( lmbddl );
   le_lxrng       = us_lineedit( lxsmry, -1, true );
   cb_lplot       = us_comboBox();
   cb_lstart      = us_comboBox();
   cb_lend        = us_comboBox();

   cb_lplot ->setFont( font );
   cb_lstart->setFont( font );
   cb_lend  ->setFont( font );

   pb_custom      = us_pushbutton(  tr( "Custom Lambdas" ),      false, -1 );
   pb_incall      = us_pushbutton(  tr( "Include All Lambdas" ), true,  -1 );
   pb_larrow      = us_pushbutton(  tr( "previous" ), true, -2 );
   pb_rarrow      = us_pushbutton(  tr( "next"     ), true, -2 );
   pb_larrow->setIcon( US_Images::getIcon( US_Images::ARROW_LEFT ) );
   pb_rarrow->setIcon( US_Images::getIcon( US_Images::ARROW_RIGHT ) );

   lo_radius      = us_radiobutton( tr( "x axis Radius" ),     rb_radius,
                                    true  );
   lo_waveln      = us_radiobutton( tr( "x axis Wavelength" ), rb_waveln,
                                    false );
   rb_radius->setFont( font );
   rb_waveln->setFont( font );
   x_group->addButton( rb_radius, 0 );
   x_group->addButton( rb_waveln, 1 );

   QStringList lambdas;
lambdas << "250" << "350" << "450" << "550" << "580" << "583" << "650";
   cb_lplot ->addItems( lambdas );
   cb_lstart->addItems( lambdas );
   cb_lend  ->addItems( lambdas );

   cb_lplot ->setCurrentIndex( 2 );
   cb_lstart->setCurrentIndex( 0 );
   cb_lend  ->setCurrentIndex( 6 );

   connect_mwl_ctrls( true );

   // Scan Controls
   QLabel* lb_scan = us_banner( tr( "Scan Controls" ) );
   
   // Scans
   QLabel* lb_from = us_label(  tr( "Scan Focus from:" ), -1 );
   lb_from->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

   ct_from        = us_counter( 3, 0.0, 0.0 ); // Update range upon load
   ct_from->setStep( 1 );

   QLabel* lb_to  = us_label( tr( "to:" ), -1 );
   lb_to->setAlignment( Qt::AlignVCenter | Qt::AlignRight );

   ct_to          = us_counter( 3, 0.0, 0.0 ); // Update range upon load
   ct_to->setStep( 1 );
   
   // Exclude and Include pushbuttons
   pb_excludeRange = us_pushbutton( tr( "Exclude Scan Range" ), false );
   pb_exclusion    = us_pushbutton( tr( "Exclusion Profile" ),  false );
   pb_edit1       = us_pushbutton( tr( "Edit Single Scan" ), false );
   pb_include     = us_pushbutton( tr( "Include All" ), false );

   // Edit controls 
   QLabel* lb_edit = us_banner( tr( "Edit Controls" ) );

   // Edit Triple:Speed display (Equilibrium only)
   lb_edtrsp      = us_label( tr( "Edit Triple:Speed :" ), -1, true );
   le_edtrsp      = us_lineedit( "" );
   lb_edtrsp->setVisible(  false );
   le_edtrsp->setVisible(  false );

   // Meniscus
   pb_meniscus    = us_pushbutton( tr( "Specify Meniscus" ), false );
   le_meniscus    = us_lineedit( "", 1 );

   // Air Gap (hidden by default)
   pb_airGap = us_pushbutton( tr( "Specify Air Gap" ), false );
   le_airGap = us_lineedit( "", 1, true );
   pb_airGap->setHidden( true );
   le_airGap->setHidden( true );

   // Data range
   pb_dataRange   = us_pushbutton( tr( "Specify Data Range" ), false );
   le_dataRange   = us_lineedit( "", 1, true );
   // Plateau
   pb_plateau     = us_pushbutton( tr( "Specify Plateau" ), false );
   le_plateau     = us_lineedit( "", 1, true );
   // Baseline
   lb_baseline    = us_label(      tr( "Baseline:" ), -1 );
   le_baseline    = us_lineedit( "", 1, true );

   // OD Limit
   lb_odlim       = us_label( tr( "OD Limit:" ), -1 );
   odlimit        = 1.5;
   ct_odlim       = us_counter( 3, 0.1, 50000.0, odlimit );
   ct_odlim ->setFont( font );
   ct_odlim ->setStep( 0.01 );
   ct_odlim ->setMinimumWidth( lwid );
   ct_odlim ->resize( rhgt, swid );

   // Noise, Residuals, Invert, Spikes, Prior, Undo
   pb_noise       = us_pushbutton( tr( "Determine RI Noise" ),        false );
   pb_residuals   = us_pushbutton( tr( "Subtract Noise" ),            false );
   pb_invert      = us_pushbutton( tr( "Invert Sign" ),               false );
   pb_spikes      = us_pushbutton( tr( "Remove Spikes" ),             false );
   pb_priorEdits  = us_pushbutton( tr( "Apply Prior Edits" ),         false );
   pb_undo        = us_pushbutton( tr( "Undo Noise and Spikes" ),     false );

   // Review, Next Triple, Float, Save, Save-all
   pb_reviewep    = us_pushbutton( tr( "Review Edit Profile" ),       false );
   pb_nexttrip    = us_pushbutton( tr( "Next Triple" ),               false );
   pb_reviewep->setVisible( false );
   pb_nexttrip->setVisible( false );
   pb_float       = us_pushbutton( tr( "Mark Data as Floating" ),     false );
   pb_write       = us_pushbutton( tr( "Save Current Edit Profile" ), false );
   pb_writemwl    = us_pushbutton( tr( "Save to all Wavelengths" ),   false );

   connect( pb_excludeRange, SIGNAL( clicked() ), SLOT( exclude_range() ) );
   connect( pb_details,      SIGNAL( clicked() ), SLOT( details()       ) );
   connect( pb_investigator, SIGNAL( clicked() ),
                             SLOT  ( sel_investigator()         ) );
   connect( pb_load,         SIGNAL( clicked() ), SLOT( load()  ) );
   connect( cb_triple,       SIGNAL( currentIndexChanged( int ) ), 
                             SLOT  ( new_triple         ( int ) ) );
   connect( pb_exclusion,    SIGNAL( clicked() ), SLOT( exclusion()     ) );
   connect( pb_edit1,        SIGNAL( clicked() ), SLOT( edit_scan()     ) );
   connect( pb_include,      SIGNAL( clicked() ), SLOT( include()       ) );
   connect( pb_meniscus,     SIGNAL( clicked() ), SLOT( set_meniscus()  ) );
   connect( pb_airGap,       SIGNAL( clicked() ), SLOT( set_airGap()    ) );
   connect( pb_dataRange,    SIGNAL( clicked() ), SLOT( set_dataRange() ) );
   connect( pb_plateau,      SIGNAL( clicked() ), SLOT( set_plateau()   ) );
   connect( ct_odlim,        SIGNAL( valueChanged   ( double ) ),
                             SLOT  ( od_radius_limit( double ) ) );
   connect( pb_noise,        SIGNAL( clicked() ), SLOT( noise() ) );
   connect( pb_residuals,    SIGNAL( clicked() ),
                             SLOT  ( subtract_residuals() ) );
   connect( pb_invert,       SIGNAL( clicked() ), SLOT( invert_values() ) );
   connect( pb_spikes,       SIGNAL( clicked() ), SLOT( remove_spikes() ) );
   connect( pb_priorEdits,   SIGNAL( clicked() ), SLOT( apply_prior()   ) );
   connect( pb_undo,         SIGNAL( clicked() ), SLOT( undo()      ) );
   connect( pb_reviewep,     SIGNAL( clicked() ), SLOT( review_edits()  ) );
   connect( pb_nexttrip,     SIGNAL( clicked() ), SLOT( next_triple()   ) );
   connect( pb_float,        SIGNAL( clicked() ), SLOT( floating()  ) );
   connect( pb_write,        SIGNAL( clicked() ), SLOT( write()     ) );
   connect( pb_writemwl,     SIGNAL( clicked() ), SLOT( write_mwl() ) );

   // Lay out specs widgets and layouts
   int s_row = 0;
   specs->addWidget( pb_investigator, s_row,   0, 1, 1 );
   specs->addWidget( le_investigator, s_row++, 1, 1, 3 );
   specs->addLayout( disk_controls,   s_row++, 0, 1, 4 );
   specs->addWidget( pb_load,         s_row,   0, 1, 2 );
   specs->addWidget( pb_details,      s_row++, 2, 1, 2 );
   specs->addWidget( lb_triple,       s_row,   0, 1, 2 );
   specs->addWidget( cb_triple,       s_row++, 2, 1, 2 );
   specs->addWidget( lb_rpms,         s_row,   0, 1, 2 );
   specs->addWidget( cb_rpms,         s_row++, 2, 1, 2 );
   specs->addWidget( lb_gaps,         s_row,   0, 1, 2 );
   specs->addWidget( ct_gaps,         s_row++, 2, 1, 2 );
   specs->addWidget( le_lxrng,        s_row++, 0, 1, 4 );
   specs->addWidget( lb_mwlctl,       s_row++, 0, 1, 4 );
   specs->addLayout( lo_lrange,       s_row,   0, 1, 2 );
   specs->addLayout( lo_custom,       s_row++, 2, 1, 2 );
   specs->addWidget( lb_ldelta,       s_row,   0, 1, 1 );
   specs->addWidget( ct_ldelta,       s_row,   1, 1, 1 );
   specs->addWidget( le_ltrng,        s_row++, 2, 1, 2 );
   specs->addWidget( lb_lstart,       s_row,   0, 1, 1 );
   specs->addWidget( cb_lstart,       s_row,   1, 1, 1 );
   specs->addWidget( lb_lend,         s_row,   2, 1, 1 );
   specs->addWidget( cb_lend,         s_row++, 3, 1, 1 );
   specs->addWidget( pb_custom,       s_row,   0, 1, 2 );
   specs->addWidget( pb_incall,       s_row++, 2, 1, 2 );
   specs->addLayout( lo_radius,       s_row,   0, 1, 2 );
   specs->addLayout( lo_waveln,       s_row++, 2, 1, 2 );
   specs->addWidget( lb_lplot,        s_row,   0, 1, 1 );
   specs->addWidget( cb_lplot,        s_row,   1, 1, 1 );
   specs->addWidget( pb_larrow,       s_row,   2, 1, 1 );
   specs->addWidget( pb_rarrow,       s_row++, 3, 1, 1 );
   specs->addWidget( lb_scan,         s_row++, 0, 1, 4 );
   specs->addWidget( lb_from,         s_row,   0, 1, 2 );
   specs->addWidget( ct_from,         s_row++, 2, 1, 2 );
   specs->addWidget( lb_to,           s_row,   0, 1, 2 );
   specs->addWidget( ct_to,           s_row++, 2, 1, 2 );
   specs->addWidget( pb_excludeRange, s_row,   0, 1, 2 );
   specs->addWidget( pb_exclusion,    s_row++, 2, 1, 2 );
   specs->addWidget( pb_edit1,        s_row,   0, 1, 2 );
   specs->addWidget( pb_include,      s_row++, 2, 1, 2 );
   specs->addWidget( lb_edit,         s_row++, 0, 1, 4 );
   specs->addWidget( lb_edtrsp,       s_row,   0, 1, 2 );
   specs->addWidget( le_edtrsp,       s_row++, 2, 1, 2 );
   specs->addWidget( pb_meniscus,     s_row,   0, 1, 2 );
   specs->addWidget( le_meniscus,     s_row++, 2, 1, 2 );
   specs->addWidget( pb_airGap,       s_row,   0, 1, 2 );
   specs->addWidget( le_airGap,       s_row++, 2, 1, 2 );
   specs->addWidget( pb_dataRange,    s_row,   0, 1, 2 );
   specs->addWidget( le_dataRange,    s_row++, 2, 1, 2 );
   specs->addWidget( pb_plateau,      s_row,   0, 1, 2 );
   specs->addWidget( le_plateau,      s_row++, 2, 1, 2 );
   specs->addWidget( lb_baseline,     s_row,   0, 1, 2 );
   specs->addWidget( le_baseline,     s_row++, 2, 1, 2 );
   specs->addWidget( lb_odlim,        s_row,   0, 1, 2 );
   specs->addWidget( ct_odlim,        s_row++, 2, 1, 2 );
   specs->addWidget( pb_noise,        s_row,   0, 1, 2 );
   specs->addWidget( pb_residuals,    s_row++, 2, 1, 2 );
   specs->addWidget( pb_invert,       s_row,   0, 1, 2 );
   specs->addWidget( pb_spikes,       s_row++, 2, 1, 2 );
   specs->addWidget( pb_priorEdits,   s_row,   0, 1, 2 );
   specs->addWidget( pb_undo,         s_row++, 2, 1, 2 );
   specs->addWidget( pb_reviewep,     s_row,   0, 1, 2 );
   specs->addWidget( pb_nexttrip,     s_row++, 2, 1, 2 );
   specs->addWidget( pb_float,        s_row,   0, 1, 2 );
   specs->addWidget( pb_write,        s_row++, 2, 1, 2 );
   specs->addWidget( pb_writemwl,     s_row++, 2, 1, 2 );

   // Button rows
   QBoxLayout*  buttons   = new QHBoxLayout;
   QPushButton* pb_reset  = us_pushbutton( tr( "Reset" ) );
   QPushButton* pb_help   = us_pushbutton( tr( "Help" ) );
   QPushButton* pb_accept = us_pushbutton( tr( "Close" ) );

   connect( pb_reset,  SIGNAL( clicked() ), SLOT( reset() ) );
   connect( pb_help,   SIGNAL( clicked() ), SLOT( help()  ) );
   connect( pb_accept, SIGNAL( clicked() ), SLOT( close() ) );

   buttons->addWidget( pb_reset );
   buttons->addWidget( pb_help );
   buttons->addWidget( pb_accept );

   // Plot layout on right side of window
   plot = new US_Plot( data_plot, 
         tr( "Absorbance Data" ),
         tr( "Radius (in cm)" ), tr( "Absorbance" ) );
   
   data_plot->setMinimumSize( 600, 400 );

   data_plot->enableAxis( QwtPlot::xBottom, true );
   data_plot->enableAxis( QwtPlot::yLeft  , true );

   pick = new US_PlotPicker( data_plot );
   // Set rubber band to display for Control+Left Mouse Button
   pick->setRubberBand  ( QwtPicker::VLineRubberBand );
   pick->setMousePattern( QwtEventPattern::MouseSelect1,
                          Qt::LeftButton, Qt::ControlModifier );

   left->addLayout( specs );
   left->addStretch();
   left->addLayout( buttons );

   main->addLayout( left );
   main->addLayout( plot );
   main->setStretchFactor( left, 2 );
   main->setStretchFactor( plot, 3 );
   top ->addLayout( main );

   reset();
}

// Select DB investigator
void US_Edit::sel_investigator( void )
{
   int investigator = US_Settings::us_inv_ID();

   US_Investigator* dialog = new US_Investigator( true, investigator );
   dialog->exec();

   investigator = US_Settings::us_inv_ID();

   QString inv_text = QString::number( investigator ) + ": "
                      +  US_Settings::us_inv_name();
   
   le_investigator->setText( inv_text );
}

// Reset parameters to their defaults
void US_Edit::reset( void )
{
   changes_made = false;
   floatingData = false;

   step          = MENISCUS;
   meniscus      = 0.0;
   meniscus_left = 0.0;
   airGap_left   = 0.0;
   airGap_right  = 9.0;
   range_left    = 0.0;
   range_right   = 9.0;
   plateau       = 0.0;
   baseline      = 0.0;
   invert        = 1.0;  // Multiplier = 1.0 or -1.0
   noise_order   = 0;
   triple_index  = 0;
   data_index    = 0;
   isMwl         = false;
   xaxis_radius  = true;
   lsel_range    = true;

   le_info     ->setText( "" );
   le_meniscus ->setText( "" );
   le_airGap   ->setText( "" );
   le_dataRange->setText( "" );
   le_plateau  ->setText( "" );
   le_baseline ->setText( "" );

   lb_gaps->setText( tr( "Threshold for Scan Gaps" ) );
   ct_gaps->setValue( 50.0 );

   ct_from->disconnect();
   ct_from->setMinValue( 0 );
   ct_from->setMaxValue( 0 );
   ct_from->setValue   ( 0 );

   ct_to->disconnect();
   ct_to->setMinValue( 0 );
   ct_to->setMaxValue( 0 );
   ct_to->setValue   ( 0 );

   cb_triple->disconnect();

   data_plot->detachItems( QwtPlotItem::Rtti_PlotCurve );
   data_plot->detachItems( QwtPlotItem::Rtti_PlotMarker );
   v_line = NULL;
   pick     ->disconnect();

   data_plot->setAxisScale( QwtPlot::xBottom, 5.7, 7.3 );
   data_plot->setAxisScale( QwtPlot::yLeft  , 0.0, 1.5 );
   grid = us_grid( data_plot );
   data_plot->replot();

   // Disable pushbuttons
   pb_details     ->setEnabled( false );

   pb_excludeRange->setEnabled( false );
   pb_exclusion   ->setEnabled( false );
   pb_include     ->setEnabled( false );
   pb_edit1       ->setEnabled( false );
   
   pb_meniscus    ->setEnabled( false );
   pb_airGap      ->setEnabled( false );
   pb_dataRange   ->setEnabled( false );
   pb_plateau     ->setEnabled( false );
   
   pb_noise       ->setEnabled( false );
   pb_residuals   ->setEnabled( false );
   pb_spikes      ->setEnabled( false );
   pb_invert      ->setEnabled( false );
   pb_priorEdits  ->setEnabled( false );
   pb_reviewep    ->setEnabled( false );
   pb_nexttrip    ->setEnabled( false );
   pb_undo        ->setEnabled( false );
   
   pb_float       ->setEnabled( false );
   pb_write       ->setEnabled( false );
   pb_writemwl    ->setEnabled( false );

   // Remove icons
   pb_meniscus    ->setIcon( QIcon() );
   pb_airGap      ->setIcon( QIcon() );
   pb_dataRange   ->setIcon( QIcon() );
   pb_plateau     ->setIcon( QIcon() );
   pb_noise       ->setIcon( QIcon() );
   pb_residuals   ->setIcon( QIcon() );
   pb_spikes      ->setIcon( QIcon() );
   pb_invert      ->setIcon( QIcon() );

   pb_float       ->setIcon( QIcon() );
   pb_write       ->setIcon( QIcon() );
   pb_writemwl    ->setIcon( QIcon() );

   editLabel     .clear();
   data.scanData .clear();
   includes      .clear();
   changed_points.clear();
   trip_rpms     .clear();
   triples       .clear();
   cb_triple    ->clear();
   cb_rpms      ->disconnect();
   cb_rpms      ->clear();
   editGUIDs     .clear();
   editIDs       .clear();
   editFnames    .clear();
   files         .clear();
   expd_radii    .clear();
   expi_wvlns    .clear();
   rawi_wvlns    .clear();
   celchns       .clear();
   rawc_wvlns    .clear();
   expc_wvlns    .clear();
   expc_radii    .clear();
   connect_mwl_ctrls( false );
   rb_lrange    ->setChecked( true );
   rb_custom    ->setChecked( false );
   cb_lplot     ->clear();
   cb_lstart    ->clear();
   cb_lend      ->clear();
   ct_ldelta    ->setEnabled( true );
   cb_lend      ->setEnabled( true );
   rb_radius    ->setChecked( true );
   rb_waveln    ->setChecked( false );
   pb_custom    ->setEnabled( false );
   connect_mwl_ctrls( true );

   set_pbColors( NULL );
   lb_triple->setText( tr( "Cell / Channel / Wavelength" ) );

   show_mwl_controls( false );
}

// Reset parameters for a new triple
void US_Edit::reset_triple( void )
{
   step          = MENISCUS;
   meniscus      = 0.0;
   meniscus_left = 0.0;
   airGap_left   = 0.0;
   airGap_right  = 9.0;
   range_left    = 0.0;
   range_right   = 9.0;
   plateau       = 0.0;
   baseline      = 0.0;
   invert        = 1.0;  // Multiplier = 1.0 or -1.0
   noise_order   = 0;

   le_info     ->setText( "" );
   le_meniscus ->setText( "" );
   le_airGap   ->setText( "" );
   le_dataRange->setText( "" );
   le_plateau  ->setText( "" );
   le_baseline ->setText( "" );

   if ( dataType == "IP" )
      ct_gaps->setValue( 0.4 );
   else
      ct_gaps->setValue( 50.0 );

   ct_from->disconnect();
   ct_from->setMinValue( 0 );
   ct_from->setMaxValue( 0 );
   ct_from->setValue   ( 0 );

   ct_to->disconnect();
   ct_to->setMinValue( 0 );
   ct_to->setMaxValue( 0 );
   ct_to->setValue   ( 0 );

   data.scanData .clear();
   includes      .clear();
   changed_points.clear();
   trip_rpms     .clear();

   cb_triple    ->disconnect();
   cb_rpms      ->disconnect();
   cb_rpms      ->clear();
}

// Display run details
void US_Edit::details( void )
{  
   US_RunDetails2* dialog 
      = new US_RunDetails2( allData, runID, workingDir, triples );
   dialog->exec();
   qApp->processEvents();
   delete dialog;
}

// Do a Gap check against threshold value
void US_Edit::gap_check( void )
{
   int threshold = (int)ct_gaps->value();
            
   US_DataIO::Scan  s;
   QString          gaps;

   int              scanNumber    = 0;
   int              rawScanNumber = -1;
   bool             deleteAll     = false;

   foreach ( s, data.scanData )
   {
      rawScanNumber++;
      // If scan has been deleted, skip to next
      if ( ! includes.contains( scanNumber ) ) continue;

      int maxGap     = 0;
      int gapLength  = 0;
      int location   = 0;

      int leftPoint  = data.xindex( range_left  );
      int rightPoint = data.xindex( range_right );

      for ( int i = leftPoint; i <= rightPoint; i++ )
      {
         int byte = i / 8;
         int bit  = i % 8;

         if ( s.interpolated[ byte ]  &  1 << ( 7 - bit ) ) 
           gapLength++;
         else
           gapLength = 0;

         if ( gapLength > maxGap )
         {
            maxGap   = gapLength;
            location = i;
         }
      }

      if ( maxGap >= threshold )
      { 
         QwtPlotCurve* curve         = NULL;
         bool          deleteCurrent = false;

         // Hightlight scan
         ct_to->setValue( 0.0 );  // Unfocus everything

         QString         seconds = QString::number( s.seconds );
         QwtPlotItemList items   = data_plot->itemList();
         
         for ( int i = 0; i < items.size(); i++ )
         {
            if ( items[ i ]->rtti() == QwtPlotItem::Rtti_PlotCurve )
            {
               if ( items[ i ]->title().text().contains( seconds ) )
               {
                  curve = dynamic_cast< QwtPlotCurve* >( items[ i ] );
                  break;
               }
            }
         }

         if ( curve == NULL )
         {
            qDebug() << "Cannot find curve during gap check";
            return;
         }

         // Popup unless delete all is set
         if ( ! deleteAll )
         {
            // Color the selected point
            QPen   p = curve->pen();
            QBrush b = curve->brush();

            p.setColor( Qt::red );
            b.setColor( Qt::red );

            curve->setPen  ( p );
            curve->setBrush( b );
            data_plot->replot();

            // Ask the user what to do
            QMessageBox box;

            box.setWindowTitle( tr( "Excessive Scan Gaps Detected" ) );
            
            double radius = data.xvalues[ 0 ] + location * s.delta_r;
            
            gaps = tr( "Scan " ) 
                 + QString::number( rawScanNumber ) 
                 + tr( " has a maximum reading gap of " ) 
                 + QString::number( maxGap ) 
                 + tr( " starting at radius " ) 
                 + QString::number( radius, 'f', 3 );

            box.setText( gaps );
            box.setInformativeText( tr( "Delete?" ) );

            QPushButton* pb_delete = box.addButton( tr( "Delete" ), 
                  QMessageBox::YesRole );
         
            QPushButton* pb_deleteAll = box.addButton( tr( "Delete All" ), 
                  QMessageBox::AcceptRole );
         
            QPushButton* pb_skip = box.addButton( tr( "Skip" ), 
                  QMessageBox::NoRole );
         
            QPushButton* pb_cancel = box.addButton( tr( "Cancel" ), 
                  QMessageBox::RejectRole );
         
            box.setEscapeButton ( pb_cancel );
            box.setDefaultButton( pb_delete );

            box.exec();

            if ( box.clickedButton() == pb_delete )
               deleteCurrent = true;

            else if ( box.clickedButton() == pb_deleteAll )
            {
               deleteAll     = true;
               deleteCurrent = true;
            }

            else 
            {
               // Uncolor scan
               p.setColor( Qt::yellow );
               b.setColor( Qt::yellow );

               curve->setPen  ( p );
               curve->setBrush( b );
               data_plot->replot();
            }

            if ( box.clickedButton() == pb_skip )
               continue;
            
            if ( box.clickedButton() == pb_cancel )
               return;
         }

         // Delete the scan
         if ( deleteAll || deleteCurrent )
         {
            includes.removeOne( scanNumber );
            replot();

            ct_to  ->setMaxValue( includes.size() );
            ct_from->setMaxValue( includes.size() );
         }
      }
                             
      scanNumber++;
   }
}

// Load an AUC data set
void US_Edit::load( void )
{
   bool isLocal = ! disk_controls->db();
   reset();

   US_LoadAUC* dialog =
      new US_LoadAUC( isLocal, allData, triples, workingDir );

   connect( dialog, SIGNAL( progress      ( QString ) ),
            this,   SLOT  ( progress_load ( QString ) ) );
   connect( dialog, SIGNAL( changed       ( bool )    ),
            this,   SLOT  ( update_disk_db( bool )    ) );

   if ( dialog->exec() == QDialog::Rejected )  return;

   runID = workingDir.section( "/", -1, -1 );
   sData     .clear();
   sd_offs   .clear();
   sd_knts   .clear();
   cb_triple->clear();
   files     .clear();
   delete dialog;

   if ( triples.size() == 0 )
   {
      QMessageBox::warning( this,
            tr( "No Files Found" ),
            tr( "There were no files of the form *.auc\n"  
                "found in the specified directory." ) );
      return;
   }

   cb_triple->addItems( triples );
   connect( cb_triple, SIGNAL( currentIndexChanged( int ) ), 
                       SLOT  ( new_triple         ( int ) ) );
   triple_index = 0;
   data_index   = 0;
   
   le_info->setText( runID );

   data     = allData[ 0 ];
   dataType = QString( QChar( data.type[ 0 ] ) ) 
            + QString( QChar( data.type[ 1 ] ) );

   if ( dataType == "IP" )
   {
      lb_gaps->setText( tr( "Fringe Tolerance" ) );

      ct_gaps->setRange     ( 0.0, 20.0, 0.001 );
      ct_gaps->setValue     ( 0.4 );
      ct_gaps->setNumButtons( 3 );

      connect( ct_gaps, SIGNAL( valueChanged        ( double ) ), 
                        SLOT  ( set_fringe_tolerance( double ) ) );
   }
   else
   {
      lb_gaps->setText( tr( "Threshold for Scan Gaps" ) );
      
      ct_gaps->disconnect   ();
      ct_gaps->setRange     ( 10.0, 100.0, 10.0 );
      ct_gaps->setValue     ( 50.0 );
      ct_gaps->setNumButtons( 1 );
   }

   QString runtype = runID + "." + dataType;
   nwaveln         = 0;
   ncelchn         = 0;
   outData.clear();

   for ( int trx = 0; trx < triples.size(); trx++ )
   {  // Generate file names
      QString triple = QString( triples.at( trx ) ).replace( " / ", "." );
      QString file   = runtype + "." + triple + ".auc";
      files << file;

      // Save pointers as initial output data vector
      outData << &allData[ trx ];

      QString scell  = triple.section( ".", 0, 0 ).simplified();
      QString schan  = triple.section( ".", 1, 1 ).simplified();
      QString swavl  = triple.section( ".", 2, 2 ).simplified();

      if ( ! rawc_wvlns.contains( swavl ) )
      {  // Accumulate wavelengths in case this is MWL
         nwaveln++;
         rawc_wvlns << swavl;
      }

      nwavelo         = nwaveln;
      QString celchn  = scell + " / " + schan;

      if ( ! celchns.contains( celchn ) )
      {  // Accumulate cell/channel values in case this is MWL
         ncelchn++;
         celchns << celchn;
      }
   }
DbgLv(1) << "rawc_wvlns size" << rawc_wvlns.size() << nwaveln;
DbgLv(1) << " celchns    size" << celchns.size() << ncelchn;
   rawc_wvlns.sort();
   for ( int wvx = 0; wvx < nwaveln; wvx++ )
      rawi_wvlns << rawc_wvlns[ wvx ].toInt();

   workingDir   = workingDir + "/";
   QString file = workingDir + runtype + ".xml";
   expType      = "";
   QFile xf( file );

   if ( xf.open( QIODevice::ReadOnly | QIODevice::Text ) )
   {
      QXmlStreamReader xml( &xf );

      while( ! xml.atEnd() )
      {
         xml.readNext();

         if ( xml.isStartElement()  &&  xml.name() == "experiment" )
         {
            QXmlStreamAttributes xa = xml.attributes();
            expType   = xa.value( "type" ).toString();
            break;
         }
      }

      xf.close();
   }

   if ( expType.isEmpty()  &&  disk_controls->db() )
   {  // no experiment type yet and data read from DB:  try for DB exp type
      if ( dbP == NULL )
      {
         US_Passwd pw;
         dbP          = new US_DB2( pw.getPasswd() );

         if ( dbP == NULL  ||  dbP->lastErrno() != US_DB2::OK )
         {
            QMessageBox::warning( this, tr( "Connection Problem" ),
              tr( "Could not connect to database\n" )
              + ( ( dbP != NULL ) ? dbP->lastError() : "" ) );
            dbP          = NULL;
            return;
         }
      }

      QStringList query;
      query << "get_experiment_info_by_runID" << runID 
            << QString::number( US_Settings::us_inv_ID() );

      dbP->query( query );
      dbP->next();
      expType    = dbP->value( 8 ).toString();
   }

   if ( expType.isEmpty() )  // if no experiment type, assume Velocity
      expType    = "Velocity";

   else                      // insure Ulll... form, e.g., "Equilibrium"
      expType    = expType.left( 1 ).toUpper() +
                   expType.mid(  1 ).toLower();


   // Set booleans for experiment type
   expIsVelo  = ( expType.compare( "Velocity",    Qt::CaseInsensitive ) == 0 );
   expIsEquil = ( expType.compare( "Equilibrium", Qt::CaseInsensitive ) == 0 );
   expIsDiff  = ( expType.compare( "Diffusion",   Qt::CaseInsensitive ) == 0 );
   expIsOther = ( !expIsVelo  &&  !expIsEquil  &&  !expIsDiff );
   expType    = expIsOther ? "Other" : expType;
   odlimit    = 1.5;
   init_includes();

   if ( expIsEquil )
   {  // Equilibrium
      lb_rpms    ->setVisible( true  );
      cb_rpms    ->setVisible( true  );
      pb_plateau ->setVisible( false );
      le_plateau ->setVisible( false ); 
      lb_baseline->setVisible( false ); 
      le_baseline->setVisible( false ); 
      lb_edtrsp  ->setVisible( true  );
      le_edtrsp  ->setVisible( true  );
      pb_reviewep->setVisible( true  );
      pb_nexttrip->setVisible( true  );
      pb_write   ->setText( tr( "Save Edit Profiles" ) );

      sData.clear();
      US_DataIO::SpeedData  ssDat;
      int ksd    = 0;

      for ( int jd = 0; jd < allData.size(); jd++ )
      {
         data  = allData[ jd ];
         sd_offs << ksd;

         if ( jd > 0 )
            sd_knts << ( ksd - sd_offs[ jd - 1 ] );

         trip_rpms.clear();

         for ( int ii = 0; ii < data.scanData.size(); ii++ )
         {
            double  drpm = data.scanData[ ii ].rpm;
            QString arpm = QString::number( drpm );
            if ( ! trip_rpms.contains( arpm ) )
            {
               trip_rpms << arpm;
               ssDat.first_scan = ii + 1;
               ssDat.scan_count = 1;
               ssDat.speed      = drpm;
               ssDat.meniscus   = 0.0;
               ssDat.dataLeft   = 0.0;
               ssDat.dataRight  = 0.0;
               sData << ssDat;
               ksd++;
            }

            else
            {
               int jj = trip_rpms.indexOf( arpm );
               ssDat  = sData[ jj ];
               ssDat.scan_count++;
               sData[ jj ].scan_count++;
            }
         }

         if ( jd == 0 )
            cb_rpms->addItems( trip_rpms );

         total_speeds += trip_rpms.size();
      }

      sd_knts << ( ksd - sd_offs[ allData.size() - 1 ] );

      if ( allData.size() > 1 )
      {
         data   = allData[ 0 ];
         ksd    = sd_knts[ 0 ];
         trip_rpms.clear();
         cb_rpms ->clear();
         for ( int ii = 0; ii < ksd; ii++ )
         {
            QString arpm = QString::number( sData[ ii ].speed );
            trip_rpms << arpm;
         }

         cb_rpms->addItems( trip_rpms );
      }

      pick     ->disconnect();
      connect( pick, SIGNAL( cMouseUp( const QwtDoublePoint& ) ),
                     SLOT  ( mouse   ( const QwtDoublePoint& ) ) );

      pb_priorEdits->disconnect();
      connect( pb_priorEdits, SIGNAL( clicked() ), SLOT( prior_equil() ) );
      plot_scan();

      connect( cb_rpms,   SIGNAL( currentIndexChanged( int ) ), 
                          SLOT  ( new_rpmval         ( int ) ) );
   }

   else
   {  // non-Equilibrium
      bool notMwl  = ( nwaveln < 3 );
      lb_rpms    ->setVisible( false );
      cb_rpms    ->setVisible( false );
      pb_plateau ->setVisible( true  );
      le_plateau ->setVisible( true  ); 
      lb_baseline->setVisible( notMwl );
      le_baseline->setVisible( notMwl ); 
      lb_edtrsp  ->setVisible( false );
      le_edtrsp  ->setVisible( false );
      pb_reviewep->setVisible( false );
      pb_nexttrip->setVisible( false );

      pb_write   ->setText( tr( "Save Current Edit Profile" ) );

      pb_priorEdits->disconnect();
      connect( pb_priorEdits, SIGNAL( clicked() ), SLOT( apply_prior() ) );
DbgLv(1) << "LD():  triples size" << triples.size();
      if ( notMwl )
         plot_current( 0 );
   }

   // Enable pushbuttons
   pb_details   ->setEnabled( true );
   pb_include   ->setEnabled( true );
   pb_exclusion ->setEnabled( true );
   pb_meniscus  ->setEnabled( true );
   pb_airGap    ->setEnabled( false );
   pb_dataRange ->setEnabled( false );
   pb_plateau   ->setEnabled( false );
   pb_noise     ->setEnabled( false );
   pb_spikes    ->setEnabled( false );
   pb_invert    ->setEnabled( true );
   pb_priorEdits->setEnabled( true );
   pb_float     ->setEnabled( true );
   pb_undo      ->setEnabled( true );

   connect( ct_from, SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_from   ( double ) ) );

   connect( ct_to,   SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_to     ( double ) ) );

   step = MENISCUS;
   set_pbColors( pb_meniscus );

   // Temperature check
   double              dt = 0.0;
   US_DataIO::RawData  triple;

   foreach( triple, allData )
   {
       double temp_spread = triple.temperature_spread();
       dt = ( temp_spread > dt ) ? temp_spread : dt;
   }

   if ( dt > US_Settings::tempTolerance() )
   {
      QMessageBox::warning( this,
            tr( "Temperature Problem" ),
            tr( "The temperature in this run varied over the course\n"
                "of the run to a larger extent than allowed by the\n"
                "current threshold (" )
                + QString::number( US_Settings::tempTolerance(), 'f', 1 )
                + " " + DEGC + tr( ". The accuracy of experimental\n"
                "results may be affected significantly." ) );
   }

   ntriple      = triples.size();
DbgLv(1) << " triples    size" << ntriple;
   editGUIDs .fill( "",     ntriple );
   editIDs   .fill( "",     ntriple );
   editFnames.fill( "none", ntriple );

   isMwl        = ( nwaveln > 2 );
DbgLv(1) << "LD(): nwaveln isMwl" << nwaveln << isMwl << rawi_wvlns.size();

   if ( isMwl )
   {  // Set values related to MultiWaveLength
      const QChar chlamb( 955 );
      connect_mwl_ctrls( false );
DbgLv(1) << "IS-MWL: wvlns size" << rawc_wvlns.size();
      nwavelo      = nwaveln;
      slambda      = rawi_wvlns[ 0 ];
      elambda      = rawi_wvlns[ nwaveln - 1 ];
      dlambda      = 1;
      le_ltrng ->setText( tr( "%1 raw: %2 %3 to %4" )
         .arg( nwaveln ).arg( chlamb ).arg( slambda ).arg( elambda ) );
      le_lxrng ->setText( tr( "%1 MWL exports: %2 %3 to %4,"
                              " raw index increment %5" )
         .arg( nwavelo ).arg( chlamb ).arg( slambda ).arg( elambda )
         .arg( dlambda ) );

      // Load the internal object that keeps track of MWL data
      mwl_data.load_mwl( allData );
DbgLv(1) << "IS-MWL:   load_mwl complete";

      expd_radii .clear();
      expc_wvlns .clear();
      nwavelo      = mwl_data.lambdas( expi_wvlns, 0 );
DbgLv(1) << "IS-MWL:    new nwavelo" << nwavelo << expi_wvlns.count();

      // Initialize export wavelength lists for first channel
      for ( int wvx = 0; wvx < nwavelo; wvx++ )
      {
         expc_wvlns << QString::number( expi_wvlns[ wvx ] );
      }

      // Update wavelength lists in GUI elements
      cb_lstart->clear();
      cb_lend  ->clear();
      cb_lplot ->clear();
      cb_lstart->addItems( expc_wvlns );
      cb_lend  ->addItems( expc_wvlns );
      cb_lplot ->addItems( expc_wvlns );
      int lastx    = nwavelo - 1;
      plotndx      = nwavelo / 2;
      cb_lplot ->setCurrentIndex( plotndx );
      cb_lstart->setCurrentIndex( 0 );
      cb_lend  ->setCurrentIndex( lastx );
DbgLv(1) << "IS-MWL:  expi_wvlns size" << expi_wvlns.size() << nwaveln;

      edata         = &allData[ 0 ];
      nrpoint       = edata->pointCount();
      int nscan     = edata->scanCount();
      int ndset     = ncelchn * nrpoint;
      int ndpoint   = nscan * nwavelo;
DbgLv(1) << "IS-MWL:   nrpoint nscan ndset ndpoint" << nrpoint << nscan
 << ndset << ndpoint;

      for ( int ii = 0; ii < nrpoint; ii++ )
      {  // Update the list of radii that may be plotted
         expd_radii << data.xvalues[ ii ];
         expc_radii << QString().sprintf( "%.3f", data.xvalues[ ii ] );
      }
DbgLv(1) << "IS-MWL:  expd_radii size" << expd_radii.size() << nrpoint;

      QVector< double > wrdata;
      wrdata.fill( 0.0, ndpoint );
      rdata .clear();
DbgLv(1) << "IS-MWL:  wrdata size" << wrdata.size() << ndpoint;

      for ( int ii = 0; ii < ndset; ii++ )
      {  // Initialize the data vector that has wavelength as the x-axis
         rdata << wrdata;
      }
DbgLv(1) << "IS-MWL:  rdata size" << rdata.size() << ndset;

      // Update wavelength-x-axis data vector with amplitude data points
      // The input has (ncelchn * nwaveln) data sets, each of which
      //   contains (nscan * nrpoint) data points.
      // The output has (ncelchn * nrpoint) data sets, each of which
      //   contains (nscan * nwaveln) data points.
      for ( int trx = 0; trx < ntriple; trx++ )
      {  // Handle each triple of AUC data
         edata         = &allData[ trx ];
         int ccx       = trx / nwaveln;
         int wvx       = trx - ccx * nwaveln;
DbgLv(1) << "IS-MWL:   trx ccx wvx" << trx << ccx << wvx;

         for ( int scx = 0; scx < nscan; scx++ )
         {  // Handle each scan of a triple
            US_DataIO::Scan* scan  = &edata->scanData[ scx ];
            int    odx    = ccx * nrpoint;         // Output dataset index
            int    opx    = scx * nwaveln + wvx;   // Output point index
//DbgLv(1) << "IS-MWL:    scx odx opx" << scx << odx << opx;
            for ( int rax = 0; rax < nrpoint; rax++ )
            {  // Store each radius data point as a wavelength point in a scan
               rdata[ odx++ ][ opx ]  = scan->rvalues[ rax ];
            } // END: radius points loop
         } // END: scans loop
      } // END: input triples loop
DbgLv(1) << "IS-MWL:    Triples loop complete";

DbgLv(1) << "IS-MWL: celchns size" << celchns.size();
      lb_triple->setText( tr( "Cell / Channel" ) );
      cb_triple->disconnect();
      cb_triple->clear();
      cb_triple->addItems( celchns );
      connect( cb_triple, SIGNAL( currentIndexChanged( int ) ), 
                          SLOT  ( new_triple         ( int ) ) );

      odlimit   = 0.8;

      connect_mwl_ctrls( true );

      plot_mwl();
   } // END: isMwl=true
//*DEBUG* Print times,omega^ts
else
{
 triple = allData[0];
 double timel = triple.scanData[0].rpm / 400.0;
 double rpmc  = 400.0;
 int    nstep = (int)timel;
 double w2ti  = 0.0;
 for ( int ii=0; ii<nstep; ii++ )
 {
  w2ti += sq( rpmc * M_PI / 30.0 );
  rpmc += 400.0;
 }
 for ( int ii=0; ii<triple.scanData.size(); ii++ )
 {
  US_DataIO::Scan* ds=&triple.scanData[ii];
  double timec = ds->seconds;
  double rpmc  = ds->rpm;
  w2ti += ( timec - timel ) * sq( rpmc * M_PI / 30.0 );
  qDebug() << "scan" << ii+1 << "delta-r rpm seconds" << ds->delta_r
   << rpmc << timec << "omega2t w2t-integ" << ds->omega2t << w2ti;
  if(ii==0)
  {
   double deltt = ds->omega2t / sq(rpmc*M_PI/30.0);
   double time1 = timel + deltt;
   qDebug() << "   scan 1 omega2t-implied time" << time1;
  }
  timel = timec;
 }
}
//*DEBUG* Print times,omega^ts

   ct_odlim->disconnect();
   ct_odlim->setValue( odlimit );
   connect( ct_odlim,  SIGNAL( valueChanged       ( double ) ),
            this,      SLOT  ( od_radius_limit    ( double ) ) );

   show_mwl_controls( isMwl );
}

// Set pushbutton colors
void US_Edit::set_pbColors( QPushButton* pb )
{
   QPalette p = US_GuiSettings::pushbColor();
   
   pb_meniscus ->setPalette( p );
   pb_airGap   ->setPalette( p );
   pb_dataRange->setPalette( p );
   pb_plateau  ->setPalette( p );

   if ( pb != NULL )
   {
      p.setColor( QPalette::Button, Qt::green );
      pb->setPalette( p );
   }
}

// Plot the current data set
void US_Edit::plot_current( int index )
{
   if ( isMwl )
   {
      plot_mwl();
      return;
   }

   // Read the data
   QString     triple  = triples.at( index );
   QStringList parts   = triple.split( " / " );

   QString     cell    = parts[ 0 ];
   QString     channel = parts[ 1 ];
   QString     wl      = parts[ 2 ];

   QString     desc    = data.description;

   dataType = QString( QChar( data.type[ 0 ] ) ) 
            + QString( QChar( data.type[ 1 ] ) );

   if ( dataType == "IP" )
   {
      QStringList sl = desc.split( "," );
      sl.removeFirst();
      desc = sl.join( "," ).trimmed();
   }

   le_info->setText( runID + "  (" + desc + ")" );

   // Plot Title
   QString title23 = tr( "Run ID: %1\n"
                         "Cell: %2  Channel: %3  Wavelength: %4" )
      .arg( runID ).arg( cell ).arg( channel ).arg( wl );
   QString title;

   if ( dataType == "RA" )
   {
      title = "Radial Absorbance Data\n" + title23;
   }
   else if ( dataType == "RI" )
   {
      title = "Radial Intensity Data\n" + title23;
      data_plot->setAxisTitle( QwtPlot::yLeft, tr( "Intensity " ) );
   }
   else if ( dataType == "IP" )
   {

      title = "Radial Interference Data\n" + title23;
      data_plot->setAxisTitle( QwtPlot::yLeft, tr( "Fringes " ) );

      // Enable Air Gap
      pb_airGap->setHidden( false );
      le_airGap->setHidden( false );
   }
   else if ( dataType == "FI" )
   {
      title = "Fluorescence Intensity Data\n" + title23;
      data_plot->setAxisTitle( QwtPlot::yLeft, tr( "Fluorescence Intensity " ) );
   }
   else 
      title = "File type not recognized";

   data_plot->setTitle( title );

   // Initialize include list
   init_includes();

   // Plot current data for cell / channel / wavelength triple
   plot_all();

   // Set the Scan spin boxes
   ct_from->setMinValue( 0.0 );
   ct_from->setMaxValue(  data.scanData.size() );

   ct_to  ->setMinValue( 0.0 );
   ct_to  ->setMaxValue(  data.scanData.size() );

   pick     ->disconnect();
   connect( pick, SIGNAL( cMouseUp( const QwtDoublePoint& ) ),
                  SLOT  ( mouse   ( const QwtDoublePoint& ) ) );
}

// Re-plot
void US_Edit::replot( void )
{
   switch( step )
   {
      case FINISHED:
      case PLATEAU:
         plot_range();
         break;

      case BASELINE:
         plot_last();
         break;

      default:   // MENISCUS, AIRGAP, RANGE
         plot_all();
         break;
   }
}

// Handle a mouse click according to the current pick step
void US_Edit::mouse( const QwtDoublePoint& p )
{
   double maximum = -1.0e99;

   switch ( step )
   {
      case MENISCUS:
         if ( dataType == "IP" )
         {
            meniscus = p.x();
            // Un-zoom
            if ( plot->btnZoom->isChecked() )
               plot->btnZoom->setChecked( false );

            draw_vline( meniscus );
         }

         else if ( expIsEquil || men_1click )
         {  // Equilibrium
            meniscus_left = p.x();
            int ii        = data.xindex( meniscus_left );
            draw_vline( meniscus_left );
            meniscus      = data.radius( ii );
            le_meniscus->setText( QString::number( meniscus, 'f', 3 ) );

            data_plot->replot();

            pb_meniscus->setIcon( check );
         
            pb_dataRange->setEnabled( true );
        
            next_step();
            break;
         }

         else if ( meniscus_left == 0.0  )
         {
            meniscus_left = p.x();
            draw_vline( meniscus_left );
            break;
         }
         else
         {
            // Sometime we get two clicks
            if ( fabs( p.x() - meniscus_left ) < 0.005 ) return;

            double meniscus_right = p.x();
            
            // Swap values if necessary.  Use a macro in us_math.h
            if ( meniscus_right < meniscus_left )
               swap_double( meniscus_left, meniscus_right );

            // Find the radius for the max value
            maximum = -1.0e99;
            US_DataIO::Scan* s;

            for ( int i = 0; i < data.scanData.size(); i++ )
            {
               if ( ! includes.contains( i ) ) continue;

               s         = &data.scanData[ i ];
               int start = data.xindex( meniscus_left  );
               int end   = data.xindex( meniscus_right );

               for ( int j = start; j <= end; j++ )
               {
                  if ( maximum < s->rvalues[ j ] )
                  {
                     maximum  = s->rvalues[ j ];
                     meniscus = data.radius( j );
                  }
               }
            }

            // Remove the left line
            if ( v_line != NULL )
            {
               v_line->detach();
               delete v_line;
               v_line = NULL;
            }
         }

         // Display the value
         le_meniscus->setText( QString::number( meniscus, 'f', 3 ) );

         // Create a marker
         if ( dataType != "IP" )
         {
            marker = new QwtPlotMarker;
            QBrush brush( Qt::white );
            QPen   pen  ( brush, 2.0 );
            
            marker->setValue( meniscus, maximum );
            marker->setSymbol( QwtSymbol( 
                        QwtSymbol::Cross, 
                        brush,
                        pen,
                        QSize ( 8, 8 ) ) );

            marker->attach( data_plot );
         }

         data_plot->replot();

         pb_meniscus->setIcon( check );
         
         if ( dataType == "IP" )
            pb_airGap->setEnabled( true );
         else
            pb_dataRange->setEnabled( true );
        
         next_step();
         break;

      case AIRGAP:
         if ( airGap_left == 0.0 )
         {
            airGap_left = p.x();
            draw_vline( airGap_left );
         }
         else
         {
            // Sometime we get two clicks
            if ( fabs( p.x() - airGap_left ) < 0.020 ) return;

            airGap_right = p.x();

            if ( airGap_right < airGap_left ) 
               swap_double( airGap_left, airGap_right );

            US_DataIO::EditValues  edits;
            edits.airGapLeft  = airGap_left;
            edits.airGapRight = airGap_right;

            QList< int > excludes;
            
            for ( int i = 0; i < data.scanData.size(); i++ )
               if ( ! includes.contains( i ) ) edits.excludes << i;

DbgLv(1) << "AGap: L R" << airGap_left << airGap_right << " AdjIntf";
            US_DataIO::adjust_interference( data, edits );

            // Un-zoom
            if ( plot->btnZoom->isChecked() )
               plot->btnZoom->setChecked( false );

            // Display the data
            QString wkstr;
            le_airGap->setText( wkstr.sprintf( "%.3f - %.3f", 
                     airGap_left, airGap_right ) );

            step = RANGE;
DbgLv(1) << "AGap:  plot_range()";
            plot_range();

            qApp->processEvents();
            
            pb_airGap   ->setIcon( check );
            pb_dataRange->setEnabled( true );

            next_step();
         }

         break;

      case RANGE:
         if ( range_left == 0.0 )
         {
            if ( v_line != NULL )
            {
               v_line->detach();
               delete v_line;
               v_line = NULL;
            }
            range_left = p.x();
            draw_vline( range_left );
            break;
         }
         else
         {
            // Sometime we get two clicks
            if ( fabs( p.x() - range_left ) < 0.020 ) return;

            range_right = p.x();

            if ( range_right < range_left )
               swap_double( range_left, range_right );

            if ( dataType == "IP" )
            {
               US_DataIO::EditValues  edits;
               edits.rangeLeft    = range_left;
               edits.rangeRight   = range_right;
               edits.gapTolerance = ct_gaps->value();

               QList< int > excludes;
               
               for ( int i = 0; i < data.scanData.size(); i++ )
                  if ( ! includes.contains( i ) ) edits.excludes << i;
            
               US_DataIO::calc_integral( data, edits );
            }
            
            // Display the data
            QString wkstr;
            le_dataRange->setText( wkstr.sprintf( "%.3f - %.3f", 
                     range_left, range_right ) );

            step = PLATEAU;
            plot_range();

            qApp->processEvents();

            // Skip the gap check for interference data
            if ( dataType != "IP" ) gap_check();
            
            pb_dataRange->setIcon( check );
            pb_plateau  ->setEnabled( true );
            pb_noise    ->setEnabled( true );
            pb_spikes   ->setEnabled( true );

            if ( ! expIsEquil )
            {  // non-Equilibrium
               next_step();
            }

            else
            {  // Equilibrium

               int index    = cb_triple->currentIndex();
               int row      = cb_rpms  ->currentIndex();
               int jsd      = sd_offs[ index ] + row;
               QString arpm = cb_rpms->itemText( row );

               if ( sData[ jsd ].meniscus == 0.0  &&
                     meniscus > 0.0 )
                  total_edits++;

               sData[ jsd ].meniscus  = meniscus;
               sData[ jsd ].dataLeft  = range_left;
               sData[ jsd ].dataRight = range_right;

               if ( ++row >= cb_rpms->count() )
               {
                  if ( ++index < cb_triple->count() )
                  {
                     cb_triple->setCurrentIndex( index );
                     step         = MENISCUS;
                     QString trsp =
                        cb_triple->currentText() + " : " + trip_rpms[ 0 ];
                     le_edtrsp->setText( trsp );

                     data = *outData[ index ];
                     cb_rpms->clear();

                     for ( int ii = 0; ii < data.scanData.size(); ii++ )
                     {
                        QString arpm =
                           QString::number( data.scanData[ ii ].rpm );

                        if ( ! trip_rpms.contains( arpm ) )
                           trip_rpms << arpm;
                     }

                     cb_rpms->addItems( trip_rpms );
                     cb_rpms->setCurrentIndex( 0 );

                     set_meniscus();
                  }

                  else
                  {
                     pb_write   ->setEnabled( true );
                     pb_writemwl->setEnabled( true );
                     pb_reviewep->setEnabled( true );
                     pb_nexttrip->setEnabled( true );
                     step         = FINISHED;
                     next_step();

                     if ( total_edits >= total_speeds )
                        review_edits();
                  }
               }

               else
               {
                  cb_rpms->setCurrentIndex( row );
                  step         = MENISCUS;
                  QString trsp =
                     cb_triple->currentText() + " : " + trip_rpms[ row ];
                  le_edtrsp->setText( trsp );

                  set_meniscus();
               }
            }

            if ( total_edits >= total_speeds )
            {
               all_edits = true;
               pb_write   ->setEnabled( true );
               pb_writemwl->setEnabled( true );
               changes_made = all_edits;
            }
         }

         break;

      case PLATEAU:

         plateau = p.x();

         // Display the data (localize str)
         {
            QString wkstr;
            le_plateau->setText( wkstr.sprintf( "%.3f", plateau ) );
         }

         plot_range();
         pb_plateau ->setIcon( check );
         ct_to->setValue( 0.0 );  // Uncolor all scans
         pb_write      ->setEnabled( true );
         pb_writemwl   ->setEnabled( isMwl );
         changes_made = true;
         next_step();
         break;

      case BASELINE:
         {
            // Use the last scan
            US_DataIO::Scan* scan = &data.scanData.last();
            
            int start = data.xindex( range_left );
            int end   = data.xindex( range_right );
            int pt    = data.xindex( p.x() );

            if ( pt - start < 5  ||  end - pt < 5 )
            {
               QMessageBox::warning( this,
                  tr( "Position Error" ),
                  tr( "The selected point is too close to the edge." ) );
               return;
            }

            double sum = 0.0;

            // Average the value for +/- 5 points
            for ( int j = pt - 5; j <= pt + 5; j++ )
               sum += scan->rvalues[ j ];

            double bl = sum / 11.0;
            baseline  = p.x();

            QString wkstr;
            le_baseline->setText( wkstr.sprintf( "%.3f (%.3e)", p.x(), bl ) );
            plot_range();
         }

         pb_write      ->setEnabled( true );
         pb_writemwl   ->setEnabled( isMwl );
         changes_made = true;
         next_step();
         break;

      default:
         break;
   }
}

// Draw a vertical pick line
void US_Edit::draw_vline( double radius )
{
   double r[ 2 ];

   r[ 0 ] = radius;
   r[ 1 ] = radius;

   QwtScaleDiv* y_axis = data_plot->axisScaleDiv( QwtPlot::yLeft );

   double padding = ( y_axis->upperBound() - y_axis->lowerBound() ) / 30.0;

   double v[ 2 ];
   v [ 0 ] = y_axis->upperBound() - padding;
   v [ 1 ] = y_axis->lowerBound() + padding;

   v_line = us_curve( data_plot, "V-Line" );
   v_line->setData( r, v, 2 );

   QPen pen = QPen( QBrush( Qt::white ), 2.0 );
   v_line->setPen( pen );

   data_plot->replot();
}

// Set the step flag for the next step
void US_Edit::next_step( void )
{
   QPushButton* pb;
   
   if      ( meniscus == 0.0 ) 
   {
      step = MENISCUS;
      pb   = pb_meniscus;
   }
   else if ( airGap_right == 9.0  &&  dataType == "IP" )
   {
      step = AIRGAP;
      pb   = pb_airGap;
   }
   else if ( range_right == 9.0 ) 
   {
      step = RANGE;
      pb   = pb_dataRange;
   }
   else if ( plateau == 0.0 ) 
   {
      step = PLATEAU;
      pb   = pb_plateau;
   }

   else
   {  // All values up to Plateau have been entered:  Finished
      step = FINISHED;
      pb   = NULL;
      double sum = 0.0;
      int    pt  = data.xindex( range_left );
      baseline   = data.xvalues[ pt + 5 ];

      if ( !isMwl )
      {
         // Average the value for +/- 5 points
         for ( int jj = pt; jj < pt + 11; jj++ )
            sum += data.scanData.last().rvalues[ jj ];

         double bl = sum / 11.0;

         QString str;
         le_baseline->setText( str.sprintf( "%.3f (%.3e)", baseline, bl ) );
      }
   }

   set_pbColors( pb );
}

// Set up for a meniscus pick
void US_Edit::set_meniscus( void )
{
   le_meniscus ->setText( "" );
   le_airGap   ->setText( "" );
   le_dataRange->setText( "" );
   le_plateau  ->setText( "" );
   le_baseline ->setText( "" );
   
   meniscus      = 0.0;
   meniscus_left = 0.0;
   airGap_left   = 0.0;
   airGap_right  = 9.0;
   range_left    = 0.0;
   range_right   = 9.0;
   plateau       = 0.0;
   baseline      = 0.0;
   
   step          = MENISCUS;
   
   set_pbColors( pb_meniscus );
   pb_meniscus->setIcon( QIcon() );

   pb_airGap   ->setEnabled( false );
   pb_airGap   ->setIcon( QIcon() );
   pb_dataRange->setEnabled( false );
   pb_dataRange->setIcon( QIcon() );
   pb_plateau  ->setEnabled( false );
   pb_plateau  ->setIcon( QIcon() );
   pb_write    ->setEnabled( all_edits );
   pb_writemwl ->setEnabled( all_edits && isMwl );
   pb_write    ->setIcon( QIcon() );

   changes_made = all_edits;
DbgLv(1) << "set_meniscus -- changes_made" << changes_made;
   spikes       = false;
   pb_spikes   ->setEnabled( false );
   pb_spikes   ->setIcon( QIcon() );

   // Clear any existing marker
   data_plot->detachItems( QwtPlotItem::Rtti_PlotMarker );
            
   // Reset data and plot
   undo();

   if ( ! expIsEquil )
      plot_all();

   else
      plot_scan();
}

// Set up for an Air Gap pick
void US_Edit::set_airGap( void )
{
   le_airGap   ->setText( "" );
   le_dataRange->setText( "" );
   le_plateau  ->setText( "" );
   le_baseline ->setText( "" );
   
   airGap_left   = 0.0;
   airGap_right  = 9.0;
   range_left    = 0.0;
   range_right   = 9.0;
   plateau       = 0.0;
   baseline      = 0.0;
   
   step        = AIRGAP;
   set_pbColors( pb_airGap );
   pb_airGap   ->setIcon( QIcon() );

   pb_dataRange->setIcon( QIcon() );
   pb_plateau  ->setEnabled( false );
   pb_plateau  ->setIcon( QIcon() );
   pb_write    ->setEnabled( all_edits );
   pb_writemwl ->setEnabled( all_edits && isMwl );
   pb_write    ->setIcon( QIcon() );;
   changes_made = all_edits;

   spikes = false;
   pb_spikes   ->setEnabled( false );
   pb_spikes   ->setIcon( QIcon() );

   undo();
   plot_all();
}

// Set up for a data range pick
void US_Edit::set_dataRange( void )
{
   le_dataRange->setText( "" );
   le_plateau  ->setText( "" );
   le_baseline ->setText( "" );
   
   range_left    = 0.0;
   range_right   = 9.0;
   plateau       = 0.0;
   baseline      = 0.0;
   
   step        = RANGE;
   set_pbColors( pb_dataRange );

   pb_dataRange->setIcon( QIcon() );
   pb_plateau  ->setEnabled( false );
   pb_plateau  ->setIcon( QIcon() );
   pb_write    ->setEnabled( all_edits );
   pb_writemwl ->setEnabled( all_edits && isMwl );
   pb_write    ->setIcon( QIcon() );;
   changes_made = all_edits;

   spikes = false;
   pb_spikes   ->setEnabled( false );
   pb_spikes   ->setIcon( QIcon() );

   undo();

   if ( ! expIsEquil )
      plot_all();

   else
      plot_scan();
}

// Set up for a Plateau pick
void US_Edit::set_plateau( void )
{
   le_plateau  ->setText( "" );
   le_baseline ->setText( "" );
   
   plateau       = 0.0;
   baseline      = 0.0;
   
   step = PLATEAU;
   set_pbColors( pb_plateau );

   pb_plateau  ->setIcon( QIcon() );
   pb_write    ->setEnabled( all_edits );
   pb_writemwl ->setEnabled( all_edits && isMwl );
   pb_write    ->setIcon( QIcon() );;
   changes_made = all_edits;

   plot_range();
   undo();
}

// Set up for a Fringe Tolerance pick
void US_Edit::set_fringe_tolerance( double /* tolerance */)
{
   // This is only valid for interference data
   if ( dataType != "IP" ) return;

   // If we haven't yet set the range, just ignore the change
   if ( step == MENISCUS  ||  step == AIRGAP  ||  step == RANGE ) return;

   // Reset the data
   int index = index_data();
   data = *outData[ index ];

   US_DataIO::EditValues  edits;
   edits.airGapLeft  = airGap_left;
   edits.airGapRight = airGap_right;

   QList< int > excludes;
            
   for ( int i = 0; i < data.scanData.size(); i++ )
      if ( ! includes.contains( i ) ) edits.excludes << i;
         
   US_DataIO::adjust_interference( data, edits );

   edits.rangeLeft    = range_left;
   edits.rangeRight   = range_right;
   edits.gapTolerance = ct_gaps->value();

   US_DataIO::calc_integral( data, edits );
   replot();
}

// Plot all curves
void US_Edit::plot_all( void )
{
   if ( isMwl )
   {
      plot_mwl();
      return;
   }

   if ( plot->btnZoom->isChecked() )
      plot->btnZoom->setChecked( false );

   data_plot->detachItems( QwtPlotItem::Rtti_PlotCurve ); 
   v_line = NULL;

   int size = data.pointCount();

   QVector< double > rvec( size );
   QVector< double > vvec( size );
   double* r   = rvec.data();
   double* v   = vvec.data();

   double maxR = -1.0e99;
   double minR =  1.0e99;
   double maxV = -1.0e99;
   double minV =  1.0e99;

   for ( int i = 0; i < data.scanData.size(); i++ )
   {
      if ( ! includes.contains( i ) ) continue;
      
      US_DataIO::Scan*  s = &data.scanData[ i ];

      for ( int j = 0; j < size; j++ )
      {
         r[ j ] = data.xvalues[ j ];
         v[ j ] = s  ->rvalues[ j ] * invert;

         maxR = max( maxR, r[ j ] );
         minR = min( minR, r[ j ] );
         maxV = max( maxV, v[ j ] );
         minV = min( minV, v[ j ] );
      }

      QString title = tr( "Raw Data at " )
         + QString::number( s->seconds ) + tr( " seconds" )
         + " #" + QString::number( i );

      QwtPlotCurve* c = us_curve( data_plot, title );
      c->setPaintAttribute( QwtPlotCurve::ClipPolygons, true );
      c->setData( r, v, size );
   }

   // Reset the scan curves within the new limits
   double padR = ( maxR - minR ) / 30.0;
   double padV = ( maxV - minV ) / 30.0;

   data_plot->setAxisScale( QwtPlot::yLeft  , minV - padV, maxV + padV );
   data_plot->setAxisScale( QwtPlot::xBottom, minR - padR, maxR + padR );

   // Reset colors
   focus( (int)ct_from->value(), (int)ct_to->value() );
   data_plot->replot();
}

// Plot curves within the picked range
void US_Edit::plot_range( void )
{
   if ( plot->btnZoom->isChecked() )
      plot->btnZoom->setChecked( false );

   data_plot->detachItems( QwtPlotItem::Rtti_PlotCurve );
   v_line = NULL;

   int rsize   = data.pointCount();
   QVector< double > rvec( rsize );
   QVector< double > vvec( rsize );
   double* r   = rvec.data();
   double* v   = vvec.data();
   double maxR = -1.0e99;
   double minR =  1.0e99;
   double maxV = -1.0e99;
   double minV =  1.0e99;
   int indext  = cb_triple->currentIndex();

   if ( isMwl )
   {
      int ccx     = indext;
      int wvx     = cb_lplot->currentIndex();
      indext      = ccx * nwaveln + wvx;
DbgLv(1) << "plot_range(): ccx wvx indext" << ccx << wvx << indext;
   }

   // For each scan
   for ( int i = 0; i < data.scanData.size(); i++ )
   {
      if ( ! includes.contains( i ) ) continue;
      
      US_DataIO::Scan*  s = &data.scanData[ i ];
      
      int indexLeft  = data.xindex( range_left );
      int indexRight = data.xindex( range_right );
      double menp    = 0.0;

      if ( expIsEquil )
      {
         double rngl  = range_left;
         double rngr  = range_right;
         int tScan    = i + 1;
         int jsd      = sd_offs[ indext ];
         int ksd      = jsd + sd_knts[ indext ];

         for ( int jj = jsd; jj < ksd; jj++ )
         {
            int sScan  = sData[ jj ].first_scan;
            int eScan  = sData[ jj ].scan_count + sScan - 1;

            if ( tScan < sScan  ||  tScan > eScan )
               continue;

            rngl       = sData[ jj ].dataLeft;
            rngr       = sData[ jj ].dataRight;
            menp       = sData[ jj ].meniscus;
            break;
         }

         indexLeft  = data.xindex( rngl );
         indexRight = data.xindex( rngr );

         int inxm   = data.xindex( menp );

         if ( inxm < 1 )
            return;

         r[ 0 ]     = data.xvalues[ inxm          ];
         v[ 0 ]     = s  ->rvalues[ indexLeft     ];
         r[ 2 ]     = data.xvalues[ inxm + 2      ];
         v[ 2 ]     = s  ->rvalues[ indexLeft + 4 ];
         r[ 1 ]     = r[ 0 ];
         v[ 1 ]     = v[ 2 ];
         r[ 3 ]     = r[ 2 ];
         v[ 3 ]     = v[ 0 ];
         r[ 4 ]     = r[ 0 ];
         v[ 4 ]     = v[ 0 ];

         QwtPlotCurve* c = us_curve( data_plot, tr( "Meniscus at" ) + 
                  QString::number( tScan ) );
         c->setBrush( QBrush( Qt::cyan ) );
         c->setPen(   QPen(   Qt::cyan ) );
         c->setData( r, v, 5 );
         minR = min( minR, r[ 0 ] );
         minV = min( minV, v[ 0 ] );
      }
      
      int     count  = 0;
      
      for ( int j = indexLeft; j <= indexRight; j++ )
      {
         r[ count ] = data.xvalues[ j ];
         v[ count ] = s  ->rvalues[ j ] * invert;

         maxR = max( maxR, r[ count ] );
         minR = min( minR, r[ count ] );
         maxV = max( maxV, v[ count ] );
         minV = min( minV, v[ count ] );

         count++;
      }

      QString title = tr( "Raw Data at " )
         + QString::number( s->seconds ) + tr( " seconds" )
         + " #" + QString::number( i );

      QwtPlotCurve* c = us_curve( data_plot, title );
      c->setData( r, v, count );
   }

   // Reset the scan curves within the new limits
   double padR = ( maxR - minR ) / 30.0;
   double padV = ( maxV - minV ) / 30.0;

   data_plot->setAxisScale( QwtPlot::yLeft  , minV - padV, maxV + padV );
   data_plot->setAxisScale( QwtPlot::xBottom, minR - padR, maxR + padR );

   // Reset colors
   focus( (int)ct_from->value(), (int)ct_to->value() );
   data_plot->replot();
}

// Plot the last picked curve
void US_Edit::plot_last( void )
{
   if ( plot->btnZoom->isChecked() )
      plot->btnZoom->setChecked( false );

   data_plot->detachItems( QwtPlotItem::Rtti_PlotCurve );
   v_line = NULL;
   //grid = us_grid( data_plot ); 

   double maxR = -1.0e99;
   double minR =  1.0e99;
   double maxV = -1.0e99;
   double minV =  1.0e99;

   // Plot only the last scan
   US_DataIO::Scan*  s = &data.scanData[ includes.last() ];;
   
   int indexLeft  = data.xindex( range_left );
   int indexRight = data.xindex( range_right );
   
   int     count  = 0;
   uint    size   = s->rvalues.size();
   QVector< double > rvec( size );
   QVector< double > vvec( size );
   double* r      = rvec.data();
   double* v      = vvec.data();
   
   for ( int j = indexLeft; j <= indexRight; j++ )
   {
      r[ count ] = data.xvalues[ j ];
      v[ count ] = s  ->rvalues[ j ] * invert;

      maxR = max( maxR, r[ count ] );
      minR = min( minR, r[ count ] );
      maxV = max( maxV, v[ count ] );
      minV = min( minV, v[ count ] );

      count++;
   }

   QString title = tr( "Raw Data at " )
      + QString::number( s->seconds ) + tr( " seconds" )
      + " #" + QString::number( includes.last() );

   QwtPlotCurve* c = us_curve( data_plot, title );
   c->setData( r, v, count );

   // Reset the scan curves within the new limits
   double padR = ( maxR - minR ) / 30.0;
   double padV = ( maxV - minV ) / 30.0;

   data_plot->setAxisScale( QwtPlot::yLeft  , minV - padV, maxV + padV );
   data_plot->setAxisScale( QwtPlot::xBottom, minR - padR, maxR + padR );

   // Reset colors
   focus( (int)ct_from->value(), (int)ct_to->value() );
   data_plot->replot();
}

// Plot a single scan curve
void US_Edit::plot_scan( void )
{
   int    rsize = data.pointCount();
   int    ssize = data.scanCount();
   int    count = 0;
   QVector< double > rvec( rsize );
   QVector< double > vvec( rsize );
   double* r    = rvec.data();
   double* v    = vvec.data();

   data_plot->detachItems( QwtPlotItem::Rtti_PlotCurve );
   v_line = NULL;

   double maxR  = -1.0e99;
   double minR  =  1.0e99;
   double maxV  = -1.0e99;
   double minV  =  1.0e99;
   QString srpm = cb_rpms->currentText();

   // Plot only the currently selected scan(s)
   //
   for ( int ii = 0; ii < ssize; ii++ )
   {
      US_DataIO::Scan*  s = &data.scanData[ ii ];

      QString arpm        = QString::number( s->rpm );

      if ( arpm != srpm )
         continue;

      count = 0;

      for ( int jj = 0; jj < rsize; jj++ )
      {
         r[ count ] = data.xvalues[ jj ];
         v[ count ] = s  ->rvalues[ jj ] * invert;

         maxR = max( maxR, r[ count ] );
         minR = min( minR, r[ count ] );
         maxV = max( maxV, v[ count ] );
         minV = min( minV, v[ count ] );

         count++;
      }

      QString title = tr( "Raw Data at " )
         + QString::number( s->seconds ) + tr( " seconds" )
         + " #" + QString::number( ii );

      QwtPlotCurve* c = us_curve( data_plot, title );
      c->setData( r, v, count );

      // Reset the scan curves within the new limits
      double padR = ( maxR - minR ) / 30.0;
      double padV = ( maxV - minV ) / 30.0;

      data_plot->setAxisScale( QwtPlot::yLeft  , minV - padV, maxV + padV );
      data_plot->setAxisScale( QwtPlot::xBottom, minR - padR, maxR + padR );
   }

   data_plot->replot();
}

// Plot MWL curves
void US_Edit::plot_mwl( void )
{
   if ( ! isMwl )  return;

   QString     rectype = tr( "Wavelength" );
   double      recvalu = 250;
   int         index   = index_data();
   int         ccx     = triple_index;
   int         recndx  = cb_lplot ->currentIndex();
   QString     celchn  = celchns.at( ccx );
   QString     scell   = celchn.section( "/", 0, 0 ).simplified();
   QString     schan   = celchn.section( "/", 1, 1 ).simplified();
   QString     svalu;

DbgLv(1) << "PlMwl:  index celchn" << index << celchn;

   if ( xaxis_radius )
   {
DbgLv(1) << "PlMwl:   x-r index cc nw rx" << index << ccx << nwavelo << recndx;
DbgLv(1) << "PlMwl:    outData size" << outData.size();
DbgLv(1) << "PlMwl:    expc_wvlns size" << expc_wvlns.size();
      data                = *outData[ data_index ];
      recvalu             = expi_wvlns.at( recndx );
      svalu               = expc_wvlns.at( recndx );
   }

   else
   {
      index               = ccx * nrpoint + recndx;
DbgLv(1) << "PlMwl:   x-w index cc nr rx" << index << ccx << nrpoint << recndx;
      data                = *outData[ 0 ];
      rectype             = tr( "Radius" );
      recvalu             = expd_radii.at( recndx );
      svalu               = expc_radii.at( recndx );
   }
DbgLv(1) << "PlMwl: ccx index rtype rval" << ccx << index << rectype << recvalu;

   // Read the data description
   QString     desc    = data.description;

   dataType            = QString( QChar( data.type[ 0 ] ) ) 
                       + QString( QChar( data.type[ 1 ] ) );

   le_info->setText( runID + "  (" + desc + ")" );

   // Plot Title
   QString     title   = tr( "Pseudo Absorbance Data\n"
                             "Run ID: %1\n"
                             "Cell: %2  Channel: %3  %4: %5" )
                         .arg( runID ).arg( scell ).arg( schan )
                         .arg( rectype ).arg( svalu );
DbgLv(1) << "PlMwl:  title" << title;

   data_plot->setTitle    ( title );
   data_plot->setAxisTitle( QwtPlot::yLeft, tr( "Absorbance (OD)" ) );

   data_plot->detachItems ( QwtPlotItem::Rtti_PlotCurve ); 
   v_line = NULL;

   int     nscan  = data.scanData.size();
   int     npoint = xaxis_radius ? nrpoint : nwavelo;
   int     ptxs   = 0;

   if ( step != MENISCUS  &&  xaxis_radius )
   {
      ptxs           = data.xindex( range_left );
      npoint         = data.xindex( range_right ) - ptxs + 1;
   }
DbgLv(1) << "PlMwl:   xa_rad" << xaxis_radius << "nsc npt" << nscan << npoint;

   QVector< double > rvec( npoint );
   QVector< double > vvec( npoint );
   double* rr     = rvec.data();
   double* vv     = vvec.data();

   double  maxR   = -1.0e99;
   double  minR   =  1.0e99;
   double  maxV   = -1.0e99;
   double  minV   =  1.0e99;
   double  maxOD  = odlimit * 2.0;
   double  valueV;
   int     kodlim = 0;

   if ( xaxis_radius )
   {  // Build normal AUC data plot
      data_plot->setAxisTitle( QwtPlot::xBottom, tr( "Radius (cm)" ) );
DbgLv(1) << "PlMwl:    START xa_RAD";

      for ( int ii = 0; ii < nscan; ii++ )
      {
         if ( ! includes.contains( ii ) )
         {
DbgLv(1) << "PlMwl:     ii" << ii << "NOT INCLUDED";
            continue;
         }

         US_DataIO::Scan*  scn = &data.scanData[ ii ];
         int     kk     = ptxs;

         for ( int jj = 0; jj < npoint; jj++ )
         {
            rr[ jj ] = data.xvalues[ jj ];
            valueV   = qMin( maxOD, scn->rvalues[ kk++ ] * invert );
            vv[ jj ] = valueV;

            maxR     = qMax( maxR, rr[ jj ] );
            minR     = qMin( minR, rr[ jj ] );
            maxV     = qMax( maxV, valueV );
            minV     = qMin( minV, valueV );

            if ( valueV > odlimit )
               kodlim++;
         }

         QString ctitle = tr( "Raw Data at " )
            + QString::number( scn->seconds ) + tr( " seconds" )
            + " #" + QString::number( ii );

         QwtPlotCurve* cc = us_curve( data_plot, ctitle );
         cc->setPaintAttribute( QwtPlotCurve::ClipPolygons, true );
         cc->setData( rr, vv, npoint );
      }
      pick     ->disconnect();
      connect( pick, SIGNAL( cMouseUp( const QwtDoublePoint& ) ),
                     SLOT  ( mouse   ( const QwtDoublePoint& ) ) );
DbgLv(1) << "PlMwl:      END xa_RAD  kodlim odlimit" << kodlim << odlimit;
   }

   else
   {  // Build plot of radius record with wavelength points
DbgLv(1) << "PlMwl:    START xa_WAV";
      data_plot->setAxisTitle( QwtPlot::xBottom, tr( "Wavelength (nm)" ) );
      QVector< double > wrdata = rdata[ index ];
      int     dpx    = 0;

      for ( int ii = 0; ii < nscan; ii++ )
      {
         if ( ! includes.contains( ii ) ) continue;

         for ( int jj = 0; jj < npoint; jj++ )
         {
            rr[ jj ] = expi_wvlns[ jj ];
            valueV   = qMin( maxOD, wrdata[ dpx++ ] );
            vv[ jj ] = valueV;

            maxR     = qMax( maxR, rr[ jj ] );
            minR     = qMin( minR, rr[ jj ] );
            maxV     = qMax( maxV, valueV );
            minV     = qMin( minV, valueV );

            if ( valueV > odlimit )
               kodlim++;
         }

         US_DataIO::Scan*  scn = &data.scanData[ ii ];
         QString ctitle = tr( "Raw Data at " )
            + QString::number( scn->seconds ) + tr( " seconds" )
            + " #" + QString::number( ii );

         QwtPlotCurve* cc = us_curve( data_plot, ctitle );
         cc->setPaintAttribute( QwtPlotCurve::ClipPolygons, true );
         cc->setData( rr, vv, npoint );
      }
DbgLv(1) << "PlMwl:      END xa_WAV  kodlim odlimit" << kodlim << odlimit;
   }

   // Reset the scan curves within the new limits
   double padR = ( maxR - minR ) / 30.0;
   double padV = ( maxV - minV ) / 30.0;
   padV        = qMax( padV, 0.005 );

   data_plot->setAxisScale( QwtPlot::yLeft  , minV - padV, maxV + padV );
   data_plot->setAxisScale( QwtPlot::xBottom, minR - padR, maxR + padR );

DbgLv(1) << "PlMwl: call replot()";
   data_plot->replot();
DbgLv(1) << "PlMwl:  retn fr replot()";

   // Set the Scan spin boxes
   ct_from->setMinValue( 0.0 );
   ct_from->setMaxValue(  data.scanData.size() );

   ct_to  ->setMinValue( 0.0 );
   ct_to  ->setMaxValue(  data.scanData.size() );

   pick     ->disconnect();
   connect( pick, SIGNAL( cMouseUp( const QwtDoublePoint& ) ),
                  SLOT  ( mouse   ( const QwtDoublePoint& ) ) );
}

// Set focus FROM scan value
void US_Edit::focus_from( double scan )
{
   int from = (int)scan;
   int to   = (int)ct_to->value();

   if ( from > to )
   {
      ct_to->disconnect();
      ct_to->setValue( scan );
      to = from;
      
      connect( ct_to, SIGNAL( valueChanged ( double ) ),
                      SLOT  ( focus_to     ( double ) ) );
   }

   focus( from, to );
}

// Set focus TO scan value
void US_Edit::focus_to( double scan )
{
   int to   = (int)scan;
   int from = (int)ct_from->value();

   if ( from > to )
   {
      ct_from->disconnect();
      ct_from->setValue( scan );
      from = to;
      
      connect( ct_from, SIGNAL( valueChanged ( double ) ),
                        SLOT  ( focus_from   ( double ) ) );
   }

   focus( from, to );
}

// Set focus From/To
void US_Edit::focus( int from, int to )
{
   if ( from == 0 )
      pb_edit1  ->setEnabled( false );
   else
      pb_edit1  ->setEnabled( true );

   if ( to == 0 )
      pb_excludeRange->setEnabled( false );
   else
      pb_excludeRange->setEnabled( true );

   QList< int > focus;
   int ifrom = qMax( from - 1, 0 );
   int ito   = qMin( to, includes.size() );

   for ( int ii = ifrom; ii < ito; ii++ )
      focus << includes.at( ii );

   set_colors( focus );
}

// Set curve colors
void US_Edit::set_colors( const QList< int >& focus )
{
   // Get pointers to curves
   QwtPlotItemList        list = data_plot->itemList();
   QList< QwtPlotCurve* > curves;
   
   for ( int i = 0; i < list.size(); i++ )
   {
      if ( list[ i ]->title().text().contains( "Raw" ) )
         curves << dynamic_cast< QwtPlotCurve* >( list[ i ] );
   }

   QPen   p   = curves[ 0 ]->pen();
   QBrush b   = curves[ 0 ]->brush();
   QColor std = US_GuiSettings::plotCurve();
   QColor foc = Qt::red;

   // Mark these scans in red
   for ( int i = 0; i < curves.size(); i++ )
   {
      int scnnbr = curves[ i ]->title().text().section( "#", 1, 1 ).toInt();

      if ( focus.contains( scnnbr ) )
      {
         p.setColor( foc );
         b.setColor( foc );
      }
      else
      {
         p.setColor( std );
         b.setColor( std );
      }

      curves[ i ]->setPen  ( p );
      curves[ i ]->setBrush( b );
   }

   data_plot->replot();
}

// Initialize includes
void US_Edit::init_includes( void )
{
   includes.clear();
   for ( int i = 0; i < data.scanData.size(); i++ ) includes << i;
}

// Reset excludes
void US_Edit::reset_excludes( void )
{
   ct_from->disconnect();
   ct_from->setValue   ( 0 );
   ct_from->setMaxValue( includes.size() );
   connect( ct_from, SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_from   ( double ) ) );

   ct_to->disconnect();
   ct_to->setValue   ( 0 );
   ct_to->setMaxValue( includes.size() );
   connect( ct_to, SIGNAL( valueChanged ( double ) ),
                   SLOT  ( focus_to   ( double ) ) );

   pb_excludeRange->setEnabled( false );
   pb_edit1       ->setEnabled( false );

   replot();
}

// Set excludes as indicated in counters
void US_Edit::exclude_range( void )
{
   int scanStart = (int)ct_from->value();
   int scanEnd   = (int)ct_to  ->value();

   for ( int i = scanEnd; i >= scanStart; i-- )
      includes.removeAt( i - 1 );

   replot();
   reset_excludes();
}

// Show exclusion profile
void US_Edit::exclusion( void )
{
   reset_excludes();
   US_ExcludeProfile* exclude = new US_ExcludeProfile( includes );
 
   connect( exclude, SIGNAL( update_exclude_profile( QList< int > ) ), 
            this   , SLOT  ( update_excludes       ( QList< int > ) ) );
   
   connect( exclude, SIGNAL( cancel_exclude_profile( void ) ), 
            this   , SLOT  ( cancel_excludes       ( void ) ) );

   connect( exclude, SIGNAL( finish_exclude_profile( QList< int > ) ), 
            this   , SLOT  ( finish_excludes       ( QList< int > ) ) );

   exclude->exec();
   qApp->processEvents();
   delete exclude;
}

// Update based on exclusion profile
void US_Edit::update_excludes( QList< int > scanProfile )
{
DbgLv(1) << "UPD_EXCL: size excl" << scanProfile.size();
   set_colors( scanProfile );
}

// Cancel all excludes
void US_Edit::cancel_excludes( void )
{
   QList< int > focus;
   set_colors( focus );  // Focus on no curves
}

// Process excludes to rebuild include list
void US_Edit::finish_excludes( QList< int > excludes )
{
   for ( int i = 0; i < excludes.size(); i++ )
      includes.removeAll( excludes[ i ] );

DbgLv(1) << "FIN_EXCL: sizes excl incl" << excludes.size() << includes.size();
   replot();
   reset_excludes();
}

// Edit a scan
void US_Edit::edit_scan( void )
{
   int index1 = (int)ct_from->value();
   int scan  = includes[ index1 - 1 ];

   US_EditScan* dialog = new US_EditScan( data.scanData[ scan ], data.xvalues, 
         invert, range_left, range_right );
   connect( dialog, SIGNAL( scan_updated( QList< QPointF > ) ),
                    SLOT  ( update_scan ( QList< QPointF > ) ) );
   dialog->exec();
   qApp->processEvents();
   delete dialog;
}

// Update scan points
void US_Edit::update_scan( QList< QPointF > changes )
{
   // Handle excluded scans
   int              index1        = (int)ct_from->value();
   int              current_scan  = includes[ index1 - 1 ];
   US_DataIO::Scan* s             = &data.scanData[ current_scan ];

   for ( int i = 0; i < changes.size(); i++ )
   {
      int    point = (int)changes[ i ].x();
      double value =      changes[ i ].y();

      s->rvalues[ point ] = value;
   }

   // Save changes for writing output
   Edits e;
   e.scan    = current_scan;
   e.changes = changes;
   changed_points << e;

   // Set data for the curve
   int     points = data.pointCount();
   QVector< double > rvec( points );
   QVector< double > vvec( points );
   double* r      = rvec.data();
   double* v      = vvec.data();

   int left;
   int right;
   int count = 0;

   if ( range_left > 0 )
   {
      left  = data.xindex( range_left  );
      right = data.xindex( range_right );
   }
   else
   {
      left  = 0;
      right = points - 1;
   }

   for ( int i = left; i <= right; i++ )
   {
      r[ count ] = data.xvalues[ i ];
      v[ count ] = s  ->rvalues[ i ];
      count++;
   }

   // Find the pointer to the current scan
   QwtPlotItemList items   = data_plot->itemList();
   QString         seconds = " " + QString::number( s->seconds );
   bool            found   = false;

   QwtPlotCurve* c;
   for ( int i = 0; i < items.size(); i++ )
   {
      if ( items[ i ]->rtti() == QwtPlotItem::Rtti_PlotCurve )
      {
         c = dynamic_cast< QwtPlotCurve* >( items[ i ] );
         if ( c->title().text().contains( seconds ) )
         {
            found = true;
            break;
         }
      }
   }

   if ( found ) 
   {
      // Update the curve
      c->setData( r, v, count );
      data_plot->replot();
   }
   else
      qDebug() << "Can't find curve!";

}

// Handle include profile
void US_Edit::include( void )
{
   init_includes();
   reset_excludes();
}

// Reset pushbutton and plot with invert flag change
void US_Edit::invert_values( void )
{
   if ( invert == 1.0 )
   {
      invert = -1.0;
      pb_invert->setIcon( check );
   }
   else
   {
      invert = 1.0;
      pb_invert->setIcon( QIcon() );
   }

   replot();
}

// Remove spikes
void US_Edit::remove_spikes( void )
{
   double smoothed_value;

   // For each scan
   for ( int i = 0; i < data.scanData.size(); i++ ) 
   {
      US_DataIO::Scan* s = &data.scanData [ i ];

      int start  = data.xindex( range_left  );
      int end    = data.xindex( range_right );

      for ( int j = start; j < end; j++ )
      {
         if ( US_DataIO::spike_check( *s, data.xvalues, j, start, end, 
                                      &smoothed_value ) )
         {
            s->rvalues[ j ]     = smoothed_value;

            // If previous consecututive points are interpolated, then 
            // redo them
            int           index = j - 1;
            unsigned char c     = s->interpolated[ index / 8 ];

            while ( c & ( 1 << ( 7 - index % 8 ) ) )
            {
               if ( US_DataIO::spike_check( *s, data.xvalues,
                                            index, start, end, &smoothed_value ) )
                  s->rvalues[ index ] = smoothed_value;

               index--;
               c = s->interpolated[ index / 8 ];
            }
         }
      }
   }

   pb_spikes->setIcon   ( check );
   pb_spikes->setEnabled( false );
   replot();
}

// Undo changes
void US_Edit::undo( void )
{
   // Copy from outData to data
   if ( step < PLATEAU )
      data      = *outData[ index_data() ];

   // Redo some things depending on type
   if ( dataType == "IP" )
   {
      US_DataIO::EditValues edits;
      edits.airGapLeft  = airGap_left;
      edits.airGapRight = airGap_right;

      edits.rangeLeft    = range_left;
      edits.rangeRight   = range_right;
      edits.gapTolerance = ct_gaps->value();
      
      for ( int i = 0; i < data.scanData.size(); i++ )
         if ( ! includes.contains( i ) ) edits.excludes << i;

      if ( step > AIRGAP )
            US_DataIO::adjust_interference( data, edits );

      if ( step >  RANGE )
         US_DataIO::calc_integral( data, edits );
   }

   replot();

   // Reset buttons and structures
   pb_residuals->setEnabled( false );

   if ( step < PLATEAU )
   {
      pb_noise ->setEnabled( false );
      pb_spikes->setEnabled( false );
   }
   else
   {
      pb_noise ->setEnabled( true );
      pb_spikes->setEnabled( true );
   }

   spikes      = false;
   noise_order = 0;

   // Remove icons
   pb_noise       ->setIcon( QIcon() );
   pb_residuals   ->setIcon( QIcon() );
   pb_spikes      ->setIcon( QIcon() );
}

// Calculate and apply noise
void US_Edit::noise( void )
{
   residuals.clear();
   US_RiNoise* dialog = new US_RiNoise( data, includes,  
         range_left, range_right, dataType, noise_order, residuals );
   int code = dialog->exec();
   qApp->processEvents();

   if ( code == QDialog::Accepted )
   {
      pb_noise    ->setIcon( check );
      pb_residuals->setEnabled( true );
   }
   else
      pb_residuals->setEnabled( false );

   delete dialog;
}

// Subtract residuals
void US_Edit::subtract_residuals( void )
{
   for ( int i = 0; i < data.scanCount(); i++ )
   {
      for ( int j = 0; j <  data.pointCount(); j++ )
         data.scanData[ i ].rvalues[ j ] -= residuals[ i ];
   }

   pb_residuals->setEnabled( false );
   pb_residuals->setIcon   ( check );
   replot();
}

// Select a new triple
void US_Edit::new_triple( int index )
{
   triple_index    = index;
DbgLv(1) << "EDT:NewTr: tripindex" << triple_index << "chgs" << changes_made;

   if ( changes_made )
   {
      QMessageBox mb;
      mb.setIcon( QMessageBox::Question );
      mb.setText( tr( "Ignore Edits?" ) );
      mb.setInformativeText( 
            tr( "Edits have been made.  If you want to keep them,\n"
                "cancel and write the outputs first." ) );
      mb.addButton( tr( "Ignore Edits" ), QMessageBox::RejectRole );
      mb.addButton( tr( "Return to previous selection" ), QMessageBox::NoRole );
      int result = mb.exec();

      if ( result == QMessageBox::RejectRole )
      {
         cb_triple->disconnect();
         cb_triple->setCurrentIndex( triple_index );
         connect( cb_triple, SIGNAL( currentIndexChanged( int ) ), 
                             SLOT  ( new_triple         ( int ) ) );
         return;
      }
   }

   // Set up data indexes
   rb_radius->setChecked( true );
   rb_waveln->setChecked( false );

   index_data();
DbgLv(1) << "EDT:NewTr: trip,data index" << triple_index << data_index;

   if ( isMwl )
   {  // MWL:  restore the wavelengths for the newly selected triple
      QVector< int > wvs;
      mwl_data.lambdas( wvs, triple_index );
      lambda_new_list ( wvs );
DbgLv(1) << "EDT:NewTr:  nwavelo" << nwavelo;
   }

   // Reset for new triple
   reset_triple(); 

   // Need to reconnect after reset
   connect( cb_triple, SIGNAL( currentIndexChanged( int ) ), 
                       SLOT  ( new_triple         ( int ) ) );

   edata          = outData[ data_index ];
   data           = *edata;
   QString swavl  = cb_lplot ->currentText();
   QString triple = cb_triple->currentText() + ( isMwl ? " / " + swavl : "" );
   int     idax   = triples.indexOf( triple );
DbgLv(1) << "EDT:NewTr:   sw tri dx" << swavl << triple << idax;

   // Enable pushbuttons
   pb_details  ->setEnabled( true );
   pb_include  ->setEnabled( true );
   pb_exclusion->setEnabled( true );
   pb_meniscus ->setEnabled( true );
   pb_airGap   ->setEnabled( true );
   pb_dataRange->setEnabled( true );
   pb_noise    ->setEnabled( true );
   pb_plateau  ->setEnabled( true );
   pb_spikes   ->setEnabled( true );
   pb_invert   ->setEnabled( true );
   pb_undo     ->setEnabled( true );
   pb_write    ->setEnabled( all_edits );
   pb_writemwl ->setEnabled( all_edits && isMwl );
   pb_write    ->setIcon   ( all_edits          ? check : QIcon() );
   pb_writemwl ->setIcon   ( all_edits && isMwl ? check : QIcon() );
   all_edits    = false;
   changes_made = all_edits;

   init_includes();

   connect( ct_from, SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_from   ( double ) ) );

   connect( ct_to,   SIGNAL( valueChanged ( double ) ),
                     SLOT  ( focus_to     ( double ) ) );

   if ( expIsEquil )
   {  // Equilibrium
      cb_rpms->clear();
      trip_rpms.clear();

      for ( int ii = 0; ii < data.scanData.size(); ii++ )
      {  // build unique-rpm list for triple
         QString arpm = QString::number( data.scanData[ ii ].rpm );

         if ( ! trip_rpms.contains( arpm ) )
         {
            trip_rpms << arpm;
         }
      }
      cb_rpms->addItems( trip_rpms );

      le_edtrsp->setText( cb_triple->currentText() + " : " + trip_rpms[ 0 ] );
   }

   else
   {  // non-Equilibrium:  possibly re-do/un-do edits
      QString trbase   = cb_triple->currentText();
      triple           = trbase;

      if ( isMwl )
      {
         plotndx          = cb_lplot->currentIndex();
         swavl            = cb_lplot->currentText();
         triple          += " / " + swavl;
         plotrec          = swavl.toInt();
      }

      QString fname    = editFnames[ idax ];
      bool    app_edit = ( fname != "none" );
DbgLv(1) << "EDT:NewTr:   app_edit" << app_edit << "fname" << fname;

      if ( app_edit )
      {  // This data has editing,  ask if it should be applied
         QStringList editfiles;

         if ( fname == "same" )
         {  // Need to find an edit file for this channel
            QString celchn   = trbase + " / ";
            int     wvxe     = nwavelo - 1;
DbgLv(1) << "EDT:NewTr:     wvxe ewvsiz" << wvxe << expc_wvlns.count();
            int     jdax     = triples.indexOf( celchn + expc_wvlns[ 0    ] );
            int     ldax     = triples.indexOf( celchn + expc_wvlns[ wvxe ] );
DbgLv(1) << "EDT:NewTr:      jdax ldax" << jdax << ldax;

            while ( jdax <= ldax )
            {
               if ( editFnames[ jdax ].length() > 4 )
               {
                  fname         = editFnames[ jdax ];
                  idax          = jdax;
                  break;
               }

               jdax++;
            }
         }

         QString trtype   = isMwl ? "cell/channel" : "triple";
         QString trvalu   = isMwl ? trbase         : triple;
DbgLv(1) << "EDT:NewTr:   tr type,valu" << trtype << trvalu; 

         QMessageBox mbox;
         QPushButton* pb_appl;
         QString msg      = tr( "Edits have been loaded or saved<br/>"
                                "for the current %1 (%2).<br/></br/>"
                                "Do you wish to<br/>"
                                "&nbsp;&nbsp;"
                                "apply them (<b>Apply Edits</b>) or<br/>"
                                "&nbsp;&nbsp;"
                                "ignore them (<b>Ignore Edits</b>)<br/>"
                                "in data displays?" )
                            .arg( trtype ).arg( trvalu );
         mbox.setIcon      ( QMessageBox::Question );
         mbox.setIcon      ( QMessageBox::Question );
         mbox.setTextFormat( Qt::RichText );
         mbox.setText      ( msg );
                            mbox.addButton( tr( "Ignore Edits"   ),
                                            QMessageBox::RejectRole );
         pb_appl          = mbox.addButton( tr( "Apply Edits" ),
                                            QMessageBox::AcceptRole );
         mbox.setDefaultButton( pb_appl );
         mbox.exec();
         app_edit         = ( mbox.clickedButton() == pb_appl );
      }

      if ( app_edit )
      {  // If editing chosen, apply it
         US_DataIO::EditValues parameters;

         US_DataIO::readEdits( workingDir + fname, parameters );

         apply_edits( parameters );
      }

      else
      {  // If no editing, be sure to turn off edits
         set_meniscus();
      }

DbgLv(1) << "EDT:NewTr:   men" << meniscus << "dx" << idax;
      plot_current( idax );
   }

   replot();
DbgLv(1) << "EDT:NewTr: DONE";
}

// Select a new speed within a triple
void US_Edit::new_rpmval( int index )
{
   QString srpm = cb_rpms->itemText( index );

   set_meniscus();

   le_edtrsp->setText( cb_triple->currentText() + " : " + srpm );

   plot_scan();
}

// Mark data as floating
void US_Edit::floating( void )
{
   floatingData = ! floatingData;
   if ( floatingData )
      pb_float->setIcon( check );
   else
      pb_float->setIcon( QIcon() );

}

// Save edit profile(s)
void US_Edit::write( void )
{ 
   if ( !expIsEquil )
   {  // non-Equilibrium:  write single current edit
      triple_index = cb_triple->currentIndex();

      write_triple();
   }

   else
   {  // Equilibrium:  loop to write all edits
      for ( int jr = 0; jr < triples.size(); jr++ )
      {
         triple_index = jr;
         data         = *outData[ jr ];

         write_triple();
      }
   }

   changes_made = false;
   pb_write    ->setEnabled( false );
   pb_writemwl ->setEnabled( false );
   pb_write    ->setIcon   ( check );
   pb_writemwl ->setIcon   ( check );
}

// Save edits for a triple
void US_Edit::write_triple( void )
{
   QString s;

   meniscus       = le_meniscus->text().toDouble();
   baseline       = data.xvalues[ data.xindex( range_left ) + 5 ];
   int     odax   = cb_triple->currentIndex();
   int     idax   = odax;

   if ( isMwl )
   {  // For MultiWavelength, data index needs to be recomputed
      int     wvx    = cb_lplot ->currentIndex();
      QString swavl  = expc_wvlns[ wvx ];
      QString triple = cb_triple->currentText() + " / " + swavl;
      idax           = triples.indexOf( triple );
      odax           = index_data( wvx );
   }

   if ( expIsEquil )
   {  // Equilibrium:  set baseline,plateau as flag that those are "done"
      int jsd     = sd_offs[ triple_index ];
      meniscus    = sData[ jsd ].meniscus;
      range_left  = sData[ jsd ].dataLeft;
      range_right = sData[ jsd ].dataRight;
      baseline    = range_left;
      plateau     = range_right;
   }

   // Check if complete
   if ( meniscus == 0.0 )
      s = tr( "meniscus" );
   else if ( dataType == "IP" && ( airGap_left == 0.0 || airGap_right == 9.0 ) )
      s = tr( "air gap" );
   else if ( range_left == 0.0 || range_right == 9.0 )
      s = tr( "data range" );
   else if ( plateau == 0.0 )
      s = tr( "plateau" );
   else if ( baseline == 0.0 )
      s = tr( "baseline" );

   if ( ! s.isEmpty() )
   {
      QMessageBox::information( this,
            tr( "Data missing" ),
            tr( "You must define the " ) + s 
            + tr( " before writing the edit profile." ) );
      return;
   }

   QString sufx = "";

   // Ask for editLabel if not yet defined
   while ( editLabel.isEmpty() )
   {
      QString now  =  QDateTime::currentDateTime()
                      .toUTC().toString( "yyMMddhhmm" );

      bool ok;
      QString msg = tr( "The base Edit Label for this edit session is <b>" )
         + now + "</b> .<br/>"
         + tr( "You may add an optional suffix to further distinquish<br/>"
               "the Edit Label. Use alphanumeric characters, underscores,<br/>"
               "or hyphens (no spaces). Enter 0 to 10 suffix characters." );
      sufx   = QInputDialog::getText( this, 
         tr( "Create a unique session Edit Label" ),
         msg,
         QLineEdit::Normal,
         sufx,
         &ok );
      
      if ( ! ok ) return;

      sufx.remove( QRegExp( "[^\\w\\d_-]" ) );
      editLabel = now + sufx;

      if ( editLabel.length() > 20 )
      {
         QMessageBox::critical( this,
            tr( "Text length error" ),
            tr( "You entered %1 characters for the Edit Label suffix.\n"
                "Re-enter, limiting length to 10 characters." )
            .arg( sufx.length() ) );
         editLabel.clear();
         sufx = sufx.left( 10 );
      }
   }

   // Determine file name
   // workingDir + runID + editLabel + data type + cell + channel + wavelength 
   //            + ".xml"

   QString filename = files[ idax ];
DbgLv(1) << "EDT:WrTripl: tripindex" << triple_index << "idax" << idax
 << "filename" << filename;
   int     index    = filename.indexOf( '.' ) + 1;
   filename.insert( index, editLabel + "." );
   QString wvpart   = "";

   filename.replace( QRegExp( "auc$" ), "xml" );
DbgLv(1) << "EDT:WrTripl:  filename" << filename;

   if ( expType.isEmpty()  ||
        expType.compare( "other", Qt::CaseInsensitive ) == 0 )
      expType = "Velocity";

   QString editGUID = editGUIDs[ idax ];

   if ( editGUID.isEmpty() )
   {
      editGUID = US_Util::new_guid();
      editGUIDs.replace( idax, editGUID );
   }

   QString rawGUID  = US_Util::uuid_unparse( (unsigned char*)data.rawGUID );
   QString triple   = triples.at( idax );
DbgLv(1) << "EDT:WrTripl:   triple" << triple;

   // Output the edit XML file
   int wrstat       = write_xml_file( filename, triple, editGUID, rawGUID );

   if ( wrstat != 0 )
      return;
   else
      editFnames[ idax ] = filename;

   if ( disk_controls->db() )
   {
      if ( dbP == NULL )
      {
         US_Passwd pw;
         dbP          = new US_DB2( pw.getPasswd() );
         if ( dbP == NULL  ||  dbP->lastErrno() != US_DB2::OK )
         {
            QMessageBox::warning( this, tr( "Connection Problem" ),
              tr( "Could not connect to database \n" ) + dbP->lastError() );
            return;
         }
      }

      QString editID   = editIDs[ idax ];

      // Output the edit database record
      wrstat     = write_edit_db( dbP, filename, editGUID, editID, rawGUID );

      if ( wrstat != 0 )
         return;
   }
}

// Apply a prior edit profile for Velocity and like data
void US_Edit::apply_prior( void )
{
   QString filename;
   QString filepath;
   int     index1;
   US_DB2* dbP    = NULL;
DbgLv(1) << "AppPri: IN  dkdb" << disk_controls->db();

   if ( disk_controls->db() )
   {
      US_Passwd pw;
      dbP            = new US_DB2( pw.getPasswd() );

      if ( dbP->lastErrno() != US_DB2::OK )
      {
         QMessageBox::warning( this, tr( "Connection Problem" ),
           tr( "Could not connect to database: \n" ) + dbP->lastError() );
         return;
      }

      QStringList q( "get_rawDataID_from_GUID" );
     
      q << US_Util::uuid_unparse( (uchar*)data.rawGUID );
      dbP->query( q );

      // Error check    
      if ( dbP->lastErrno() != US_DB2::OK )
      {
         QMessageBox::warning( this,
           tr( "AUC Data is not in DB" ),
           tr( "Cannot find the raw data in the database.\n" ) );

         return;
      }
      
      dbP->next();
      QString rawDataID = dbP->value( 0 ).toString();

      q.clear();
      q << "get_editedDataIDs" << rawDataID;

      dbP->query( q );


      QStringList editDataIDs;
      QStringList filenames;

      while ( dbP->next() )
      {
         editDataIDs << dbP->value( 0 ).toString();
         filenames   << dbP->value( 2 ).toString();
      }

      if ( editDataIDs.size() == 0 )
      {
         QMessageBox::warning( this,
           tr( "Edit data is not in DB" ),
           tr( "Cannot find any edit records in the database.\n" ) );

         return;
      }

      int index;
      US_GetEdit dialog( index, filenames );
      if ( dialog.exec() == QDialog::Rejected ) return;

      if ( index >= 0 ) 
      {
         filepath   = workingDir + filenames[ index ];
         int dataID = editDataIDs[ index ].toInt();
         dbP->readBlobFromDB( filepath, "download_editData", dataID );
      }
   }
   else
   {
      QString filter = files[ cb_triple->currentIndex() ];
      index1 = filter.indexOf( '.' ) + 1;

      filter = "*" + filter.mid( index1 );
      filter.replace( QRegExp( "auc$" ), "xml" );
      filter = tr( "Edits(" ) + filter + tr( ");;All XML (*.xml)" );
      
      // Ask for edit file
      filepath = QFileDialog::getOpenFileName( this, 
            tr( "Select a saved edit file" ),
            workingDir, filter );
   }
DbgLv(1) << "AppPri: fpath" << filepath;

   if ( filepath.isEmpty() ) return; 

   // Get multiple edits if they exist and user so chooses
   QStringList editfiles;
   filepath         = filepath.replace( "\\", "/"   );
   filename         = filepath.section( "/", -1, -1 );
   QString triple   = filename.section( ".", -4, -2 )
                              .replace( ".", " / "  );
   int     idax     = triples.indexOf( triple );

   int     nledits  = like_edit_files( filename, editfiles, dbP );
DbgLv(1) << "AppPri: nledits" << nledits << editfiles.count();

   if ( nledits > 1 )
   {
      QPushButton* pb_defb; 
      QPushButton* pb_selo;
      QPushButton* pb_allw;
      QString edtLbl = editfiles[ 0 ].section( ".", -6, -6 );
      int swavl      = editfiles[           0 ].section( ".", -2, -2 ).toInt();
      int ewavl      = editfiles[ nledits - 1 ].section( ".", -2, -2 ).toInt();

      QMessageBox mbox;
      QString msg  = tr( "%1 wavelengths (%2 to %3) have the same<br/>"
                         "Edit Label of \"%4\" as the selected file.<br/>"
                         "<br/>"
                         "Specify whether you wish to apply edits only<br/>"
                         "to the selected file (<b>Selected Only</b>)<br/>"
                         "or to apply them to all wavelengths of the<br/>"
                         "current cell/channel (<b>All Wavelengths</b>)." )
                     .arg( nledits ).arg( swavl ).arg( ewavl ).arg( edtLbl );
      mbox.setIcon      ( QMessageBox::Question );
      mbox.setTextFormat( Qt::RichText );
      mbox.setText      ( msg );
      pb_selo        = mbox.addButton( tr( "Selected Only"   ),
                                       QMessageBox::RejectRole );
      pb_allw        = mbox.addButton( tr( "All Wavelengths" ),
                                       QMessageBox::AcceptRole );
      pb_defb        = nledits > 2 ? pb_allw : pb_selo;
      mbox.setDefaultButton( pb_defb );
      mbox.exec();

      if ( mbox.clickedButton() == pb_selo )
      {  // Only apply for the selected file
DbgLv(1) << "AppPri: SelOnly button clicked";
         nledits        = 1;
         editfiles.clear();
         editfiles << filename;

         editFnames[ idax ] = filename;
      }

      else
      {  // Apply to all like-labeled wavelengths in the channel
DbgLv(1) << "AppPri: AllWavl button clicked";
         for ( int ii = 0; ii < nledits; ii++ )
         {  // Save selected edit file name; use "same" for others in channel
            QString edfile = editfiles[ ii ];
            QString triple = edfile.section( ".", -4, -2 )
                                   .replace( ".", " / " );
            int     jdax   = triples.indexOf( triple );

            if ( edfile != filename )
            {  // The selected file
               editFnames[ jdax ] = filename;
            }

            else
            {  // Non-selected file
               editFnames[ jdax ] = QString( "same" );
            }
         }
      }
   }

   else
   {  // Save edit file name for the single triple
      editFnames[ idax ] = filename;
   }

   // Reset data from input data
   data           = allData[ idax ];

   // Read the edits
   US_DataIO::EditValues parameters;

   int     result = US_DataIO::readEdits( filepath, parameters );

   if ( result != US_DataIO::OK )
   {
      QMessageBox::warning( this,
         tr( "XML Error" ),
         tr( "An error occurred when reading edit file\n\n" ) 
         +  US_DataIO::errorString( result ) );
      return;
   }

   QString uuid   = US_Util::uuid_unparse( (unsigned char*)data.rawGUID );

   if ( parameters.dataGUID != uuid )
   {
      QMessageBox::warning( this,
         tr( "Data Error" ),
         tr( "The edit file was not created using the current data" ) );
DbgLv(1) << "parsGUID rawGUID" << parameters.dataGUID << uuid;
      return;
   }
   
   // Apply the edits with specified parameters
   apply_edits( parameters );

   pb_undo    ->setEnabled( true  );
   pb_write   ->setEnabled( true  );
   pb_writemwl->setEnabled( isMwl );

   changes_made= false;
   plot_range();
}

// Apply prior edits to an Equilibrium set
void US_Edit::prior_equil( void )
{
   int     cndxt     = cb_triple->currentIndex();
   int     cndxs     = cb_rpms  ->currentIndex();
   int     index1;
   QString     filename;
   QStringList cefnames;
   data    = *outData[ 0 ];

   if ( disk_controls->db() )
   {  // Get prior equilibrium edits from DB
      US_Passwd pw;
      US_DB2 db( pw.getPasswd() );

      if ( db.lastErrno() != US_DB2::OK )
      {
         QMessageBox::warning( this, tr( "Connection Problem" ),
           tr( "Could not connect to database \n" ) + db.lastError() );
         return;
      }

      QStringList q( "get_rawDataID_from_GUID" );
      
      q << US_Util::uuid_unparse( (uchar*)data.rawGUID );

      db.query( q );

      // Error check    
      if ( db.lastErrno() != US_DB2::OK )
      {
         QMessageBox::warning( this,
           tr( "AUC Data is not in DB" ),
           tr( "Cannot find the raw data in the database.\n" ) );

         return;
      }
      
      db.next();
      QString rawDataID = db.value( 0 ).toString();

      q.clear();
      q << "get_editedDataIDs" << rawDataID;

      db.query( q );


      QStringList editDataIDs;
      QStringList filenames;

      while ( db.next() )
      {
         editDataIDs << db.value( 0 ).toString();
         filenames   << db.value( 2 ).toString();
      }

      if ( editDataIDs.size() == 0 )
      {
         QMessageBox::warning( this,
           tr( "Edit data is not in DB" ),
           tr( "Cannot find any edit records in the database.\n" ) );

         return;
      }

      int index;
      US_GetEdit dialog( index, filenames );
      if ( dialog.exec() == QDialog::Rejected ) return;

      if ( index < 0 )
         return;

      filename          = filenames[ index ];
      int     dataID    = editDataIDs[ index ].toInt();

      QString editLabel = filename.section( ".", 1, 1 );
      filename          = workingDir + filename;
      db.readBlobFromDB( filename, "download_editData", dataID );

      cefnames << filename;   // save the first file name

      // Loop to get files with same Edit Label from other triples
      for ( int ii = 1; ii < outData.size(); ii++ )
      {
         data    = *outData[ ii ];
         q.clear();
         q << "get_rawDataID_from_GUID" 
           << US_Util::uuid_unparse( (uchar*)data.rawGUID );
         db.query( q );
         db.next();
         rawDataID = db.value( 0 ).toString();

         q.clear();
         q << "get_editedDataIDs" << rawDataID;
         db.query( q );
         filename.clear();
         bool found = false;

         while ( db.next() )
         {
            dataID      = db.value( 0 ).toString().toInt();
            filename    = db.value( 2 ).toString();
            QString elb = filename.section( ".", 1, 1 );
            
            if ( elb == editLabel )
            {
               found = true;
               filename = workingDir + filename;
               db.readBlobFromDB( filename, "download_editData", dataID );
               cefnames << filename;
               break;
            }
         }

         if ( ! found )
            cefnames << "";
      }
   }

   else
   {  // Get prior equilibrium edits from Local Disk
      QString filter = files[ cb_triple->currentIndex() ];
      index1 = filter.indexOf( '.' ) + 1;

      filter.insert( index1, "*." );
      filter.replace( QRegExp( "auc$" ), "xml" );
      
      // Ask for edit file
      filename = QFileDialog::getOpenFileName( this, 
            tr( "Select a saved edit file" ),
            workingDir, filter );

      if ( filename.isEmpty() ) return; 

      filename          = filename.replace( "\\", "/" );
      QString editLabel = filename.section( "/", -1, -1 ).section( ".", 1, 1 );
      QString runID     = filename.section( "/", -1, -1 ).section( ".", 0, 0 );

      for ( int ii = 0; ii < files.size(); ii++ )
      {
         filename          = files[ ii ];
         filename          = runID + "." + editLabel + "."
                             + filename.section( ".", 1, -2 ) + ".xml";
         filename          = workingDir + filename;

         if ( QFile( filename ).exists() )
            cefnames << filename;

         else
            cefnames << "";
      }
   }

   for ( int ii = 0; ii < cefnames.size(); ii++ )
   {  // Read and apply edits from edit files
      data     = *outData[ ii ];
      filename = cefnames[ ii ];

      // Read the edits
      US_DataIO::EditValues parameters;

      int result = US_DataIO::readEdits( filename, parameters );

      if ( result != US_DataIO::OK )
      {
         QMessageBox::warning( this,
               tr( "XML Error" ),
               tr( "An error occurred when reading edit file\n\n" ) 
               +  US_DataIO::errorString( result ) );
         continue;
      }

      QString uuid = US_Util::uuid_unparse( (unsigned char*)data.rawGUID );

      if ( parameters.dataGUID != uuid )
      {
         QMessageBox::warning( this,
               tr( "Data Error" ),
               tr( "The edit file was not created using the current data" ) );
         continue;
      }
   
      // Apply the edits
      QString wkstr;

      meniscus    = parameters.meniscus;
      range_left  = parameters.rangeLeft;
      range_right = parameters.rangeRight;
      plateau     = parameters.plateau;
      baseline    = parameters.baseline;

      if ( parameters.speedData.size() > 0 )
      {
         meniscus    = parameters.speedData[ 0 ].meniscus;
         range_left  = parameters.speedData[ 0 ].dataLeft;
         range_right = parameters.speedData[ 0 ].dataRight;
         baseline    = range_left;
         plateau     = range_right;

         int jsd     = sd_offs[ ii ];

         for ( int jj = 0; jj < sd_knts[ ii ]; jj++ )
            sData[ jsd++ ] = parameters.speedData[ jj ];
      }

      le_meniscus->setText( wkstr.sprintf( "%.3f", meniscus ) );
      pb_meniscus->setIcon( check );
      pb_meniscus->setEnabled( true );

      airGap_left  = parameters.airGapLeft;
      airGap_right = parameters.airGapRight;

      if ( dataType == "IP" )
      {
         US_DataIO::adjust_interference( data, parameters );
         US_DataIO::calc_integral      ( data, parameters );
         le_airGap->setText( wkstr.sprintf( "%.3f - %.3f", 
                  airGap_left, airGap_right ) ); 
         pb_airGap->setIcon( check );
         pb_airGap->setEnabled( true );
      }

      le_dataRange->setText( wkstr.sprintf( "%.3f - %.3f",
              range_left, range_right ) );
      pb_dataRange->setIcon( check );
      pb_dataRange->setEnabled( true );
   
      // Invert
      invert = parameters.invert;
   
      if ( invert == -1.0 ) pb_invert->setIcon( check );
      else                  pb_invert->setIcon( QIcon() );

      // Excluded scans
      init_includes();
      reset_excludes(); // Zero exclude combo boxes
      qSort( parameters.excludes );

      for ( int i = parameters.excludes.size(); i > 0; i-- )
         includes.removeAt( parameters.excludes[ i - 1 ] );

      // Edited points
      changed_points.clear();

      for ( int i = 0; i < parameters.editedPoints.size(); i++ )
      {
         int    scan   = parameters.editedPoints[ i ].scan;
         int    index1 = (int)parameters.editedPoints[ i ].radius;
         double value  = parameters.editedPoints[ i ].value;
      
         Edits e;
         e.scan = scan;
         e.changes << QPointF( index1, value );
     
         changed_points << e;
      
         data.scanData[ scan ].rvalues[ index1 ] = value;
      }

      // Spikes
      spikes = parameters.removeSpikes;

      pb_spikes->setIcon( QIcon() );
      pb_spikes->setEnabled( true );
      if ( spikes ) remove_spikes();
   
      // Noise
      noise_order = parameters.noiseOrder;
      if ( noise_order > 0 )
      {
         US_RiNoise::calc_residuals( data, includes, range_left, range_right, 
               noise_order, residuals );
      
         subtract_residuals();
      }
      else
      {
         pb_noise    ->setIcon( QIcon() );
         pb_residuals->setIcon( QIcon() );
         pb_residuals->setEnabled( false );
      }

      // Floating data
      floatingData = parameters.floatingData;

      if ( floatingData )
         pb_float->setIcon( check );

      else
         pb_float->setIcon( QIcon() );
   }

   step        = FINISHED;
   set_pbColors( NULL );

   pb_undo    ->setEnabled( true );
   pb_write   ->setEnabled( true );
   pb_writemwl->setEnabled( isMwl );

   cndxt       = ( cndxt < 0 ) ? 0 : cndxt;
   cndxs       = ( cndxs < 0 ) ? 0 : cndxs;
   cb_triple->setCurrentIndex( cndxt );
   cb_rpms  ->setCurrentIndex( cndxs );

   //changes_made= false;
   //plot_range();

   pb_reviewep->setEnabled( true );
   pb_nexttrip->setEnabled( true );

   all_edits    = all_edits_done();
   pb_write   ->setEnabled( all_edits );
   pb_writemwl->setEnabled( all_edits && isMwl );
   changes_made = all_edits;

   review_edits();
}

// Initialize edit review for first triple
void US_Edit::review_edits( void )
{
   cb_triple->disconnect();
   cb_triple->setCurrentIndex( cb_triple->count() - 1 );

   le_meniscus ->setText( "" );
   le_dataRange->setText( "" );
   pb_plateau  ->setIcon( QIcon() );
   pb_dataRange->setIcon( QIcon() );
   pb_meniscus ->setIcon( QIcon() );

   step    = FINISHED;
   next_triple();
}

// Advance to next triple and plot edited curves
void US_Edit::next_triple( void )
{
   int row = cb_triple->currentIndex() + 1;
   row     = ( row < cb_triple->count() ) ? row : 0;

   cb_triple->disconnect();
   cb_triple->setCurrentIndex( row );
   connect( cb_triple, SIGNAL( currentIndexChanged( int ) ), 
                       SLOT  ( new_triple         ( int ) ) );

   if ( le_edtrsp->isVisible() )
   {
      QString trsp = cb_triple->currentText() + " : " + trip_rpms[ 0 ];
      le_edtrsp->setText( trsp );
      cb_rpms  ->setCurrentIndex( 0 );
   }

   data    = *outData[ index_data() ];
   plot_range();
}

// Evaluate whether all edits are complete
bool US_Edit::all_edits_done( void )
{
   bool all_ed_done = false;

   if ( expIsEquil )
   {
      total_edits  = 0;
      total_speeds = 0;

      for ( int jd = 0; jd < outData.size(); jd++ )
      {  // Examine each data set to evaluate whether edits complete
         int jsd = sd_offs[ jd ];
         int ksd = jsd + sd_knts[ jd ];
         QList< double > drpms;
         US_DataIO::RawData* rawdat = outData[ jd ];

         // Count edits done on this data set
         for ( int js = jsd; js < ksd; js++ )
         {
            if ( sData[ js ].meniscus > 0.0 )
               total_edits++;
         }

         // Count speeds present in this data set
         for ( int js = 0; js < rawdat->scanData.size(); js++ )
         {
            double  drpm = rawdat->scanData[ js ].rpm;

            if ( ! drpms.contains( drpm ) )
               drpms << drpm;
         }

         total_speeds += drpms.size();
      }

      // Set flag:  are all edits complete?
      all_ed_done = ( total_edits == total_speeds );
   }

   else
   {
      all_ed_done = ( range_left  != 0  &&
                      range_right != 0  &&
                      plateau     != 0  &&
                      baseline    != 0 );
   }

DbgLv(1) << "all_ed_done" << all_ed_done;
   return all_ed_done;
}

// Private slot to update disk/db control when dialog changes it
void US_Edit::update_disk_db( bool isDB )
{
   if ( isDB )
      disk_controls->set_db();
   else
      disk_controls->set_disk();
}

// Private slot to show progress text for load-auc
void US_Edit::progress_load( QString progress )
{
   le_info->setText( progress );
}

// Show or hide MWL Controls
void US_Edit::show_mwl_controls( bool show )
{
   lb_gaps    ->setVisible( !show );
   ct_gaps    ->setVisible( !show );
   le_lxrng   ->setVisible( show );
   lb_mwlctl  ->setVisible( show );
   lb_ldelta  ->setVisible( show );
   ct_ldelta  ->setVisible( show );
   le_ltrng   ->setVisible( show );
   lb_lstart  ->setVisible( show );
   cb_lstart  ->setVisible( show );
   lb_lend    ->setVisible( show );
   cb_lend    ->setVisible( show );
   lb_lplot   ->setVisible( show );
   cb_lplot   ->setVisible( show );
   pb_larrow  ->setVisible( show );
   pb_rarrow  ->setVisible( show );
   pb_custom  ->setVisible( show );
   pb_incall  ->setVisible( show );
   pb_writemwl->setVisible( show );

   lo_lrange->itemAtPosition( 0, 0 )->widget()->setVisible( show );
   lo_lrange->itemAtPosition( 0, 1 )->widget()->setVisible( show );
   lo_custom->itemAtPosition( 0, 0 )->widget()->setVisible( show );
   lo_custom->itemAtPosition( 0, 1 )->widget()->setVisible( show );
   lo_radius->itemAtPosition( 0, 0 )->widget()->setVisible( show );
   lo_radius->itemAtPosition( 0, 1 )->widget()->setVisible( show );
   lo_waveln->itemAtPosition( 0, 0 )->widget()->setVisible( show );
   lo_waveln->itemAtPosition( 0, 1 )->widget()->setVisible( show );

   adjustSize();
}

// Connect or disconnect MWL Controls
void US_Edit::connect_mwl_ctrls( bool conn )
{
   if ( conn )
   {
      connect( rb_lrange, SIGNAL( toggled            ( bool   ) ),
               this,      SLOT  ( lselect_range_on   ( bool   ) ) );
      connect( rb_custom, SIGNAL( toggled            ( bool   ) ),
               this,      SLOT  ( lselect_custom_on  ( bool   ) ) );
      connect( ct_ldelta, SIGNAL( valueChanged       ( double ) ),
               this,      SLOT  ( ldelta_value       ( double ) ) );
      connect( cb_lstart, SIGNAL( currentIndexChanged( int    ) ),
               this,      SLOT  ( lambda_start_value ( int    ) ) );
      connect( cb_lend,   SIGNAL( currentIndexChanged( int    ) ),
               this,      SLOT  ( lambda_end_value   ( int    ) ) );
      connect( rb_radius, SIGNAL( toggled            ( bool   ) ),
               this,      SLOT  ( xaxis_radius_on    ( bool   ) ) );
      connect( rb_waveln, SIGNAL( toggled            ( bool   ) ),
               this,      SLOT  ( xaxis_waveln_on    ( bool   ) ) );
      connect( pb_custom, SIGNAL( clicked            (        ) ),
               this,      SLOT  ( lambda_custom_list (        ) ) );
      connect( pb_incall, SIGNAL( clicked            (        ) ),
               this,      SLOT  ( lambda_include_all (        ) ) );
      connect( cb_lplot,  SIGNAL( currentIndexChanged( int    ) ),
               this,      SLOT  ( lambda_plot_value  ( int    ) ) );
      connect( pb_larrow, SIGNAL( clicked            (        ) ),
               this,      SLOT  ( lambda_plot_prev   (        ) ) );
      connect( pb_rarrow, SIGNAL( clicked            (        ) ),
               this,      SLOT  ( lambda_plot_next   (        ) ) );
   }

   else
   {
      rb_lrange->disconnect();
      rb_custom->disconnect();
      ct_ldelta->disconnect();
      cb_lstart->disconnect();
      cb_lend  ->disconnect();
      rb_radius->disconnect();
      rb_waveln->disconnect();
      pb_custom->disconnect();
      pb_incall->disconnect();
      cb_lplot ->disconnect();
      pb_larrow->disconnect();
      pb_rarrow->disconnect();
   }
}

// Lambda selection has been changed to Range or Custom
void US_Edit::lselect_range_on( bool checked )
{
DbgLv(1) << "lselect range checked" << checked;
   if ( checked )
   {
      connect_mwl_ctrls( false );
      ct_ldelta->setValue( 1 );
      cb_lstart->setCurrentIndex( 0 );
      cb_lend  ->setCurrentIndex( nwaveln - 1 );
      connect_mwl_ctrls( true );

      reset_plot_lambdas();
   }

   ct_ldelta    ->setEnabled( checked );
   cb_lstart    ->setEnabled( checked );
   cb_lend      ->setEnabled( checked );
   pb_custom    ->setEnabled( !checked );
   lsel_range = checked;
}

// Lambda selection has been changed to Range or Custom
void US_Edit::lselect_custom_on( bool checked )
{
DbgLv(1) << "lselect custom checked" << checked;
   if ( checked )
   {
      connect_mwl_ctrls( false );
      ct_ldelta->setValue( 1 );
      cb_lstart->setCurrentIndex( 0 );
      cb_lend  ->setCurrentIndex( nwaveln - 1 );
      connect_mwl_ctrls( true );

      reset_plot_lambdas();
   }

   ct_ldelta    ->setEnabled( !checked );
   cb_lstart    ->setEnabled( !checked );
   cb_lend      ->setEnabled( !checked );
   pb_custom    ->setEnabled( checked );
   lsel_range = !checked;
}

// Lambda Delta has changed
void US_Edit::ldelta_value( double value )
{
DbgLv(1) << "ldelta_value  value" << value;
   dlambda     = (int)value;

   reset_plot_lambdas();
}

// Lambda Start has changed
void US_Edit::lambda_start_value( int value )
{
   slambda     = cb_lstart->itemText( value ).toInt();
DbgLv(1) << "lambda_start_value  value" << value << slambda;

   reset_plot_lambdas();
}

// Lambda End has changed
void US_Edit::lambda_end_value( int value )
{
   elambda     = cb_lend  ->itemText( value ).toInt();
DbgLv(1) << "lambda_end_value  value" << value << elambda;

   reset_plot_lambdas();
}

// Adjust the plot wavelengths list, after a lambda range change
void US_Edit::reset_plot_lambdas()
{
   dlambda        = (int)ct_ldelta->value();
   slambda        = cb_lstart->currentText().toInt();
   elambda        = cb_lend  ->currentText().toInt();
   int     strtx  = cb_lstart->currentIndex();
   int     endx   = cb_lend  ->currentIndex() + 1;
   int     plotx  = cb_lplot ->currentIndex();
DbgLv(1) << "rpl: dl sl el px" << dlambda << slambda << elambda << plotx;
   expc_wvlns.clear();
   expi_wvlns.clear();

   for ( int ii = strtx; ii < endx; ii += dlambda )
   {  // Accumulate new list of export lambdas by looking at all raw lambas
      QString clam   = rawc_wvlns[ ii ];
      int     rlam   = clam.toInt();      // Current raw input lambda
      expc_wvlns << clam;
      expi_wvlns << rlam;
   }

   nwavelo        = expi_wvlns.size();
   plotx          = ( plotx < nwavelo ) ? plotx : ( nwavelo / 2 );
DbgLv(1) << "rpl:   nwavelo plotx" << nwavelo << plotx;
DbgLv(1) << "rpl:    pl1 pln" << expi_wvlns[0] << expi_wvlns[nwavelo-1];

   if ( xaxis_radius )
   {  // If x-axis is radius, reset wavelength-to-plot list
      cb_lplot->disconnect();
      cb_lplot->clear();
      cb_lplot->addItems( expc_wvlns );
      connect( cb_lplot,  SIGNAL( currentIndexChanged( int    ) ),
               this,      SLOT  ( lambda_plot_value  ( int    ) ) );
      cb_lplot->setCurrentIndex( plotx );
   }

   // Report export lambda range
   const QChar chlamb( 955 );
   le_lxrng ->setText( tr( "%1 MWL exports: %2 %3 to %4," )
      .arg( nwavelo ).arg( chlamb ).arg( slambda ).arg( elambda )
      + ( lsel_range ? tr( " raw index increment %1." ).arg( dlambda )
                     : tr( " from custom selections." ) ) );
}

// X-axis has been changed to Radius or Wavelength
void US_Edit::xaxis_radius_on( bool checked )
{
DbgLv(1) << "xaxis_radius_on  checked" << checked;
   if ( checked )
   {
      xaxis_radius = true;
      lb_lplot->setText( tr( "Plot (W nm):" ) );

      cb_lplot->disconnect();
      cb_lplot->clear();
      cb_lplot->addItems( expc_wvlns );
      connect( cb_lplot,  SIGNAL( currentIndexChanged( int    ) ),
               this,      SLOT  ( lambda_plot_value  ( int    ) ) );
      cb_lplot->setCurrentIndex( expc_wvlns.size() / 2 );
   }
}

// X-axis has been changed to Radius or Wavelength
void US_Edit::xaxis_waveln_on( bool checked )
{
DbgLv(1) << "xaxis_waveln_on  checked" << checked;
   if ( checked )
   {
      xaxis_radius = false;
      lb_lplot->setText( tr( "Plot (R cm):" ) );

      cb_lplot->disconnect();
      cb_lplot->clear();
      cb_lplot->addItems( expc_radii );
      connect( cb_lplot,  SIGNAL( currentIndexChanged( int    ) ),
               this,      SLOT  ( lambda_plot_value  ( int    ) ) );
      cb_lplot->setCurrentIndex( expc_radii.size() / 2 );
   }
}

// Plot Lambda/Radius value has changed
void US_Edit::lambda_plot_value( int value )
{
   if ( value < 0 )  return;

   double  menissv  = meniscus;
   plotndx          = value;

   if ( ! xaxis_radius )
   {  // If plotting radius records, go straight to plotting
      plot_mwl();
      return;
   }

   // If plotting wavelength records, check need to re-do/un-do edits
   QString swavl    = cb_lplot ->itemText( plotndx );
   QString triple   = cb_triple->currentText() + " / " + swavl;
   int     idax     = triples.indexOf( triple );
   plotrec          = swavl.toInt();
DbgLv(1) << "lambda_plot_value  value" << value << plotrec;
   QString fname    = ( menissv != 0.0 ) ? editFnames[ idax ] : "none";


   if ( fname == "none" )
   {  // New wavelength has no edit:  turn off edits
      set_meniscus();
   }

   else if ( fname != "same" )
   {  // New wavelength has its own edit:  apply it
      US_DataIO::EditValues parameters;

      US_DataIO::readEdits( workingDir + fname, parameters );

      apply_edits( parameters );
   }

   else if ( step != MENISCUS )
   {  // New wavelength has same edit as others in channel:  make sure applied
   }

   plot_mwl();
}

// Plot-previous has been clicked
void US_Edit::lambda_plot_prev()
{
DbgLv(1) << "lambda_plot_prev  clicked";
   plotndx--;

   if ( plotndx <= 0 )
   {
      plotndx = 0;
      pb_larrow->setEnabled( false );
   }

   pb_rarrow->setEnabled     ( true );
   cb_lplot ->setCurrentIndex( plotndx );
}

// Plot-next has been clicked
void US_Edit::lambda_plot_next()
{
DbgLv(1) << "lambda_plot_next  clicked";
   plotndx++;

   int lstx = xaxis_radius ? ( expc_wvlns.size() - 1 )
                           : ( expc_radii.size() - 1 );

   if ( plotndx >= lstx )
   {
      plotndx = lstx;
      pb_rarrow->setEnabled( false );
   }

   pb_larrow->setEnabled     ( true );
   cb_lplot ->setCurrentIndex( plotndx );
}

// Custom Lambdas has been clicked
void US_Edit::lambda_custom_list()
{
DbgLv(1) << "lambda_custom_list  clicked";
   US_SelectLambdas* sel_lambd = new US_SelectLambdas( rawi_wvlns );

   connect( sel_lambd, SIGNAL( new_lambda_list( QVector< int > ) ),
            this,      SLOT  ( lambda_new_list( QVector< int > ) ) );

   if ( sel_lambd->exec() == QDialog::Accepted )
   {
DbgLv(1) << "  lambda_custom_list  ACCEPTED";
      int  plotx  = cb_lplot ->currentIndex();
      plotx       = ( plotx < nwavelo ) ? plotx : ( nwavelo / 2 );

      if ( xaxis_radius )
      {  // If x-axis is radius, reset wavelength-to-plot list
         cb_lplot->disconnect();
         cb_lplot->clear();
         cb_lplot->addItems( expc_wvlns );
         connect( cb_lplot,  SIGNAL( currentIndexChanged( int    ) ),
                  this,      SLOT  ( lambda_plot_value  ( int    ) ) );
         cb_lplot->setCurrentIndex( plotx );
      }

      // Report export lambda range
      const QChar chlamb( 955 );
      le_lxrng ->setText( tr( "%1 MWL exports: %2 %3 to %4,"
                              " from custom selections." )
         .arg( nwavelo ).arg( chlamb ).arg( slambda ).arg( elambda ) );
   }
}

// Custom Lambdas have been specified
void US_Edit::lambda_new_list( QVector< int > newlams )
{
   nwavelo     = newlams.count();
   expi_wvlns  = newlams;
   slambda     = expi_wvlns[ 0 ];
   elambda     = expi_wvlns[ nwavelo - 1 ];
   expc_wvlns.clear();

   for ( int ii = 0; ii < nwavelo; ii++ )
      expc_wvlns << QString::number( expi_wvlns[ ii ] );
}

// Include-all-lambda has been clicked
void US_Edit::lambda_include_all()
{
DbgLv(1) << "lambda_include_all  clicked";
   nwavelo     = nwaveln;
   expc_wvlns  = rawc_wvlns;
   expi_wvlns  = rawi_wvlns;
   connect_mwl_ctrls( false );
   ct_ldelta->setValue( 1 );
   cb_lstart->setCurrentIndex( 0 );
   cb_lend  ->setCurrentIndex( nwaveln - 1 );
   connect_mwl_ctrls( true );

   reset_plot_lambdas();
}

// OD-limit-on-radii has changed
void US_Edit::od_radius_limit( double value )
{
DbgLv(1) << "od_radius_limit  value" << value;
   odlimit     = value;

   plot_mwl();
}

// Write edit to all wavelengths of the current cell/channel
void US_Edit::write_mwl()
{
   QString saved_info = le_info->text();
   QString str;
   if ( ! isMwl )
   {
      QMessageBox::warning( this,
            tr( "Invalid Selection" ),
            tr( "The \"Save to all Wavelengths\" button is only valid\n"
                "for MultiWavelength data. No Save will be performed." ) );
      return;
   }

   meniscus  = le_meniscus->text().toDouble();
   baseline  = data.xvalues[ data.xindex( range_left ) + 5 ];

   if ( expIsEquil )
   {  // Equilibrium:  set baseline,plateau as flag that those are "done"
      int jsd     = sd_offs[ triple_index ];
      meniscus    = sData[ jsd ].meniscus;
      range_left  = sData[ jsd ].dataLeft;
      range_right = sData[ jsd ].dataRight;
      baseline    = range_left;
      plateau     = range_right;
   }

   // Check if complete
   if ( meniscus == 0.0 )
      str = tr( "meniscus" );
   else if ( dataType == "IP" && ( airGap_left == 0.0 || airGap_right == 9.0 ) )
      str = tr( "air gap" );
   else if ( range_left == 0.0 || range_right == 9.0 )
      str = tr( "data range" );
   else if ( plateau == 0.0 )
      str = tr( "plateau" );
   else if ( baseline == 0.0 )
      str = tr( "baseline" );

   if ( ! str.isEmpty() )
   {
      QMessageBox::information( this,
            tr( "Data missing" ),
            tr( "You must define the " ) + str +
            tr( " before writing the edit profile." ) );
      return;
   }

   QString sufx = "";

   // Ask for editLabel if not yet defined
   while ( editLabel.isEmpty() )
   {
      QString now  =  QDateTime::currentDateTime()
                      .toUTC().toString( "yyMMddhhmm" );

      bool ok;
      QString msg  = tr( "The base Edit Label for this edit session is <b>" )
         + now + "</b> .<br/>"
         + tr( "You may add an optional suffix to further distinquish<br/>"
               "the Edit Label. Use alphanumeric characters, underscores,<br/>"
               "or hyphens (no spaces). Enter 0 to 10 suffix characters." );
      sufx         = QInputDialog::getText( this, 
         tr( "Create a unique session Edit Label" ),
         msg,
         QLineEdit::Normal,
         sufx,
         &ok );
      
      if ( ! ok ) return;

      sufx.remove( QRegExp( "[^\\w\\d_-]" ) );
      editLabel    = now + sufx;

      if ( editLabel.length() > 20 )
      {
         QMessageBox::critical( this,
            tr( "Text length error" ),
            tr( "You entered %1 characters for the Edit Label suffix.\n"
                "Re-enter, limiting length to 10 characters." )
            .arg( sufx.length() ) );
         editLabel.clear();
         sufx = sufx.left( 10 );
      }
   }

   QVector< int > oldi_wvlns;
   int     kwavelo  = mwl_data.lambdas( oldi_wvlns );
   int     nwavelo  = expi_wvlns.count();
   int     wvx;
   bool    chg_lamb = ( kwavelo != nwavelo );

   if ( ! chg_lamb )
   {  // If no change in number of wavelengths, check actual lists
      for ( wvx = 0; wvx < nwavelo; wvx++ )
      {
         if ( oldi_wvlns[ wvx ] != expi_wvlns[ wvx ] )
         {  // There is a difference in the lists:  mark as such
            chg_lamb      = true;
            break;
         }
      }
   }

   if ( chg_lamb )
   {  // If wavelengths have changed, save new list and rebuild some vectors
      mwl_data.set_lambdas( expi_wvlns );  // Save new lambdas for channel

      int dax1         = data_index;       // Index of data start for channel
      int dax2         = dax1;

      QVector< US_DataIO::RawData* > oldData   = outData;
      QVector< QString >             oedtGUIDs = editGUIDs;
      QVector< QString >             oedtIDs   = editIDs;
      outData  .clear();
      editGUIDs.clear();
      editIDs  .clear();

      for ( int trx = 0; trx < dax2; trx++ )
      {  // Copy any data preceding this channel
         outData   << oldData  [ trx ];
         editGUIDs << oedtGUIDs[ trx ];
         editIDs   << oedtIDs  [ trx ];
      }

      for ( wvx = 0; wvx < nwavelo; wvx++ )
      {  // Copy data associated with the new wavelengths list
         int wvxo         = oldi_wvlns.indexOf( expi_wvlns[ wvx ] );

         if ( wvxo < 0 )
         {
            qDebug() << "*ERROR* wavelength unexpectedly missing:"
                     << expi_wvlns[ wvx ];
            continue;
         }

         int trx          = dax2 + wvxo;
         outData   << oldData  [ trx ];
         editGUIDs << oedtGUIDs[ trx ];
         editIDs   << oedtIDs  [ trx ];
         dax1++;
      }

      dax2             = oldData.count();

      for ( int trx = dax1; trx < dax2; trx++ )
      {  // Copy any data following the current channel
         outData   << oldData  [ trx ];
         editGUIDs << oedtGUIDs[ trx ];
         editIDs   << oedtIDs  [ trx ];
      }
   }

   QString celchn   = celchns.at( triple_index );
   QString scell    = celchn.section( "/", 0, 0 ).simplified();
   QString schan    = celchn.section( "/", 1, 1 ).simplified();
   QString tripbase = scell + " / " + schan + " / ";
   int     idax     = triples.indexOf( tripbase + expc_wvlns[ 0 ] );
   int     odax     = index_data( 0 );
DbgLv(1) << "EDT:WrMwl:  dax celchn" << odax << celchn;

   QString filebase = files[ idax ].section( ".",  0, -6 )
                    + "." + editLabel + "."
                    + files[ idax ].section( ".", -5, -5 )
                    + "." + scell + "." + schan + ".";
   QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );

   // Loop to output a file/db-record for each wavelength of the cell/channel

DbgLv(1) << "EDT:WrMwl: files,wvlns.size" << files.size() << expc_wvlns.size();
   for ( wvx = 0; wvx < expc_wvlns.size(); wvx++ )
   {
      QString swavl    = expc_wvlns[ wvx ];
      QString triple   = tripbase + swavl;
      QString filename = filebase + swavl + ".xml";
      idax             = triples.indexOf( triple );
      odax             = index_data( wvx );
DbgLv(1) << "EDT:WrMwl:  wvx triple" << wvx << triple << "filename" << filename;

DbgLv(1) << "EDT:WrMwl:   idax,editGUIDs.size" << idax << editGUIDs.size();
      QString editGUID = editGUIDs[ idax ];

      if ( editGUID.isEmpty() )
      {
         editGUID      = US_Util::new_guid();
         editGUIDs.replace( idax, editGUID );
      }
DbgLv(1) << "EDT:WrMwl:    editGUID" << editGUID;

DbgLv(1) << "EDT:WrMwl:   odax,outData.size" << odax << outData.size();
      QString rawGUID  = US_Util::uuid_unparse(
            (unsigned char*)outData[ odax ]->rawGUID );
DbgLv(1) << "EDT:WrMwl:    rawGUID" << rawGUID;

      // Output the edit XML file
      le_info->setText( tr( "Writing " ) + filename + " ..." );
      qApp->processEvents();
      int wrstat       = write_xml_file( filename, triple, editGUID, rawGUID );
DbgLv(1) << "EDT:WrMwl:     write_xml_file stat" << wrstat;

      if ( wrstat != 0 )
         return;
      else
         editFnames[ idax ] = filename;

      if ( disk_controls->db() )
      {
         if ( dbP == NULL )
         {
            US_Passwd pw;
            dbP          = new US_DB2( pw.getPasswd() );
            if ( dbP == NULL  ||  dbP->lastErrno() != US_DB2::OK )
            {
               QMessageBox::warning( this, tr( "Connection Problem" ),
                 tr( "Could not connect to database \n" ) + dbP->lastError() );
               return;
            }
         }

         QString editID   = editIDs[ idax ];

         // Output the edit database record
         wrstat = write_edit_db( dbP, filename, editGUID, editID, rawGUID );
DbgLv(1) << "EDT:WrMwl:  dax fname" << idax << filename << "wrstat" << wrstat;

         if ( wrstat != 0 )
            return;
      }  // END:  DB output
   }  // END:  wavelength-in-cellchannel loop

   QApplication::restoreOverrideCursor();
   changes_made = false;
   pb_write    ->setEnabled( false );
   pb_writemwl ->setEnabled( false );
   pb_write    ->setIcon   ( check );
   pb_writemwl ->setIcon   ( check );
   le_info->setText( saved_info );
   qApp->processEvents();
DbgLv(1) << "EDT:WrMwl: DONE";
}

// Write edit xml file
int US_Edit::write_xml_file( QString& fname, QString& triple,
      QString& editGUID, QString& rawGUID )
{
   QFile efo( workingDir + fname );

   if ( ! efo.open( QFile::WriteOnly | QFile::Text ) )
   {
      QMessageBox::information( this,
            tr( "File write error" ),
            tr( "Could not open the file\n" ) + workingDir + fname
            + tr( "\n for writing.  Check your permissions." ) );
      return 1;
   }

DbgLv(1) << "EDT:WrXml: IN: fname,triple,editGUID,rawGUID"
 << fname << triple << editGUID << rawGUID;
   QXmlStreamWriter xml( &efo );

   xml.setAutoFormatting( true );
   xml.writeStartDocument();
   xml.writeDTD         ( "<!DOCTYPE UltraScanEdits>" );
   xml.writeStartElement( "experiment" );
   xml.writeAttribute   ( "type", expType );

   // Write identification
   xml.writeStartElement( "identification" );

   xml.writeStartElement( "runid" );
   xml.writeAttribute   ( "value", runID );
   xml.writeEndElement  ();

   xml.writeStartElement( "editGUID" );
   xml.writeAttribute   ( "value", editGUID );
   xml.writeEndElement  ();

   xml.writeStartElement( "rawDataGUID" );
   xml.writeAttribute   ( "value", rawGUID );
   xml.writeEndElement  ();

   xml.writeEndElement  ();  // identification

   QStringList parts   = triple.contains( " / " ) ?
                         triple.split( " / " ) :
                         triple.split( "." );
DbgLv(1) << "EDT:WrXml:  parts.size" << parts.size();

   QString     cell    = parts[ 0 ];
   QString     channel = parts[ 1 ];
   QString     waveln  = parts[ 2 ];

DbgLv(1) << "EDT:WrXml:  waveln" << waveln;

   xml.writeStartElement( "run" );
   xml.writeAttribute   ( "cell",       cell    );
   xml.writeAttribute   ( "channel",    channel );
   xml.writeAttribute   ( "wavelength", waveln  );

   // Write excluded scans
   if ( data.scanData.size() > includes.size() )
   {
      xml.writeStartElement( "excludes" );

      for ( int ii = 0; ii < data.scanData.size(); ii++ )
      {
         if ( ! includes.contains( ii ) )
         {
            xml.writeStartElement( "exclude" );
            xml.writeAttribute   ( "scan", QString::number( ii ) );
            xml.writeEndElement  ();
         }
      }

      xml.writeEndElement  ();  // excludes
   }

   // Write edits
   if ( ! changed_points.isEmpty() )
   {
      xml.writeStartElement( "edited" );

      for ( int ii = 0; ii < changed_points.size(); ii++ )
      {
         Edits* e = &changed_points[ ii ];

         for ( int jj = 0; jj < e->changes.size(); jj++ )
         {
            xml.writeStartElement( "edit" );
            xml.writeAttribute   ( "scan",   QString::number( e->scan ) );
            xml.writeAttribute   ( "radius",
               QString::number( e->changes[ jj ].x(), 'f', 4 ) );
            xml.writeAttribute   ( "value",
               QString::number( e->changes[ jj ].y(), 'f', 4 ) );
            xml.writeEndElement  ();
         }
      }

      xml.writeEndElement  ();  // edited
   }

   // Write meniscus, range, plateau, baseline, odlimit
   xml.writeStartElement( "parameters" );

   if ( ! expIsEquil )
   {  // non-Equilibrium
      xml.writeStartElement( "meniscus" );
      xml.writeAttribute   ( "radius",
         QString::number( meniscus, 'f', 4 ) );
      xml.writeEndElement  ();

      if ( dataType == "IP" )
      {
         xml.writeStartElement( "air_gap" );
         xml.writeAttribute   ( "left",
            QString::number( airGap_left,      'f', 4 ) );
         xml.writeAttribute   ( "right",
            QString::number( airGap_right,     'f', 4 ) );
         xml.writeAttribute   ( "tolerance",
            QString::number( ct_gaps->value(), 'f', 4 ) );
         xml.writeEndElement  ();
      }

      xml.writeStartElement( "data_range" );
      xml.writeAttribute   ( "left",
         QString::number( range_left,  'f', 4 ) );
      xml.writeAttribute   ( "right",
         QString::number( range_right, 'f', 4 ) );
      xml.writeEndElement  ();

      xml.writeStartElement( "plateau" );
      xml.writeAttribute   ( "radius",
         QString::number( plateau,  'f', 4 ) );
      xml.writeEndElement  ();

      xml.writeStartElement( "baseline" );
      xml.writeAttribute   ( "radius",
         QString::number( baseline, 'f', 4 ) );
      xml.writeEndElement  ();

      xml.writeStartElement( "od_limit" );
      xml.writeAttribute   ( "value",
         QString::number( odlimit, 'f', 4 ) );
      xml.writeEndElement  ();
   }

   else
   {  // Equilibrium
      if ( dataType == "IP" )
      {
         xml.writeStartElement( "air_gap" );
         xml.writeAttribute   ( "left",
            QString::number( airGap_left,      'f', 4 ) );
         xml.writeAttribute   ( "right",
            QString::number( airGap_right,     'f', 4 ) );
         xml.writeAttribute   ( "tolerance",
            QString::number( ct_gaps->value(), 'f', 4 ) );
         xml.writeEndElement  ();
      }

      int jsd  = sd_offs[ triple_index ];
      int ksd  = jsd + sd_knts[ triple_index ];

      for ( int ii = jsd; ii < ksd; ii++ )
      {
         double speed     = sData[ ii ].speed;
         int    sStart    = sData[ ii ].first_scan;
         int    sCount    = sData[ ii ].scan_count;
         double meniscus  = sData[ ii ].meniscus;
         double dataLeft  = sData[ ii ].dataLeft;
         double dataRight = sData[ ii ].dataRight;

         xml.writeStartElement( "speed" );
         xml.writeAttribute   ( "value",     QString::number( speed )  );
         xml.writeAttribute   ( "scanStart", QString::number( sStart ) );
         xml.writeAttribute   ( "scanCount", QString::number( sCount ) );

         xml.writeStartElement( "meniscus" );
         xml.writeAttribute   ( "radius", QString::number( meniscus, 'f', 4 ) );
         xml.writeEndElement  ();  // meniscus

         xml.writeStartElement( "data_range" );
         xml.writeAttribute   ( "left",  QString::number( dataLeft,  'f', 4 ) );
         xml.writeAttribute   ( "right", QString::number( dataRight, 'f', 4 ) );
         xml.writeEndElement  ();  // data_range

         xml.writeEndElement  ();  // speed
      }
   }
 
   xml.writeEndElement  ();  // parameters

   if ( ! pb_residuals->icon().isNull()  ||
        ! pb_spikes->icon().isNull()     ||
        invert == -1.0                   ||
        floatingData )
   {
      xml.writeStartElement( "operations" );
 
      // Write RI Noise
      if ( ! pb_residuals->icon().isNull() )
      {
         xml.writeStartElement( "subtract_ri_noise" );
         xml.writeAttribute   ( "order", QString::number( noise_order ) );
         xml.writeEndElement  ();
      }

      // Write Remove Spikes
      if ( ! pb_spikes->icon().isNull() )
      {
         xml.writeStartElement( "remove_spikes" );
         xml.writeEndElement  ();
      }

      // Write Invert
      if ( invert == -1.0 )
      {
         xml.writeStartElement( "invert" );
         xml.writeEndElement  ();
      }

      // Write indication of floating data
      if ( floatingData )
      {
         xml.writeStartElement( "floating_data" );
         xml.writeEndElement  ();
      }

      xml.writeEndElement  ();  // operations
   }

   xml.writeEndElement  ();  // run
   xml.writeEndElement  ();  // experiment
   xml.writeEndDocument ();

   efo.close();
   return 0;
}

// Write edit database record
int US_Edit::write_edit_db( US_DB2* dbP, QString& fname, QString& editGUID,
      QString& editID, QString& rawGUID )
{
   int idEdit;

   if ( dbP == NULL )
   {
      QMessageBox::warning( this, tr( "Connection Problem" ),
         tr( "Could not connect to database \n" ) + dbP->lastError() );
      return 1;
   }

   QStringList query( "get_rawDataID_from_GUID" );
   query << rawGUID;
   dbP->query( query );

   if ( dbP->lastErrno() != US_DB2::OK )
   {
      QMessageBox::warning( this, 
         tr( "AUC Data is not in DB" ),
         tr( "Cannot save edit data to the database.\n"
             "The associated AUC data is not present." ) );
      return 2;
   }

   dbP->next();
   QString rawDataID = dbP->value( 0 ).toString();


   // Save edit file to DB
   query.clear();

   if ( editID.isEmpty() )
   {
      query << "new_editedData" << rawDataID << editGUID << runID
            << fname << "";

      dbP->query( query );

      if ( dbP->lastErrno() != US_DB2::OK )
      {
         QMessageBox::warning( this, tr( "Database Problem" ),
            tr( "Could not insert metadata into the database\n" ) + 
                dbP->lastError() );

         return 3;
      }

      idEdit   = dbP->lastInsertID();
      editID   = QString::number( idEdit );
      int dax  = editGUIDs.indexOf( editGUID );
      editIDs.replace( dax, editID );
   }

   else
   {
      query << "update_editedData" << editID << rawDataID << editGUID
            << runID << fname << "";
      dbP->query( query );

      if ( dbP->lastErrno() != US_DB2::OK )
      {
         QMessageBox::warning( this, tr( "Database Problem" ),
            tr( "Could not update metadata in the database \n" ) + 
            dbP->lastError() );

         return 4;
      }

      idEdit   = editID.toInt();
   }

   dbP->writeBlobToDB( workingDir + fname, "upload_editData", idEdit );

   if ( dbP->lastErrno() != US_DB2::OK )
   {
      QMessageBox::warning( this, tr( "Database Problem" ),
         tr( "Could not insert edit xml data into database \n" ) + 
         dbP->lastError() );
      return 5;
   }

   return 0;
}

// Return the index in the output data of the current or specified wavelength
int US_Edit::index_data( int wvx )
{
   triple_index = cb_triple->currentIndex();  // Triple index
   int odatx    = triple_index;               // Default output data index

   if ( isMwl )
   {  // For MWL, compute data index from wavelength and triple indexes
      if ( wvx < 0 )
      {  // For the default case, use the current wavelength index
         plotndx      = cb_lplot->currentIndex();
         int iwavl    = expi_wvlns[ plotndx ];
         data_index   = mwl_data.data_index( iwavl, triple_index );
         odatx        = data_index;
      }

      else
      {  // Use a specified wavelength index (and do not set internal value)
         int iwavl    = expi_wvlns[ wvx ];
         odatx        = mwl_data.data_index( iwavl, triple_index );
      }
   }

   else  // for non-MWL, set data index internal value to triple index
      data_index   = triple_index;

   return odatx;
}

// Get a list of filenames with Edit Label like a specified file name
int US_Edit::like_edit_files( QString filename, QStringList& editfiles,
      US_DB2* dbP )
{
   // Determine local-disk files with same edit label and cell/channel
   QString filebase = filename.section( ".", 0, -3 );
   QStringList filter( filebase + ".*.xml" );
   QStringList ldfiles = QDir( workingDir ).entryList(
         filter, QDir::Files, QDir::Name );
   int     nledits  = ldfiles.count();
DbgLv(1) << "LiEdFi: nledits" << ldfiles.count() << filter;

   if ( dbP != NULL )
   {  // Determine database like files and compare to local
      QString     invID = QString::number( US_Settings::us_inv_ID() );
      QStringList dbfiles;
      QStringList dbEdIds;
      QStringList dbquery;
//      QString rawGUID   = US_Util::uuid_unparse( (unsigned char*)data.rawGUID );

      // Get the rawData ID for this data
//      dbquery.clear();
//      dbquery << "get_rawDataID_from_GUID" << rawGUID;
//      dbP->query( dbquery );
//      dbP->next();
//      QString rawDataID = dbP->value( 0 ).toString();
//DbgLv(1) << "LiEdFi:  rawDataID" << rawDataID;

      // Get the filenames and edit IDs for like-named files in the DB
      dbquery.clear();
//      dbquery << "get_editedDataIDs" << rawDataID;
      dbquery << "all_editedDataIDs" << invID;
      dbP->query( dbquery );

      while ( dbP->next() )
      {  // Accumulate filenames and editIDs from the database
         QString idData     = dbP->value( 0 ).toString();
         QString efname     = dbP->value( 2 ).toString()
                              .section( "/", -1, -1 );
//DbgLv(1) << "LiEdFi:   db0,2" << idData << efname << "fbase" << filebase;

         if ( efname.startsWith( filebase ) )
         {  // Where the file name has like edit label and cell/chan, save it
            dbfiles   << efname;
            dbEdIds   << idData;
DbgLv(1) << "LiEdFi:     id fn dbfsiz" << idData << efname << dbfiles.count();
         }
      }

      int nlocals      = ldfiles.count();
      nledits          = dbfiles.count();
DbgLv(1) << "LiEdFi:  nlocals nledits" << nlocals << nledits;

      if ( nledits != nlocals )
      {  // DB records are not matched locally, so download from DB
         if ( nlocals == 0 )
            QDir().mkpath( workingDir );

         dbfiles.sort();

         for ( int ii = 0; ii < dbfiles.count(); ii++ )
         {  // Download each DB record to a local file
            QString fname     = workingDir + dbfiles[ ii ];
            int     dataID    = dbEdIds[ ii ].toInt();

            dbP->readBlobFromDB( fname, "download_editData", dataID );
         }

         ldfiles           = dbfiles;
      }
   }

   editfiles        = ldfiles;
   nledits          = editfiles.count();

   if ( nledits < 2 )
   {  // Only one file exists, so just pass it back
      editfiles << filename;
      nledits          = 1;
   }

   else
   {  // If multiple files, sort them
      editfiles.sort();
   }
DbgLv(1) << "LiEdFi: nle fn0" << nledits << editfiles[0];

   return nledits;
}

// Apply edits to the current triple data
int US_Edit::apply_edits( US_DataIO::EditValues parameters )
{
   int status = 0;

   // Apply the edits with specified parameters
   QString wkstr;

   meniscus    = parameters.meniscus;
   range_left  = parameters.rangeLeft;
   range_right = parameters.rangeRight;
   plateau     = parameters.plateau;
   baseline    = parameters.baseline;
   odlimit     = parameters.ODlimit;

   le_meniscus->setText( wkstr.sprintf( "%.3f", meniscus ) );
   pb_meniscus->setIcon( check );
   pb_meniscus->setEnabled( true );

   airGap_left  = parameters.airGapLeft;
   airGap_right = parameters.airGapRight;

   if ( dataType == "IP" )
   {
      US_DataIO::adjust_interference( data, parameters );
      US_DataIO::calc_integral      ( data, parameters );
      le_airGap->setText( wkstr.sprintf( "%.3f - %.3f", 
               airGap_left, airGap_right ) ); 
      pb_airGap->setIcon( check );
      pb_airGap->setEnabled( true );
   }

   le_dataRange->setText( wkstr.sprintf( "%.3f - %.3f",
           range_left, range_right ) );
   pb_dataRange->setIcon( check );
   pb_dataRange->setEnabled( true );
   
   le_plateau  ->setText( wkstr.sprintf( "%.3f", plateau ) );
   pb_plateau  ->setIcon( check );
   pb_plateau  ->setEnabled( true );

   US_DataIO::Scan  scan  = data.scanData.last();
   int              pt    = data.xindex( baseline );
   double           sum   = 0.0;

   // Average the value for +/- 5 points
   for ( int jj = pt - 5; jj <= pt + 5; jj++ )
      sum += scan.rvalues[ jj ];

   le_baseline->setText( wkstr.sprintf( "%.3f (%.3e)", baseline, sum / 11.0 ) );

   // Invert
   invert = parameters.invert;
   
   if ( invert == -1.0 ) pb_invert->setIcon( check );
   else                  pb_invert->setIcon( QIcon() );

   // Excluded scans
   init_includes();
   reset_excludes(); // Zero exclude combo boxes
   qSort( parameters.excludes );

   for ( int ii = parameters.excludes.size(); ii > 0; ii-- )
      includes.removeAt( parameters.excludes[ ii - 1 ] );

   // Edited points
   changed_points.clear();

   for ( int ii = 0; ii < parameters.editedPoints.size(); ii++ )
   {
      int    scan   = parameters.editedPoints[ ii ].scan;
      int    index1 = (int)parameters.editedPoints[ ii ].radius;
      double value  = parameters.editedPoints[ ii ].value;
      
      Edits e;
      e.scan = scan;
      e.changes << QPointF( index1, value );
     
      changed_points << e;
      
      data.scanData[ scan ].rvalues[ index1 ] = value;
   }

   // Spikes
   spikes = parameters.removeSpikes;

   pb_spikes->setIcon( QIcon() );
   pb_spikes->setEnabled( true );
   if ( spikes ) remove_spikes();
   
   // Noise
   noise_order = parameters.noiseOrder;
   if ( noise_order > 0 )
   {
      US_RiNoise::calc_residuals( data, includes, range_left, range_right, 
            noise_order, residuals );
      
      subtract_residuals();
   }
   else
   {
      pb_noise    ->setIcon( QIcon() );
      pb_residuals->setIcon( QIcon() );
      pb_residuals->setEnabled( false );
   }

   // Floating data
   floatingData = parameters.floatingData;
   if ( floatingData )
      pb_float->setIcon( check );
   else
      pb_float->setIcon( QIcon() );

   ct_odlim->disconnect();
   ct_odlim->setValue( odlimit );
   connect( ct_odlim,  SIGNAL( valueChanged       ( double ) ),
            this,      SLOT  ( od_radius_limit    ( double ) ) );

   set_pbColors( NULL );
   step        = FINISHED;

   return status;
}

