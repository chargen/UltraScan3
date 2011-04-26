//! \file us_2dsa_process.cpp

#include "us_2dsa_process.h"
#include "us_util.h"
#include "us_settings.h"
#include "us_astfem_math.h"
#include "us_astfem_rsa.h"
#include "us_sleep.h"
#include "us_math2.h"
#include "us_constants.h"

#ifndef WIN32
#include <sys/user.h>
#endif

// Class to process 2DSA simulations
US_2dsaProcess::US_2dsaProcess( US_DataIO2::EditedData* da_exper,
   US_SimulationParameters* sim_pars, QObject* parent /*=0*/ )
   : QObject( parent )
{
   bdata            = da_exper;       // pointer to base experiment data
   edata            = bdata;          // working pointer to experiment
   simparms         = sim_pars;       // simulation parameters
   parentw          = parent; 
   dbg_level        = US_Settings::us_debug();
   maxrss           = 0;              // max memory
   maxdepth         = 0;              // maximum depth index of tasks
   ntisols          = 0;              // number total task input solutes
   ntcsols          = 0;              // number total task computed solutes
   varitol          = 1e-12;          // variance difference tolerance
   r_iter           = 0;              // refinement iteration index
   mm_iter          = 0;              // meniscus/MC iteration index
   mintsols         = 100;            // minimum solutes per task, depth 1 ff.
   maxiters         = 1;              // maximum refinement iterations
   mmtype           = 0;              // meniscus/montecarlo type (NONE)
   mmiters          = 0;              // meniscus/montecarlo iterations
   fnoionly         = US_Settings::debug_match( "2dsaFinalNoiseOnly" );

   itvaris  .clear();                 // iteration variances
   ical_sols.clear();                 // iteration final calculated solutes
}

// Get maximum used memory
long int US_2dsaProcess::max_rss( void )
{
   // Read /prod/$pid/stat
#ifndef WIN32
   QFile f( "/proc/" + QString::number( getpid() ) + "/stat" );
   f.open( QIODevice::ReadOnly );
   QByteArray ba = f.read( 512 );
   f.close();

   const static int kk = PAGE_SIZE / 1024;

   maxrss = max( maxrss, QString( ba ).section( " ", 23, 23 ).toLong() * kk );
#else
	maxrss = 0;
#endif
   return maxrss;
}

// Start a specified 2DSA fit run
void US_2dsaProcess::start_fit( double sll, double sul,  int nss,
                                double kll, double kul,  int nks,
                                int    ngr, int    nthr, int noif )
{
DbgLv(1) << "2P(2dsaProc): start_fit()";
   abort       = false;
   slolim      = sll;
   suplim      = sul;
   nssteps     = nss;
   klolim      = kll;
   kuplim      = kul;
   nksteps     = nks;
   ngrefine    = ngr;
   nthreads    = nthr;
   noisflag    = noif;
   errMsg      = tr( "NO ERROR: start" );
   maxrss      = 0;
   maxdepth    = 0;
   ntisols     = 0;
   ntcsols     = 0;
   r_iter      = 0;
   mm_iter     = 0;

   wkstates .resize(  nthreads );
   wkdepths .resize(  nthreads );
   wthreads .clear();
   tkdepths .clear();
   job_queue.clear();
   c_solutes.clear();

   orig_sols.clear();
   itvaris  .clear();
   ical_sols.clear();

DbgLv(1) << "2P: sll sul nss" << slolim << suplim << nssteps
 << " kll kul nks" << klolim << kuplim << nksteps
 << " ngref nthr noif" << ngrefine << nthreads << noisflag;

   timer.start();              // start a timer to measure run time

   edata       = bdata;        // initial iteration base experiment data

   if ( mmtype > 0 )
   {  // for meniscus or monte carlo, make a working copy of the base data
      wdata = *bdata;

      if ( mmtype == 1 )
      {  // if meniscus, use the start meniscus value
         edata              = &wdata;
         double bmeniscus   = bdata->meniscus;
         edata->meniscus    = bmeniscus - menrange * 0.5;
         simparms->meniscus = edata->meniscus;
DbgLv(1) << "MENISC: mm_iter meniscus bmeniscus"
 << mm_iter << edata->meniscus << bmeniscus;
      }
   }

   // experiment data dimensions
   nscans      = edata->scanData.size();
   npoints     = edata->x.size();
   // subgrid deltas  (increments between subgrid points)
   int nsubp_s = ( nssteps + ngrefine - 1 ) / ngrefine;
   int nsubp_k = ( nksteps + ngrefine - 1 ) / ngrefine;
   sdelta_s    = ( suplim - slolim ) / (double)( max( nsubp_s - 1, 1 ) );
   sdelta_k    = ( kuplim - klolim ) / (double)( max( nsubp_k - 1, 1 ) );
   // grid deltas     (overall increment between grid points)
   gdelta_s    = sdelta_s / (double)ngrefine;
   gdelta_k    = sdelta_k / (double)ngrefine;
   nsubgrid    = sq( ngrefine );
   int kgref   = ngrefine - 1;
   int kgrefsq = sq( kgref );
   int kksubg  = nksteps * nssteps
                 - ( kgref + kgrefsq ) * ( nsubp_s + nsubp_k ) + kgrefsq;
DbgLv(1) << "2P:    kgref kgrefsq kksubg" << kgref << kgrefsq << kksubg;
   maxtsols    = nsubp_s * nsubp_k;
   maxtsols    = max( maxtsols, mintsols );

   int ktcsol  = maxtsols - 5;
   int nnstep  = ( noisflag > 0 ? ( sq( ktcsol ) / 10 + 2 ) : 2 ) * nsubgrid;
   nctotal     = kksubg + nnstep + estimate_steps( ( kksubg / 8 ) );

   kcsteps     = 0;
   emit stage_complete( kcsteps, nctotal );
   kctask      = 0;
   kstask      = 0;
   nthreads    = ( nthreads < nsubgrid ) ? nthreads : nsubgrid;
DbgLv(1) << "2P:   nscans npoints" << nscans << npoints;
DbgLv(1) << "2P:   gdelta_s gdelta_k" << gdelta_s << gdelta_k
 << " sdelta_s sdelta_k" << sdelta_s << sdelta_k;
DbgLv(1) << "2P:   nsubgrid nctotal nthreads maxtsols"
 << nsubgrid << nctotal << nthreads << maxtsols;
   max_rss();
DbgLv(1) << "2P: (1)maxrss" << maxrss;

   int    ktask = 0;
   int    jdpth = 0;
   int    jnois = fnoionly ? 0 : noisflag;
   double llss  = slolim;

   // Define work unit parameters, including solutes for each subgrid
   for ( int ii = 0; ii < ngrefine; ii++ )
   {
      double llsk = klolim;

      for ( int jj = 0; jj < ngrefine; jj++ )
      {
         WorkPacket wtask;

         QVector< Solute > isolutes = create_solutes( llss, suplim, sdelta_s,
                                                      llsk, kuplim, sdelta_k );

         queue_task( wtask, llss, llsk, ktask++, jdpth, jnois, isolutes );

         orig_sols << isolutes;

         maxtsols       = max( maxtsols, wtask.isolutes.size() );
         llsk          += gdelta_k;
      }

      llss     += gdelta_s;
   }

   // Start the first threads. This will begin the first work units (subgrids).
   // Thereafter, work units are started in new threads when previous
   // threads signal that they have completed their work.

   for ( int ii = 0; ii < nthreads; ii++ )
   {
      wthreads << 0;

      WorkPacket wtask = job_queue.takeFirst();
      submit_job( wtask, ii );
   }

   max_rss();
   kstask = nthreads;     // count of started tasks is initially thread count
DbgLv(1) << "2P:   kstask nthreads" << kstask << nthreads << job_queue.size();

   emit message_update( pmessage_head() +
      tr( "Starting computations of %1 subgrids\n using %2 threads ..." )
      .arg( nsubgrid ).arg( nthreads ), false );
}

