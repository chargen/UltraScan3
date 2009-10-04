//! \file us_analysis_base.cpp

#include "us_analysis_base.h"
#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_run_details.h"
#include "us_vbar.h"
#include "us_buffer.h"
#include "us_math.h"

US_AnalysisBase::US_AnalysisBase() : US_Widgets()
{
   setPalette( US_GuiSettings::frameColor() );


   mainLayout      = new QHBoxLayout( this );
   mainLayout->setSpacing        ( 2 );
   mainLayout->setContentsMargins( 2, 2, 2, 2 );

   leftLayout      = new QVBoxLayout();
   rightLayout     = new QVBoxLayout();
   
   analysisLayout  = new QGridLayout();
   runInfoLayout   = new QGridLayout();
   parameterLayout = new QGridLayout();
   controlsLayout  = new QGridLayout();
   buttonLayout    = new QHBoxLayout();

   leftLayout->addLayout( analysisLayout  );
   leftLayout->addLayout( runInfoLayout   );
   leftLayout->addLayout( parameterLayout );
   leftLayout->addLayout( controlsLayout  );
   leftLayout->addStretch();
   leftLayout->addLayout( buttonLayout    );

   // Plots
   plotLayout1 = new US_Plot( data_plot1,
            tr( "Plot 1 Title" ),
            tr( "X-Axis Title" ),
            tr( "Y-Axis Title" ) );

   data_plot1->setMinimumSize( 600, 300 );

   plotLayout2 = new US_Plot( data_plot2,
            tr( "Plot 2 Title" ),
            tr( "X-Axis Title" ),
            tr( "Y-Axis Title" ) );

   data_plot2->setMinimumSize( 600, 300 );

   rightLayout->addLayout( plotLayout1 );
   rightLayout->addLayout( plotLayout2 );

   mainLayout->addLayout( leftLayout  );
   mainLayout->addLayout( rightLayout );

   // Analysis buttons
   pb_load    = us_pushbutton( tr( "Load Data" ) );
   connect( pb_load, SIGNAL( clicked() ), SLOT( load() ) );
   pb_details = us_pushbutton( tr( "Run Details" ) );
   connect( pb_details, SIGNAL( clicked() ), SLOT( details() ) );
   pb_view    = us_pushbutton( tr( "View Data Report" ) );
   pb_save    = us_pushbutton( tr( "Save Data" ) );

   pb_details->setEnabled( false );
   pb_view   ->setEnabled( false );
   pb_save   ->setEnabled( false );

   analysisLayout->addWidget( pb_load,    0, 0 );
   analysisLayout->addWidget( pb_details, 0, 1 );
   analysisLayout->addWidget( pb_view,    1, 0 );
   analysisLayout->addWidget( pb_save,    1, 1 );

   // Standard buttons
   pb_reset = us_pushbutton( tr( "Reset" ) );
   pb_help  = us_pushbutton( tr( "Help"  ) );
   pb_close = us_pushbutton( tr( "Close" ) );

   buttonLayout->addWidget( pb_reset );
   connect( pb_reset, SIGNAL( clicked() ), SLOT( reset() ) );
   buttonLayout->addWidget( pb_help  );
   buttonLayout->addWidget( pb_close );
   connect( pb_close, SIGNAL( clicked() ), SLOT( close() ) );

   // Run info
   QLabel* lb_info    = us_banner( tr( "Information for this Run" ) );
   QLabel* lb_triples = us_banner( tr( "Cell / Channel / Wavelength" ) );
   QLabel* lb_id      = us_label ( tr( "Run ID / Edit ID:" ) );
   QLabel* lb_temp    = us_label ( tr( "Avg Temperature:" ) );

   le_id      = us_lineedit();
   le_temp    = us_lineedit();

   te_desc    = us_textedit();
   lw_triples = us_listwidget();
   connect( lw_triples, SIGNAL( currentRowChanged( int ) ), 
                        SLOT  ( new_triple       ( int ) ) );

   QFont        font( US_GuiSettings::fontFamily(), US_GuiSettings::fontSize() );
   QFontMetrics fm  ( font );

   int fontHeight = fm.lineSpacing();

   te_desc   ->setMaximumHeight( fontHeight * 2 + 12 );  // Add for border
   lw_triples->setMaximumHeight( fontHeight * 4 + 12 );

   le_id     ->setReadOnly( true );
   le_temp   ->setReadOnly( true );
   te_desc   ->setReadOnly( true );

   runInfoLayout->addWidget( lb_info   , 0, 0, 1, 2 );
   runInfoLayout->addWidget( lb_id     , 1, 0 );
   runInfoLayout->addWidget( le_id     , 1, 1 );
   runInfoLayout->addWidget( lb_temp   , 2, 0 );
   runInfoLayout->addWidget( le_temp   , 2, 1 );
   runInfoLayout->addWidget( te_desc   , 3, 0, 2, 2 );
   runInfoLayout->addWidget( lb_triples, 5, 0, 1, 2 );
   runInfoLayout->addWidget( lw_triples, 6, 0, 4, 2 );

   // Parameters

   QPushButton* pb_density = us_pushbutton( tr( "Density"   ) );
   connect( pb_density, SIGNAL( clicked() ), SLOT( get_buffer() ) );
   
   QPushButton* pb_viscosity = us_pushbutton( tr( "Viscosity" ) );
   connect( pb_viscosity, SIGNAL( clicked() ), SLOT( get_buffer() ) );
   
   QPushButton* pb_vbar = us_pushbutton( tr( "vbar"   ) );
   connect( pb_vbar, SIGNAL( clicked() ), SLOT( get_vbar() ) );
   
   QLabel* lb_skipped   = us_label     ( tr( "Skipped:"  ) );

   le_density   = us_lineedit( "0.998234" );
   le_viscosity = us_lineedit( "1.001940" );
   le_vbar      = us_lineedit( "0.7200" );
   le_skipped   = us_lineedit( "0" );
   le_skipped->setReadOnly( true );

   density   = 0.998234;
   viscosity = 1.001940;
   vbar      = 0.72;

   parameterLayout->addWidget( pb_density  , 0, 0 );
   parameterLayout->addWidget( le_density  , 0, 1 );
   parameterLayout->addWidget( pb_viscosity, 0, 2 );
   parameterLayout->addWidget( le_viscosity, 0, 3 );
   parameterLayout->addWidget( pb_vbar     , 1, 0 );
   parameterLayout->addWidget( le_vbar     , 1, 1 );
   parameterLayout->addWidget( lb_skipped  , 1, 2 );
   parameterLayout->addWidget( le_skipped  , 1, 3 );

   // Analysis Controls

   QLabel* lb_analysis     = us_banner( tr( "Analysis Controls"  ) ); 
   QLabel* lb_scan         = us_banner( tr( "Scan Control"       ) ); 
   QLabel* lb_smoothing    = us_label ( tr( "Data Smoothing:"    ) ); 
   QLabel* lb_boundPercent = us_label ( tr( "% of Boundary:"     ) ); 
   QLabel* lb_boundPos     = us_label ( tr( "Boundary Pos. (%):" ) ); 

   QLabel* lb_from         = us_label ( tr( "From:" ) );
   QLabel* lb_to           = us_label ( tr( "to:"   ) );

   pb_exclude = us_pushbutton( tr( "Exclude Scan Range" ) );
   pb_exclude->setEnabled( false );
   connect( pb_exclude, SIGNAL( clicked() ), SLOT( exclude() ) );

   ct_smoothing = us_counter( 2,  1,  50,  1 );
   ct_smoothing->setStep( 1.0 );
   connect( ct_smoothing, SIGNAL( valueChanged( double ) ),
                          SLOT  ( smoothing   ( double ) ) );

   ct_boundaryPercent = us_counter( 3, 10, 100, 90 );
   ct_boundaryPos     = us_counter( 3,  0,  10,  5 );
   ct_boundaryPercent->setStep( 0.1 );
   ct_boundaryPos    ->setStep( 0.1 );
   connect( ct_boundaryPercent, SIGNAL( valueChanged( double ) ),
                                SLOT  ( boundary_pct( double ) ) );
   connect( ct_boundaryPos,     SIGNAL( valueChanged( double ) ),
                                SLOT  ( boundary_pos( double ) ) );
   
   ct_from            = us_counter( 2, 0, 0 );
   ct_to              = us_counter( 2, 0, 0 );
   
   connect( ct_from, SIGNAL( valueChanged( double ) ),
                     SLOT  ( exclude_from( double ) ) );
   connect( ct_to,   SIGNAL( valueChanged( double ) ),
                     SLOT  ( exclude_to  ( double ) ) );

   controlsLayout->addWidget( lb_analysis       , 0, 0, 1, 4 );
   controlsLayout->addWidget( lb_smoothing      , 1, 0, 1, 2 );
   controlsLayout->addWidget( ct_smoothing      , 1, 2, 1, 2 );
   controlsLayout->addWidget( lb_boundPercent   , 2, 0, 1, 2 );
   controlsLayout->addWidget( ct_boundaryPercent, 2, 2, 1, 2 );
   controlsLayout->addWidget( lb_boundPos       , 3, 0, 1, 2 );
   controlsLayout->addWidget( ct_boundaryPos    , 3, 2, 1, 2 );
   controlsLayout->addWidget( lb_scan           , 4, 0, 1, 4 );
   controlsLayout->addWidget( lb_from           , 5, 0 );
   controlsLayout->addWidget( ct_from           , 5, 1 );
   controlsLayout->addWidget( lb_to             , 5, 2 );
   controlsLayout->addWidget( ct_to             , 5, 3 );
   controlsLayout->addWidget( pb_exclude        , 6, 0, 1, 4 );

   dataLoaded = false;
}

