#ifndef CATIScriptManager_h
#define CATIScriptManager_h

// COPYRIGHT DASSAULT SYSTEMES 2000

/**
 * @CAA2Level L1
 * @CAA2Usage U3
 */

#include "JS0LOGRP.h"
#include "CATBaseUnknown.h"
#include <iostream.h>
#include <fstream.h>
#include "CATScriptLanguage.h"
#include "CATBoolean.h"
#include "CATString.h"
#include "CATUnicodeString.h"

class CATBaseDispatch;
class CATIScriptJournal;

#ifndef LOCAL_DEFINITION_FOR_IID
extern ExportedByJS0LOGRP IID IID_CATIScriptManager;
#else
extern "C" const IID IID_CATIScriptManager;
#endif

/**
 * Interface to represent the manager of macros generation.
 * <b>Role:</b>
 * <tt>CATScriptManager</tt> is the high level object used to trigger user interaction
 * recording and script generation in various scripting syntaxes.
 * @see CATIScriptJournal, CATScriptLanguage
 */
class ExportedByJS0LOGRP CATIScriptManager : public CATBaseUnknown {
  CATDeclareInterface;

  public :

		/**
		 * Starts recording.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT StartRecording() = 0;

		/**
		 * Stops recording.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT StopRecording() = 0;

		/**
		 * Returns TRUE if the Script Manager is recording, FALSE otherwise.
		 * @param oIsRecording
		 * The returned boolean descripting the state of the script manager.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT IsRecording(CATBoolean & oIsRecording) = 0;

		/**
		 * Temporarily suspends the recording. 
		 * <br><b>Role:</b>
		 * Use this function when you want to inhibit macro generation for
		 * Automation API calls which would normally result in 
		 * declarations in the journal. <tt>SuspendRecording</tt> can
		 * be called several times in a row. If such is the case, the
		 * same number of calls to <tt>ResumeRecording</tt> must be done
		 * before the macro generationa actually resumes.
		 * This method must be called only after the call to <tt>StartRecording</tt>
		 * has been made.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT SuspendRecording() = 0;

		/**
		 * Resumes the recording halted by <tt>SuspendRecording</tt>.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT ResumeRecording() = 0;

		/**
		 * Flushes the journal used for internal storage.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT Reset() = 0;

		/**
		 * Generates a script.
		 * <br><b>Role:</b>
		 * Use this method to write a script in the proper syntax of the specified language with
		 * the given method name and write it to the supplied stream.
		 * @param iLanguage
		 * The specified language of the generated script.
		 * @param iMainMethodName
		 * The main method name that can be used later as the entry point of the script.
		 * @param ioStream
		 * The supplied stream in which the script will be generated.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT GenerateScript(CATScriptLanguage iLanguage, const CATString & iMainMethodName, iostream * ioStream) = 0;

		/**
		 * Generates the script representing the resolution of an automation object in the specified language.
		 * @param iObject
		 * The automation object to resolve.
		 * @param iLanguage
		 * The specified language of the generated script.
		 * @param ioScript
		 * The supplied CATUnicodeString in which the script will be written.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT GenerateResolutionScriptOfObject(CATBaseDispatch * iObject, CATScriptLanguage iLanguage, CATUnicodeString& ioScript) = 0;

		/**
		 * Returns the journal used for internal storage.
		 * <br>
		 * Returns an error if the script manager is not recording.
		 * @param oJournal
		 * The returned journal.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT GetJournal(CATIScriptJournal *& oJournal) = 0;

		/**
		 * @nodoc
		 * Enables or disables the dump of the recorded method calls following this call.
		 * @param iDumpMethods
		 * TRUE to enable the dump of the method calls, FALSE otherwise.
		 * @return 
		 * S_OK if the operation succeeded, E_FAIL otherwise.
		 */
		virtual HRESULT MustDumpMethods(CATBoolean iDumpMethods) = 0;

};


/**
 * Returns the global CATIScriptManager instance.
 * @param oScriptManager
 * The returned script manager.
 * @return 
 * S_OK if the operation succeeded, E_FAIL otherwise.
 */
HRESULT ExportedByJS0LOGRP GetScriptManager(CATIScriptManager *& oScriptManager);

#endif