// Set iteration parameters
void US_2dsaProcess::set_iters( int    mxiter, int    mciter, int    mniter,
                                double vtoler, double menrng )
{
   maxiters   = mxiter;
   mmtype     = ( mniter > 1 ) ? 1 : 0;
   mmtype     = ( mciter > 1 ) ? 2 : mmtype;
   mmiters    = ( mmtype == 0 ) ? 0 : max( mciter, mniter );
   varitol    = vtoler;
   menrange   = menrng;
}

// Abort a fit run
void US_2dsaProcess::stop_fit()
{
   abort   = true;

   for ( int ii = 0; ii < nthreads; ii++ )
   {
DbgLv(1) << "StopFit test Thread" << ii + 1;
      WorkerThread* wthr = wthreads[ ii ];

      if ( wthr != 0  &&  wthr->isRunning() )
      {
         wthr->flag_abort();
DbgLv(1) << "  STOPTHR:  thread aborted";
      }

      wthreads[ ii ] = 0;
   }

   job_queue.clear();
   tkdepths .clear();
   c_solutes.clear();
   maxdepth  = 0;
   ntisols   = 0;
   ntcsols   = 0;

   emit message_update( pmessage_head() +
      tr( "All computations have been aborted." ), false );
}

// Slot for thread step progress:  signal control progress bar
void US_2dsaProcess::step_progress( int ksteps )
{
   max_rss();
   kcsteps   += ksteps;              // bump completed steps count
//DbgLv(2) << "StpPr: ks kcs    " << ksteps << kcsteps;

   emit progress_update( ksteps );   // pass progress on to control window
}

// Use a worker thread one last time to compute, using all computed solutes
void US_2dsaProcess::final_computes()
{
   if ( abort ) return;

   max_rss();

   WorkPacket wtask;

   int depth      = maxdepth++;
   wtask.thrn     = 0;
   wtask.taskx    = tkdepths.size();
   wtask.depth    = maxdepth;
   wtask.noisf    = noisflag;    // in this case, we use the noise flag
   wtask.edata    = edata;
   wtask.sparms   = simparms;
   wtask.csolutes.clear();
   wtask.ti_noise.clear();
   wtask.ri_noise.clear();

   tkdepths << wtask.depth;

   // This time, input solutes are all the subgrid-computed ones where
   // the concentration is positive.
   qSort( c_solutes[ depth ] );
DbgLv(1) << "FinalComp: szSoluC" << c_solutes[ depth ].size();
   wtask.isolutes.clear();

   for ( int ii = 0; ii < c_solutes[ depth ].size(); ii++ )
   {
      if ( c_solutes[ depth ][ ii ].c > 0.0 )
         wtask.isolutes << c_solutes[ depth ][ ii ];
   }
DbgLv(1) << "FinalComp: szSoluI" << wtask.isolutes.size();

   c_solutes[ depth ].clear();

   WorkerThread* wthr = new WorkerThread( this );

   int thrx = wkstates.indexOf( READY );
   while ( thrx < 0 )
   {
      US_Sleep::msleep( 1 );
      thrx = wkstates.indexOf( READY );
   }

   wtask.thrn = thrx + 1;
   wthr->define_work( wtask );

   connect( wthr, SIGNAL( work_complete( WorkerThread* ) ),
            this, SLOT(   process_final( WorkerThread* ) ) );
   connect( wthr, SIGNAL( work_progress( int           ) ),
            this, SLOT(   step_progress( int           ) ) );

   emit message_update( pmessage_head() + tr( "Computing final NNLS ..." ),
      false );

DbgLv(1) << "(FC)SUBMIT_JOB taskx" << wtask.taskx << "depth" << wtask.depth;
   wthreads[ thrx ] = wthr;
   wthr->start( );
}

