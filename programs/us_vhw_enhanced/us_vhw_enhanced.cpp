//! \file us_vhw_enhanced.cpp

#include <QApplication>

#include <uuid/uuid.h>

#include "us_vhw_enhanced.h"
#include "us_license_t.h"
#include "us_license.h"
#include "us_settings.h"
#include "us_gui_settings.h"
#include "us_matrix.h"
#include "us_constants.h"

// main program
int main( int argc, char* argv[] )
{
   QApplication application( argc, argv );

   #include "main1.inc"

   // License is OK.  Start up.
   
   US_vHW_Enhanced w;
   w.show();                   //!< \memberof QWidget
   return application.exec();  //!< \memberof QApplication
}

// US_vHW_Enhanced class constructor
US_vHW_Enhanced::US_vHW_Enhanced() : US_AnalysisBase()
{
   // set up the GUI (mostly handled in US_AnalysisBase)

   setWindowTitle( tr( "Enhanced van Holde - Weischet Analysis:" ) );

   pb_dstrpl     = us_pushbutton( tr( "Distribution Plot" ) );
   pb_dstrpl->setEnabled( false );
   connect( pb_dstrpl, SIGNAL( clicked() ),
            this,       SLOT(  distr_plot() ) );

   pb_selegr     = us_pushbutton( tr( "Select Groups" ) );
   pb_selegr->setEnabled( true );
   connect( pb_selegr, SIGNAL( clicked() ),
            this,       SLOT(  sel_groups() ) );

   parameterLayout->addWidget( pb_dstrpl, 2, 0, 1, 2 );
   parameterLayout->addWidget( pb_selegr, 2, 2, 1, 2 );

   QLayoutItem* litem = parameterLayout->itemAtPosition( 1, 0 );
   QWidget*     witem = litem->widget();

   if ( witem )
   {  // change "vbar" to "Vbar" on pushbutton
      QPushButton* bitem = (QPushButton*)witem;
      bitem->setText( "Vbar" );
   }
   else
      qDebug() << "parLay 1,0 item is NOT widget";

   QLabel* lb_analysis     = us_banner( tr( "Analysis Controls"      ) );
   QLabel* lb_scan         = us_banner( tr( "Scan Control"           ) );
   QLabel* lb_smoothing    = us_label ( tr( "Data Smoothing:"        ) );
   QLabel* lb_boundPercent = us_label ( tr( "% of Boundary:"         ) );
   QLabel* lb_boundPos     = us_label ( tr( "Boundary Position (%):" ) );

   QLabel* lb_from         = us_label ( tr( "From:" ) );
   QLabel* lb_to           = us_label ( tr( "to:"   ) );

   lb_tolerance  = us_label( tr( "Back Diffusion Tolerance:" ) );
   lb_tolerance->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );

   ct_tolerance = us_counter( 3, 0.0, 1000.0, 0.001 );
   bdtoler      = 0.001;
   ct_tolerance->setStep( 0.001 );
   ct_tolerance->setEnabled( true );
   connect( ct_tolerance, SIGNAL( valueChanged(   double ) ),
            this,          SLOT(  update_bdtoler( double ) ) );

   lb_division   = us_label( tr( "Divisions:" ) );
   lb_division->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );

   ct_division  = us_counter( 3, 0.0, 1000.0, 50.0 );
   ct_division->setStep( 1 );
   ct_division->setEnabled( true );
   connect( ct_division, SIGNAL( valueChanged(  double ) ),
            this,         SLOT(  update_divis(  double ) ) );

   controlsLayout->addWidget( lb_analysis       , 0, 0, 1, 4 );
   controlsLayout->addWidget( lb_tolerance      , 1, 0, 1, 2 );
   controlsLayout->addWidget( ct_tolerance      , 1, 2, 1, 2 );
   controlsLayout->addWidget( lb_division       , 2, 0, 1, 2 );
   controlsLayout->addWidget( ct_division       , 2, 2, 1, 2 );
   controlsLayout->addWidget( lb_smoothing      , 3, 0, 1, 2 );
   controlsLayout->addWidget( ct_smoothing      , 3, 2, 1, 2 );
   controlsLayout->addWidget( lb_boundPercent   , 4, 0, 1, 2 );
   controlsLayout->addWidget( ct_boundaryPercent, 4, 2, 1, 2 );
   controlsLayout->addWidget( lb_boundPos       , 5, 0, 1, 2 );
   controlsLayout->addWidget( ct_boundaryPos    , 5, 2, 1, 2 );
   controlsLayout->addWidget( lb_scan           , 6, 0, 1, 4 );
   controlsLayout->addWidget( lb_from           , 7, 0 );
   controlsLayout->addWidget( ct_from           , 7, 1 );
   controlsLayout->addWidget( lb_to             , 7, 2 );
   controlsLayout->addWidget( ct_to             , 7, 3 );
   controlsLayout->addWidget( pb_exclude        , 8, 0, 1, 4 );

   connect( pb_help, SIGNAL( clicked() ),
            this,    SLOT(   help() ) );
   dataLoaded = false;
   haveZone   = false;

}

