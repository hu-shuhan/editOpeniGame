
#ifndef CATIIRBase_h
#define CATIIRBase_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/


// COPYRIGHT DASSAULT SYSTEMES 2000

#include "CATCORBASequence.h"

#include "CATBaseUnknown.h"


#include "CATJHNTypeLib.h"

typedef char *  CORBA_Identifier;

typedef char *  ScopedName;

typedef char *  RepositoryId;

typedef char *  VersionSpec;

enum DefinitionKind{dk_none,
                    dk_all,
                    dk_Attribute,
                    dk_Constant,
                    dk_Exception,
                    dk_Interface,
                    dk_Module,
                    dk_Operation,
                    dk_Typedef,
                    dk_Alias,
                    dk_Struct,
                    dk_Union,
                    dk_Enum,
                    dk_Primitive,
                    dk_String,
                    dk_Sequence,
                    dk_Array,
                    dk_Repository};

enum PrimitiveKind{pk_null,
                   pk_void,
                   pk_short,
                   pk_long,
                   pk_ushort,
                   pk_ulong,
                   pk_float,
                   pk_double,
                   pk_boolean,
                   pk_char,
                   pk_octet,
                   pk_any,
                   pk_TypeCode,
                   pk_Principal,
                   pk_string,
                   pk_objref};

enum AttributeMode{ATTR_NORMAL,
                   ATTR_READONLY};

enum OperationMode{OP_NORMAL,
                   OP_ONEWAY};

enum ParameterMode{PARAM_IN,
                   PARAM_OUT,
                   PARAM_INOUT};


#endif