// Slot to handle output of final pass on composite calculated solutes
void US_2dsaProcess::process_final( WorkerThread* wthrd )
{
   if ( abort ) return;

   WorkPacket wresult;

   wthrd->get_result( wresult );     // get results of thread task
DbgLv(1) << "(FF)PROCESS_JOB thrn" << wresult.thrn << "taskx" << wresult.taskx
 << "depth" << wresult.depth;

   if ( c_solutes.size() < ( maxdepth + 1 ) )
      c_solutes << QVector< Solute >();

   c_solutes[ maxdepth ] =  wresult.csolutes;  // final iter calc'd solutes
   int nsolutes = c_solutes[ maxdepth ].size();
DbgLv(1) << "FIN_FIN:    c_sol size" << nsolutes;

   QVector< double > tinvec( npoints,  0.0 );
   QVector< double > rinvec( nscans,   0.0 );

   if ( ( noisflag & 1 ) != 0 )
   {  // copy TI noise to caller and internal vector
      ti_noise.minradius = edata->radius( 0 );
      ti_noise.maxradius = edata->radius( npoints - 1 );
      ti_noise.values.resize( npoints );
      ti_noise.count = npoints;

      for ( int rr = 0; rr < npoints; rr++ )
      {
         ti_noise.values[ rr ] = wresult.ti_noise[ rr ];
         tinvec         [ rr ] = wresult.ti_noise[ rr ];
      }
   }

   if ( ( noisflag & 2 ) != 0 )
   {  // copy RI noise to caller and internal vector
      ri_noise.values.resize( nscans );
      ri_noise.count = nscans;

      for ( int ss = 0; ss < nscans; ss++ )
      {
         ri_noise.values[ ss ] = wresult.ri_noise[ ss ];
         rinvec         [ ss ] = wresult.ri_noise[ ss ];
      }
   }
DbgLv(1) << "FIN_FIN:    ti,ri counts" << ti_noise.count << ri_noise.count;

   // pick up solution/buffer values passed via data's dataType string
   QString dvv      = edata->dataType;
   double density   = dvv.section( " ", 1, 1 ).toDouble();
   double viscosity = dvv.section( " ", 2, 2 ).toDouble();
   double vbar      = dvv.section( " ", 3, 3 ).toDouble();
   double vbar_tb   = dvv.section( " ", 4, 4 ).toDouble();
   double avgtemp   = edata->average_temperature();

   // computed s,D correction factors
   US_Math2::SolutionData solution;
   solution.density   = density;
   solution.viscosity = viscosity;
   solution.vbar20    = vbar;
   solution.vbar      = vbar_tb;
   US_Math2::data_correction( avgtemp, solution );
   double sfactor     = 1.0 / solution.s20w_correction;
   double dfactor     = 1.0 / solution.D20w_correction;
DbgLv(1) << "FIN_FIN: dens visc vbar vbar_tb"
 << density << viscosity << vbar << vbar_tb
 << " s20w,D20w_corr" << solution.s20w_correction << solution.D20w_correction;
   model.components.resize( nsolutes );

   // build the final model
   for ( int cc = 0; cc < nsolutes; cc++ )
   {
      // Get standard-space solute values (20,W)
      US_Model::SimulationComponent mcomp;
      mcomp.s     = qAbs( c_solutes[ maxdepth ][ cc ].s );
      mcomp.D     = 0.0;
      mcomp.mw    = 0.0;
      mcomp.f     = 0.0;
      mcomp.f_f0  = c_solutes[ maxdepth ][ cc ].k;
      mcomp.signal_concentration
                  = c_solutes[ maxdepth ][ cc ].c;

      // Complete other coefficients in standard-space
      model.calc_coefficients( mcomp );
DbgLv(1) << " Bcc comp D" << mcomp.D;

      // Convert to experiment-space for simulation below
      mcomp.s    *= sfactor;
      mcomp.D    *= dfactor;
DbgLv(1) << "  Bcc 20w comp D" << mcomp.D;

      model.components[ cc ]  = mcomp;
   }

DbgLv(1) << "FIN_FIN:    c0 cn" << c_solutes[maxdepth][0].c
 << c_solutes[maxdepth][nsolutes-1].c;
   // calculate the simulation data

   US_AstfemMath::initSimData( sdata, *edata, 0.0 );
   US_AstfemMath::initSimData( rdata, *edata, 0.0 );
DbgLv(1) << "FIN_FIN: dens visc vbar" << density << viscosity << vbar;
DbgLv(1) << "FIN_FIN: rotorcoefs"
   << simparms->rotorcoeffs[0] << simparms->rotorcoeffs[1];
DbgLv(1) << "FIN_FIN: bottom bposition" << simparms->bottom
   << simparms->bottom_position;
DbgLv(1) << "FIN_FIN: rreso menisc temp" << simparms->radial_resolution
   << simparms->meniscus << simparms->temperature;
   US_Astfem_RSA astfem_rsa( model, *simparms );

   astfem_rsa.calculate( sdata );

   nscans           = edata->scanData.size();
   npoints          = edata->x.size();
   double vari      = 0.0;

   // build residuals data set (experiment minus simulation minus any noise)
   for ( int ss = 0; ss < nscans; ss++ )
   {
      for ( int rr = 0; rr < npoints; rr++ )
      {
         double rval = edata->value( ss, rr ) - sdata.value( ss, rr )
                       - tinvec[ rr ] - rinvec[ ss ];
         vari       += sq( rval );

         rdata.scanData[ ss ].readings[ rr ] = US_DataIO2::Reading( rval );
      }
   }

   // set variance and communicate to control through residual's scan 0
   vari      /= (double)( nscans * npoints );
   itvaris   << vari;
   ical_sols << c_solutes[ maxdepth ];
   US_DataIO2::Scan* rscan0 = &rdata.scanData[ 0 ];
   rscan0->delta_r    = vari;
   rscan0->rpm        = (double)( r_iter + 1 );
   rscan0->seconds    = ( mmtype == 0 ) ? 0.0 : (double)( mm_iter + 1 );
   rscan0->plateau    = ( mmtype != 1 ) ? 0.0 : edata->meniscus;

   // determine elapsed time
   int ktimes = ( timer.elapsed() + 500 ) / 1000;
   int ktimeh = ktimes / 3600;
   int ktimem = ( ktimes - ktimeh * 3600 ) / 60;
   ktimes     = ktimes - ktimeh * 3600 - ktimem * 60;

   // compose final iteration status message
   QString pmsg = pmessage_head() +
      tr( "The Solution has converged...\n"
          "Iterations:  %1\n"
          "Threads:  %2 ;   Subgrids:  %3\n"
          "Grid points (s x f/f0):  %4 x %5\n"
          "Run time:  " )
      .arg( r_iter + 1 )
      .arg( nthreads ).arg( nsubgrid ).arg( nssteps ).arg( nksteps );

   if ( ktimeh > 0 )
      pmsg += tr( "%1 hr., %2 min., %3 sec.\n" )
         .arg( ktimeh ).arg( ktimem ).arg( ktimes );

   else
      pmsg += tr( "%1 min., %2 sec.\n" )
         .arg( ktimem ).arg( ktimes );

   max_rss();
   double memmb  = (double)maxrss / 1024.0;

   pmsg += tr( "Maximum memory used:  " )
           + QString().sprintf( "%.1f", memmb ) + " MB";

   emit message_update( pmsg, false );          // signal final message

   int thrx   = wresult.thrn - 1;
   free_worker( thrx );
DbgLv(1) << "FIN_FIN: Run Time: hr min sec" << ktimeh << ktimem << ktimes;
DbgLv(1) << "FIN_FIN: maxrss memmb nthr" << maxrss << memmb << nthreads
 << " nsubg nsst nkst noisf" << nsubgrid << nssteps << nksteps << noisflag;
DbgLv(1) << "FIN_FIN:   kcsteps nctotal" << kcsteps << nctotal;

   bool   neediter = false;       // need-more-iterations false by default

   if ( ( r_iter + 1 ) < maxiters )
   {  // possibly iterate if not yet at maximum iterations

      if ( r_iter < 1 )
      {  // if max is 2 or more, we must do at least 2 iterations to compare
         neediter     = true;
      }

      else
      {  // otherwise, we must compare solutes and variance difference
         int    jc     = r_iter;
         int    jp     = r_iter - 1;
         double pvari  = itvaris[   jp ];
         double dvari  = fabs( pvari - vari );
         int    nccsol = ical_sols[ jc ].size();
         int    npcsol = ical_sols[ jp ].size();
         bool   sdiffs = true;

         if ( nccsol == npcsol )
         {  // determine if calculated solutes match previous in s and k
            sdiffs       = false;
            for ( int jj = 0; jj < nccsol; jj++ )
            {
               if ( ical_sols[ jc ][ jj ] != ical_sols[ jp ][ jj ] )
               {  // if any mismatch, we may need to iterate
                  sdiffs       = true;
                  break;
               }
            }
         }

         // iterate if solutes different and variance change large enough
         neediter     = ( sdiffs  &&  dvari > varitol ); 
DbgLv(1) << "FIN_FIN: neediter" << neediter << "  sdiffs" << sdiffs
 << " dvari varitol" << dvari << varitol << " iter" << r_iter;
      }
   }

   if ( neediter )
   {  // we must iterate
      emit process_complete( 0 );       // signal that iteration is complete
      iterate();                        // reset to run another iteration
      return;
   }

   // Convert model components s,D back to 20,w form for output
   for ( int cc = 0; cc < nsolutes; cc++ )
   {
DbgLv(1) << "cc comp D" << model.components[ cc ].D;
      model.components[ cc ].s *= solution.s20w_correction;
      model.components[ cc ].D *= solution.D20w_correction;
DbgLv(1) << " cc 20w comp D" << model.components[ cc ].D;
   }

   // done with iterations:   check for meniscus or MC iteration

   if ( mmtype > 0  &&  ++mm_iter < mmiters )
   {  // doing meniscus or monte carlo and more to do
      emit process_complete( mmtype );  // signal that iteration is complete

      if ( mmtype == 1 )
         set_meniscus();

      else if ( mmtype == 2 )
         set_monteCarlo();

      return;
   }

   emit process_complete( 9 );     // signal that all processing is complete
}