void US_AnalysisBase::load( void )
{
   // Determine the edit ID
   dataLoaded     = false;
   reset();
   QString filter = "*.*.*.*.*.*.xml";

   QString filename = QFileDialog::getOpenFileName( this, 
         tr( "Select a file with the desired Edit ID" ),
         US_Settings::resultDir(),
         filter );

   if ( filename.isEmpty() ) return;

   QStringList sl = filename.split( "." );
   runID  = sl[ 0 ];
   editID = sl[ 1 ];

   filename.replace( "\\", "/" );  // For WIN32
   directory = filename.left( filename.lastIndexOf( "/" ) );
   
   // Get the raw data and edit parameters file names
   filter = "*." + editID + ".*.*.*.*.xml";
   
   QDir d( directory );
   sl = d.entryList( QStringList() << filter, 
                     QDir::Files | QDir::Readable, QDir::Name );

   // Read the data into the structure
   lw_triples  ->clear();
   dataList     .clear();
   rawList      .clear();
   excludedScans.clear();
   triples      .clear();

   try
   {
      for ( int i = 0; i < sl.size(); i++ )
      {
         filename = sl[ i ];
         US_DataIO::loadData( directory, filename, dataList, rawList );

         US_DataIO::editedData* ed = &dataList[ i ];

         QString ccw = ed->cell + " / " + ed->channel + " / " + ed->wavelength;

         lw_triples->addItem( ccw );
         triples << ccw;
      }
   }
   catch ( US_DataIO::ioError error )
   {
      // There was an error reading filname or it's associated raw data
      QString message = US_DataIO::errorString( error );
      QMessageBox::critical( this, tr( "Bad Data" ), message );
      return;
   }

   savedValues.clear();

   for ( int i = 0; i < dataList[ 0 ].scanData.size(); i++ )
   {
      US_DataIO::scan* s = &dataList[ 0 ].scanData[ i ];
      int points = s->readings.size();

      QVector< double > v;
      v.resize( points );

      for ( int j = 0; j < points; j++ ) v[ j ] = s->readings[ j ].value;

      savedValues << v;
   }

   lw_triples->setCurrentRow( 0 );
   update( 0 );

   // Enable other buttons
   pb_details->setEnabled( true );
   pb_view   ->setEnabled( true );
   pb_save   ->setEnabled( true );
   pb_exclude->setEnabled( true );

   ct_from->disconnect();
   ct_from->setValue( 0 );

   connect( ct_from, SIGNAL( valueChanged( double ) ),
                     SLOT  ( exclude_from( double ) ) );
   dataLoaded = true;
}

