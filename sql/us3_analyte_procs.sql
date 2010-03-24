--
-- us3_analyte_procs.sql
--
-- Script to set up the MySQL stored procedures for the US3 system
--   These are related to the analyte tables
-- Run as us3admin
--

DELIMITER $$

-- Verifies that the user has permission to view or modify
--  the specified analyte
DROP FUNCTION IF EXISTS verify_analyte_permission$$
CREATE FUNCTION verify_analyte_permission( p_guid      CHAR(36),
                                           p_password  VARCHAR(80),
                                           p_analyteID INT )
  RETURNS TINYINT
  READS SQL DATA

BEGIN
  DECLARE count_analytes INT;
  DECLARE status         TINYINT;

  CALL config();
  SET status   = @ERROR;

  SELECT COUNT(*)
  INTO   count_analytes
  FROM   analytePerson
  WHERE  analyteID = p_analyteID
  AND    personID = @US3_ID;
 
  IF ( verify_user( p_guid, p_password ) = @OK &&
       count_analytes > 0 ) THEN
    SET status = @OK;

  ELSEIF ( verify_userlevel( p_guid, p_password, @US3_ADMIN ) = @OK ) THEN
    SET status = @OK;

  ELSE
    SET @US3_LAST_ERRNO = @NOTPERMITTED;
    SET @US3_LAST_ERROR = 'MySQL: you do not have permission to view or modify this analyte';

    SET status = @NOTPERMITTED;

  END IF;

  RETURN( status );

END$$

-- Returns the count of analytes associated with p_ID
--  If p_ID = 0, retrieves count of all analytes in db
--  Regular user can only get count of his own analytes
DROP FUNCTION IF EXISTS count_analytes$$
CREATE FUNCTION count_analytes( p_guid     CHAR(36),
                                p_password VARCHAR(80),
                                p_ID       INT )
  RETURNS INT
  READS SQL DATA

BEGIN
  
  DECLARE count_analytes INT;

  CALL config();
  SET count_analytes = 0;

  IF ( verify_userlevel( p_guid, p_password, @US3_ADMIN ) = @OK ) THEN
    -- This is an admin; he can get more info
    IF ( p_ID > 0 ) THEN
      SELECT COUNT(*)
      INTO   count_analytes
      FROM   analytePerson
      WHERE  personID = p_ID;

    ELSE
      SELECT COUNT(*)
      INTO   count_analytes
      FROM   analytePerson;

    END IF;

  ELSEIF ( verify_user( p_guid, p_password ) = @OK ) THEN
    IF ( (p_ID != 0) && (p_ID != @US3_ID) ) THEN
      -- Uh oh, can't do that
      SET @US3_LAST_ERRNO = @NOTPERMITTED;
      SET @US3_LAST_ERROR = 'MySQL: you do not have permission to view those analytes';
     
    ELSE
      -- This person is asking about his own analytes
      -- Ignore p_ID and return user's own
      SELECT COUNT(*)
      INTO   count_analytes
      FROM   analytePerson
      WHERE  personID = @US3_ID;

    END IF;
    
  END IF;

  RETURN( count_analytes );

END$$

-- INSERTs a new analyte with the specified information
DROP PROCEDURE IF EXISTS new_analyte$$
CREATE PROCEDURE new_analyte ( p_guid        CHAR(36),
                               p_password    VARCHAR(80),
                               p_type        VARCHAR(16),
                               p_sequence    TEXT,
                               p_vbar        FLOAT,
                               p_description TEXT,
                               p_spectrum    TEXT,
                               p_mweight     FLOAT )
  MODIFIES SQL DATA

BEGIN
  DECLARE l_analyteID INT;

  CALL config();
  SET @US3_LAST_ERRNO = @OK;
  SET @US3_LAST_ERROR = '';
  SET @LAST_INSERT_ID = 0;
 
  IF ( verify_user( p_guid, p_password ) = @OK ) THEN
    INSERT INTO analyte SET
      type        = p_type,
      sequence    = p_sequence,
      vbar        = p_vbar,
      description = p_description,
      spectrum    = p_spectrum,
      molecularWeight = p_mweight ;
    SET @LAST_INSERT_ID = LAST_INSERT_ID();

    INSERT INTO analytePerson SET
      analyteID   = @LAST_INSERT_ID,
      personID    = @US3_ID;

  END IF;

  SELECT @US3_LAST_ERRNO AS status;

END$$

-- UPDATEs an existing analyte with the specified information
DROP PROCEDURE IF EXISTS update_analyte$$
CREATE PROCEDURE update_analyte ( p_guid        CHAR(36),
                                  p_password    VARCHAR(80),
                                  p_analyteID   INT,
                                  p_type        VARCHAR(16),
                                  p_sequence    TEXT,
                                  p_vbar        FLOAT,
                                  p_description TEXT,
                                  p_spectrum    TEXT,
                                  p_mweight     FLOAT )
  MODIFIES SQL DATA

BEGIN

  CALL config();
  SET @US3_LAST_ERRNO = @OK;
  SET @US3_LAST_ERROR = '';

  IF ( verify_analyte_permission( p_guid, p_password, p_analyteID ) = @OK ) THEN
    UPDATE analyte SET
      type        = p_type,
      sequence    = p_sequence,
      vbar        = p_vbar,
      description = p_description,
      spectrum    = p_spectrum,
      molecularWeight = p_mweight 
    WHERE analyteID = p_analyteID;

  END IF;
      
  SELECT @US3_LAST_ERRNO AS status;

