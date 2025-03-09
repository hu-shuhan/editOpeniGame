#ifndef CATTIMESPAN_H
#define CATTIMESPAN_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#ifndef ExportedByNS0S7TIM
# if defined(__NS0S7TIM)
/** @nodoc */
#   define ExportedByNS0S7TIM   DSYExport
# else  // __NS0S7TIM
/** @nodoc */
#   define ExportedByNS0S7TIM   DSYImport
# endif // __NS0S7TIM
#endif  // !ExportedByNS0S7TIM

#include <time.h>
#include "DSYExport.h"

class CATUnicodeString ;  

/**
 * Class to use for a time difference representation.
 * <p>
 * CATTimeSpan has to be used whenever a time representation is
 * needed, ie either seen, entered, or manipulated by the 
 * end user. In this way, the application is shielded from 
 * implementation details regarding language, locale, and so forth.
 * Applications thus become portable since they can handle
 * time expressed in any language.
 * <p>
 */
class ExportedByNS0S7TIM CATTimeSpan
{
 friend class CATTime; 

public :

/**
 * Constructs an empty instance.
 */
   CATTimeSpan();

/**
 * Copy constructor.
 * @param iTimeSpan The CATTimeSpan to copy
 */
   CATTimeSpan(const CATTimeSpan& iTimeSpan);

/**
 * Constructs an instance from time_t variable (see C RunTime Library).
 * @param iTimeSpan
 *   time_t representation of Elapsed Time.
 */
   CATTimeSpan(time_t iTimeSpan); 

/**
 * Constructs an instance from elapsed time components,
 * Elapsed TimeSpan range : ±68 years. 
 * @param iDays
 *   Days <b>Legal values</b>: from -25296 to +25296.
 * @param iHours
 *   Hours <b>Legal values</b>: from -23 to +23.
 * @param iMins
 *   Minutes <b>Legal values</b>: from -59 to +59.
 * @param iSecs
 *   Seconds <b>Legal values</b>: from -59 to +59.
 */
   CATTimeSpan(long iDays, int iHours, int iMins, int iSecs);
   
