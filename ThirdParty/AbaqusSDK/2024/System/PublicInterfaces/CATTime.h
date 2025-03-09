#ifndef CATTIME_H
#define CATTIME_H

// COPYRIGHT DASSAULT SYSTEMES 1999

/**
 * @CAA2Level L1
 * @CAA2Usage U1
 */

#ifndef ExportedByNS0S7TIM
# if defined(__NS0S7TIM)
#   define ExportedByNS0S7TIM   DSYExport
# else  // __NS0S7TIM
#   define ExportedByNS0S7TIM   DSYImport
# endif // __NS0S7TIM
#endif  // !ExportedByNS0S7TIM


#include <time.h>
#include "DSYExport.h"
#define CATGMT	1

class CATUnicodeString;
class CATTimeSpan;

class CATSysTimeData;

/**
 * Time zone symbolic variables definition
 */
typedef enum
{
  // Time in the current time zone
  CATLocal = 0

  // Universal Coordinated Time (brand new name)
, CATUTC = 1

  // Greenwhich Mean Time (deprecated name)
, CATGMT0 = CATUTC

  // Keep retro conversion
#ifndef CATGMT
, CATGMT = CATUTC
#endif
} CATTz;

/**
 * Class to use for date and time representation.
 * <p>
 * CATTime has to be used whenever a time representation is
 * needed, ie either seen, entered, or manipulated by the 
 * end user. In this way, the application is shielded from 
 * implementation details regarding language, locale, and so forth.
 * Applications thus become portable since they can handle
 * time expressed in any language.
 * <p>
 */
class ExportedByNS0S7TIM CATTime
{


public :

/**
 * Constructs an instance.
 * Current Date & Time are used for member variable initialization
 */
   CATTime();

/**
 * Copy constructor.
 * Constructs an instance from another CATTime instance.
 */
   CATTime(const CATTime& iTime);

/**
 * Constructs an instance from time_t variable (see C RunTime Library).
 * @param iTime
 *   time_t representation of Date & Time.
 */
   CATTime(time_t iTime); 

/**
 * Constructs an instance from every Date & Time components,
 * @param iYear
 *   Year <b>Legal values</b>: from 1970 to 2038.
 * @param iMonth
 *   Month <b>Legal values</b>: from 1 to 12.
 * @param iDay
 *   Day <b>Legal values</b>: from 1 to 31.
 * @param iHour
 *   Hour
 * @param iMin
 *   Minute
 * @param iSec
 *   Second
 * @param iTimeZone
 *   Time zone value (Default is <b>CATLocal</b>).
 */
   CATTime(int iYear, int iMonth, int iDay, int iHour, int iMin, int iSec, const CATTz& iTimeZone = CATLocal);
   
