// ---------------------------------------------------------------------------------------------
// --------------- WARNING: this code is generated by an automatic code generator --------------
// ---------------------------------------------------------------------------------------------
// -------------- WARNING: any modifications made to this code will be overwritten -------------
// ---------------------------------------------------------------------------------------------

#ifndef US_HYDRODYN_CLUSTER_DAMMIN_H
#define US_HYDRODYN_CLUSTER_DAMMIN_H

#include "us_hydrodyn_cluster.h"
#include "qlabel.h"
#include "qstring.h"
#include "qlayout.h"
#include "qlineedit.h"
#include "qfontmetrics.h"
#include "qfile.h"
#include "q3filedialog.h"
//Added by qt3to4:
#include <QCloseEvent>
#include "us_hydrodyn.h"
#include "qpushbutton.h"
#include "qmessagebox.h"
#include "qregexp.h"
#include "us_json.h"

using namespace std;

class US_EXTERN US_Hydrodyn_Cluster_Dammin : public QDialog
{
   Q_OBJECT

   public:
      US_Hydrodyn_Cluster_Dammin(
                                 void                     *              us_hydrodyn,
                                 map < QString, QString > *              parameters,
                                 QWidget *                               p = 0,
                                 const char *                            name = 0
                                 );

      ~US_Hydrodyn_Cluster_Dammin();

   private:

      US_Config *                             USglobal;

      QLabel *                                lbl_title;
      QLabel *                                lbl_credits_1;
      QLabel *                                lbl_dammingnomfile;
      QLineEdit *                             le_dammingnomfile;
      QLabel *                                lbl_damminmode;
      QLineEdit *                             le_damminmode;
      QLabel *                                lbl_dammindescription;
      QLineEdit *                             le_dammindescription;
      QLabel *                                lbl_damminangularunits;
      QLineEdit *                             le_damminangularunits;
      QLabel *                                lbl_damminfitcurvelimit;
      QLineEdit *                             le_damminfitcurvelimit;
      QLabel *                                lbl_damminknotstofit;
      QLineEdit *                             le_damminknotstofit;
      QLabel *                                lbl_damminconstantsubtractionprocedure;
      QLineEdit *                             le_damminconstantsubtractionprocedure;
      QLabel *                                lbl_damminmaxharmonics;
      QLineEdit *                             le_damminmaxharmonics;
      QLabel *                                lbl_dammininitialdamtype;
      QLineEdit *                             le_dammininitialdamtype;
      QLabel *                                lbl_damminsymmetry;
      QLineEdit *                             le_damminsymmetry;
      QLabel *                                lbl_damminspherediameter;
      QLineEdit *                             le_damminspherediameter;
      QLabel *                                lbl_damminpackingradius;
      QLineEdit *                             le_damminpackingradius;
      QLabel *                                lbl_damminradius1stcoordinationsphere;
      QLineEdit *                             le_damminradius1stcoordinationsphere;
      QLabel *                                lbl_damminloosenesspenaltyweight;
      QLineEdit *                             le_damminloosenesspenaltyweight;
      QLabel *                                lbl_dammindisconnectivitypenaltyweight;
      QLineEdit *                             le_dammindisconnectivitypenaltyweight;
      QLabel *                                lbl_damminperipheralpenaltyweight;
      QLineEdit *                             le_damminperipheralpenaltyweight;
      QLabel *                                lbl_damminfixingthersholdsLosandRf;
      QLineEdit *                             le_damminfixingthersholdsLosandRf;
      QLabel *                                lbl_damminrandomizestructure;
      QLineEdit *                             le_damminrandomizestructure;
      QLabel *                                lbl_damminweight;
      QLineEdit *                             le_damminweight;
      QLabel *                                lbl_dammininitialscalefactor;
      QLineEdit *                             le_dammininitialscalefactor;
      QLabel *                                lbl_damminfixscalefactor;
      QLineEdit *                             le_damminfixscalefactor;
      QLabel *                                lbl_dammininitialannealingtemperature;
      QLineEdit *                             le_dammininitialannealingtemperature;
      QLabel *                                lbl_damminannealingschedulefactor;
      QLineEdit *                             le_damminannealingschedulefactor;
      QLabel *                                lbl_damminnumberofindependentatomstomodify;
      QLineEdit *                             le_damminnumberofindependentatomstomodify;
      QLabel *                                lbl_damminmaxnumberiterationseachT;
      QLineEdit *                             le_damminmaxnumberiterationseachT;
      QLabel *                                lbl_damminmaxnumbersuccesseseachT;
      QLineEdit *                             le_damminmaxnumbersuccesseseachT;
      QLabel *                                lbl_damminminnumbersuccessestocontinue;
      QLineEdit *                             le_damminminnumbersuccessestocontinue;
      QLabel *                                lbl_damminmaxnumberannealingsteps;
      QLineEdit *                             le_damminmaxnumberannealingsteps;
      QLabel *                                lbl_damminexpectedshape;
      QLineEdit *                             le_damminexpectedshape;
      QPushButton *                           pb_save;
      QPushButton *                           pb_load;

      QPushButton *                           pb_help;
      QPushButton *                           pb_close;
      void                     *              us_hydrodyn;
      map < QString, QString > *              parameters;
      void                                    update_fields();


      void                                    setupGUI();

   private slots:

      void                                    update_dammingnomfile( const QString & );
      void                                    update_damminmode( const QString & );
      void                                    update_dammindescription( const QString & );
      void                                    update_damminangularunits( const QString & );
      void                                    update_damminfitcurvelimit( const QString & );
      void                                    update_damminknotstofit( const QString & );
      void                                    update_damminconstantsubtractionprocedure( const QString & );
      void                                    update_damminmaxharmonics( const QString & );
      void                                    update_dammininitialdamtype( const QString & );
      void                                    update_damminsymmetry( const QString & );
      void                                    update_damminspherediameter( const QString & );
      void                                    update_damminpackingradius( const QString & );
      void                                    update_damminradius1stcoordinationsphere( const QString & );
      void                                    update_damminloosenesspenaltyweight( const QString & );
      void                                    update_dammindisconnectivitypenaltyweight( const QString & );
      void                                    update_damminperipheralpenaltyweight( const QString & );
      void                                    update_damminfixingthersholdsLosandRf( const QString & );
      void                                    update_damminrandomizestructure( const QString & );
      void                                    update_damminweight( const QString & );
      void                                    update_dammininitialscalefactor( const QString & );
      void                                    update_damminfixscalefactor( const QString & );
      void                                    update_dammininitialannealingtemperature( const QString & );
      void                                    update_damminannealingschedulefactor( const QString & );
      void                                    update_damminnumberofindependentatomstomodify( const QString & );
      void                                    update_damminmaxnumberiterationseachT( const QString & );
      void                                    update_damminmaxnumbersuccesseseachT( const QString & );
      void                                    update_damminminnumbersuccessestocontinue( const QString & );
      void                                    update_damminmaxnumberannealingsteps( const QString & );
      void                                    update_damminexpectedshape( const QString & );
      void                                    save();
      void                                    load();

      void                                    help();
      void                                    cancel();

   protected slots:

      void                                    closeEvent( QCloseEvent * );
};

#endif