// load data
void US_vHW_Enhanced::load( void )
{
   dataLoaded = false;
   // query the directory where .auc and .xml file are
   workingDir = QFileDialog::getExistingDirectory( this,
         tr( "Raw Data Directory" ),
         US_Settings::resultDir(),
         QFileDialog::DontResolveSymlinks );

   if ( workingDir.isEmpty() )
      return;

   // insure we have a .auc file
   QStringList nameFilters = QStringList( "*.auc" );
   workingDir.replace( "\\", "/" );
   QDir wdir( workingDir );
   files    = wdir.entryList( nameFilters,
         QDir::Files | QDir::Readable, QDir::Name );

   if ( files.size() == 0 )
   {
      QMessageBox::warning( this,
            tr( "No Files Found" ),
            tr( "There were no files of the form *.auc\n"
                "found in the specified directory." ) );
      return;
   }

   // Look for cell / channel / wavelength combinations
   lw_triples->clear();
   dataList.clear();
   rawList.clear();
   excludedScans.clear();
   triples.clear();

   // Read all data
   if ( workingDir.right( 1 ) != "/" )
      workingDir += "/"; // Ensure trailing '/'

   files       = wdir.entryList( QStringList() << "*.*.*.*.*.*.xml",
         QDir::Files | QDir::Readable, QDir::Name );

   for ( int ii = 0; ii < files.size(); ii++ )
   {  // load all data in directory; get triples
      QString file     = files[ ii ];
      QStringList part = file.split( "." );
      QString filename = workingDir + file;
 
      // load edit data (xml) and raw data (auc)
      int result = US_DataIO::loadData( workingDir, file, dataList, rawList );

      if ( result != US_DataIO::OK )
      {
         QMessageBox::warning( this,
            tr( "UltraScan Error" ),
            tr( "Could not read edit file.\n" ) 
            + US_DataIO::errorString( result ) + "\n" + filename );
         return;
      }

      QString t = part[ 3 ] + " / " + part[ 4 ] + " / " + part[ 5 ];
      runID     = part[ 0 ];
      editID    = part[ 1 ];

      if ( ! triples.contains( t ) )
      {  // update ListWidget with cell / channel / wavelength triple
         triples << t;
         lw_triples->addItem( t );
      } 
   }

   lw_triples->setCurrentRow( 0 );

   d         = &dataList[ 0 ];
   scanCount = d->scanData.size();
   le_id->setText( d->runID + " / " + d->editID );  // set ID text

   tempera         = 0.0;
   savedValues.clear();

   for ( int ii = 0; ii < scanCount; ii++ )
   {
      s          = &d->scanData[ ii ];

      // sum temperature from each scan to determine average
      tempera   += s->temperature;

      // save the data
      valueCount = s->readings.size();
      QVector< double > v;
      v.resize( valueCount );

      for ( int jj = 0; jj < valueCount; jj++ )
      {
         v[ jj ] = s->readings[ jj ].value;
      }

      savedValues << v;
   }

   // display average temperature and data description
   tempera  /= (double)scanCount;
   QString t = QString::number( tempera, 'f', 1 ) + " " + QChar( 176 ) + "C";
   le_temp->setText( t );                            // set avg temp text
   te_desc->setText( d->description );               // set description text

   // Enable pushbuttons
   pb_details->setEnabled( true );
   pb_save   ->setEnabled( true );
   pb_view   ->setEnabled( true );
   pb_exclude->setEnabled( true );

   data_plot1->setCanvasBackground( Qt::black );
   data_plot2->setCanvasBackground( Qt::black );
   data_plot1->setMinimumSize( 600, 500 );
   data_plot2->setMinimumSize( 600, 300 );

   update( 0 );

   pb_details->disconnect( );                        // reset details connect
   connect( pb_details, SIGNAL( clicked() ),
            this,       SLOT(   details() ) );
   dataLoaded = true;
   le_temp->setText( t );                            // set avg temp text
}

// details
void US_vHW_Enhanced::details( void )
{
   US_RunDetails* dialog
      = new US_RunDetails( rawList, runID, workingDir, triples );
   dialog->exec();
   qApp->processEvents();
   delete dialog;
}

// distribution plot
void US_vHW_Enhanced::distr_plot(  void )
{
}

