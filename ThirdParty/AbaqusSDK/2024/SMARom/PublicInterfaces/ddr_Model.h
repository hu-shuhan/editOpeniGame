//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef ddr_Model_h
#define ddr_Model_h

// Begin local includes
#include <cow_String.h>
#include <cow_ArrayCOW.h>
#include <typ_typeTag.h>
#include <cls_Uid.h>
#include <cls_IntCOW.h>
#include <ddr_ExtensionTable.h>
#include <mdl_ExtensionSubject.h>
#include <mdl_Repository.h>
#include <nex_dynamicCast.h>
#include <SMABasStringMode.h>
// End local includes

// Forward declarations
class cls_ClassRegistrar;
class cls_ReadVisitor;
class cls_WriteVisitor;
class ddr_Model;

MDL_EXTENSIONSUBJECT_DECL(ddr_Model, ddr_ExtensionTable, ddr_ModelExtension);

class ddr_Model : public ddr_ModelExtension
{
public:
    ddr_Model();
    ddr_Model( const cow_String  &fileName,
               const cow_String  &modelName,
               STRMODE_DECL);
    virtual ~ddr_Model();

    virtual ddr_Model* Copy() const = 0;
     
    virtual typ_typeTag DynTypeId() const;
    static typ_typeTag TypeId();

    ddr_Model(const cls_ReadVisitor& rv);
    static void* Ctor(cls_ReadVisitor& rv);
    static void CowDtor(cls_IntCOW* cow);
    virtual void DBWrite(const cls_WriteVisitor& wv) const;
    static void InitializeMetadata(cls_ClassRegistrar& reg);

    cow_String  FileName(STRMODE_DECL) const;
    cow_String  ModelName() const;

    static omn_FixedString materialCmd();
    static omn_FixedString sectionCmd();
    static omn_FixedString profileCmd();
    static omn_FixedString fieldCmd();
    static omn_FixedString fieldOutputRequestCmd();
    static omn_FixedString historyOutputRequestCmd();
    static omn_FixedString interactionCmd();
    static omn_FixedString interactionPropertyCmd();
    static omn_FixedString stepCmd();
    static omn_FixedString adaptiveMeshControlCmd();
    static omn_FixedString timePointCmd();
    static omn_FixedString adaptiveMeshConstraintCmd();
    static omn_FixedString integratedOutputSectionCmd();
    static omn_FixedString amplitudeCmd();
    static omn_FixedString filterCmd();
    static omn_FixedString contactControlCmd();
    static omn_FixedString contactInitializationCmd();
    static omn_FixedString contactStabilizationCmd();
    static omn_FixedString bcCmd();
    static omn_FixedString loadCmd();
    static omn_FixedString constraintCmd();
    static omn_FixedString connectorCmd();
    static omn_FixedString taskCmd();
    static omn_FixedString tableCollCmd();
    static omn_FixedString paramTableCmd();
    static omn_FixedString paramColCmd();
    static omn_FixedString compTableCmd();
    static omn_FixedString dataTableCmd();
    static omn_FixedString eventSeriesTypeCmd();
    static omn_FixedString eventSeriesCmd();