void US_AnalysisBase::update( int selection )
{
   US_DataIO::editedData* d = &dataList[ selection ];
   int scanCount = d->scanData.size();
   le_id->setText( d->runID + " / " + d->editID );

   double sum = 0.0;
   
   for ( int i = 0; i < scanCount; i++ ) 
      sum += d->scanData[ i ].temperature;

   QString t = QString::number( sum / scanCount, 'f', 1 ) 
             + tr( " deg C" );
   le_temp->setText( t );

   te_desc->setText( d->description );

   ct_smoothing      ->setValue( 1  );  // Signals?
   ct_boundaryPercent->setValue( 90 );
   ct_boundaryPos    ->setValue( 5  );

   ct_from->setMaxValue( scanCount - excludedScans.size() );
   ct_from->setStep( 1.0 );
   ct_to  ->setMaxValue( scanCount - excludedScans.size() );
   ct_to  ->setStep( 1.0 );

   data_plot();
}

void US_AnalysisBase::details( void )
{
   US_RunDetails* dialog
      = new US_RunDetails( rawList, runID, directory, triples );
   dialog->exec();
   qApp->processEvents();
   delete dialog;
}

void US_AnalysisBase::get_vbar( void )
{
   US_Vbar* vbar_dialog = new US_Vbar( -1, true ); // Delete on close set
   connect( vbar_dialog, SIGNAL( valueChanged( double ) ),
                         SLOT  ( update_vbar ( double ) ) );
   vbar_dialog->exec();
   qApp->processEvents();
}