// data plot
void US_vHW_Enhanced::data_plot( void )
{
   double  xmax        = 0.0;
   double  ymax        = 0.0;
   int     count       = 0;
   int     totalCount;

   // let AnalysisBase do the lower plot
   US_AnalysisBase::data_plot();

   // handle upper (vHW Extrapolation) plot, here
   row        = lw_triples->currentRow();
   d          = &dataList[ row ];

   scanCount  = d->scanData.size();
   divsCount  = qRound( ct_division->value() );
   totalCount = scanCount * divsCount;
   divfac     = 1.0 / (double)divsCount;
   exclude    = 0;
   boundPct   = ct_boundaryPercent->value() / 100.0;
   positPct   = ct_boundaryPos    ->value() / 100.0;
   baseline   = calc_baseline();
   meniscus   = d->meniscus;
   correc     = solution.correction * 1.0e13;
	C0         = 0.0;
	Swavg      = 0.0;
   omega      = d->scanData[ 0 ].rpm * M_PI / 30.0;
   plateau    = d->scanData[ 0 ].plateau;

   for ( int ii = 0; ii < scanCount; ii++ )
   {  // count the scans excluded due to position percent
      if ( excludedScans.contains( ii ) ) continue;
      
      s          = &d->scanData[ ii ];
      valueCount = s->readings.size();
      plateau    = avg_plateau( );
      range      = plateau - baseline;
      basecut    = baseline + range * positPct;
      
      if ( d->scanData[ ii ].readings[ 0 ].value > basecut ) exclude++;
   }
qDebug() << " valueCount  totalCount" << valueCount << totalCount;
qDebug() << "  scanCount  divsCount" << scanCount << divsCount;
   le_skipped->setText( QString::number( exclude ) );

   // Do first experimental plateau calcs based on horizontal zones

   int     nrelp = 0;
   int     nunrp = 0;
   double* ptx   = new double[ scanCount ];
   double* pty   = new double[ totalCount ];

   QList< double > plats;
   QList< int >    isrel;
   QList< int >    isunr;

   if ( !haveZone )
   {  // accumulate reliable,unreliable scans and plateaus
      for ( int ii = exclude; ii < scanCount; ii++ )
      {
         s        = &d->scanData[ ii ];
//qDebug() << "p: scan " << ii+1;
         plateau  = zone_plateau( );

         if ( plateau > 0.0 )
         {  // save reliable scan plateaus
            plats.append( plateau );
            isrel.append( ii );
            nrelp++;
//qDebug() << "p:    *RELIABLE* " << ii+1 << nrelp;
         }

         else
         {  // save index to scan with no reliable plateau
            isunr.append( ii );
            nunrp++;
//qDebug() << "p:    -UNreliable- " << ii+1 << nunrp;
         }
//qDebug() << "p: nrelp nunrp " << nrelp << nunrp;
//qDebug() << "  RELIABLE: 1st " << isrel.at(0)+1 << "  last " << isrel.last()+1;
//if(nunrp>0) {
//qDebug() << "  UNreli: 1st " << isunr.at(0)+1 << "  last " << isunr.last()+1;
//for (int jj=0;jj<isunr.size();jj++) qDebug() << "    UNr: " << isunr.at(jj)+1;
//}
      }
   }

   else
   {  // had already found flat zones, so just set up to find Swavg,C0
      for ( int ii = exclude; ii < scanCount; ii++ )
      {
         plats.append( d->scanData[ ii ].plateau );
         isrel.append( ii );
         nrelp++;
      }
   }

   haveZone          = true;

   // Find Swavg and C0 by line fit
   // Solve for slope "a" and intercept "b" in
   // set of equations: y = ax + b 
   //   where x is corrected time
   //   and   y is log of plateau concentration
   // log( Cp ) = (-2 * Swavg * omega-sq ) t + log( C0 );
   //   for scans with reliable plateau values

   for ( int jj = 0; jj < nrelp; jj++ )
   {  // accumulate x,y of corrected time and log of plateau concentration
      int ii     = isrel.at( jj );
      ptx[ jj ]  = d->scanData[ ii ].seconds - time_correction;
      pty[ jj ]  = log( plats.at( jj ) );
   }

   QList< double >  scpds;
   double  cconc;
   double  pconc;
   double  mconc;
   double  cinc;
   double  eterm;
   double  oterm;
   double  slope;
   double  intcp;
   double  sigma;
   double  corre;

   US_Math::linefit( &ptx, &pty, &slope, &intcp, &sigma, &corre, nrelp );

   Swavg      = slope / ( -2.0 * omega * omega );  // Swavg func of slope
	C0         = exp( intcp );                      // C0 func of intercept
qDebug() << "Swavg(c): " << Swavg*correc << " C0: " << C0 ;

   // Determine Cp for each of the unreliable scans
   //   y = ax + b, using "a" and "b" determined above.
   // Since y = log( Cp ), we get Cp by exponentiating
   //   the left-hand term.

   for ( int jj = 0; jj < nunrp; jj++ )
   {  // each extrapolated plateau is exp of Y for X of corrected time
      int     ii  = isunr.at( jj );
      double  tc  = d->scanData[ ii ].seconds - time_correction;

      d->scanData[ ii ].plateau = exp( tc * slope + intcp );

qDebug() << " jj scan plateau " << jj << ii+1 << d->scanData[ii].plateau;
   }

   // initialize plateau values for components of scans

   for ( int ii = 0; ii < scanCount; ii++ )
   {
      scpds.clear();                       // clear this scan's Cp list
      if ( ii < exclude  ||  excludedScans.contains( ii ) )
      {
         cpds << scpds;
         continue;
      }
      s          = &d->scanData[ ii ];
      valueCount = s->readings.size();
      range      = s->plateau - baseline;
      basecut    = baseline + range * positPct;
      platcut    = basecut  + range * boundPct;
      span       = platcut - basecut;
      cconc      = basecut;
      pconc      = basecut;
      mconc      = basecut;
      sumcpij    = 0.0;
      cinc       = span * divfac;
      omega      = s->rpm * M_PI / 30.0;
      oterm      = ( s->seconds - time_correction ) * omega * omega;
      eterm      = -2.0 * oterm / correc;
      c0term     = ( C0 - baseline ) * boundPct * divfac;

      for ( int jj = 0; jj < divsCount; jj++ )
      {  // calculate partial plateaus
         pconc      = cconc;              // prev (baseline) div concentration
         cconc     += cinc;               // curr (plateau) div concentration
         mconc      = pconc + cinc * 0.5; // mid div concentration

         // get sedimentation coefficient for concentration
         sedc       = sed_coeff( mconc, oterm );

         if ( jj == 0  &&  ii == 0 )
         { // calculate back diffusion coefficient at 1st div of 1st scan
            //bdiffc     = back_diff_coeff( sedc * 1.0e-13 );
            //cpij       = sed_coeff( cconc + cinc, oterm );
            //bdiffc     = back_diff_coeff( cpij * 1.0e-13 );
            bdiffc     = back_diff_coeff( Swavg );
//qDebug() << "  sedcM sedcC" << sedc << cpij;
qDebug() << "  sedcM sedcC" << sedc << Swavg*correc;
         }

         // calculate the partial concentration for this division
         cpij       = c0term * exp( sedc * eterm );
//qDebug() << " scn div cinc cpij " << ii+1 << jj+1 << cinc << cpij;
//qDebug() << "  sedc eterm eso " << sedc << eterm << (eterm*sedc);

         // update Cpij sum and add to divisions list for scan
         sumcpij   += cpij;
         scpds.append( cpij );
      }

      // get span-minus-sum_cpij and divide by number of divisions
      sdiff    = ( span - sumcpij ) * divfac;
qDebug() << "   sumcpij span " << sumcpij << span
   << " sumcpij/span " << (sumcpij/span);

      for ( int jj = 0; jj < divsCount; jj++ )
      {  // spread difference to each partial plateau concentration
         cpij     = scpds.at( jj ) + sdiff;
         scpds.replace( jj, cpij );
      }

      cpds << scpds;  // add cpij list to scan's list-of-lists
   }

   // iterate to adjust plateaus until none needed or max iters reached

   int     iter      = 1;
   int     mxiter    = 3;          // maximum iterations
   double  avdthr    = 2.0e-5;     // threshold cp-absavg-diff

   while( iter <= mxiter )
   {
      double avgdif  = 0.0;
      double adiff   = 0.0;
      count          = 0;
qDebug() << "iter mxiter " << iter << mxiter;

      // get division sedimentation coefficient values (intercepts)

      div_seds();

      // reset division plateaus

      for ( int ii = 0; ii < scanCount; ii++ )
      {
         if ( ii < exclude  ||  excludedScans.contains( ii ) )
            continue;

         s        = &d->scanData[ ii ];
         range    = s->plateau - baseline;
         basecut  = baseline + range * positPct;
         platcut  = basecut  + range * boundPct;
         span     = platcut - basecut;
         sumcpij  = 0.0;
         oterm    = ( s->seconds - time_correction ) * omega * omega;
         eterm    = -2.0 * oterm / correc;
         c0term   = ( C0 - baseline ) * boundPct * divfac;
         scpds    = cpds.at( ii );
         scpds.clear();

         // split difference between divisions

         for ( int jj = 0; jj < divsCount; jj++ )
         {  // recalculate partial concentrations based on sedcoeff intercepts
            sedc     = dseds[ jj ];
            cpij     = c0term * exp( sedc * eterm );
            scpds.append( cpij );
            sumcpij += cpij;
//qDebug() << "    div " << jj+1 << "  tcdps cpij " << tcpds.at(jj) << cpij;
         }

         // set to split span-sum difference over each division
         sdiff    = ( span - sumcpij ) * divfac;

         for ( int jj = 0; jj < divsCount; jj++ )
         {  // spread difference to each partial plateau concentration
            cpij     = scpds.at( jj ) + sdiff;
            scpds.replace( jj, cpij );
         }

         cpds.replace( ii, scpds );  // replace scan's list of divison Cp vals

         adiff    = ( sdiff < 0 ) ? -sdiff : sdiff;
         avgdif  += adiff;  // sum of difference magnitudes
         count++;
qDebug() << "   iter scn " << iter << ii+1 << " sumcpij span "
   << sumcpij << span << "  sdiff sumabsdif" << sdiff << avgdif;
      }

      avgdif  /= (double)count;  // average of difference magnitudes
qDebug() << " iter" << iter << " avg(abs(sdiff))" << avgdif;

      if ( avgdif < avdthr )     // if differences are small, we're done
      {
qDebug() << "   +++ avgdif < avdthr (" << avgdif << avdthr << ") +++";
         break;
      }

      iter++;
   }

   int     kk     = exclude * divsCount;  // index to sed. coeff. values
   int     kl     = 0;                    // index/count of live scans

   // Calculate the corrected sedimentation coefficients

   for ( int ii = exclude; ii < scanCount; ii++ )
   {
      s        = &d->scanData[ ii ];

      if ( excludedScans.contains( ii ) )
      {
         kk         += divsCount;
         continue;
      }

      double  timev  = s->seconds - time_correction;
      double  timex  = 1.0 / sqrt( timev );
      double  bdrad  = bdrads.at( kl );   // back-diffus cutoff radius for scan
      double  bdcon  = bdcons.at( kl++ ); // back-diffus cutoff concentration
      double  divrad = 0.0;               // division radius value
qDebug() << "scn liv" << ii+1 << kl
   << " radius concen time" << bdrad << bdcon << timev;

      ptx[ ii ]  = timex;         // save corrected time and accum max
      xmax       = ( xmax > timex ) ? xmax : timex;

      valueCount = s->readings.size();
      range      = s->plateau - baseline;
      cconc      = baseline + range * positPct; // initial conc for span
      omega      = s->rpm * M_PI / 30.0;
      oterm      = ( timev > 0.0 ) ? ( timev * omega * omega ) : -1.0;
      scpds      = cpds.at( ii );  // list of conc values of divs this scan

      for ( int jj = 0; jj < divsCount; jj++ )
      {  // walk through division points; get sed. coeff. by place in readings
         pconc       = cconc;               // div base
         cpij        = scpds.at( jj );      // div partial concentration
         cconc       = pconc + cpij;        // absolute concentration
         mconc       = pconc + cpij * 0.5;  // mid div concentration

         int rx      = first_gteq( mconc, s->readings, valueCount );
         divrad      = s->readings[ rx ].d.radius;  // radius this division pt.

         if ( divrad < bdrad  ||  mconc < bdcon )
         {  // corresponding sedimentation coefficient
            sedc        = sed_coeff( mconc, oterm );
         }

         else
         {  // mark a point to be excluded by back-diffusion
            sedc        = -1.0;
qDebug() << " *excl* div" << jj+1 << " drad dcon " << divrad << mconc;
         }

         // y value of point is sedcoeff; accumulate y max
         pty[ kk++ ] = sedc;
         ymax        = ( ymax > sedc ) ? ymax : sedc;
      }
   }

   // Draw plot
   data_plot1->clear();
   us_grid( data_plot1 );

   data_plot1->setTitle( tr( "Run " ) + d->runID + tr( ": Cell " ) + d->cell
             + " (" + d->wavelength + tr( " nm) - vHW Extrapolation Plot" ) );

   data_plot1->setAxisTitle( QwtPlot::xBottom, tr( "(Time)^-0.5" ) );
   data_plot1->setAxisTitle( QwtPlot::yLeft  , 
         tr( "Corrected Sed. Coeff. (1e-13 s)" ) );

   int nxy    = ( scanCount > divsCount ) ? scanCount : divsCount;
   double* x  = new double[ nxy ];
   double* y  = new double[ nxy ];
   
   QwtPlotCurve* curve;
   QwtSymbol     sym;
   sym.setStyle( QwtSymbol::Ellipse );
   sym.setPen  ( QPen( Qt::blue ) );
   sym.setBrush( QBrush( Qt::white ) );
   sym.setSize ( 8 );
   
   kk         = exclude * divsCount;  // index to sed. coeff. values

   // Set points for each division of each scan
   for ( int ii = exclude; ii < scanCount; ii++ )
   {
      if ( excludedScans.contains( ii ) )
      {
         kk        += divsCount;      // excluded:  bump to next scan
         continue;
      }
      
      count      = 0;
      double xv  = ptx[ ii ];         // reciprocal square root of time value

      for ( int jj = 0; jj < divsCount; jj++ )
      {
         double yv  = pty[ kk++ ];    // sed.coeff. values for divs in scan
         if ( xv >= 0.0  &&  yv >= 0.0 )
         {  // points in a scan
            x[ count ] = xv;
            y[ count ] = yv;
            count++;
         }
      }

      if ( count > 0 )
      {  // plot the points in a scan
         curve = us_curve( data_plot1,
               tr( "Sed Coeff Points, scan %1" ).arg( ii+1 ) );

         curve->setStyle ( QwtPlotCurve::NoCurve );
         curve->setSymbol( sym );
         curve->setData  ( x, y, count );
      }
   }

   // fit lines for each division to all scan points

   for ( int jj = 0; jj < divsCount; jj++ )
   {  // walk thru divisions, fitting line to points from all scans
      count          = 0;
      for ( int ii = exclude; ii < scanCount; ii++ )
      {
         if ( excludedScans.contains( ii ) ) continue;

         kk         = ii * divsCount + jj;  // sed. coeff. index

         if ( ptx[ ii ] > 0.0  &&  pty[ kk ] > 0.0 )
         {  // points for scans in a division
            x[ count ] = ptx[ ii ];
            y[ count ] = pty[ kk ];
            count++;
         }
      }

      if ( count > 0 )
      {  // fit a line to the scan points in a division
         double slope;
         double intcept;
         double sigma = 0.0;
         double correl;

         US_Math::linefit( &x, &y, &slope, &intcept, &sigma, &correl, count );

         x[ 0 ] = 0.0;                      // x from 0.0 to max
         x[ 1 ] = xmax + 0.001;
         y[ 0 ] = intcept;                  // y from intercept to y at x-max
         y[ 1 ] = y[ 0 ] + x[ 1 ] * slope;

         curve  = us_curve( data_plot1, tr( "Fitted Line %1" ).arg( jj ) );
         curve->setPen( QPen( Qt::yellow ) );
         curve->setData( x, y, 2 );
      }
   }

   // set scales, then plot the points and lines
   xmax  *= 1.05;
   xmax   = (double)qRound( ( xmax + 0.0009 ) / 0.001 ) * 0.001;
   ymax   = (double)qRound( ( ymax + 0.3900 ) / 0.400 ) * 0.400;
   data_plot1->setAxisScale( QwtPlot::xBottom, 0.0, xmax, 0.005 );
   data_plot1->setAxisScale( QwtPlot::yLeft,   0.0, ymax, 0.500 );
   data_plot1->replot();
   count  = 0;

   for ( int ii = exclude; ii < scanCount; ii++ )
   {  // accumulate points of back-diffusion cutoff line

      if ( !excludedScans.contains( ii ) )
      {
         x[ count ] = bdrads.at( count );
         y[ count ] = bdcons.at( count );
         count++;
      }
   }

   // plot the red back-diffusion cutoff line
   dcurve  = us_curve( data_plot2, tr( "Fitted Line BD" ) );
   dcurve->setPen( QPen( QBrush( Qt::red ), 3.0 ) );
   dcurve->setData( x, y, count );
qDebug() << " xr0 yr0 " << x[0]       << y[0];
qDebug() << " xrN yrN " << x[count-1] << y[count-1];
   data_plot2->replot();

   delete [] x;                             // clean up
   delete [] y;
   delete [] ptx;
   delete [] pty;
}