    static void RegisterMaterialContainer(const mdl_Extension& materials);
    static void UnRegisterMaterialContainer();
    static void RegisterSectionContainer( const mdl_Extension& sections);
    static void UnRegisterSectionContainer();
    static void RegisterBeamProfileContainer( const mdl_Extension& profiles);
    static void UnRegisterBeamProfileContainer();
    static void RegisterFieldContainer( const mdl_Extension& fields);
    static void UnRegisterFieldContainer();
    static void RegisterFieldOutputContainer( const mdl_Extension& foutput);
    static void UnRegisterFieldOutputContainer();
    static void RegisterHistoryOutputContainer( const mdl_Extension& houtput);
    static void UnRegisterHistoryOutputContainer();
    static void RegisterInteractionContainer( const mdl_Extension& interactions);
    static void UnRegisterInteractionContainer();
    static void RegisterInteractionPropertyContainer( const mdl_Extension& interactionProps);
    static void UnRegisterInteractionPropertyContainer();
    static void RegisterStepContainer( const mdl_Extension& steps);
    static void UnRegisterStepContainer();
    static void RegisterAdaptiveMeshControlContainer( const mdl_Extension& amcs);
    static void UnRegisterAdaptiveMeshControlContainer();
    static void RegisterTimePointContainer( const mdl_Extension& amcs);
    static void UnRegisterTimePointContainer();
    static void RegisterAdaptiveMeshConstraintContainer( const mdl_Extension& amcs);
    static void UnRegisterAdaptiveMeshConstraintContainer();
    static void RegisterIntegratedOutputSectionContainer( const mdl_Extension& amcs);
    static void UnRegisterIntegratedOutputSectionContainer();
    static void RegisterAmplitudeContainer( const mdl_Extension& amplitudes);
    static void UnRegisterAmplitudeContainer();
    static void RegisterFilterContainer( const mdl_Extension& filters);
    static void UnRegisterFilterContainer();
    static void RegisterContactControlContainer( const mdl_Extension& contactControls);
    static void UnRegisterContactControlContainer();
    static void RegisterContactInitContainer( const mdl_Extension& initializations);
    static void UnRegisterContactInitContainer();
    static void RegisterContactStabContainer( const mdl_Extension& stabilizations);
    static void UnRegisterContactStabContainer();
    static void RegisterBCContainer( const mdl_Extension& bcs);
    static void UnRegisterBCContainer();
    static void RegisterLoadContainer( const mdl_Extension& loads);
    static void UnRegisterLoadContainer();
    static void RegisterConstraintContainer( const mdl_Extension& constraints);
    static void UnRegisterConstraintContainer();
    static void RegisterConnectorContainer( const mdl_Extension& connectors);
    static void UnRegisterConnectorContainer();
    static void RegisterTaskContainer( const mdl_Extension& eos);
    static void UnRegisterTaskContainer();
    static void RegisterTableCollContainer( const mdl_Extension& tableCollections);
    static void UnRegisterTableCollContainer();
    static void RegisterParamTableContainer( const mdl_Extension& paramTables);
    static void UnRegisterParamTableContainer();
    static void RegisterParamColContainer( const mdl_Extension& paramCols);
    static void UnRegisterParamColContainer();
    static void RegisterCompoTableContainer( const mdl_Extension& compTables);
    static void UnRegisterCompoTableContainer();
    static void RegisterDataTableContainer( const mdl_Extension& dataTables);
    static void UnRegisterDataTableContainer();
    static void RegisterEventSeriesTypeCont( const mdl_Extension& eventSeriesTypes);
    static void UnRegisterEventSeriesTypeCont();
    static void RegisterEventSeriesCont( const mdl_Extension& eventSeriesDatas);
    static void UnRegisterEventSeriesCont();

    mdl_Repository& GetMaterials();
    const mdl_Repository& ConstGetMaterials() const;
    void SetMaterials(const mdl_Repository&);
    int NumMaterials() const;

    mdl_Repository& GetSections();
    const mdl_Repository& ConstGetSections() const;
    void SetSections( const mdl_Repository& );
    int NumSections() const;

    mdl_Repository& GetBeamProfiles();
    const mdl_Repository& ConstGetBeamProfiles() const;
    void SetBeamProfiles( const mdl_Repository& );
    int NumBeamProfiles() const;

    mdl_Repository& GetFields();
    const mdl_Repository& ConstGetFields() const;
    void SetFields( const mdl_Repository& );
    int NumFields() const;

    mdl_Repository& GetFieldOutputs();
    const mdl_Repository& ConstGetFieldOutputs() const;
    void SetFieldOutputs( const mdl_Repository& );
    int NumFieldOutputs() const;

    mdl_Repository& GetHistoryOutputs();
    const mdl_Repository& ConstGetHistoryOutputs() const;
    void SetHistoryOutputs( const mdl_Repository& );
    int NumHistoryOutputs() const;

    mdl_Repository& GetInteractions();
    const mdl_Repository& ConstGetInteractions() const;
    void SetInteractions( const mdl_Repository& );
    int NumInteractions() const;

    mdl_Repository& GetInteractionProperties();
    const mdl_Repository& ConstGetInteractionProperties() const;
    void SetInteractionProperties( const mdl_Repository& );
    int NumInteractionProperties() const;

    mdl_Repository& GetSteps();
    const mdl_Repository& ConstGetSteps() const;
    void SetSteps( const mdl_Repository& );
    int NumSteps() const;

    mdl_Repository& GetAdaptiveMeshControls();
    const mdl_Repository& ConstGetAdaptiveMeshControls() const;
    void SetAdaptiveMeshControls( const mdl_Repository& );
    int NumAdaptiveMeshControls() const;