// Public slot to get results upon completion of all refinements
bool US_2dsaProcess::get_results( US_DataIO2::RawData* da_sim,
                                  US_DataIO2::RawData* da_res,
                                  US_Model*            da_mdl,
                                  US_Noise*            da_tin,
                                  US_Noise*            da_rin )
{
   bool all_ok = true;

   if ( abort ) return false;

   *da_sim     = sdata;                           // copy simulation data
   *da_res     = rdata;                           // copy residuals data
   *da_mdl     = model;                           // copy model

   if ( ( noisflag & 1 ) != 0  &&  da_tin != 0 )
      *da_tin     = ti_noise;                     // copy any ti noise

   if ( ( noisflag & 2 ) != 0  &&  da_rin != 0 )
      *da_rin     = ri_noise;                     // copy any ri noise

DbgLv(1) << " GET_RES:   ti,ri counts" << ti_noise.count << ri_noise.count;
   return all_ok;
}

// Build solutes vector for a subgrid
QVector< Solute > US_2dsaProcess::create_solutes(
   double ll_s, double ul_s, double delta_s,
   double ll_k, double ul_k, double delta_k )
{
   QVector< Solute > solu;
   ll_s    *= 1.0e-13;
   ul_s    *= 1.0e-13;
   delta_s *= 1.0e-13;

   for ( double sval = ll_s; sval <= ul_s; sval += delta_s )
      for ( double kval = ll_k; kval <= ul_k; kval += delta_k )
         solu << Solute( sval, kval );

   return solu;
}