// save the enhanced data
void US_vHW_Enhanced::save_data( void )
{ 
   QString filter = tr( "vHW data files (*.vHW.dat);;" )
      + tr( "Any files (*)" );
   QString fsufx = "." + cell + wavelength + ".vHW.dat";
   QString fname = run_name + fsufx;
   fname         = QFileDialog::getSaveFileName( this,
      tr( "Save vHW Data File" ),
      US_Settings::resultDir() + "/" + fname,
      filter,
      0, 0 );

}

void US_vHW_Enhanced::print_data(  void ) {}
void US_vHW_Enhanced::view_report( void ) {}
void US_vHW_Enhanced::show_densi(  void ) {}
void US_vHW_Enhanced::show_visco(  void ) {}

void US_vHW_Enhanced::show_vbar(   void )
{
   //this->get_vbar();
}

// reset the GUI
void US_vHW_Enhanced::reset_data( void )
{
   update( lw_triples->currentRow() );
#if 0
   data_plot1->detachItems( );
   data_plot1->replot();
   epick      = new US_PlotPicker( data_plot1 );
#endif
}

void US_vHW_Enhanced::sel_groups(  void ) {}

// update density
void US_vHW_Enhanced::update_density(  double dval )
{
   density   = dval;
}

// update viscosity
void US_vHW_Enhanced::update_viscosity( double dval )
{
   viscosity  = dval;
}