    mdl_Repository& GetTimePoints();
    const mdl_Repository& ConstGetTimePoints() const;
    void SetTimePoints( const mdl_Repository& );
    int NumTimePoints() const;

    mdl_Repository& GetAdaptiveMeshConstraints();
    const mdl_Repository& ConstGetAdaptiveMeshConstraints() const;
    void SetAdaptiveMeshConstraints( const mdl_Repository& );
    int NumAdaptiveMeshConstraints() const;

    mdl_Repository& GetIntegratedOutputSections();
    const mdl_Repository& ConstGetIntegratedOutputSections() const;
    void SetIntegratedOutputSections( const mdl_Repository& );
    int NumIntegratedOutputSections() const;

    mdl_Repository& GetAmplitudes();
    const mdl_Repository& ConstGetAmplitudes() const;
    void SetAmplitudes( const mdl_Repository& );
    int NumAmplitudes() const;

    mdl_Repository& GetFilters();
    const mdl_Repository& ConstGetFilters() const;
    void SetFilters( const mdl_Repository& );
    int NumFilters() const;

    mdl_Repository& GetContactControls();
    const mdl_Repository& ConstGetContactControls() const;
    void SetContactControls( const mdl_Repository& );
    int NumContactControls() const;

    mdl_Repository& GetContactInitializations();
    const mdl_Repository& ConstGetContactInitializations() const;
    void SetContactInitializations( const mdl_Repository& );
    int NumContactInitializations() const;

    mdl_Repository& GetContactStabilizations();
    const mdl_Repository& ConstGetContactStabilizations() const;
    void SetContactStabilizations( const mdl_Repository& );
    int NumContactStabilizations() const;

    mdl_Repository& GetBCs();
    const mdl_Repository& ConstGetBCs() const;
    void SetBCs( const mdl_Repository& );
    int NumBCs() const;

    mdl_Repository& GetLoads();
    const mdl_Repository& ConstGetLoads() const;
    void SetLoads( const mdl_Repository& );
    int NumLoads() const;

    mdl_Repository& GetConstraints();
    const mdl_Repository& ConstGetConstraints() const;
    void SetConstraints( const mdl_Repository& );
    int NumConstraints() const;

    mdl_Repository& GetConnectors();
    const mdl_Repository& ConstGetConnectors() const;
    void SetConnectors( const mdl_Repository& );
    int NumConnectors() const;

    mdl_Repository& GetTasks();
    const mdl_Repository& ConstGetTasks() const;
    void SetTasks( const mdl_Repository& );
    int NumTasks() const;

    mdl_Repository& GetTableCollections();
    const mdl_Repository& ConstGetTableCollections() const;
    void SetTableCollections( const mdl_Repository& );
    int NumTableCollections() const;

    mdl_Repository& GetParamTables();
    const mdl_Repository& ConstGetParamTables() const;
    void SetParamTables( const mdl_Repository& );
    int NumParamTables() const;

    mdl_Repository& GetParamCols();
    const mdl_Repository& ConstGetParamCols() const;
    void SetParamCols( const mdl_Repository& );
    int NumParamCols() const;

    mdl_Repository& GetCompTables();
    const mdl_Repository& ConstGetCompTables() const;
    void SetCompTables( const mdl_Repository& );
    int NumCompTables() const;

    mdl_Repository& GetDataCols();
    const mdl_Repository& ConstGetDataCols() const;
    void SetDataCols( const mdl_Repository& );
    int NumDataCols() const;

    mdl_Repository& GetEventSeriesTypes();
    const mdl_Repository& ConstGetEventSeriesTypes() const;
    void SetEventSeriesTypes( const mdl_Repository& );
    int NumEventSeriesTypes() const;

    mdl_Repository& GetEventSeriesDatas();
    const mdl_Repository& ConstGetEventSeriesDatas() const;
    void SetEventSeriesDatas( const mdl_Repository& );
    int NumEventSeriesDatas() const;
    
    ddr_ModelExtension::RegisterExtension;
    ddr_ModelExtension::Initialize;
    ddr_ModelExtension::Finalize;

protected:
    // Only allow ddr_Ddb to change the names.
    friend class  ddr_Ddb;
    cow_String  fileName, modelName;

private:
    cls_Uid m_ClsUid;
};

COW_ARRAYCOW2_DECL(ddr_Model, cow_Virtual);

#endif


