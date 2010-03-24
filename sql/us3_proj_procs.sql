--
-- us3_proj_procs.sql
--
-- Script to set up the MySQL stored procedures for the US3 system
--   These are procedures related to the project
-- Run as us3admin
--

DELIMITER $$

-- Verifies that the user has permission to view or modify
--  the specified project
DROP FUNCTION IF EXISTS verify_project_permission$$
CREATE FUNCTION verify_project_permission( p_guid      CHAR(36),
                                           p_password  VARCHAR(80),
                                           p_projectID INT )
  RETURNS TINYINT
  READS SQL DATA

BEGIN
  DECLARE count_projects INT;
  DECLARE status         TINYINT;

  CALL config();
  SET status   = @ERROR;

  SELECT COUNT(*)
  INTO   count_projects
  FROM   projectPerson
  WHERE  projectID = p_projectID
  AND    personID = @US3_ID;
 
  IF ( verify_user( p_guid, p_password ) = @OK &&
       count_projects > 0 ) THEN
    SET status = @OK;

  ELSEIF ( verify_userlevel( p_guid, p_password, @US3_ADMIN ) = @OK ) THEN
    SET status = @OK;

  ELSE
    SET @US3_LAST_ERRNO = @NOTPERMITTED;
    SET @US3_LAST_ERROR = 'MySQL: you do not have permission to view or modify this project';

    SET status = @NOTPERMITTED;

  END IF;

  RETURN( status );

END$$

-- Returns the count of projects associated with p_ID
--  If p_ID = 0, retrieves count of all projects in db
--  Regular user can only get count of his own projects
DROP FUNCTION IF EXISTS count_projects$$
CREATE FUNCTION count_projects( p_guid         CHAR(36),
                                p_password     VARCHAR(80),
                                p_ID           INT )
  RETURNS INT
  READS SQL DATA

BEGIN
  
  DECLARE count_projects INT;

  CALL config();
  SET count_projects = 0;

  IF ( verify_userlevel( p_guid, p_password, @US3_ADMIN ) = @OK ) THEN
    -- This is an admin; he can get more info
    IF ( p_ID > 0 ) THEN
      SELECT COUNT(*)
      INTO   count_projects
      FROM   projectPerson
      WHERE  personID = p_ID;

    ELSE
      SELECT COUNT(*)
      INTO   count_projects
      FROM   projectPerson;

    END IF;

  ELSEIF ( verify_user( p_guid, p_password ) = @OK ) THEN
    IF ( (p_ID != 0) && (p_ID != @US3_ID) ) THEN
      -- Uh oh, can't do that
      SET @US3_LAST_ERRNO = @NOTPERMITTED;
      SET @US3_LAST_ERROR = 'MySQL: you do not have permission to view those projects';
     
    ELSE
      -- This person is asking about his own projects
      -- Ignore p_projectID and return user's own
      SELECT COUNT(*)
      INTO   count_projects
      FROM   projectPerson
      WHERE  personID = @US3_ID;

    END IF;
    
  END IF;

  RETURN( count_projects );

END$$

-- INSERTs a new project with the specified information
DROP PROCEDURE IF EXISTS new_project$$
CREATE PROCEDURE new_project ( p_guid             CHAR(36),
                               p_password         VARCHAR(80),
                               p_goals            TEXT,
                               p_molecules        TEXT,
                               p_purity           TEXT,
                               p_expense          TEXT,
                               p_bufferComponents TEXT,
                               p_saltInformation  TEXT,
                               p_AUC_questions    TEXT,
                               p_notes            TEXT,
                               p_description      TEXT,
                               p_status           ENUM('submitted', 'designed', 
                                                       'scheduled', 'uploaded', 
                                                       'anlyzed',   'invoiced', 
                                                       'paid',      'other') )
  MODIFIES SQL DATA

BEGIN
  DECLARE l_projectID INT;

  CALL config();
  SET @US3_LAST_ERRNO = @OK;
  SET @US3_LAST_ERROR = '';
  SET @LAST_INSERT_ID = 0;
 
  IF ( verify_user( p_guid, p_password ) = @OK ) THEN
    INSERT INTO project SET
      goals            = p_goals,
      molecules        = p_molecules,
      purity           = p_purity,
      expense          = p_expense,
      bufferComponents = p_bufferComponents,
      saltInformation  = p_saltInformation,
      AUC_questions    = p_AUC_questions,
      notes            = p_notes,
      description      = p_description,
      status           = p_status ;

    SET @LAST_INSERT_ID  = LAST_INSERT_ID();

    INSERT INTO projectPerson SET
      projectID = @LAST_INSERT_ID,
      personID  = @US3_ID;

  END IF;

  SELECT @US3_LAST_ERRNO AS status;

END$$

-- UPDATEs an existing project with the specified information
DROP PROCEDURE IF EXISTS update_project$$
CREATE PROCEDURE update_project ( p_guid             CHAR(36),
                                  p_password         VARCHAR(80),
                                  p_projectID        INT,
                                  p_goals            TEXT,
                                  p_molecules        TEXT,
                                  p_purity           TEXT,
                                  p_expense          TEXT,
                                  p_bufferComponents TEXT, 
                                  p_saltInformation  TEXT,
                                  p_AUC_questions    TEXT,
                                  p_notes            TEXT,
                                  p_description      TEXT,
                                  p_status           ENUM('submitted', 'designed', 
                                                          'scheduled', 'uploaded', 
                                                          'anlyzed',   'invoiced', 
                                                          'paid',      'other') )
  MODIFIES SQL DATA               
                                  