// update vbar
void US_vHW_Enhanced::update_vbar(      double  dval )
{
   vbar       = dval;
}


void US_vHW_Enhanced::update_bdtoler(    double dval )
{
   bdtoler   = dval;
}

void US_vHW_Enhanced::update_divis(      double dval )
{ 
   divsCount = qRound( dval );

   data_plot();
}

// find index to first value greater than or equal to given concentration
int US_vHW_Enhanced::first_gteq( double concenv,
      QVector< US_DataIO::reading >& readings, int valueCount )
{
   int index = -1;

   for ( int jj = 0; jj < valueCount; jj++ )
   {
      if ( concenv < readings[ jj ].value )
      {
         index     = ( jj > 0 ) ? jj : -1;
         break;
      }

      else if ( concenv == readings[ jj ].value )
      {
         index     = ( jj > 0 ) ? jj : 0;
         break;
      }
   }
   return index;
}

//  get average scan plateau value for 11 points around input value
double US_vHW_Enhanced::avg_plateau( )
{
   double plato  = s->plateau;
//qDebug() << "avg_plateau in: " << plato << "   points " << points;
//qDebug() << " rd0 rdn " << s->readings[0].value << s->readings[points-1].value;
   int    j2     = first_gteq( plato, s->readings, valueCount );

   if ( j2 > 0 )
   {
      int    j1     = j2 - 7;
      j2           += 4;
      j1            = ( j1 > 0 )          ? j1 : 0;
      j2            = ( j2 < valueCount ) ? j2 : valueCount;
      plato         = 0.0;
      for ( int jj = j1; jj < j2; jj++ )
      {  // walk through division points; get index to place in readings
         plato     += s->readings[ jj ].value;
      }
      plato        /= (double)( j2 - j1 );
//qDebug() << "     plateau out: " << plato << "    j1 j2 " << j1 << j2;
   }
   return plato;
}

