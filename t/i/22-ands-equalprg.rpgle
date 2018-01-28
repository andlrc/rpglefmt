<%@ free="*YES %>
<%
ctl-opt bnddir('MY_DIR');

dcl-c ISA_R 1 /* read */
dcl-s ISA_W 2 /* write */
dcl-s ISA_CMP 3 /* complete */

/*
* more code here...
*/

/**
* Check if one is allowed to ....
* ....
* ....
* @param id   the ID in question
* @parem type what to be checked
* @return     {@code *ON} if allowed, otherwise {@code *OFF}
*/
dcl-proc isAllowed;
dcl-pi *n ind;
id int(10) value;
type int(10) value; /* One of ISA_* */
end-pi;

dcl-s sqlStmt varchar(2048);
dcl-s pRow pointer;
dcl-ds dsSite likeds(hostSite_t);

/*
* and more code here...
\********************************************/
sqlStmt = `
  select a,
         b
    from c
   where d in (select e
                 from f
                where g = ${%char(id)})
`;
/* if yaya then you are allowed */
if long_proc_call(firstArg :
secondArg);
return *ON;
end-proc;
