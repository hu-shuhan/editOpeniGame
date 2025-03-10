/* -*-c++-*- */
#ifndef _CATCORBASequence_h
#define _CATCORBASequence_h
/** @CAA2Required */
/**********************************************************************/
/* DON T DIRECTLY INCLUDE THIS HEADER IN YOUR APPLICATION CODE. IT IS */
/* REQUIRED TO BUILD CAA APPLICATIONS BUT IT MAY DISAPEAR AT ANY TIME */
/**********************************************************************/


// COPYRIGHT DASSAULT SYSTEMES 2000

#include <stdarg.h>
#include "CATBaseUnknown.h"
#include "CATCORBABoolean.h"
#include "CATMacForIUnknown.h"

//
//	X : a one word type 
//

//
//      Window source flavoring
//

/*#ifdef _WINDOWS_SOURCE
 #undef max
 #define max max
#endif*/


#define DEF_SEQ(classname, type)  			\
class Exported classname : public CATExtendable\
{							\
  CATDeclareClass;\
private :\
  unsigned long _length;\
  unsigned long _max;\
  type *   buffer;\
  boolean _release;\
  friend class classname##_marsh;\
public:							\
   classname ();				        \
   classname (unsigned long max);\
   classname (unsigned long max, unsigned long length, type * data, boolean release=FALSE);\
   classname (const classname& from);\
   ~classname ();\
   classname & operator= (const classname& from);\
   unsigned long length() const;\
   void length(unsigned long newLength);\
   unsigned long maximum() const;\
   type& operator [] (unsigned long i);\
   const type& operator [] (unsigned long i) const;\
}

#define IMPL_SEQUENCE(classname,type) \
classname::classname () : _length(0), _max(0), buffer(NULL),_release(FALSE)\
{\
}\
	\
classname::classname (unsigned long max) : _length(0), _max(max), buffer(NULL),_release(FALSE)\
{	 						\
}							\
							\
classname::classname (unsigned long max, unsigned long length, type * data, boolean release) : _length(length), _max(max), buffer(NULL),_release(release)	\
   {	 						\
     if (_release && (_length > 0))\
       {\
         buffer = new type [_length];\
         for (unsigned long i = 0; i< _length; i++)\
           buffer[i] = data[i];\
       }\
     else if (_length >0)\
       buffer = data;\
   }\
\
classname::classname (const classname& from) : _length(from._length), _max(from._max), buffer(NULL),_release(from._release)		\
   {	 						\
     if (_release && (_length > 0))\
       {\
         buffer = new type [_length];\
         for (unsigned long i = 0; i< _length; i++)\
	   buffer[i] = (type) from.buffer[i];\
       }\
   }							\
							\
classname::~classname () 				\
   {	 						\
     if (_release && (buffer != NULL))\
       delete [] buffer;\
   }\
\
classname & classname::operator= (const classname& from)\
   {	 						\
     if (_release && (buffer !=NULL))\
       delete [] buffer;\
     buffer=NULL;\
     _length=from._length;\
     _max=from._max;\
     _release = (from._release);\
     if (_release && (_length > 0))\
       {\
         buffer = new type [_length];\
         for (unsigned long i = 0; i< _length; i++)\
	   buffer[i] = (type) from.buffer[i];\
       }\
     else if (_length> 0)\
       buffer = from.buffer;\
     return *this ;\
   }\
\
unsigned long classname::length() const\
   {\
     return _length ;\
   }\
							\
void classname::length(unsigned long newLength) 			\
	{							\
		if (_length != newLength) { \
			_length = newLength ; \
			if(_release && buffer)  delete[] buffer; \
			if(_length > 0) { \
				_release = TRUE; \
				buffer = new type[_length]; \
			} else { \
				_release = FALSE; \
				buffer = 0; \
			} \
		} \
	}							\
							\
unsigned long classname::maximum() const 				\
   {							\
     return _max ;					\
   }							\
							\
type& classname::operator [] (unsigned long i) 			\
     {							\
       return buffer [i] ;					\
     }									\
								 	        \
CATImplementClass(classname , Implementation, CATBaseUnknown, CATNull)

#define DEF_LIMSEQ(classname, dimension, type)  			\
class Exported classname : public CATBaseUnknown\
{							\
  CATDeclareClass;\
private :\
  unsigned long _length;\
  static unsigned long _max;\
  type *   buffer;\
  boolean _release;\
  friend class classname##_marsh;\
public:							\
   classname ();				        \
   classname (unsigned long length, type * data, boolean release=FALSE);\
   classname (const classname& from);\
   ~classname ();\
   classname & operator= (const classname& from);\
   unsigned long length() const;\
   void length(unsigned long newLength);\
   unsigned long maximum() const;\
   type& operator [] (unsigned long i);\
   const type& operator [] (unsigned long i) const;\
}

#define IMPL_LIMSEQUENCE(classname,dimension,type) \
classname::classname () : _length(0), buffer(NULL),_release(FALSE)\
{\
}\
\
classname::classname (unsigned long length, type * data, boolean release) : _length(length), buffer(NULL),_release(release)	\
   {	 						\
     buffer = new type [_length];\
     if (_release && (_length > 0))\
       {\
         buffer = new type [_length];\
         for (unsigned long i = 0; i< _length; i++)\
           buffer[i] = data[i];\
       }\
     else if (_length>0)\
       buffer = data;\
   }							\
							\
classname::classname (const classname& from) : _length(from._length), buffer(NULL),_release(from._release)		\
   {	 						\
     if (_release && (_length>0))\
       {\
         buffer = new type [_length];\
         for (unsigned long i = 0; i< _length; i++)\
	   buffer[i] = from[i];\
       }\
     else if (_length>0)\
       buffer = from.buffer;\
   }\
\
classname::~classname ()\
   {\
     if (_release && (buffer !=NULL))\
       delete [] buffer;\
   }							\
							\
classname & classname::operator= (const classname& from)\
   {	 						\
     if (_release)\
       delete [] buffer;\
     buffer=NULL;\
     _length=from._length;\
     _release = (from._release);\
     if (_release && (_length>0))\
       {\
         buffer = new type [_length];\
         for (unsigned long i = 0; i< _length; i++)\
	   buffer[i] = from[i];\
       }\
     else if (_length> 0)\
       buffer = from.buffer;\
     return *this ;\
   }							\
							\
unsigned long classname::_max= dimension ;\
							\
unsigned long classname::length() const 				\
   {							\
     return _length ;					\
   }							\
							\
void classname::length(unsigned long newLength) 			\
   {							\
     _length=newLength ;		\
   }							\
							\
unsigned long classname::maximum() const 				\
   {							\
     return _max ;					\
   }							\
							\
type& classname::operator [] (unsigned long i) 			\
{\
  return ((type*) buffer) [i] ;\
}\
								 	        \
const type& classname::operator [] (unsigned long i) const					\
     {									\
       if (_length>=i ) \
          {                                                            	\
           if ( i != 0 )                                        	\
             {                                                         	\
             }                                                         	\
          }                                                            	\
       return ((type*) buffer) [i] ;					\
     }\
CATImplementClass(classname , Implementation, CATBaseUnknown, CATNull)

#endif