   ~CATTimeSpan();         

/**
 * Recycles an existing instance from time_t variable (see C RunTime Library).
 * @param iTimeSpan
 *   time_t representation of elapsed time.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>0 </dt>
 *        <dd>An error occured.</dd>
 *     <dt>1 </dt>
 *        <dd>Successful</dd>
 *   </dl>
 */
   int Settime_t(time_t iTimeSpan); 

/**
 * Recycles an existing instance from every Elapsed Time components.
 * Elapsed TimeSpan range : ±68 years. 
 * @param iDays
 *   Days <b>Legal values</b>: from -25296 to +25296.
 * @param iHours
 *   Hours <b>Legal values</b>: from -23 to +23.
 * @param iMins
 *   Minutes <b>Legal values</b>: from -59 to +59.
 * @param iSecs
 *   Seconds <b>Legal values</b>: from -59 to +59.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>0 </dt>
 *        <dd>An error occured.</dd>
 *     <dt>1 </dt>
 *        <dd>Successful</dd>
 *   </dl>
 */
   int SetTimeSpan(long iDays, int iHours, int iMins, int iSecs);

/**
 * Returns the time_t representing elapsed time.
 * The result is inconsistent if this is an Invalid CATTimeSpan Object.
 * @return
 *   A time_t representation of elapsed time.
 */
 time_t Gettime_t() const;

/**
 * Returns the number of complete days in this CATTimeSpan.
 * The result is inconsistent if this is an invalid CATTimeSpan object.
 * @return the number of complete days.
 */
  int GetDays() const;

/**
 * Returns the number of hours in the current day.
 * The result is inconsistent if this is an invalid CATTimeSpan object.
 * @return the number of hours in the current day.
 * <b>Legal values</b>: -23 through 23
 */	
  int GetHours() const;

/**
 * Returns the total number of complete hours in this CATTimeSpan.
 * The result is inconsistent if this is an invalid CATTimeSpan object.
 * @return the total number of complete hours.
 */
  long GetTotalHours() const;

/**
 * Returns the number of minutes in the current hour.
 * The result is inconsistent if this is an invalid CATTimeSpan object.
 * @return the number of minutes in the current hour.
 * <b>Legal values</b>: -59 through 59
 */
  int GetMinutes() const;

/**
 * Returns the total number of complete minutes in this CATTimeSpan.
 * The result is inconsistent if this is an invalid CATTimeSpan object.
 * @return the total number of complete minutes.
 */
  long GetTotalMinutes() const;

/**
 * Returns the number of seconds in the current minute.
 * The result is inconsistent if this is an invalid CATTimeSpan object.
 * @return the number of seconds in the current minute.
 * <b>Legal values</b>: -59 through 59
 */
  int GetSeconds() const;

/**
 * Returns the total number of complete seconds in this CATTimeSpan.
 * The result is inconsistent if this is an invalid CATTimeSpan object.
 * @return the total number of complete seconds.
 */
  long GetTotalSeconds() const;

/**
 * Returns the status of this CATTimeSpan Object.
 * The status of a CATTimeSpan object is invalid in the following cases :
 * <ol>
 * <li>If its value is set from a time_t value that could not be converted to a valid elapsed time value.</li>
 * <li>If its value is set by @href CATTime#SetDateTime with invalid parameter values.</li>
 * <li>If this object has experienced an overflow or underflow during an arithmetic assignement operation ( +, -, +=, etc.).</li>
 * <li>If an invalid value was assigned to this object.</li>
 * </ol>
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>this is a Valid CATTimeSpan Object</dd>
 *     <dt>0 </dt>
 *        <dd>this is a not Valid CATTimeSpan Object</dd>
 *   </dl>
 */
   int GetStatus() const;

/**
 * Converts elapsed time to CATUnicodeString depending on locale conventions and geographical standpoint.
 * @param iFormatString The format of the requested string in which the following substitutions occur:
 * <ol>
 * <dt> %D -> @href #GetDays </dt>
 * <dt> %H -> @href #GetHours </dt>
 * <dt> %M -> @href #GetMinutes </dt>
 * <dt> %S -> @href #GetSeconds </dt>
 * <dt> %h -> @href #GetTotalHours </dt>
 * <dt> %m -> @href #GetTotalMinutes </dt>
 * <dt> %s -> @href #GetTotalSeconds </dt>
 * <dt> %% -> % </dt>
 * </ol>
 * @return CATUnicodeString representing elapsed time.
 */
   CATUnicodeString ConvertToString( const CATUnicodeString& iFormatString ) const; 

/**
 * Assignment operator from a time_t.
 * @param iTimeSpanToCopy
 *   time_t variable to be copied.
 */
   CATTimeSpan& operator =(time_t iTimeSpanToCopy);
/**
 * Assignment operator from a CATTimeSpan instance.
 * @param iTimeSpanToCopy
 *   CATTimeSpan instance to copy.
 */
   CATTimeSpan& operator =(const CATTimeSpan &iTimeSpanToCopy) ;

/**
 * Equality operator.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>if compared Objects are equal,</dd>
 *     <dt>0 </dt>
 *        <dd>otherwise,</dd>
 *     <dt>-1 </dt>
 *        <dd>if one or the two compared objects are Invalid for @href #GetStatus </dd>
 *   </dl>
 */
   int operator == ( const CATTimeSpan &iTimeSpanToCompare ) const ;

/**
 * Inequality operator.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>if compared Objects are different,</dd>
 *     <dt>0 </dt>
 *        <dd>otherwise,</dd>
 *     <dt>-1 </dt>
 *        <dd>if one or the two compared objects are Invalid for @href #GetStatus </dd>
 *   </dl>
 */
   int operator != ( const CATTimeSpan &iTimeSpanToCompare ) const ;
/**
 * Less-than operator.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>if the Object is inferior than iTimeSpanToCompare,</dd>
 *     <dt>0 </dt>
 *        <dd>otherwise,</dd>
 *     <dt>-1 </dt>
 *        <dd>if one or the two compared objects are Invalid for @href #GetStatus </dd>
 *   </dl>
 */
   int operator <  ( const CATTimeSpan &iTimeSpanToCompare ) const ;
/**
 * Less-than or equal operator.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>if the Object is inferior than or equal to iTimeSpanToCompare,</dd>
 *     <dt>0 </dt>
 *        <dd>otherwise,</dd>
 *     <dt>-1 </dt>
 *        <dd>if one or the two compared objects are Invalid for @href #GetStatus </dd>
 *   </dl>
 */
   int operator <= ( const CATTimeSpan &iTimeSpanToCompare ) const ;
/**
 * Greater-than or equal operator.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>if the Object is superior than or equal to iTimeSpanToCompare,</dd>
 *     <dt>0 </dt>
 *        <dd>otherwise,</dd>
 *     <dt>-1 </dt>
 *        <dd>if one or the two compared objects are Invalid for @href #GetStatus </dd>
 *   </dl>
 */
   int operator >= ( const CATTimeSpan &iTimeSpanToCompare ) const ;
/**
 * Greater-than operator.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>if the Object is superior than iTimeSpanToCompare,</dd>
 *     <dt>0 </dt>
 *        <dd>otherwise,</dd>
 *     <dt>-1 </dt>
 *        <dd>if one or the two compared objects are Invalid for @href #GetStatus </dd>
 *   </dl>
 */
   int operator >  ( const CATTimeSpan &iTimeSpanToCompare ) const ;

/**
 * Addition operator.
 * @param iTimeSpan
 *   CATTimeSpan instance representing elapsed time to add to. 
 */
  CATTimeSpan operator + ( const CATTimeSpan &iTimeSpan ) const;
/**
 * Substraction operator.
 * @param iTimeSpan
 *   CATTimeSpan instance representing elapsed time to substract from. 
 */
  CATTimeSpan operator - ( const CATTimeSpan &iTimeSpan ) const;

/**
 * Addition assignment operator.
 * @param iTimeSpan
 *   CATTimeSpan instance representing elapsed time to add from to. 
 */
  const CATTimeSpan& operator += ( const CATTimeSpan &iTimeSpan );
/**
 * Substraction assignment operator.
 * @param iTimeSpan
 *   CATTimeSpan instance representing elapsed time to substract from to. 
 */
  const CATTimeSpan& operator -= ( const CATTimeSpan &iTimeSpan );
	
 private:

  int _StatusTime; 
  time_t _TimeSpan;
  
};

#endif