// find scan's plateau for identifying flat zone in its curve
double US_vHW_Enhanced::zone_plateau( )
{
   double  plato  = -1.0;
   valueCount     = s->readings.size();
   int     j0     = first_gteq( basecut, s->readings, valueCount );
           j0     = ( j0 < 0 ) ? 0 : j0;
//qDebug() << "      j0=" << j0;
   int     nzp    = PZ_POINTS;
   int     j1     = 0;
   int     j2     = 0;
   int     j3     = 0;
   int     j8     = 0;
   int     j9     = 0;
   int     jj     = 0;
   int     l0     = nzp;
   double* x      = new double[ valueCount ];
   double* y      = new double[ valueCount ];

   // get the first potential zone and get its slope

   for ( jj = j0; jj < valueCount; jj++ )
   {  // accumulate x,y for all readings in the scan
      x[ j9 ]       = s->readings[ jj ].d.radius;
      y[ j9++ ]     = s->readings[ jj ].value;
   }

   double  sumx;
   double  sumy;
   double  sumxy;
   double  sumxs;

   double  slope = calc_slope( x, y, nzp, sumx, sumy, sumxy, sumxs );
//qDebug() << "         slope0 " << slope;

   // get slopes for sliding zone and detect where flat

   double  x0    = x[ 0 ];
   double  y0    = y[ 0 ];
   double  x1;
   double  y1;
   double  sllo1 = slope;
   double  sllo2 = slope;
   double  slhi1 = slope;
   double  slavg = 0.0;
   double  dypl  = 0.0;
   j9            = 0;
   jj            = 0;

   while ( l0 < valueCount )
   {  // loop until zone end is at readings end or flat zone ends
      x1       = x[ l0 ];     // new values to use in slope sums
      y1       = y[ l0++ ];
      jj++;
      slope    = update_slope( nzp, x0, y0, x1, y1, sumx, sumy, sumxy, sumxs );
//qDebug() << "         jj " << jj << " slope " << slope;

      if ( slope < PZ_THRLO )
      {  // slope is below threshold, so we're in flat area
         if ( j1 == 0 )
         {  // first slope to fall below threshold (near zero)
            j1     = jj;
            j2     = jj;
            sllo1  = slope;
            sllo2  = slope;
//qDebug() << "           1st flat jj " << jj;
         }

         else if ( slope < sllo2 )
         {  // slope is lowest so far
            j2     = jj;
            sllo2  = slope;
//qDebug() << "           low flat jj " << jj;
         }
         slavg += slope;
      }

      else if ( j1 > 0  &&  slope > PZ_THRHI )
      {  // after flat area, first slope to get too high
         j9     = jj;
//qDebug() << "           high after flat jj " << jj;
         slhi1  = slope;
         dypl   = y[ jj + nzp / 2 ] - s->plateau;
         dypl   = dypl > 0.0 ? dypl : -dypl;
         dypl  /= s->plateau;
         if ( dypl > 0.2 )
         {  // not near enough to plateau, assume another flat zone follows
//qDebug() << "             reset for dypl " << dypl;
            j3     = j1;     // save indecies in case this is last found
            j8     = j9;
            j1     = 0;      // reset to search for new flat zone
            j9     = 0;
         }
         else
         {  // flat zone found near enough to end, so break out of loop
            break;
         }
      }

      x0       = x[ jj ];     // values to remove from next iteration
      y0       = y[ jj ];
   }

   if ( j1 < 1 )
   {  // no 2nd or subsequent flat zone:  use original
      j1       = j3;
      j9       = j8;
   }

   // average plateau over flat zone

   if ( j1 > j0 )
   {  // flat zone found:  get average plateau
      plato      = 0.0;
//qDebug() << "        j1 j2 j9 " << j1 << j2 << j9;
      jj         = nzp / 2;            // bump start to middle of 1st gate`
      j1        += jj;
      j9         = ( j9 < j1 ) ? ( j1 + jj ) : j9;
//qDebug() << "         sll1 sll2 slh1 " << sllo1 << sllo2 << slhi1;
      nzp        = j9 - j1;            // size of overall flat zone
      if ( nzp > PZ_HZLO )
      {
         for ( jj = j1; jj < j9; jj++ )
            plato     += y[ jj ];      // sum y's in flat zone

         plato     /= (double)nzp;     // plateau is average
//qDebug() << "          plati plato " << s->plateau << plato;
         s->plateau = plato;
      }
   }

   delete [] x;                             // clean up
   delete [] y;

   return plato;
}