void US_AnalysisBase::update_vbar( double new_vbar )
{
   vbar = new_vbar;
   le_vbar->setText( QString::number( new_vbar, 'f', 4 ) );
}

void US_AnalysisBase::get_buffer( void )
{
   US_Buffer_DB* buf_dialog = new US_Buffer_DB( true ); // Delete on close set
   connect( buf_dialog, SIGNAL( valueChanged ( double, double ) ),
                        SLOT  ( update_buffer( double, double ) ) );
   buf_dialog->exec();
   qApp->processEvents();
}

void US_AnalysisBase::update_buffer( double new_density, double new_viscosity )
{
   density   = new_density;
   viscosity = new_viscosity;

   le_density  ->setText( QString::number( new_density,   'f', 6 ) );
   le_viscosity->setText( QString::number( new_viscosity, 'f', 6 ) );
}

void US_AnalysisBase::data_plot( void )
{
   int                    row  = lw_triples->currentRow();
   US_DataIO::editedData* d    = &dataList[ row ];

   QString header = tr( "Velocity Data for ") + d->runID;
   data_plot2->setTitle( header );

   header = tr( "Absorbance at " ) + d->wavelength + tr( " nm" );
   data_plot2->setAxisTitle( QwtPlot::yLeft, header );
   data_plot2->clear();
   us_grid( data_plot2 );

   int     scan_number = 0;
   int     from        = (int)ct_from->value();
   int     to          = (int)ct_to  ->value();

   int     scanCount   = d->scanData.size();
   int     points      = d->scanData[ 0 ].readings.size();
   double  boundaryPct = ct_boundaryPercent->value() / 100.0;
   double  positionPct = ct_boundaryPos    ->value() / 100.0;
   double  baseline    = calc_baseline();
   double* r           = new double[ points ];
   double* v           = new double[ points ];

   for ( int i = 0; i < scanCount; i++ )
   {
      if ( excludedScans.contains( i ) ) continue;

      scan_number++;
      bool highlight = scan_number >= from  &&  scan_number <= to;

      US_DataIO::scan* s = &d->scanData[ i ];

      double range       = s->plateau - baseline;
      double lower_limit = baseline    + range * positionPct;
      double upper_limit = lower_limit + range * boundaryPct;

      int j     = 0;
      int count = 0;

      // Plot each scan in (up to) three segments: below, in, and above
      // the specified boundaries
      while ( s->readings[ j ].value < lower_limit  &&  j < points )
      {
         r[ count ] = s->readings[ j ].d.radius;
         v[ count ] = s->readings[ j ].value;
         j++;
         count++;
      }

      QString       title; 
      QwtPlotCurve* c;
      
      if ( count > 1 )
      {
         title = tr( "Curve " ) + QString::number( i ) + tr( " below range" );
         c     = us_curve( data_plot2, title );

         if ( highlight )
            c->setPen( QPen( Qt::red ) );
         else
            c->setPen( QPen( Qt::cyan ) );
         
         c->setData( r, v, count );
      }

      count = 0;

      while ( s->readings[ j ].value < upper_limit  &&  j < points )
      {
         r[ count ] = s->readings[ j ].d.radius;
         v[ count ] = s->readings[ j ].value;
         j++;
         count++;
      }

      if ( count > 1 )
      {
         title = tr( "Curve " ) + QString::number( i ) + tr( " in range" );
         c = us_curve( data_plot2, title );

         if ( highlight )
            c->setPen( QPen( Qt::red ) );
         else
            c->setPen( QPen( US_GuiSettings::plotCurve() ) );
         
         c->setData( r, v, count );
      }

      count = 0;

      while ( j < points )
      {
         r[ count ] = s->readings[ j ].d.radius;
         v[ count ] = s->readings[ j ].value;
         j++;
         count++;
      }

      if ( count > 1 )
      {
         title = tr( "Curve " ) + QString::number( i ) + tr( " above range" );
         c = us_curve( data_plot2, title );

         if ( highlight )
            c->setPen( QPen( Qt::red ) );
         else
            c->setPen( QPen( Qt::cyan ) );
        
         c->setData( r, v, count );
      }
   }

   data_plot2->replot();

   delete [] r;
   delete [] v;
}

