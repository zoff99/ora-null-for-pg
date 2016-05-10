##################################
#
# by Zoff <zoff@zoff.cc>
# in Aug.2006
#
##################################


-- use this in postgres:
          
          
-- show null values as string 'null'
\pset null null
          
        
-- use this if you have this trigger already installed and on a table !!
-- drop trigger test_trig on test;
-- DROP function c_oracle_null_style();
 
        
-- create the trigger function
CREATE FUNCTION c_oracle_null_style() RETURNS TRIGGER
   AS 'oracle_null_style' LANGUAGE 'C'
   WITH (iscachable);   
          
-- install trigger on table
CREATE TRIGGER test_trig BEFORE INSERT or UPDATE ON test
  FOR EACH ROW EXECUTE PROCEDURE c_oracle_null_style();

