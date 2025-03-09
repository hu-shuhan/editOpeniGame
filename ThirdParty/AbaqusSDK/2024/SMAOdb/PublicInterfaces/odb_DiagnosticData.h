//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
////////////////////////////////////////////////////////////////////////////////
//
// Class       :  odb_DiagnosticData
//
// Description :  Class odb_DiagnosticData represents the "root" object in a
//                hierarchy of diagnostic objects. All diagnostic information
//                is ultimately accessed through the diagnostic data object.
//
//                There is one diagnostic data object associated with each
//                job. Data that is associated with the job as a whole, such
//                as CPU times and the job status, is accessible directly
//                through the diagnostic data object. Data that is associated
//                with a particular time in the job history (such as the
//                stabilization factor that was used during a step or the
//                maximum force residual that occurred during an iteration)
//                must be accessed by navigating from the diagnostic data
//                object to a lower-level object that contains the data
//                of interest.
//
////////////////////////////////////////////////////////////////////////////////


#ifndef odb_DiagnosticData_h
#define odb_DiagnosticData_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <cow_ArrayCOW.h>
// End local includes

// Forward declarations
struct dia_JobStatusOdbEnum;
class dia_DiagnosticData;
class odb_DiagnosticStep;
class odb_SequenceDiagnosticStep;
class odb_AnalysisWarning;
class odb_AnalysisError;
class odb_SequenceAnalysisWarning;
class odb_SequenceAnalysisError;
class odb_JobTime;
class odb_NumericalProblemSummary;
COW_ARRAYCOW_DECL(dia_DiagnosticData)


////////////////////////////////////////////////////////////////////////////////

class odb_DiagnosticData  : public mem_AllocationOperators
{
public:
   // Symbolic constant indicating the job status.
   dia_JobStatusOdbEnum jobStatus() const;

   // Check for double precision explicit
   bool isXplDoublePrecision() const;
   
   // number of domains
   int GetNumDomains()const;

   // User cpu time, system cpu time, and wallclock time in seconds.
   // If the job status is unknown then all the job times will be zero.
   odb_JobTime jobTime() const;

   // Number of steps for which diagnostic information is available.
   // In general, data will NOT be available for all steps that were
   // performed during the job.
   int numberOfSteps() const;

   // Access step object by zero-based index. Steps objects will be
   // returned in order of increasing step number. If the index is
   // out of bounds then an exception of type odb_Exception will be
   // thrown.
   odb_DiagnosticStep getStep(int i) const;

   // Sequence of step objects.
   odb_SequenceDiagnosticStep steps() const;

   // Number of analysis warnings that occurred during the job.
   int numberOfAnalysisWarnings() const;

   // Access a warning object by zero-based index. If the index
   // is out of bounds then an exception of type odb_Exception
   // will be thrown.
   odb_AnalysisWarning getAnalysisWarning(int i) const;

   // Sequence of odb_AnalysisWarning objects
   odb_SequenceAnalysisWarning analysisWarnings() const;

   // Number of analysis errors that occurred during the job.
   int numberOfAnalysisErrors() const;

   // Access an error object by zero-based index. If the index
   // is out of bounds then an exception of type odb_Exception
   // will be thrown.
   odb_AnalysisError getAnalysisError(int i) const;

   // Sequence of odb_AnalysisError objects
   odb_SequenceAnalysisError analysisErrors() const;

   // Summary of numerical problems for the entire job
   odb_NumericalProblemSummary numericalProblemSummary() const;

private:
   dia_DiagnosticDataCOW m_diagnosticData;

public:
   // Unsupported
   odb_DiagnosticData();
   odb_DiagnosticData(const dia_DiagnosticData& diagnosticData);
   odb_DiagnosticData(const odb_DiagnosticData& diagnosticData);
   odb_DiagnosticData& operator = (const odb_DiagnosticData& diagnosticData);
   odb_SequenceInt jobHistoryDescription() const;
   ~odb_DiagnosticData();
};

#endif