   ~CATTime();


/**
 * Returns an object that represents the current time.
 */
   static CATTime GetCurrentLocalTime();


/**
 * Recycles an existing instance from time_t variable (see C RunTime Library).
 * @param iTime
 *   time_t representation of Date and Time.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>Successful.</dd>
 *     <dt>0 </dt>
 *        <dd>An error occurred.</dd>
 *   </dl>
 */
   int Settime_t(time_t iTime);

/**
 * Recycles an existing instance from every Date and Time components.
 * @param iYear
 *   Year <b>Legal values</b>: from 1970 to 2038.
 * @param iMonth
 *   Month <b>Legal values</b>: from 1 to 12.
 * @param iDay
 *   Day <b>Legal values</b>: from 1 to 31.
 * @param iHour
 *   Hour
 * @param iMin
 *   Minute
 * @param iSec
 *   Second
 * @param iTimeZone
 *   Time zone value (Default is <b>CATLocal</b>).
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>Successful.</dd>
 *     <dt>0 </dt>
 *        <dd>An error occurred.</dd>
 *   </dl>
 */
   int SetDateTime(int iYear, int iMonth, int iDay, int iHour, int iMin, int iSec, const CATTz& iTimeZone = CATLocal);

/**
 * Recycles an existing instance from every Date components.
 * @param iYear
 *   Year <b>Legal values</b>: from 1970 to 2038.
 * @param iMonth
 *   Month <b>Legal values</b>: from 1 to 12.
 * @param iDay
 *   Day <b>Legal values</b>: from 1 to 31.
 * @param iTimeZone
 *   Time zone value (Default is <b>CATLocal</b>).
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>Successful.</dd>
 *     <dt>0 </dt>
 *        <dd>An error occurred.</dd>
 *   </dl>
 */
   int SetDate(int iYear, int iMonth, int iDay, const CATTz& iTimeZone = CATLocal);

/**
 * Recycles an existing instance from every Time components.
 * @param iHour
 *   Hour
 * @param iMin
 *   Minutes
 * @param iSec
 *   Seconds
 * @param iTimeZone
 *   Time zone value (Default is <b>CATLocal</b>).
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>Successful.</dd>
 *     <dt>0 </dt>
 *        <dd>An error occurred.</dd>
 *   </dl>
 */
   int SetTime(int iHour, int iMin, int iSec, const CATTz& iTimeZone = CATLocal);

/**
 * Returns the hour.
 * The result is inconsistent if this is an Invalid CATTime Object.
 * @param iTimeZone
 *   Time zone value (Default is <b>CATLocal</b>).
 * @return the hour 
 */
   int GetHour(const CATTz& iTimeZone = CATLocal) const;

/**
 * Returns the minute.
 * The result is inconsistent if this is an Invalid CATTime Object.
 * @param iTimeZone
 *   Time zone value (Default is <b>CATLocal</b>).
 * @return the minute.
 */
   int GetMinute(const CATTz& iTimeZone = CATLocal) const;

/**
 * Returns the second.
 * The result is inconsistent if this is an Invalid CATTime Object.
 * @param iTimeZone
 *   Time zone value (Default is <b>CATLocal</b>).
 * @return the second.
 */
   int GetSecond(const CATTz& iTimeZone = CATLocal) const;

/**
 * Returns the day.
 * The result is inconsistent if this is an Invalid CATTime Object.
 * @param iTimeZone
 *   Time zone value (Default is <b>CATLocal</b>).
 * @return the day.
 */
   int GetDay(const CATTz& iTimeZone = CATLocal) const;

/**
 * Returns the month.
 * The result is inconsistent if this is an Invalid CATTime Object.
 * @param iTimeZone
 *   Time zone value (Default is <b>CATLocal</b>).
 * @return the month.
 */
   int GetMonth(const CATTz& iTimeZone = CATLocal) const;

/**
 * Returns the year.
 * The result is inconsistent if this is an Invalid CATTime Object.
 *   Time zone value (Default is <b>CATLocal</b>).
 * @return the year.
 */
   int GetYear(const CATTz& iTimeZone = CATLocal) const;

/**
 * Returns the day of the year.
 * The result is inconsistent if this is an Invalid CATTime Object.
 * @param iTimeZone
 *   Time zone value (Default is <b>CATLocal</b>).
 * @return the day.
 */
   int GetDayOfYear(const CATTz& iTimeZone = CATLocal) const;

/**
 * Returns the day of the week.
 * The result is inconsistent if this is an Invalid CATTime Object.
 * @param iTimeZone
 *   Time zone value (Default is <b>CATLocal</b>).
 * @return the day.
 */
   int GetDayOfWeek(const CATTz& iTimeZone = CATLocal) const;

/**
 * Returns a time_t structure.
 * The result is inconsistent if this is an Invalid CATTime Object.
 * @return A time_t representation of Date & Time
 */
   time_t Gettime_t() const;

/**
 * Returns the status of this CATTime Object.
 * The status of a CATTime object is invalid in the following cases :
 * <ol>
 * <li>If its value is set from a time_t value that could not be converted to a valid date and time value.</li>
 * <li>If its value is set by @href #SetDateTime with invalid parameter values.</li>
 * <li>If this object has experienced an overflow or underflow during an arithmetic assignement operation ( +, -, +=, etc.).</li>
 * <li>If an invalid value was assigned to this object.</li>
 * </ol>
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>this is a Valid CATTime Object</dd>
 *     <dt>0 </dt>
 *        <dd>this is a not Valid CATTime Object</dd>
 *   </dl>
 */
   int GetStatus() const;

/**
 * Converts date and time to CATUnicodeString depending on locale conventions and geographical standpoint.
 * @param iFormatString The Format-control string. See <tt>strftime</tt> in C RunTime Library for valid format.
 * @return CATUnicodeString representing date and time.
 * @note This method is deprecated, use the below one instead
 */
   CATUnicodeString ConvertToString ( const CATUnicodeString& iFormatString,
				      int iFlag=0 ) const;

/**
 * Same as @href #ConvertToString.
 * @param iTimeZone
 *   Time zone value
 */
   CATUnicodeString ConvertToString(const CATUnicodeString& iFormatString, const CATTz& iTimeZone) const;

/**
 * Assignment operator from a time_t.
 * @param iTimeToCopy
 *   time_t variable to be copied.
 */
   CATTime& operator = (time_t iTimeToCopy);
/**
 * Assignment operator from a CATTime instance.
 * @param iTimeToCopy
 *   CATTime instance to copy.
 */
   CATTime& operator = (const CATTime &iTimeToCopy);

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
   int operator == ( const CATTime &iTimeToCompare ) const;
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
   int operator != ( const CATTime &iTimeToCompare ) const;
/**
 * Less-than operator.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>if the Object is inferior than iTimeToCompare,</dd>
 *     <dt>0 </dt>
 *        <dd>otherwise,</dd>
 *     <dt>-1 </dt>
 *        <dd>if one or the two compared objects are Invalid for @href #GetStatus </dd>
 *   </dl>
 */
   int operator <  ( const CATTime &iTimeToCompare ) const;
/**
 * Less-than or equal operator.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>if the Object is inferior than or equal to iTimeToCompare,</dd>
 *     <dt>0 </dt>
 *        <dd>otherwise,</dd>
 *     <dt>-1 </dt>
 *        <dd>if one or the two compared objects are Invalid for @href #GetStatus </dd>
 *   </dl>
 */
   int operator <= ( const CATTime &iTimeToCompare ) const;
/**
 * Greater-than or equal operator.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>if the Object is superior than or equal to iTimeToCompare,</dd>
 *     <dt>0 </dt>
 *        <dd>otherwise,</dd>
 *     <dt>-1 </dt>
 *        <dd>if one or the two compared objects are Invalid for @href #GetStatus </dd>
 *   </dl>
 */
   int operator >= ( const CATTime &iTimeToCompare ) const;
/**
 * Greater-than operator.
 * @return
 * <br><b>Legal values</b>:
 *   <dl>
 *     <dt>1 </dt>
 *        <dd>if the Object is superior than iTimeToCompare,</dd>
 *     <dt>0 </dt>
 *        <dd>otherwise,</dd>
 *     <dt>-1 </dt>
 *        <dd>if one or the two compared objects are Invalid for @href #GetStatus </dd>
 *   </dl>
 */
   int operator >  ( const CATTime &iTimeToCompare ) const;


/**
 * Addition operator from  CATTimeSpan.
 * @param iTimeSpan
 *   CATTimeSpan instance representing elapsed time to add to. 
 */
  CATTime operator + ( const CATTimeSpan &iTimeSpan ) const;
/**
 * Substraction operator from CATTimeSpan.
 * @param iTimeSpan
 *   CATTimeSpan instance representing elapsed time to substract from. 
 */
  CATTime operator - ( const CATTimeSpan &iTimeSpan ) const;

/**
 * Substraction operator from CATTime.
 * @param iTimeToSubstract
 *   CATTime instance representing date and time to substract from. 
 */
  CATTimeSpan operator - ( const CATTime &iTimeToSubstract ) const;

/**
 * Addition assignment operator from CATTimeSpan.
 * @param iTimeSpan
 *   CATTimeSpan instance representing elapsed time to add from to. 
 */
  const CATTime& operator += ( const CATTimeSpan &iTimeSpan );
/**
 * Substraction assignment operator from CATTimeSpan.
 * @param iTimeSpan
 *   CATTimeSpan instance representing elapsed time to substract from to. 
 */
  const CATTime& operator -= ( const CATTimeSpan &iTimeSpan );


private:
  time_t _Time;
  CATSysTimeData *_Data;
};

#endif