// calculate slope of x,y and return sums used in calculations
double US_vHW_Enhanced::calc_slope( double* x, double* y, int valueCount,
      double& sumx, double& sumy, double& sumxy, double& sumxs )
{
   sumx    = 0.0;
   sumy    = 0.0;
   sumxy   = 0.0;
   sumxs   = 0.0;

   for ( int ii = 0; ii < valueCount; ii++ )
   {
      sumx   += x[ ii ];
      sumy   += y[ ii ];
      sumxy  += ( x[ ii ] * y[ ii ] );
      sumxs  += ( x[ ii ] * x[ ii ] );
   }

   return fabs( ( (double)valueCount * sumxy - sumx * sumy ) /
                ( (double)valueCount * sumxs - sumx * sumx ) );
}

// update slope of sliding x,y by simply modifying running sums used
double US_vHW_Enhanced::update_slope( int valueCount,
      double x0, double y0, double x1, double y1,
      double& sumx, double& sumy, double& sumxy, double& sumxs )
{
   sumx   += ( x1 - x0 );
   sumy   += ( y1 - y0 );
   sumxy  += ( x1 * y1 - x0 * y0 );
   sumxs  += ( x1 * x1 - x0 * x0 );

   return fabs( ( (double)valueCount * sumxy - sumx * sumy ) /
                ( (double)valueCount * sumxs - sumx * sumx ) );
}

// get sedimentation coefficient for a given concentration
double US_vHW_Enhanced::sed_coeff( double cconc, double oterm )
{
   int    j2   = first_gteq( cconc, s->readings, valueCount );
   double rv0  = -1.0;          // mark radius excluded
   double sedc = -1.0;

   if ( j2 >= 0  &&  oterm >= 0.0 )
   {  // likely need to interpolate radius from two input values
      int j1      = j2 - 1;

      if ( j2 > 0 )
      {  // interpolate radius value
         double av1  = s->readings[ j1 ].value;
         double av2  = s->readings[ j2 ].value;
         double rv1  = s->readings[ j1 ].d.radius;
         double rv2  = s->readings[ j2 ].d.radius;
         double rra  = av2 - av1;
         rra         = ( rra == 0.0 ) ? 0.0 : ( ( rv2 - rv1 ) / rra );
         rv0         = rv1 + ( cconc - av1 ) * rra;
      }

      else
      {
         rv0         = -1.0;
      }
   }

   if ( rv0 > 0.0 )
   {  // use radius and other terms to get corrected sed. coeff. value
      sedc        = correc * log( rv0 / meniscus ) / oterm;
   }
   return sedc;
}

// calculate division sedimentation coefficient values (fitted line intercepts)
void US_vHW_Enhanced::div_seds( )
{
   double* xx       = new double[ scanCount ];
   double* yy       = new double[ scanCount ];
   double* zz       = new double[ scanCount ];
   double* pp       = new double[ scanCount ];
   double* xr       = new double[ scanCount ];
   double* yr       = new double[ scanCount ];
   int*    ll       = new int   [ scanCount ];
   int     nscnu    = 0;  // number used (non-excluded) scans
   int     kscnu    = 0;  // count of scans of div not affected by diffusion
   double  bdifsqr  = sqrt( bdiffc );
   double  pconc;
   double  cconc;
   double  mconc;
   bdtoler          = ct_tolerance->value();
   meniscus         = d->meniscus;

   dseds.clear();
   dcons.clear();

   for ( int jj = 0; jj < divsCount; jj++ )
   {  // loop to fit points across scans in a division
      double  dsed;
      double  dcon;
      double  slope;
      double  sigma;
      double  corre;
      int     ii;
      double  oterm;
      double  timecor;
      double  timesqr;
      double  bdleft;
      double  xbdleft;
      double  bottom;
      double  radD;
      double  omegasq;
//qDebug() << "div_sed div " << jj+1;

      if ( jj == 0 )
      {  // we only need to calculate x values, bcut the 1st time thru

         for ( ii = exclude; ii < scanCount; ii++ )
         {
            if ( !excludedScans.contains( ii ) )
            {
               s           = &d->scanData[ ii ];
               valueCount  = s->readings.size();
               omega       = s->rpm * M_PI / 30.0;
               omegasq     = omega * omega;
               timecor     = s->seconds - time_correction;
               timesqr     = sqrt( timecor );
               xx[ nscnu ] = 1.0 / timesqr;
               ll[ nscnu ] = ii;
               pp[ nscnu ] = baseline + ( s->plateau - baseline) * positPct;

               // accumulate limits based on back diffusion

//left=tolerance*pow(diff,0.5)/(2*intercept[0]*omega_s*
//  (bottom+run_inf.meniscus[selected_cell])/2
//  *pow(run_inf.time[selected_cell][selected_lambda][i],0.5);
//radD=bottom-(2*find_root(left)
//  *pow((diff*run_inf.time[selected_cell][selected_lambda][i]),0.5));
               bottom      = s->readings[ valueCount - 1 ].d.radius;
               oterm       = timecor * omegasq;
               cpij        = cpds.at( ii ).at( jj );
               //mconc       = pp[ 0 ] + cpij * 0.5;
               //mconc       = baseline + cpij * 0.5;
               mconc       = pp[ nscnu] + cpij * 0.5;
               dsed        = sed_coeff( mconc, oterm ) * 1.0e-13;
//dsed *= 2.0;
               //bdleft      = bdtoler * bdifsqr
               //   / ( 2.0 * dsed * omegasq * ( bottom + meniscus ) / 2.0 );
               bdleft      = bdtoler * bdifsqr
                  / ( dsed * omegasq * ( bottom + meniscus ) * timesqr );
               xbdleft     = find_root( bdleft );
//xbdleft *= 0.8;
               radD        = bottom - ( 2.0 * xbdleft * bdifsqr * timesqr );
               int mm      = 0;
               int mmlast  = valueCount - 1;

               while ( s->readings[ mm ].d.radius < radD  &&  mm < mmlast )
                  mm++;

               xr[ nscnu ] = radD;
               yr[ nscnu ] = s->readings[ mm ].value;
qDebug() << "  bottom meniscus bdleft" << bottom << meniscus << bdleft;
qDebug() << "  dsed find_root" << dsed << xbdleft;
qDebug() << "  bdiffc bdifsqr mm" << bdiffc << bdifsqr << mm << mmlast;
qDebug() << "BD x,y " << nscnu+1 << radD << yr[nscnu];

               nscnu++;
            }
         }
      }

      kscnu      = nscnu;

      // accumulate y values for this division

      for ( int kk = 0; kk < nscnu; kk++ )
      { // accumulate concentration, sed.coeff. for all scans, this div
         ii         = ll[ kk ];                   // scan index
         s          = &d->scanData[ ii ];         // scan info
         valueCount = s->readings.size();         // readings in this scan
         pconc      = pp[ kk ];                   // prev concen (baseline)
         cpij       = cpds.at( ii ).at( jj );     // partial concen (increment)
         cconc      = pconc + cpij;               // curr concen (plateau)
         mconc      = ( cconc + pconc ) * 0.5;    // mid div concentration
         omega      = s->rpm * M_PI / 30.0;       // omega
         oterm      = ( s->seconds - time_correction ) * omega * omega;

         pp[ kk ]   = cconc;        // division mark of concentration for scan
         yy[ kk ]   = sed_coeff( mconc, oterm );  // sedimentation coefficient
         zz[ kk ]   = mconc;        // mid-division concentration

         ii         = first_gteq( mconc, s->readings, valueCount );
         radD       = s->readings[ ii ].d.radius;  // radius this division pt.

         if ( radD > xr[ kk ]  &&  mconc > yr[ kk ] )
         {
            kscnu      = kk;
qDebug() << " div kscnu" << jj+1 << kscnu
   << " radD xrkk" << radD << xr[kk] << " mconc yrkk" << mconc << yr[kk];
            break;
         }
//if ( kk < 2 || kk > (nscnu-3) )
//qDebug() << "div scn " << jj+1 << ii+1 << " pconc cconc " << pconc << cconc;

      }
//qDebug() << " nscnu pp0 yy0 ppn yyn " << nscnu << yy[0] << pp[0]
//   << yy[nscnu-1] << pp[nscnu-1];

      // calculate and save the division sedcoeff

      US_Math::linefit( &xx, &yy, &slope, &dsed, &sigma, &corre, kscnu );

      dseds.append( dsed );

      US_Math::linefit( &xx, &zz, &slope, &dcon, &sigma, &corre, kscnu );

      dcons.append( dcon );
//if((jj&7)==0||jj==(divsCount-1))
// qDebug() << "     div dsed dcon " << jj+1 << dsed << dcon;

   }
qDebug() << " dsed[0] " << dseds.at(0);
qDebug() << " dsed[L] " << dseds.at(divsCount-1);
qDebug() << " xr0 yr0 " << xr[0] << yr[0];
qDebug() << " xrN yrN " << xr[nscnu-1] << yr[nscnu-1];

   for ( int kk = 0; kk < nscnu; kk++ )
   {
      bdrads.append( xr[ kk ] );
      bdcons.append( yr[ kk ] );
   }

   delete [] xx;                             // clean up
   delete [] yy;
   delete [] zz;
   delete [] pp;
   delete [] ll;
   delete [] xr;
   delete [] yr;

   return;
}

