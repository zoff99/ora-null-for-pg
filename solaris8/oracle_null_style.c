/******************************************************************************
*
*
*   PostgreSQL C Function (Solaris 8 and Postgres 8.1.x Version)
*
*   by Zoff <zoff@zoff.cc>
*   in Aug.2004
*
*   If you should use this function, or find it in anyway useful
*   please mention me as the author, and keep these lines in here
*
*
*   !! I take no responsibility if something goes horribly !!
*   !! wrong when using this function                      !!
*
*
*****************************************************************************/

#include "postgres.h"			/* general Postgres declarations */
#include "commands/trigger.h"           /* triggers */


/* These prototypes just prevent possible warnings from gcc. */

Datum		c_oracle_null_style(PG_FUNCTION_ARGS);


/**********************************************************************
*
*   Trigger function to change every '' in input to NULL in output
*   by Zoff <zoff@zoff.cc>
*   in Aug.2004
*
***********************************************************************/

PG_FUNCTION_INFO_V1(c_oracle_null_style);

Datum
c_oracle_null_style(PG_FUNCTION_ARGS)
{

        TriggerData     *trigdata = (TriggerData *) fcinfo->context;
	Relation        rel = NULL;             /* triggered relation */
	char            *relname=NULL;          /* triggered relation name */
	char            *attr_name=NULL;
	char            *attr_value=NULL;
        HeapTuple       tuple = NULL;           /* input ROW */
        HeapTuple       result_tuple = NULL;    /* ROW to return */
	TupleDesc       tupdesc = NULL;         /* tuple description */
        int             numberOfAttributes = NULL;
	int             change_att_num = 0;
	int             *change_nums=NULL;


        /* Called by trigger manager ? */
        if (!CALLED_AS_TRIGGER(fcinfo))
	  /* internal error */
	  elog(ERROR, "c_oracle_null_style: not fired by trigger manager");



        /* If INSERTion then must check Tuple to being inserted */
        if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
	  tuple = trigdata->tg_trigtuple;

        /* Not should be called for DELETE */
        else if (TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
	  /* internal error */
	  elog(ERROR, "c_oracle_null_style: can't process DELETE events");
                
        /* If UPDATion the must check new Tuple, not old one */
        else
	  tuple = trigdata->tg_newtuple;

	rel = trigdata->tg_relation;
	tupdesc = trigdata->tg_relation->rd_att;
	numberOfAttributes = tupdesc->natts;


	relname = SPI_getrelname(rel);
	// elog(NOTICE, "c_oracle_null_style: working on table %s",relname);
 
        if (numberOfAttributes > MaxTupleAttributeNumber)
	  ereport(ERROR,
		  (errcode(ERRCODE_TOO_MANY_COLUMNS),
		   errmsg("number of columns (%d) exceeds limit (%d)",
			  numberOfAttributes, MaxTupleAttributeNumber)));


	// SPI_connect();

	Datum	   *volatile modvalues;
	char	   *volatile modnulls;

	modvalues=NULL;
	modnulls=NULL;

	change_nums = palloc(numberOfAttributes* sizeof(int));
	modvalues = palloc(numberOfAttributes * sizeof(Datum));

	/* run through all items in a row (UPDATE or INSERT) */

	int i;
        for (i = 0; i < numberOfAttributes; i++)
	{
	  // elog(NOTICE, "c_oracle_null_style: working on field #%d",i);

	  // first MUST check if value is NULL , or elese function CRASH !!
	  if (SPI_getvalue(tuple,tupdesc,i+1)==NULL)
	  {
	    attr_value=NULL;
	    // elog(NOTICE, "c_oracle_null_style: field value is NULL");
	  }
	  else
	  {
	    // value is not NULL, now get real value
	    attr_value = SPI_getvalue(tuple,tupdesc,i+1);
	    // elog(NOTICE, "c_oracle_null_style: field value %s",attr_value);
	  }

	  attr_name = SPI_fname(tupdesc,i+1);
	  // elog(NOTICE, "c_oracle_null_style: working on field %s",attr_name);

	  if (attr_value==NULL)
	  {
	    // use this only for DEBUG, it makes too much output !!
	    // elog(NOTICE, "c_oracle_null_style: field %s.%s value is already NULL",relname,attr_name);
	  }
	  else if (strlen(attr_value)==0)
	  {
	    /* change all '' (empty strings) in row items to NULL */
	    // elog(NOTICE, "c_oracle_null_style: field %s.%s value converted to NULL",relname,attr_name);

	    change_nums[change_att_num] = i + 1;
	    modvalues[change_att_num] = (Datum) NULL;

	    change_att_num++;

	  }

	  SPI_pfree(attr_name);

          if (attr_value!=NULL)
          {
          	SPI_pfree(attr_value);
          }

        }

	
	modnulls = palloc(change_att_num + 1);
	memset(modnulls, 'n', change_att_num);
	modnulls[change_att_num] = '\0';

	SPI_pfree(relname);

	// SPI_finish();

	if (change_att_num==0)
	{

	  pfree(change_nums);
	  pfree(modnulls);
	  pfree(modvalues);

	  // nothing was changed , just return the input unchanged !
	  return PointerGetDatum(tuple);
	}
	else
	{

	  // make changes , and get changed row back
	  result_tuple=SPI_modifytuple(rel,tuple,change_att_num,change_nums,modvalues,modnulls);

	  if (result_tuple == NULL)
	    elog(ERROR, "c_oracle_null_style: SPI_modifytuple failed");

	  pfree(change_nums);
	  pfree(modnulls);
	  pfree(modvalues);

	  // now return changed row to UPDATE or INSERT statement
	  return PointerGetDatum(result_tuple);

	}


}

/*


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


*/

