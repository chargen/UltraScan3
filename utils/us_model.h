#ifndef US_MODEL_H
#define US_MODEL_H

#include <QtCore>
#include "us_extern.h"
#include "us_db2.h"

//! A class to define a model and provide Disk/DB IO for models.
class US_EXTERN US_Model
{
   public:
      US_Model();

      class SimulationComponent;
      class Association;

      //! Enumeration of the general shapes of molecules
      enum ShapeType { SPHERE, PROLATE, OBLATE, ROD };

      //! The type of optics used to acquire data for the current model
      enum OpticsType{ ABSORBANCE, INTERFERENCE, FLUORESCENCE };

      //! The type of analysis used with the model
      enum ModelType { MANUAL, TWODSA, TWODSA_MW, GA, GA_MW, GA_RA, ONEDSA,
                       COFS, FE, GLOBAL };

      int        iterations;      //!< Number of iterations accomplished
      double     wavelength;      //!< Wavelength of the data acquisition
      QString    description;     //!< Text description of the model
      QString    modelGUID;       //!< Identifier of the model
      QString    editGUID;        //!< Identifier of the edit data
      OpticsType optics;          //!< The optics used for the data acquisition
      ModelType  type;            //!< The analysis used with this model

      //! An index into components (-1 means none).  Generally buffer data 
      //! in the form of a component that affects the data readings.
      int        coSedSolute;  

      //! The components being analyzed
      QVector< SimulationComponent > components;

      //! The association constants for interacting solutes
      QVector< Association >         associations;

      QString message;  //!< Used internally for communication

      //! Read a model from the disk or database
      //! \param db_access - A flag to indicate if the DB (true) or disk (false)
      //!                    should be searched for the model
      //! \param guid      - The guid of the model to be loaded
      //! \param db        - For DB access, pointer to open database connection
      //! \returns         - The \ref US_DB2 retrun code for the operation
      int load( bool, const QString&, US_DB2* = 0 );

      //! An overloaded function to read a model from a database
      //! \param id        - Database ModelID
      //! \param db        - For DB access, pointer to open database connection
      //! \returns         - The \ref US_DB2 retrun code for the operation
      int load( const QString&, US_DB2* ); 


      //! An overloaded function to read a model from the disk
      //! \param filename  The name, including full path, of the analyte file
      //! \returns         - The \ref US_DB2 retrun code for the operation
      int load( const QString& );  
      
      //! A test for model equality
      bool operator== ( const US_Model& ) const;      

      //! A test for model inequality
      inline bool operator!= ( const US_Model& m )
         const { return ! operator==(m); }

      //! Write a model to the disk or database
      //! \param db_access - A flag to indicate if the DB (true) or disk (false)
      //!                    should be used to save the model
      //! \param filename  - The filename (with path) where the xml file
      //!                    be written if disk access is specified
      //! \param db        - For DB access, pointer to open database connection
      //! \returns         - The \ref US_DB2 retrun code for the operation
      int write( bool, const QString&, US_DB2* = 0 );

      //! An overloaded function to write a model to the DB
      //! \param db        - A pointer to an open database connection 
      //! \returns         - The \ref US_DB2 retrun code for the operation
      int write( US_DB2* );

      //! An overloaded function to write a model to a file on disk
      //! \param filename  - The filename to write
      //! \returns         - The \ref US_DB2 retrun code for the operation
      int write( const QString& );

      //! Update any missing analyte coefficient values
      //! \returns    - Success if values already existed or were successfully
      //!               filled in by calculations.
      bool update_coefficients( void );

      //! Calculate any missing coefficient values in an analyte component
      //! \returns    - Success if values already existed or were successfully
      //!               filled in by calculations.
      static bool calc_coefficients( SimulationComponent& );

      //! Model directory path existence test and creation
      //! \param path - A reference to the directory path on the disk where
      //!               model files are to be written
      //! \returns    - Success if the path is found or created and failure
      //!               if the path cannot be created
      static bool       model_path( QString& );

      //! A class representing the initial concentration distribution of a
      //! solute in the buffer.
      class MfemInitial
      {
         public:
         QVector< double > radius;        //!< The radii of the distribution
         QVector< double > concentration; //!< The concentration values
      };

      //! Each analyte in the model is a component.  A sedimenting solute
      //! can also be a component.
      class SimulationComponent
      {
         public:
         SimulationComponent();
         
         //! A test for identical components
         bool operator== ( const SimulationComponent& ) const;

         //! A test for unequal components
         inline bool operator!= ( const SimulationComponent& sc ) const 
         { return ! operator==(sc); }

         QString     analyteGUID;          //!< GUID for the analyte in the DB
         double      molar_concentration;  //!< Signal attenuation, molar basis
         double      signal_concentration; //!< Analyte concentration
         double      vbar20;               //!< Analyte specific volume
         double      mw;                   //!< Analyte molecular weight
         double      s;                    //!< Sedimentation coefficient
         double      D;                    //!< Diffusion coefficient
         double      f;                    //!< Frictional coefficient
         double      f_f0;                 //!< Frictional ratio
         double      extinction;           //!< Coefficient of light extinction
                                           //!<   at model wavelength
         double      axial_ratio;          //!< Ratio of major/minor shape axes
         double      sigma;         //!< Concentration dependency of s
         double      delta;         //!< Concentration dependency of D
         int         oligomer;      //!< Molecule count for this experiment
                                    //!<   (e.g. dimer = 2)
         ShapeType   shape;         //!< Classification of shape
         QString     name;          //!< Descriptive name
         int         analyte_type;  //!< Protein, RNA, DNA, Corbohydrate, etc
         MfemInitial c0;            //!< The radius/concentration points for a
                                    //!< user-defined initial concentration grid
      };

      //! The chemical constants associated with a reaction.
      class Association
      {
         public:
         Association();
         double k_eq;             //!< Equilibrium Constant
         double k_off;            //!< Dissociation Constant 
         QVector< int > rcomps;   //!< List of all system components
                                  //!<  involved in this reaction
         QVector< int > stoichs;  //!< List of Stoichiometry values of
                                  //!<  components in chemical equation.
                                  //!<  Positive->reactant; negative->product

         //! A test for equal Associations
         bool operator== ( const Association& ) const;

         //! A test for unequal associations
         inline bool operator!= ( const Association& a ) const 
         { return ! operator==(a); }
      };

   private:

      int  load_db         ( const QString&, US_DB2* );
      int  load_disk       ( const QString& );
      void mfem_scans      ( QXmlStreamReader&, SimulationComponent& );
      void get_associations( QXmlStreamReader&, Association& );
                           
      void write_temp      ( QTemporaryFile& );

      void debug( void );
};
#endif