void US_AnalysisBase::boundary_pct( double percent )
{
   ct_boundaryPos->disconnect();
   ct_boundaryPos->setMaxValue( 100 - percent );

   if ( ct_boundaryPos->value() >  100 - percent )
      ct_boundaryPos->setValue( 100.0 - percent );

   connect( ct_boundaryPos, SIGNAL( valueChanged( double ) ),
                            SLOT  ( boundary_pos( double ) ) );
   data_plot();
}

void US_AnalysisBase::boundary_pos( double /* position */ )
{
   data_plot();
}

void US_AnalysisBase::exclude_from( double from )
{
   double to = ct_to->value();

   if ( to < from )
   {
      ct_to->disconnect();
      ct_to->setValue( from );

      connect( ct_to,   SIGNAL( valueChanged( double ) ),
                        SLOT  ( exclude_to  ( double ) ) );
   }

   data_plot();
}

void US_AnalysisBase::exclude_to( double to )
{
   double from = ct_from->value();

   if ( from > to )
   {
      ct_from->disconnect();
      ct_from->setValue( to );

      connect( ct_from, SIGNAL( valueChanged( double ) ),
                        SLOT  ( exclude_from( double ) ) );
   }

   data_plot();
}

void US_AnalysisBase::exclude( void )
{
   double from = ct_from->value();
   double to   = ct_to  ->value();

   int                    displayedScan = 1; 
   int                    index         = lw_triples->currentRow();
   US_DataIO::editedData* d             = &dataList[ index ];
   int                    totalScans    = d->scanData.size();
   
   for( int i = 0; i < totalScans; i++ )
   {
      if ( excludedScans.contains( i ) ) continue;
      
      if ( displayedScan >= from  &&  displayedScan <= to ) excludedScans << i;
   
      displayedScan++;
   }

   ct_to->setValue( 0 );  // Resets both counters and replots

   ct_from->setMaxValue( totalScans - excludedScans.size() );
   ct_to  ->setMaxValue( totalScans - excludedScans.size() );
}