BEGIN                             
                                  
  CALL config();                  
  SET @US3_LAST_ERRNO = @OK;      
  SET @US3_LAST_ERROR = '';

  IF ( verify_project_permission( p_guid, p_password, p_projectID ) = @OK ) THEN
    UPDATE project SET
           goals            = p_goals,
           molecules        = p_molecules,
           purity           = p_purity,
           expense          = p_expense,
           bufferComponents = p_bufferComponents,
           saltInformation  = p_saltInformation,
           AUC_questions    = p_AUC_questions,
           notes            = p_notes,
           description      = p_description,
           status           = p_status 
    WHERE  projectID        = p_projectID;

  END IF;
      
  SELECT @US3_LAST_ERRNO AS status;

END$$

-- Returns the projectID, description, and status of all projects associated with p_ID
--  If p_ID = 0, retrieves information about all projects in db
--  Regular user can only get info about his own projects
DROP PROCEDURE IF EXISTS get_project_desc$$
CREATE PROCEDURE get_project_desc ( p_guid     CHAR(36),
                                    p_password VARCHAR(80),
                                    p_ID       INT )
  READS SQL DATA

BEGIN

  CALL config();
  SET @US3_LAST_ERRNO = @OK;
  SET @US3_LAST_ERROR = '';

  IF ( verify_userlevel( p_guid, p_password, @US3_ADMIN ) = @OK ) THEN
    -- This is an admin; he can get more info
    IF ( count_projects( p_guid, p_password, p_ID ) < 1 ) THEN
      SET @US3_LAST_ERRNO = @NOROWS;
      SET @US3_LAST_ERROR = 'MySQL: no rows returned';
   
      SELECT @US3_LAST_ERRNO AS status;

    ELSE
      SELECT @OK AS status;
  
      IF ( p_ID > 0 ) THEN
        SELECT   j.projectID, description, status
        FROM     project j, projectPerson p
        WHERE    j.projectID = p.projectID
        AND      p.personID = p_ID
        ORDER BY description;
   
      ELSE
        SELECT   j.projectID, description, status
        FROM     project j, projectPerson p
        WHERE    j.projectID = p.projectID
        ORDER BY description;

      END IF;

    END IF;

  ELSEIF ( verify_user( p_guid, p_password ) = @OK ) THEN
    IF ( (p_ID != 0) && (p_ID != @US3_ID) ) THEN
      -- Uh oh, can't do that
      SET @US3_LAST_ERRNO = @NOTPERMITTED;
      SET @US3_LAST_ERROR = 'MySQL: you do not have permission to view this project';
     
      SELECT @US3_LAST_ERRNO AS status;

    ELSEIF ( count_projects( p_guid, p_password, @US3_ID ) < 1 ) THEN
      SET @US3_LAST_ERRNO = @NOROWS;
      SET @US3_LAST_ERROR = 'MySQL: no rows returned';
   
      SELECT @US3_LAST_ERRNO AS status;

    ELSE
      -- Ok, user wants his own info
      SELECT @OK AS status;

      SELECT   j.projectID, description, status
      FROM     project j, projectPerson p
      WHERE    j.projectID = p.projectID
      AND      p.personID = @US3_ID
      ORDER BY description;

    END IF;

  END IF;

END$$

-- Returns a more complete list of information about one project
DROP PROCEDURE IF EXISTS get_project_info$$
CREATE PROCEDURE get_project_info ( p_guid      CHAR(36),
                                    p_password  VARCHAR(80),
                                    p_projectID INT )
  READS SQL DATA

BEGIN
  DECLARE count_projects INT;

  CALL config();
  SET @US3_LAST_ERRNO = @OK;
  SET @US3_LAST_ERROR = '';

  SELECT     COUNT(*)
  INTO       count_projects
  FROM       project
  WHERE      projectID = p_projectID;

  IF ( verify_project_permission( p_guid, p_password, p_projectID ) = @OK ) THEN
    IF ( count_projects = 0 ) THEN
      SET @US3_LAST_ERRNO = @NOROWS;
      SET @US3_LAST_ERROR = 'MySQL: no rows returned';

      SELECT @US3_LAST_ERRNO AS status;

    ELSE
      SELECT @OK AS status;

      SELECT   j.projectID, goals, molecules, purity, expense, bufferComponents,
               saltInformation, AUC_questions, notes, description, status, personID
      FROM     project j, projectPerson p
      WHERE    j.projectID = p.projectID
      AND      j.projectID = p_projectID;

    END IF;

  ELSE
    SELECT @US3_LAST_ERRNO AS status;

  END IF;

END$$

-- DELETEs an project, plus information in related tables
DROP PROCEDURE IF EXISTS delete_project$$
CREATE PROCEDURE delete_project ( p_guid      CHAR(36),
                                  p_password  VARCHAR(80),
                                  p_projectID INT )
  MODIFIES SQL DATA

BEGIN
  CALL config();
  SET @US3_LAST_ERRNO = @OK;
  SET @US3_LAST_ERROR = '';

  IF ( verify_project_permission( p_guid, p_password, p_projectID ) = @OK ) THEN

    DELETE FROM projectPerson
    WHERE projectID = p_projectID;

    DELETE FROM project
    WHERE projectID = p_projectID;

  END IF;

  SELECT @US3_LAST_ERRNO AS status;

END$$
