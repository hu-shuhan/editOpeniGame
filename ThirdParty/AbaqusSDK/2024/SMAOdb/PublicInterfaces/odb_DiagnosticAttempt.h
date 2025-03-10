//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
///////////////////////////////////////////////////////////////////////////////
//
// Class       : odb_DiagnosticAttempt
//
// Description : This class represents a single attempt in the incremental
//               iterative solution process. Every attempt is assigned a
//               number that uniquely identifies the attempt within the
//               context of a given increment. The attempt number is an 
//               internal artifice and should not be confused with attempt
//               numbers used by the solver. In the solver, attempt numbers
//               need not be unique within the context of a given increment. 
//               In particular, if an attempt is abandoned to perform an
//               equation reordering but a cutback is not necessary then
//               the solver will reuse the same attempt number (the reasoning
//               behind this is that the attempt number factors into the
//               automatic incrementation scheme and if an attempt is 
//               abandoned to perform a reordering but the attempt size
//               does not need to be cutback then the automatic incrementation
//               scheme should not be affected).
//
//               Attempt objects serve as a container for iteration objects
//               and provide information as to the status of the automatic
//               incrementation scheme (such as the attempt size and the
//               reason(s) why an attempt was abandoned).
//
////////////////////////////////////////////////////////////////////////////////


#ifndef odb_DiagnosticAttempt_h
#define odb_DiagnosticAttempt_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <cow_ArrayCOW.h>
#include <odb_Types.h>
#include <odb_ContactDiagnostic.h>
// End local includes


// Forward declarations
struct dia_CutbackDiagnosticOdbEnum;
class odb_DiagnosticIteration;
class odb_SequenceDiagnosticIteration;
class odb_SequenceCutbackDiagnostic;
class dia_Attempt;
COW_ARRAYCOW_DECL(dia_Attempt);


class odb_DiagnosticAttempt  : public mem_AllocationOperators
{
public:

   // Returns the attempt number. Attempts are numbered starting from
   // one within the context of a given increment. 
   int number() const;

   // Returns the attempt size. The attempt size represents the increment
   // of time that was used in the incremental-iterative solution procedure
   // when performing this attempt
   double size() const;

   // Returns true if the attempt converged.
   bool isConverged() const;

   // Returns the total number of iterations that were performed during 
   // this attempt
   int numberOfIterations() const;

   // Returns the number of equilibrium iterations that were performed
   // during this attempt
   int numberOfEquilibriumIterations() const;

   // Returns the number of severe discontinuity iterations that were
   // performed during this attempt
   int numberOfSevereDiscontinuityIterations() const;

   // Returns an odb_Iteration object by zero-based index. If the index is 
   // out of bounds then an exception of type odb_Exception will be thrown. 
   odb_DiagnosticIteration getIteration(int i) const;

   // Returns an odb_SequenceDiagnosticIteration object
   odb_SequenceDiagnosticIteration iterations() const;

   // Returns true if the attempt was cutback
   bool isCutback() const;

   // Returns the number of cutback diagnostics associated with the 
   // attempt. If the attempt converged then there will no cutback
   // diagnostics. If the attempt did not converge then there may or
   // may not be any cutback diagnostics (sometimes attempts are
   // abandoned and reattempted without converging and without a 
   // cutback - for example equation reordering). Commonly, when
   // an attempt is cutback there will be only a single cutback
   // diagnostic but it is possible that there can be more than one.
   int numberOfCutbackDiagnostics() const; 

   // Returns an enum indicating the type of cutback diagnostic. The argument
   // in an index that must range from 0 to one less than the number of 
   // cutback diagnostics. If the index is out of bounds then an exception of 
   // type odb_Exception will be thrown. 
   dia_CutbackDiagnosticOdbEnum getCutbackDiagnostic(int i) const;

   // Returns an odb_SequenceCutbackDiagnostic object
   odb_SequenceCutbackDiagnostic cutbackDiagnostics() const;

   // Returns true if it was determined during the attempt that the 
   // equations need to be reordered. Equation reorderings may or may
   // not trigger a cutback. A return value of true does not necessarily 
   // indicate that the attempt size was cutback.
   bool needsReordering();

   // Returns true if the attempt was performed for purposes of 
   // calculating the automatic stabilization damping factor.
   bool autoStabilize();

   const dia_Attempt &constGet() const { return m_attempt.ConstGet(); }

private:
   dia_AttemptCOW m_attempt;

public:
   // Unsupported
   odb_DiagnosticAttempt(const dia_Attempt& attempt);
   odb_DiagnosticAttempt(const odb_DiagnosticAttempt& attempt);
   ~odb_DiagnosticAttempt();
};

#endif