// Submit a job
void US_2dsaProcess::submit_job( WorkPacket& wtask, int thrx )
{
   wtask.thrn         = thrx + 1;

   WorkerThread* wthr = new WorkerThread( this );
   wthreads[ thrx ]   = wthr;
   wkstates[ thrx ]   = WORKING;
   wkdepths[ thrx ]   = wtask.depth;

   wthr->define_work( wtask );

   connect( wthr, SIGNAL( work_complete( WorkerThread* ) ),
            this, SLOT(   process_job(   WorkerThread* ) ) );
   connect( wthr, SIGNAL( work_progress( int           ) ),
            this, SLOT(   step_progress( int           ) ) );
DbgLv(1) << "SUBMIT_JOB taskx" << wtask.taskx << "depth" << wtask.depth;

   wthr->start();
}

// Slot to handle the results of a just-completed worker thread.
// Accumulate computed solutes.
// If there is more work to do, start a new thread for a new work unit.
void US_2dsaProcess::process_job( WorkerThread* wthrd )
{
   WorkPacket wresult;

   wthrd->get_result( wresult );   // get results of thread task
   int thrn   = wresult.thrn;      // thread number of task
   int thrx   = thrn - 1;          // index into thread list
   int taskx  = wresult.taskx;     // task index of task
   int depth  = wresult.depth;     // depth of result
DbgLv(1) << "PROCESS_JOB thrn" << thrn << "taskx" << taskx
 << "depth" << wresult.depth;
   int nrcso  = wresult.csolutes.size();
   ntcsols   += nrcso;
if ( taskx < 9 || taskx > (nsubgrid-4) )
DbgLv(1) << "PJ: taskx csolutes size tot" << taskx << nrcso << ntcsols;

   max_rss();

   free_worker( thrx );            // free up this worker thread

   if ( abort )
      return;

   // This loop should only execute, at most, once per result
   while( c_solutes.size() < ( depth + 1 ) )
      c_solutes << QVector< Solute >();

   int nextc    = c_solutes[ depth ].size() + wresult.csolutes.size();
   int jnois    = fnoionly ? 0 : noisflag;

   if ( nextc > maxtsols )
   {  // if new solutes push count over limit, queue a job at next depth
      WorkPacket wtask = wresult;
      int taskx    = tkdepths.size();
      int depthn   = depth + 1;

      queue_task( wtask, slolim, klolim, taskx, depthn, jnois,
                  c_solutes[ depth ] );

      maxdepth     = max( maxdepth, depthn );
      c_solutes[ depth ].clear();
DbgLv(1) << "THR_FIN: depth" << wtask.depth << " #solutes"
 << wtask.isolutes.size() << " nextc maxtsols" << nextc << maxtsols
 << "wres#sols" << wresult.csolutes.size();;
   }

   if ( depth == 0 )
   {  // this result is from depth 0, it is from initial subgrids pass
      kctask++;                      // bump count of completed subgrid tasks
DbgLv(1) << "THR_FIN: thrn" << thrn << " taskx" << taskx
 << " kct kst" << kctask << kstask << "csols size" << c_solutes.size();

      emit message_update( pmessage_head() +
         tr( "Computations for %1 of %2 subgrids are complete" )
         .arg( kctask ).arg( nsubgrid ), false );

      if ( kctask == nsubgrid )
      {  // all subgrid tasks are now complete
         if ( r_iter == 0 )
         {  // in 1st iteration, re-estimate total progress steps
            int nsolest   = ( nksteps * nssteps ) / 8 ;  // solutes estimated
            int nsolact   = ntcsols;                     // actually computed

            int todoest   = estimate_steps( nsolest );
            int todoact   = estimate_steps( nsolact );

DbgLv(1) << "THR_FIN:   (est)kcst ncto" <<  kcsteps << nctotal
 << " nsol ntodo" << nsolest << todoest;
            // adjust the estimate of total progress steps
            nctotal       = kcsteps + todoact;
DbgLv(1) << "THR_FIN:   (new)kcst ncto" <<  kcsteps << nctotal
 << " nsol ntodo" << nsolact << todoact;
         }

         emit stage_complete( kcsteps, nctotal );

         emit message_update( pmessage_head() +
                              tr( "Computing depth 1 ff. solutions..." ),
                              false );

         maxdepth       = 1;

         if ( nextc < maxtsols  &&  jobs_at_depth( 1 ) == 0 )
            maxdepth       = 0;  // handle no depth 1 jobs yet submitted
      }
   }

   // Add the current results
   c_solutes[ depth ] += wresult.csolutes;

   // At this point we need to clean up. For each depth below the current one,
   // if there is nothing in the queue or working and there are calculated
   // solutes left, those need to be submitted

   for ( int dd = 0; dd < depth; dd++ )
   {
      if ( jobs_at_depth( dd ) == 0  &&  c_solutes[ dd ].size() > 0 )
      {  // queue a task to handle last solutes at this depth
         WorkPacket wtask = wresult;
         int taskx    = tkdepths.size();
         int depthn   = dd + 1;
DbgLv(1) << "THR_FIN:    QT: taskx depth solsz" << taskx << depth << c_solutes[dd].size();
         queue_task( wtask, slolim, klolim, taskx, depthn, jnois,
                     c_solutes[ dd ] );

         c_solutes[ dd ].clear();
      }
   }

   // Is anyone working?
   bool no_working = wkstates.indexOf( WORKING ) < 0;

   // Submit one last time with all solutes if necessary
   if ( depth == maxdepth    &&
        job_queue.isEmpty()  &&
        no_working           &&
        c_solutes[ depth ].size() > wresult.csolutes.size() )
   {
      final_computes();
      return;
   }

   // Submit jobs while queue is not empty and a worker thread is ready
   while ( ! job_queue.isEmpty() && ( thrx = wkstates.indexOf( READY ) ) >= 0 )
   {
      WorkPacket wtask = job_queue.takeFirst();

      submit_job( wtask, thrx );
      kstask++;                       // bump count of started worker threads

      if ( wtask.depth > 1  &&
           wtask.taskx == tkdepths.indexOf( wtask.depth ) )
      {  // submitting the first job of a depth pass
         QString pmsg = tr( "Computing depth %1 solutions..." )
                        .arg( wtask.depth );
         emit message_update( pmsg, true );
DbgLv(1) << pmsg;
      }
   }

}