END$$

-- Returns the analyteID, description, and type of all analytes associated with p_ID
--  If p_ID = 0, retrieves information about all analytes in db
--  Regular user can only get info about his own analytes
DROP PROCEDURE IF EXISTS get_analyte_desc$$
CREATE PROCEDURE get_analyte_desc ( p_guid     CHAR(36),
                                    p_password VARCHAR(80),
                                    p_ID       INT )
  READS SQL DATA

BEGIN

  CALL config();
  SET @US3_LAST_ERRNO = @OK;
  SET @US3_LAST_ERROR = '';

  IF ( verify_userlevel( p_guid, p_password, @US3_ADMIN ) = @OK ) THEN
    -- This is an admin; he can get more info
    IF ( count_analytes( p_guid, p_password, p_ID ) < 1 ) THEN
      SET @US3_LAST_ERRNO = @NOROWS;
      SET @US3_LAST_ERROR = 'MySQL: no rows returned';
   
      SELECT @US3_LAST_ERRNO AS status;

    ELSE
      SELECT @OK AS status;
  
      IF ( p_ID > 0 ) THEN
        SELECT   a.analyteID, description, type
        FROM     analyte a, analytePerson
        WHERE    a.analyteID = analytePerson.analyteID
        AND      analytePerson.personID = p_ID
        ORDER BY a.analyteID DESC;
   
      ELSE
        SELECT   a.analyteID, description, type
        FROM     analyte a, analytePerson
        WHERE    a.analyteID = analytePerson.analyteID
        ORDER BY a.analyteID DESC;

      END IF;

    END IF;

  ELSEIF ( verify_user( p_guid, p_password ) = @OK ) THEN
    IF ( (p_ID != 0) && (p_ID != @US3_ID) ) THEN
      -- Uh oh, can't do that
      SET @US3_LAST_ERRNO = @NOTPERMITTED;
      SET @US3_LAST_ERROR = 'MySQL: you do not have permission to view this analyte';
     
      SELECT @US3_LAST_ERRNO AS status;

    ELSEIF ( count_analytes( p_guid, p_password, @US3_ID ) < 1 ) THEN
      SET @US3_LAST_ERRNO = @NOROWS;
      SET @US3_LAST_ERROR = 'MySQL: no rows returned';
   
      SELECT @US3_LAST_ERRNO AS status;

    ELSE
      -- Ok, user wants his own info
      SELECT @OK AS status;

      SELECT   a.analyteID, description, type
      FROM     analyte a, analytePerson
      WHERE    a.analyteID = analytePerson.analyteID
      AND      analytePerson.personID = @US3_ID
      ORDER BY a.analyteID DESC;
      

    END IF;

  END IF;

END$$

-- Returns a more complete list of information about one analyte
DROP PROCEDURE IF EXISTS get_analyte_info$$
CREATE PROCEDURE get_analyte_info ( p_guid      CHAR(36),
                                    p_password  VARCHAR(80),
                                    p_analyteID INT )
  READS SQL DATA

BEGIN
  DECLARE count_analytes INT;

  CALL config();
  SET @US3_LAST_ERRNO = @OK;
  SET @US3_LAST_ERROR = '';

  SELECT     COUNT(*)
  INTO       count_analytes
  FROM       analyte
  WHERE      analyteID = p_analyteID;

  IF ( verify_analyte_permission( p_guid, p_password, p_analyteID ) = @OK ) THEN
    IF ( count_analytes = 0 ) THEN
      SET @US3_LAST_ERRNO = @NOROWS;
      SET @US3_LAST_ERROR = 'MySQL: no rows returned';

      SELECT @US3_LAST_ERRNO AS status;

    ELSE
      SELECT @OK AS status;

      SELECT   type, sequence, vbar, description, spectrum, molecularWeight, personID 
      FROM     analyte a, analytePerson ap
      WHERE    a.analyteID = ap.analyteID
      AND      a.analyteID = p_analyteID;

    END IF;

  ELSE
    SELECT @US3_LAST_ERRNO AS status;

  END IF;

END$$

-- DELETEs a analyte, plus information in related tables
DROP PROCEDURE IF EXISTS delete_analyte$$
CREATE PROCEDURE delete_analyte ( p_guid      CHAR(36),
                                  p_password  VARCHAR(80),
                                  p_analyteID INT )
  MODIFIES SQL DATA

BEGIN
  CALL config();
  SET @US3_LAST_ERRNO = @OK;
  SET @US3_LAST_ERROR = '';

  IF ( verify_analyte_permission( p_guid, p_password, p_analyteID ) = @OK ) THEN

    DELETE FROM solutionAnalyte
    WHERE analyteID = p_analyteID;

    DELETE FROM analytePerson
    WHERE analyteID = p_analyteID;

    DELETE FROM analyteExtinction
    WHERE analyteID = p_analyteID;

    DELETE FROM analyteRefraction
    WHERE analyteID = p_analyteID;

    DELETE FROM analyte
    WHERE analyteID = p_analyteID;

  END IF;

  SELECT @US3_LAST_ERRNO AS status;

END$$
