//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
/* -*- mode: c++ -*- */
#ifndef odb_Odb_h
#define odb_Odb_h

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_UserData.h>
#include <odb_CustomData.h>
#include <odb_JobData.h>
#include <odb_ApiExtension.h>
#include <odb_Types.h>
#include <SMABasStringMode.h>

// Forward declarations
class odb_String;
class odb_Frame;
class odb_Part;
class odb_Step;
class odb_SectionCategory;
class odb_DiagnosticData;
class odiK_Model;

class ddb_Ddb;
COW_COW_DECL(ddb_Ddb);

class odb_PartRepository;
COW_COW_DECL(odb_PartRepository);

class odb_SectionCategoryRepository;
COW_COW_DECL(odb_SectionCategoryRepository);

class odb_StepRepository;
COW_COW_DECL(odb_StepRepository);

class odb_Assembly;
COW_COW_DECL(odb_Assembly);

class odb_MaterialRepository;
COW_COW_DECL(odb_MaterialRepository);

class odb_SectionRepository;
COW_COW_DECL(odb_SectionRepository);

class odb_BCRepository;
COW_COW_DECL(odb_BCRepository);

class odb_IPRepository;
COW_COW_DECL(odb_IPRepository);

class odb_ContactControlsRepository;
COW_COW_DECL(odb_ContactControlsRepository);

class odb_InteractionRepository;
COW_COW_DECL(odb_InteractionRepository);

class odb_PredefinedFieldRepository;
COW_COW_DECL(odb_PredefinedFieldRepository);

class odb_LoadRepository;
COW_COW_DECL(odb_LoadRepository);

class odb_ConstraintRepository;
COW_COW_DECL(odb_ConstraintRepository);

class odb_FieldOutputRequestRepository;
class odb_HistoryOutputRequestRepository;
COW_COW_DECL(odb_HistoryOutputRequestRepository);
COW_COW_DECL(odb_FieldOutputRequestRepository);

class odb_SectorDefinition;

class odb_Odb : public mem_AllocationOperators
{
public:
    // member access
    odb_String name(STRMODE_DECL) const;
    odb_String analysisTitle() const;
    odb_String description() const;
    odb_String globalJob() const;
    odb_String shellGlobalModelToSolid() const;
    // member mutator
    void setAnalysisTitle(const odb_String& title);
    void setDescription(const odb_String& desc);
    void setGlobalJob(const odb_String& globalJob);
    void setTopLevelSolverInput(const odb_String& fileHeader);
    odb_String topLevelSolverInput() const;
    void setShellGlobalModelToSolid(bool shellToSolid);
    void setCyclicSymmetry(const odb_SequenceFloat& ptA,
			   const odb_SequenceFloat& ptB,
			   int numSectors,
			   const odb_SequenceString& tieConstraintNames );
			      
    odb_String path(STRMODE_DECL) const;
    odb_JobData jobData() const;
    odb_DiagnosticData diagnosticData() const;
    odb_StepRepository& steps();
    const odb_StepRepository& steps() const;
    odb_PartRepository& parts();
    odb_SectionCategoryRepository& sectionCategories();
    const odb_SectionCategoryRepository& sectionCategories() const;

    odb_BCRepository& boundaryConditions();
    odb_IPRepository& interactionProperties();
    odb_ContactControlsRepository& contactControls();
    odb_InteractionRepository& interactions();
    odb_PredefinedFieldRepository& predefinedFields();
    odb_LoadRepository& loads();
    odb_ConstraintRepository& constraints();
    odb_FieldOutputRequestRepository& fieldOutputRequests();
    odb_HistoryOutputRequestRepository& historyOutputRequests();

    // methods
    void save(odb_String path="", STRMODE_DECL);
    odb_Frame getFrame(double frameValue,
                        odb_Enum::odb_MatchEnum=odb_Enum::CLOSEST);

    // part
    odb_Part& Part(const odb_String& name,
                   odb_Enum::odb_DimensionEnum space,
                   odb_Enum::odb_PartTypeEnum type);