// Build a task and add it to the queue
void US_2dsaProcess::queue_task( WorkPacket& wtask, double llss, double llsk,
      int taskx, int depth, int noisf, QVector< Solute > isolutes )
{
   wtask.thrn     = 0;             // thread number (none while queued)
   wtask.taskx    = taskx;         // task index
   wtask.noisf    = noisf;         // noise flag
   wtask.typeref  = UGRID;         // refinement type: uniform grid
   wtask.state    = READY;         // initialized state, ready for submit
   wtask.depth    = depth;         // depth index
   wtask.iter     = r_iter;        // refine-iteration index
   wtask.ll_s     = llss;          // lower limit s (x 1e13)
   wtask.ll_k     = llsk;          // lower limit k
   wtask.edata    = edata;         // pointer to experiment data
   wtask.sparms   = simparms;      // pointer to simulation parameters
   wtask.isolutes = isolutes;      // solutes for calc_residuals task

   wtask.csolutes.clear();         // clear output vectors
   wtask.ti_noise.clear();
   wtask.ri_noise.clear();
   int nrisols    = isolutes.size();
   ntisols       += nrisols;
if ( taskx < 9 || taskx > (nsubgrid-4) )
DbgLv(1) << "QT: taskx isolutes size tot" << taskx << nrisols << ntisols;

   tkdepths << depth;              // record work task depth

   job_queue << wtask;             // put the task on the queue

   if ( tkdepths.count( depth ) == 1 )
   {  // if first task at this depth, report it
      emit message_update( tr( "Submitting first depth %1 calculations..." )
                           .arg( depth ), true );
DbgLv(1) << "Submit 1st calcs, depth" << depth;
   }
}