void US_AnalysisBase::smoothing( double smoothCount )
{
   int smoothPoints = (int) smoothCount;

   // Restore saved data
   int                    index  = lw_triples->currentRow();
   US_DataIO::editedData* d      = &dataList[ index ];
   
   for ( int i = 0; i < d->scanData.size(); i++ )
   {
      US_DataIO::scan* s = &d->scanData[ i ];

      for ( int j = 0; j < s->readings.size(); j++ )
         s->readings[ j ].value = savedValues[ i ][ j ];
   }
   
   // Smooth the data
   if ( smoothPoints > 1 )
   {
      x_weights = new double [ smoothPoints ];
      y_weights = new double [ smoothPoints ];
                  
      // Divide the count into 2 standard deviations
      double increment = 2.0 / smoothCount;

      // Only calculate half a Gaussian curve, since the other side is symmetric
      for ( int i = 0; i < smoothPoints; i++ ) 
      {
         x_weights[ i ] = increment * i;

         // Use a standard deviation of 0.7 to narrow the spread and multiply
         // by 0.7 to scale the result as an empirical weighting factor
         
         // Standard deviation = 0.7, mean = 0.0, point = 0.0;
         y_weights[ i ] = 
            0.7 * US_Math::normal_distribution( 0.7, 0.0, x_weights[ i ] );
      }

      // For each scan
      for ( int i = 0; i < d->scanData.size(); i++ )
      {
         US_DataIO::scan* s          = &d->scanData[ i ];
         int              scanPoints = s->readings.size();
         
         // Loop over all border point centers
         for ( int j = 0; j < smoothPoints; j++ )
         {
            s->readings[ j ].value = smooth_point( i, j, -1, smoothPoints );
         }
      
         // Now deal with all non-border points
         for ( int j = smoothPoints; j < scanPoints - smoothPoints - 1; j++ )
         {
            s->readings[ j ].value = smooth_point( i, j,  0, smoothPoints );
         }

         // Finally the points on the right border
         for ( int j = scanPoints - smoothPoints - 1; j < scanPoints; j++ )
         {
            s->readings[ j ].value 
               = smooth_point( i, j,  1, smoothPoints, scanPoints );
         }
      }
   
      delete [] x_weights;
      delete [] y_weights;
   }
   
   data_plot();
}

double US_AnalysisBase::smooth_point( 
      int scan, int point, int type, int smoothPoints, int scanPoints )
{
   // type ==  0 means no reflection
   // type ==  1 means to reflect on the right
   // type == -1 means to reflect on the left

   double  sum      = 0.0;
   double  sum_y    = 0.0;
   int     start;
   int     stop;
   int     direction;

   // Sum all applicable points left of center
   if ( type == -1 ) // reflect left
   {
      start     = point + 1;
      stop      = point + smoothPoints;
      direction = 1;
   }
   else
   {
      start     = point - 1;
      stop      = point - smoothPoints;
      direction = -1;
   }

   // This is a bit complex because the test for leaving the loop
   // is different if we are incrementing or decrementing.
   
   int position = 0;
   int k        = start;
   
   do 
   {
      position++;
      double value = savedValues[ scan ][ k ];

      if ( type ==  -1 )
      {
         if ( point - position < 0 ) // we need a reflected value
         {
            double dy = savedValues[ scan ][ k ] - savedValues[ scan ][ point ];
            value     = savedValues[ scan ][ point ] - dy;
         }
         else
            value     = savedValues[ scan ][ point - position ];
      }
      
      sum   += value * y_weights[ position ];
      sum_y +=         y_weights[ position ];

      if ( type == -1 )
      {
         if ( k > stop ) break;
      }
      else
      {
         if ( k <= stop ) break;
      }

      k += direction;
   } while ( true );

   // Add the center point
   sum   += savedValues[ scan ][ point ] * y_weights[ 0 ];
   sum_y +=                                y_weights[ 0 ];
   
   // Sum all applicable points right of center
   if ( type == 1 ) // reflect right
   {
      start     = point - 1;
      stop      = point - smoothPoints;
      direction = -1;
   }
   else
   {
      start     = point + 1;
      stop      = point + smoothPoints;
      direction = 1;
   }

   position = 0;
   k        = start;

   do
   {
      position++;
      double value = savedValues[ scan ][ k ];

      if ( type == 1 )
      {
         if ( point + position >= scanPoints ) // Need reflection
         {
            double dy = savedValues[ scan ][ k ] 
                      - savedValues[ scan ][ point ];
            value     = savedValues[ scan ][ point ] - dy;
         }
         else
            value     = savedValues[ scan ][ point + position ];
      }

      sum   += value * y_weights[ position ];
      sum_y +=         y_weights[ position ];

      if ( type == 1 )
      {
         if ( k <= stop ) break;
      }
      else
      {
         if ( k > stop ) break;
      }

      k += direction;
   } while ( true );

   // Normalize by the sum of all weights that were used 
   return sum / sum_y;
}

