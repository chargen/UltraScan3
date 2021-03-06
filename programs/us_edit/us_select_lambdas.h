#ifndef US_SEL_LAMBDAS_H
#define US_SEL_LAMBDAS_H

#include "us_extern.h"
#include "us_widgets_dialog.h"
#include "us_help.h"

#ifndef DbgLv
#define DbgLv(a) if(dbg_level>=a)qDebug()
#endif

class US_SelectLambdas : public US_WidgetsDialog
{
   Q_OBJECT

   public:
      US_SelectLambdas( QVector< int > );

   signals:
      void new_lambda_list( QVector< int > );

   private:
      QLineEdit*       le_original;
      QLineEdit*       le_selected;

      QListWidget*     lw_original;
      QListWidget*     lw_selected;

      QPushButton*     pb_add;
      QPushButton*     pb_remove;
      QPushButton*     pb_accept;

      int              dbg_level;
      int              nbr_orig;
      int              nbr_select;

      QVector< int >   original;
      QVector< int >   selected;

      US_Help          showHelp;

   private slots:
      void add_selections( void );
      void rmv_selections( void );
      void cancel        ( void );
      void done          ( void );
      void reset         ( void );
      void help          ( void )
      { showHelp.show_help( "manual/edit_select_lambdas.html" ); };
};
#endif