// queue up jobs for a new iteration
void US_2dsaProcess::iterate()
{
   r_iter++;                         // bump iteration index

   tkdepths .clear();
   job_queue.clear();
   QVector< Solute > csolutes = c_solutes[ maxdepth ];

   int    ncsol = csolutes.size();   // number of solutes calculated last iter

   for ( int ii = 0; ii < ncsol; ii++ )
      csolutes[ ii ].c = 0.0;        // clear concentration for compares
DbgLv(1) << "ITER: start of iteration" << r_iter+1;

   // Bump total steps estimate based on additional solutes in subgrids
   nctotal      = ( r_iter == 1 ) ? nctotal : ( ( nctotal + kcsteps ) / 2 );
   nctotal      = max( kcsteps, nctotal ) + 10;
DbgLv(1) << "ITER:   r-iter0 ncto ncsol" << nctotal << ncsol;
   int ktisol   = maxtsols - 5;
   int ktask    = nsubgrid + 1;
   int kstep    = ( r_iter == 1 ) ? ( ncsol + 8 ) : 4;
   int knois    = ktask * ( ( kstep * ktisol + sq( kstep ) ) / 10 );
DbgLv(1) << "ITER:    nsubg ktask kstep knois" << nsubgrid << ktask << kstep << knois;
   int kadd     = ( ktask * kstep ) + ( ( noisflag == 0 ) ? 0 : knois );
   nctotal     += kadd;
DbgLv(1) << "ITER:   r-iter1 ncto (diff)" << nctotal << kadd;

   kcsteps      = 0;
   emit stage_complete( kcsteps, nctotal );
   kctask       = 0;
   kstask       = 0;
   maxdepth     = 0;
   ntisols      = 0;
   ntcsols      = 0;
   max_rss();

   ktask        = 0;                 // task index
   int    jdpth = 0;
   int    jnois = fnoionly ? 0 : noisflag;
   double llss  = slolim;            // lower limit s to identify each subgrid

   // Build and queue the subgrid tasks

   for ( int ii = 0; ii < ngrefine; ii++ )
   {
      double llsk  = klolim;         // lower limit k to identify each subgrid

      for ( int jj = 0; jj < ngrefine; jj++ )
      {
         // get the solutes originally created for this subgrid
         QVector< Solute > isolutes = orig_sols[ ktask ];

         // add any calculated solutes not already in subgrid
         for ( int cc = 0; cc < ncsol; cc++ )
            if ( ! isolutes.contains( csolutes[ cc ] ) )
               isolutes << csolutes[ cc ];
//*DEBUG
int kosz=orig_sols[ktask].size();
int kasz=isolutes.size();
int kadd=kasz-kosz;
if ( kadd < ncsol )
DbgLv(1) << "ITER: kt" << ktask << "iterate nisol o a c +"
 << kosz << kasz << ncsol << kadd;
//*DEBUG

         // queue a subgrid task and update maximum task solute count
         WorkPacket wtask;
         queue_task( wtask, llss, llsk, ktask++, jdpth, jnois, isolutes );

         maxtsols       = max( maxtsols, isolutes.size() );
         llsk          += gdelta_k;
      }

      llss     += gdelta_s;
   }

   // Make sure calculated solutes are cleared out for new iteration
   for ( int ii = 0; ii < c_solutes.size(); ii++ )
      c_solutes[ ii ].clear();

   // Start the first threads. This will begin the first work units (subgrids).
   // Thereafter, work units are started in new threads when threads signal
   // that they have completed their work.
   for ( int ii = 0; ii < nthreads; ii++ )
   {
      WorkPacket wtask = job_queue.takeFirst();
      submit_job( wtask, ii );
   }

   max_rss();
   kstask = nthreads;     // count of started tasks is initially thread count

   emit message_update(
      tr( "Starting iteration %1 computations of %2 subgrids\n"
          "  using %3 threads ..." )
      .arg( r_iter + 1 ).arg( nsubgrid ).arg( nthreads ), true );

}

// Free up a worker thread
void US_2dsaProcess::free_worker( int tx )
{
   if ( tx >= 0  &&  tx < nthreads )
   {
      if ( wthreads[ tx ] != 0 )
         delete wthreads[ tx ];       // destroy thread

      wthreads[ tx ] = 0;             // set thread pointer to null
      wkstates[ tx ] = READY;         // mark its slot as available
      wkdepths[ tx ] = -1;            // mark no valid depth yet for worker
   }
}

// Estimate progress steps after depth 0 from given total of calculated solutes.
//   For maxtsols=100, estimate 95 solutes per task
//   Calculate tasks needed for given depth 0 calculated steps
//   Bump a bit for NNLS and any noise
//   Multiply steps per task times number of tasks
//     Repeat for subsequent depths, assuming calculated solutes 1/8 of input
int US_2dsaProcess::estimate_steps( int ncsol )
{
   // Estimate number of solutes and steps per task
   int ktisol  = maxtsols - 5;
   int ktcsol  = ktisol / 8 + 1;
   int ktstep  = ktisol + ( ( noisflag > 0 ) ? ( sq( ktisol ) / 10 + 2 ) : 2 );
DbgLv(1) << "ES: ncsol ktisol ktcsol ktstep" << ncsol << ktisol
 << ktcsol << ktstep;

   // Estimate number of depth 1 tasks, solutes,and steps
   int n1task  = ( ncsol + ktisol / 2 ) / ktisol + 1;
   int ntasks  = n1task + 1;
   int n1csol  = n1task * ktcsol;
DbgLv(1) << "ES: D1 n1task n1csol" << n1task << n1csol;

   // Estimate number of solutes for depths beyond 1
   n1task      = ( n1csol + ktisol / 2 ) / ktisol + 1;
   ntasks     += n1task;
   n1csol      = n1task * ktcsol;
DbgLv(1) << "ES: D2 n1task n1csol" << n1task << n1csol;

   while ( n1task >  1 )
   {  // Sum in steps for depth 3 and following until just 1 task left
      n1task      = ( n1csol + ktisol / 2 ) / ktisol + 1;
      ntasks     += n1task;
      n1csol      = n1task * ktcsol;
DbgLv(1) << "ES:  D3ff n1task n1csol" << n1task << n1csol;
   }

   // Return estimate of remaining steps
DbgLv(1) << "ES: returned nsteps ntasks" << (ntasks*ktstep) << ntasks;
   return ( ntasks * ktstep );
}