void US_AnalysisBase::reset( void )
{
   if ( ! dataLoaded ) return;

   excludedScans.clear();

   le_density  ->setText( "0.998234" );
   le_viscosity->setText( "1.001940" );
   le_vbar     ->setText( "0.7200"   );
   le_skipped  ->setText( "0" );

   density   = 0.998234;
   viscosity = 1.001940;
   vbar      = 0.72;

   // Restore saved data
   int                    index  = lw_triples->currentRow();
   US_DataIO::editedData* d      = &dataList[ index ];
   
   for ( int i = 0; i < d->scanData.size(); i++ )
   {
      US_DataIO::scan* s = &d->scanData[ i ];

      for ( int j = 0; j < s->readings.size(); j++ )
         s->readings[ j ].value = savedValues[ i ][ j ];
   }

   ct_from           ->disconnect();
   ct_to             ->disconnect();
   ct_smoothing      ->disconnect();
   ct_boundaryPercent->disconnect();
   ct_boundaryPos    ->disconnect();
   
   ct_from           ->setValue( 0 );
   ct_to             ->setValue( 0 );
   ct_smoothing      ->setValue( 1 );
   ct_boundaryPercent->setValue( 90 );
   ct_boundaryPos    ->setValue( 5 );

   connect( ct_from,            SIGNAL( valueChanged( double ) ),
                                SLOT  ( exclude_from( double ) ) );
                                
   connect( ct_to,              SIGNAL( valueChanged( double ) ),
                                SLOT  ( exclude_to  ( double ) ) );

   connect( ct_boundaryPercent, SIGNAL( valueChanged( double ) ),
                                SLOT  ( boundary_pct( double ) ) );

   connect( ct_boundaryPos,     SIGNAL( valueChanged( double ) ),
                                SLOT  ( boundary_pos( double ) ) );
   
   connect( ct_smoothing,       SIGNAL( valueChanged( double ) ),
                                SLOT  ( smoothing   ( double ) ) );
   data_plot();
   
}

void US_AnalysisBase::new_triple( int index )
{
   // Save the data for the new triple
   US_DataIO::editedData* d = &dataList[ index ];
   
   for ( int i = 0; i < d->scanData.size(); i++ )
   {
      US_DataIO::scan* s = &d->scanData[ i ];

      for ( int j = 0; j < s->readings.size(); j++ )
         savedValues[ i ][ j ] = s->readings[ j ].value;
   }

   reset();
   data_plot();
}

QString US_AnalysisBase::table_row( const QString& s1, const QString& s2 )
{
   QString s = "<tr><td>" + s1 + "</td><td>" + s2 + "</td></tr>\n";
   return s;
}

QString US_AnalysisBase::table_row( const QString& s1, const QString& s2, 
                                    const QString& s3 )
{
   QString s = "<tr><td>" + s1 + "</td><td>" + s2 + "</td><td>" + s3 
             + "</td></tr>\n";
   return s;
}

double US_AnalysisBase::calc_baseline( void )
{
   int              row   = lw_triples->currentRow();
   US_DataIO::scan* scan  = &dataList[ row ].scanData.last();
   int              point = US_DataIO::index( *scan, dataList[ row ].baseline );
   double           sum   = 0.0;
   
   for ( int j = point - 5;  j <= point + 5; j++ )
      sum += scan->readings[ j ].value;

   return sum / 11.0;
}
