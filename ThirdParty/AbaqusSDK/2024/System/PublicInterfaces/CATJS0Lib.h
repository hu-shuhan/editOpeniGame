#ifndef CATJS0LIB_H
#define CATJS0LIB_H 
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/

/*
// COPYRIGHT DASSAULT SYSTEMES 1999
*/


/*Ia64+ */ 
# ifdef PLATEFORME_DS64
#include "CATDataType.h"
# endif
/*Ia64- */ 

#include "JS0LIB.h"

#ifdef  CATSYSV5_SYSTEMCUT
#ifdef __cplusplus
extern "C" {
#endif

/** @nodoc */
ExportedByJS0LIB CATLibStatus CATStartLibServices (void);
/** @nodoc */
ExportedByJS0LIB CATLibStatus CATStopLibServices (void);


/** @nodoc */
ExportedByJS0LIB CATLibStatus CATGetFilePathFromOfficialVariable ( const char * Filename,
                                                                   const char * OffVar,
                                                                         char * Path );
/** @nodoc */
ExportedByJS0LIB const char *CATGetOfficialVariable ( const char *varname );
/** @nodoc */
ExportedByJS0LIB int CATIsActiveLevel ( const char *levname );
/** @nodoc */
ExportedByJS0LIB const char *CATGetEnvName ();
/** @nodoc */
ExportedByJS0LIB const char *CATGetEnvDir ();
/** @nodoc */
ExportedByJS0LIB int CATGetEnvStatus ( int  * env_mode,    int *is_default_env,
                                       char * env_name, int env_name_size,
                                       char * env_path, int env_path_size);
/** @nodoc */
ExportedByJS0LIB int MakeEnvManager ();
/** @nodoc */
ExportedByJS0LIB CATLibStatus CATPutEnvReg (const char *assignexpr);



#ifdef __cplusplus
};
#endif
#endif //SYSTEM_IS_CUT

#endif