// count queued jobs at a given depth
int US_2dsaProcess::queued_at_depth( int depth )
{
   int count = 0;

   for ( int ii = 0; ii < job_queue.size(); ii++ )
   {
      if ( job_queue[ ii ].depth == depth )
         count++;
   }

   return count;
}

// count running jobs at a given depth
int US_2dsaProcess::running_at_depth( int depth )
{
   int count = 0;

   for ( int ii = 0; ii < nthreads; ii++ )
   {
      if ( wkstates[ ii ] == WORKING  &&
           wkdepths[ ii ] == depth    &&
           wthreads[ ii ] != 0 )
         count++;
   }

   return count;
}

// count all queued and running jobs at a given depth
int US_2dsaProcess::jobs_at_depth( int depth )
{
   return queued_at_depth( depth ) + running_at_depth( depth );
}

// Set up for another meniscus pass
void US_2dsaProcess::set_meniscus()
{
   // Give the working data set an appropriate meniscus value
   double bmeniscus   = bdata->meniscus;
   double mendelta    = menrange / (double)( mmiters - 1 );
   double smeniscus   = bmeniscus - menrange * 0.5;
   edata->meniscus    = smeniscus + (double)mm_iter * mendelta;
   simparms->meniscus = edata->meniscus;
DbgLv(1) << "MENISC: mm_iter meniscus" << mm_iter << edata->meniscus;

   // Re-queue all the original subgrid tasks

   requeue_tasks();
}

// Set up for another monte carlo pass
void US_2dsaProcess::set_monteCarlo()
{
DbgLv(1) << "MCARLO: mm_iter" << mm_iter;
   // Set up new data modified by a gaussian distribution (MC iteration 2 start)
   if ( mm_iter == 1 )
      set_gaussians();

   // Get a randomized variation of the cencentrations
   // Use a gaussian distribution with the residual as the standard deviation
   int kk = 0;

   for ( int ss = 0; ss < nscans; ss++ )
   {
      for ( int rr = 0; rr < npoints; rr++ )
      {
         double variation = US_Math2::box_muller( 0.0, sigmas[ kk++ ] );
         wdata.scanData[ ss ].readings[ rr ] =
            US_DataIO2::Reading( sdata.value( ss, rr ) + variation );
      }
   }

   edata       = &wdata;
DbgLv(1) << "MCARLO: mm_iter" << mm_iter << " sigma0 c0 v0 cn vn"
 << sigmas[0] << bdata->value(0,0) << edata->value(0,0)
 << bdata->value(nscans-1,npoints-1) << edata->value(nscans-1,npoints-1);

   // Re-queue all the original subgrid tasks

   requeue_tasks();
}

// Re-queue all the original subgrid tasks
void US_2dsaProcess::requeue_tasks()
{
   kcsteps      = 0;
   emit stage_complete( kcsteps, nctotal );

   int    ktask = 0;
   int    jdpth = 0;
   int    jnois = 0;
   double llss  = slolim;

   for ( int ii = 0; ii < ngrefine; ii++ )
   {
      double llsk  = klolim;

      for ( int jj = 0; jj < ngrefine; jj++ )
      {
         QVector< Solute > isolutes = orig_sols[ ktask ];
         WorkPacket wtask;
         queue_task( wtask, llss, llsk, ktask++, jdpth, jnois, isolutes );
         llsk          += gdelta_k;
      }
      llss     += gdelta_s;
   }

   // Make sure calculated solutes are cleared out for new iteration
   for ( int ii = 0; ii < c_solutes.size(); ii++ )
      c_solutes[ ii ].clear();

   // Start the first threads
   for ( int ii = 0; ii < nthreads; ii++ )
   {
      WorkPacket wtask = job_queue.takeFirst();
      submit_job( wtask, ii );
   }

   kstask = nthreads;
   kctask       = 0;
   maxdepth     = 0;
   ntisols      = 0;
   ntcsols      = 0;
   r_iter       = 0;

   emit message_update(
      tr( "Starting iteration %1 computations of %2 subgrids\n"
          "  using %3 threads ..." )
      .arg( r_iter + 1 ).arg( nsubgrid ).arg( nthreads ), true );
}

void US_2dsaProcess::set_gaussians()
{
   sigmas.clear();
DbgLv(1) << "MCARLO:setgau";

   for ( int ss = 0; ss < nscans; ss++ )
   {
      QVector< double > vv( npoints );

      for ( int rr = 0; rr < npoints; rr++ )
      {
         vv[ rr ] = fabs( rdata.value( ss, rr ) );
      }

      // Smooth using 5 points to the left and right of each point
if ( ss < 2 ) DbgLv(1) << "MCARLO:setgau:gausmoo vv9" << vv[9];
      US_Math2::gaussian_smoothing( vv, 5 );
if ( ss < 2 ) DbgLv(1) << "MCARLO:setgau:smoothd vv9" << vv[9];
      sigmas << vv;
   }
}

QString US_2dsaProcess::pmessage_head()
{
   return tr( "Model %1,  Iteration %2:\n" )
          .arg( mm_iter + 1 ).arg( r_iter + 1 );
}