// find root X where evaluated Y is virtually equal to a goal, using a
//  calculation including the inverse complementary error function (erfc).
double US_vHW_Enhanced::find_root( double goal )
{
#define _FR_MXKNT 100
   double  tolerance = 1.0e-7;
   double  x1        = 0.0;
   double  x2        = 10.0;
   double  xv        = 5.0;
   double  xdiff     = 2.5;
   double  xsqr      = xv * xv;
   double  rsqr_pi   = 1.0 / sqrt( M_PI );
   double  test      = exp( -xsqr ) * rsqr_pi - ( xv * erfc( xv ) );
qDebug() << "      find_root: goal test" << goal << test << " xv" << xv;
//qDebug() << "        erfc(x)" << erfc(xv);
   int     count     = 0;

   // iterate until the difference between subsequent x value evaluations
   //  is too small to be relevant;

   while ( fabs( test - goal ) > tolerance )
   {
      xdiff  = ( x2 - x1 ) / 2.0;

      if ( test < goal )
      { // at less than goal, adjust top (x2) limit
         x2     = xv;
         xv    -= xdiff;
      }

      else
      { // at greater than goal, adjust bottom (x1) limit
         x1     = xv;
         xv    += xdiff;
      }

      // then update the test y-value
      xsqr   = xv * xv;
      test   = ( 1.0 + 2.0 * xsqr ) * erfc( xv )
         - ( 2.0 * xv * exp( -xsqr ) ) * rsqr_pi;
//qDebug() << "      find_root:  goal test" << goal << test << " x" << xv;

      if ( (++count) > _FR_MXKNT )
         break;
   }
qDebug() << "      find_root:  goal test" << goal << test
   << " xv" << xv << "  count" << count;

   return xv;
}

// calculate back diffusion coefficient
double US_vHW_Enhanced::back_diff_coeff( double sedc )
{
   double  RT       = R * ( K0 + tempera );
   double  D1       = AVOGADRO * 0.06 * M_PI * viscosity;
   double  D2       = 0.045 * sedc * vbar * viscosity;
   double  D3       = 1.0 - vbar * density;

   double  bdcoef   = RT / ( D1 * sqrt( D2 / D3 ) );

   qDebug() << "BackDiffusion:";
qDebug() << " RT " << RT << " R K0 tempera  " << R << K0 << tempera;
qDebug() << " D1 " << D1 << " viscosity AVO " << viscosity << AVOGADRO; 
qDebug() << " D2 " << D2 << " sedc vbar     " << sedc << vbar;
qDebug() << " D3 " << D3 << " density       " << density;
qDebug() << "  bdiffc" << bdcoef << " = RT/(D1*sqrt(D2/D3))";
   return bdcoef;
}

