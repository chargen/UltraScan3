//! \file us_analysis_control.h
#ifndef US_ANALYSIS_CTL_H
#define US_ANALYSIS_CTL_H

#include <QtGui>

#include "us_extern.h"
#include "us_widgets_dialog.h"
#include "us_2dsa_process.h"
#include "us_plot.h"
#include "us_help.h"

//! \brief A class to provide a window with 2DSA analysis controls

class US_EXTERN US_AnalysisControl : public US_WidgetsDialog
{
   Q_OBJECT

   public:
      //! \brief US_AnalysisControl constructor
      //! \param dat_exp Pointer to the experiment data
      //! \param         Pointer to the parent of this widget
      US_AnalysisControl( US_DataIO2::EditedData*, QWidget* p = 0 );

   public slots:
      void update_progress (  int  );
      void completed_process( bool );
      void progress_message(  QString, bool = true );
      void reset_steps(       int,     int );

   private:
      int           dbg_level;
      int           ncsteps;
      int           nctotal;

      QHBoxLayout*  mainLayout;
      QGridLayout*  controlsLayout;
      QGridLayout*  optimizeLayout;

      US_DataIO2::EditedData*      edata;
      US_DataIO2::RawData*         sdata;
      US_DataIO2::RawData*         rdata;
      US_Model*                    model;
      US_Noise*                    ri_noise;
      US_Noise*                    ti_noise;
      US_SimulationParameters*     sparms;
      QPointer< QTextEdit    >     mw_stattext;

      QWidget*                     parentw;
      US_2dsaProcess*              processor;

      QwtCounter*   ct_lolimits;
      QwtCounter*   ct_uplimits;
      QwtCounter*   ct_nstepss;
      QwtCounter*   ct_lolimitk;
      QwtCounter*   ct_uplimitk;
      QwtCounter*   ct_nstepsk;
      QwtCounter*   ct_thrdcnt;
      QwtCounter*   ct_grrefine;
      QwtCounter*   ct_menisrng;
      QwtCounter*   ct_menispts;
      QwtCounter*   ct_repetloc;
      QwtCounter*   ct_scfactor;
      QwtCounter*   ct_scfact2;
      QwtCounter*   ct_repetran;
      QwtCounter*   ct_stddevia;
      QwtCounter*   ct_coaldist;
      QwtCounter*   ct_nbrclips;
      QwtCounter*   ct_mciters;
      QwtCounter*   ct_iters;

      QCheckBox*    ck_tinoise;
      QCheckBox*    ck_rinoise;
      QCheckBox*    ck_autoplt;
      QCheckBox*    ck_unifgr;
      QCheckBox*    ck_menisc;
      QCheckBox*    ck_locugr;
      QCheckBox*    ck_ranlgr;
      QCheckBox*    ck_soluco;
      QCheckBox*    ck_clipcs;
      QCheckBox*    ck_mcarlo;
      QCheckBox*    ck_iters;

      QLineEdit*    le_estmemory;
      QLineEdit*    le_iteration;
      QLineEdit*    le_oldvari;
      QLineEdit*    le_newvari;
      QLineEdit*    le_improve;

      QTextEdit*    te_status;

      QPushButton*  pb_strtfit;
      QPushButton*  pb_stopfit;
      QPushButton*  pb_plot;
      QPushButton*  pb_save;

   protected:
      US_Help       showHelp;
      QProgressBar* b_progress;

   private slots:
      void optimize_options( void );
      void uncheck_optimize( int  );
      void checkUniGrid (    bool );
      void checkMeniscus(    bool );
      void checkIterate (    bool );
      void checkLocalUni(    bool );
      void checkRandLoc (    bool );
      void checkSoluCoal(    bool );
      void checkClipLow (    bool );
      void checkMonteCar(    bool );
      void checkRegular (    bool );
      void grid_change(      void );
      void start(            void );
      void stop_fit(         void );
      void plot(             void );
      void close_all(        void );
      void advanced(         void );

      void help     ( void )
      { showHelp.show_help( "us_2dsa_analctl.html" ); };
};
#endif