    // step
    odb_Step& Step(const odb_String& name,
                   const odb_String& description,
                   odb_Enum::odb_DomainEnum domain,
                   double timePeriod=0.0,
                   const odb_String& previousStepName="",
                   const odb_String& procedure="",
                   double totalTime=-1.0,
                   bool nlGeom=true);

    // sectionCategory
    odb_SectionCategory& SectionCategory(const odb_String& name,
                                         const odb_String& description);

    // rootAssembly
    odb_Assembly& rootAssembly();

    // isReadOnly
    void setReadOnly(bool iRO) {m_readOnly = iRO;}
    bool isReadOnly() const {return m_readOnly;}

    void setReadInternalSets( bool is ) { m_readInternalSets = is; }
    bool readInternalSets() const {return m_readInternalSets;}

    // userData
    odb_UserData &userData();
    odb_CustomData &customData();

    bool update();
    bool hasSectorDefinition() const;
    const odb_SectorDefinition sectorDefinition() const;

    // solverInput snippet
    void setSolverInput(const odb_String& solverInput);
    odb_String solverInput() const;

    void close();


private:
    ddb_DdbCOW m_ddb;

    odb_String m_name;
    odb_String m_description;
    odb_String m_analysisTitle;
    odb_String m_path;
    bool m_onlyHasUserDefinedData;

    // COWs are used to only to prevent excessive compiles
    // Do not use ContainerCOW.Get() in implementation,
    // use the respective containers() method instead

    odb_StepRepositoryCOW            m_stepContainer;
    odb_PartRepositoryCOW            m_partContainer;
    odb_SectionCategoryRepositoryCOW m_sectCatCon;
    odb_AssemblyCOW                  m_rootAssembly;
    odb_BCRepositoryCOW m_bcContainer;
    odb_IPRepositoryCOW m_ipContainer;
    odb_ContactControlsRepositoryCOW m_ccContainer;
    odb_InteractionRepositoryCOW m_interactionContainer;
    odb_PredefinedFieldRepositoryCOW m_predefinedFieldContainer;
    odb_LoadRepositoryCOW m_loadContainer;
    odb_ConstraintRepositoryCOW m_constraintContainer;
    odb_FieldOutputRequestRepositoryCOW m_fieldOutputRequestContainer;
    odb_HistoryOutputRequestRepositoryCOW m_historyOutputRequestContainer;

    odb_UserData                  m_userData;
    odb_CustomData                m_customData;

    bool m_readOnly;
    bool m_isScratch;
    odb_AssemblyCOW m_scratchAssembly;
    bool isRead;
    odb_ApiExtensionRepositoryCOW m_extensionContainer;
    bool m_readInternalSets;

    odb_String m_globalJob;
    bool m_shellGlobalModelToSolid;
    

public:   // undocumented and unsupported
    odb_Odb();
    odb_Odb(const odb_String name,
            const odb_String analysisTitle,
            const odb_String descrip,
            const odb_String path,
            ddb_Ddb& ddb,
            bool readOnly,
            bool createUserData = true,
            bool readInternalSets = false, 
            STRMODE_DECL);

    void Read();
    void ReadStepCon();
    void ReadStep( const odb_String& stepName,
                   const odb_String& previousStepName );
    void ReadParts();
    void ReadSectionCategories();
    void ReadRootAssy();
    void ReadUserData();
    void setOnlyHasUserDefinedData(bool);
    bool onlyHasUserDefinedData() const { return m_onlyHasUserDefinedData; }
    void setScratch(bool isS, const odb_Assembly &rootAss);
    bool isScratch() const { return m_isScratch; }
    const odb_Assembly& ScratchAssembly() const { return m_scratchAssembly.ConstGet(); }
    bool isClosed() const;


    ddb_Ddb ddb() const;
    ddb_ProcedureTypeEnm procedureTypeToEnum(const odb_String& procedure) const;
    const odiK_Model& model() const;

    const odb_Assembly& constRootAssembly() const;

    void extendApi( odb_ApiTypeEnm type, odb_ApiExtension& extension );
    odb_ApiExtension getApiExtension( odb_ApiTypeEnm type ) const;
    void removeExtension( odb_ApiTypeEnm type );

    void release() const;
};

#endif // odb_Odb_h
