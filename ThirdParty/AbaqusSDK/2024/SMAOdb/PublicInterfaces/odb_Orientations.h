//=============================================================================
// COPYRIGHT DASSAULT SYSTEMES 2001-2012
/**
* @CAA2Level L0
* @CAA2Usage U0
*/
//=============================================================================
#ifndef odb_Orientations_h
#define odb_Orientations_h

///////////////////////////////////////////////////////////////////////////////
//
// This file combines various Orientation objects and their sequences:
//
// * odb_MaterialOrientation, odb_SequenceMaterialOrientation
// * odb_RebarOrientation, odb_SequenceRebarOrientation
// * odb_BeamOrientation, odb_SequenceBeamOrientation
// * odb_ConnectorOrientation, odb_SequenceConnectorOrientation
//

// Begin local includes
#include <mem_AllocationOperators.h>
#include <odb_Types.h>
#include <odb_Enum.h>
#include <odb_MeshList.h>
#include <cow_COW.h>
#include <ptoC_PartAttributeShortcut.h>
#include <ptoC_PartAttributeListShortcut.h>
// End local includes

// Forward declarations
class odb_DatumCsys;
class odb_Set;
class ddb_MaterialOrientation;
class ptoC_ConnectorOrientation;
COW_COW_DECL(ddb_MaterialOrientation);
class ddb_MaterialOrientationContainer;
COW_COW_DECL(ddb_MaterialOrientationContainer);
typedef ddb_MaterialOrientation ddb_RebarOrientation;
COW_COW_DECL(ddb_RebarOrientation);
typedef ddb_MaterialOrientationContainer ddb_RebarOrientationContainer;
COW_COW_DECL(ddb_RebarOrientationContainer);
class ddb_BeamOrientation;
COW_COW_DECL(ddb_BeamOrientation);
class ddb_BeamOrientationContainer;
COW_COW_DECL(ddb_BeamOrientationContainer);
class odiK_Model;
template <class T>
class ddr_Shortcut;
typedef ddr_Shortcut<odiK_Model> odiK_ModelShortcut;

// Class definition

///////////////////////////////////////////////////////////////////////////////
//
// Material Orientation
//
//// object

class odb_MaterialOrientation : public mem_AllocationOperators
{
public:
    odb_Set                region() const;
    odb_DatumCsys          localCsys() const; 
    odb_Enum::odb_AxisEnum axis()   const;
    float                  angle()  const;
    odb_Enum::odb_StackDirectionEnum stackDirection() const;

private:
    ddb_MaterialOrientationCOW m_object;

public: // Undocumented - internal stuff
    odb_DatumCsys          csys()   const; // DEPRECATED AS PUBLIC CALL
    odb_MaterialOrientation(); // required for sequence
    odb_MaterialOrientation(const ddb_MaterialOrientation & orient);
};

//// sequence

class odb_SequenceMaterialOrientation : public mem_AllocationOperators
{
public:
    int size() const;
    odb_MaterialOrientation constGet(int index) const;
    odb_MaterialOrientation operator[](int index) const;
    
private:
    ddb_MaterialOrientationContainerCOW m_container;

public: // Undocumented - internal stuff
    odb_SequenceMaterialOrientation(const ddb_MaterialOrientationContainer & orc);
};

///////////////////////////////////////////////////////////////////////////////
//
// Rebar Orientation
//
//// object

class odb_RebarOrientation : public mem_AllocationOperators
{
public:
    odb_Set                region() const;
    odb_DatumCsys          csys()   const;
    odb_Enum::odb_AxisEnum axis()   const;
    float                  angle()  const;
    
private:
    ddb_RebarOrientationCOW m_object;

public: // Undocumented - internal stuff
    odb_RebarOrientation(); // required for sequence
    odb_RebarOrientation(const ddb_RebarOrientation & orient);
};

//// sequence

class odb_SequenceRebarOrientation : public mem_AllocationOperators
{
public:
    int size() const;
    odb_RebarOrientation constGet(int index) const;
    odb_RebarOrientation operator[](int index) const;
    
private:
    ddb_RebarOrientationContainerCOW m_container;

public: // Undocumented - internal stuff
    odb_SequenceRebarOrientation(const ddb_RebarOrientationContainer & orc);
};

///////////////////////////////////////////////////////////////////////////////
//
// Beam Orientation
//
//// object

class odb_BeamOrientation : public mem_AllocationOperators
{
public:
    odb_Set                             region() const;
    odb_Enum::odb_OrientationMethodEnum method() const;
    odb_SequenceFloat                   vector() const;
    
private:
    ddb_BeamOrientationCOW m_object;

public: // Undocumented - internal stuff
    odb_BeamOrientation(); // required for sequence
    odb_BeamOrientation(const ddb_BeamOrientation & orient);
};

//// sequence

class odb_SequenceBeamOrientation : public mem_AllocationOperators
{
public:
    int size() const;
    odb_BeamOrientation constGet(int index) const;
    odb_BeamOrientation operator[](int index) const;
    
private:
    ddb_BeamOrientationContainerCOW m_container;

public: // Undocumented - internal stuff
    odb_SequenceBeamOrientation(const ddb_BeamOrientationContainer & orc);
};

///////////////////////////////////////////////////////////////////////////////
//
// Material Orientation
//
//// object

class odb_ConnectorOrientation : public mem_AllocationOperators
{
  public:

    odb_Set                     region() const;
    bool                      hasCsys1() const;
    odb_DatumCsys                csys1() const;
    bool                      hasCsys2() const;
    odb_DatumCsys                csys2() const;
    odb_Enum::odb_AxisEnum       axis1() const;
    odb_Enum::odb_AxisEnum       axis2() const;
    float                       angle1() const;
    float                       angle2() const;
    bool                orient2sameAs1() const;
    
  private:

    const ptoC_ConnectorOrientation& ConstGetConnectorOrientation() const;

    ptoC_PartAttributeShortcut m_shortcut;
    odb_MeshList m_meshList;
    odiK_ModelShortcut m_modelSC;

  public: // Undocumented - internal stuff

    odb_ConnectorOrientation(); // required for sequence
    odb_ConnectorOrientation(const ptoC_PartAttributeShortcut&,
			     const odb_MeshList& meshList,
			     const odiK_ModelShortcut& modelSC);
};

//// sequence

class odb_SequenceConnectorOrientation : public mem_AllocationOperators
{
public:
    int size() const;
    odb_ConnectorOrientation constGet(int index) const;
    odb_ConnectorOrientation operator[](int index) const;
    
private:
    ptoC_PartAttributeListShortcut m_shortcut;
    odb_MeshList m_meshList;
    odiK_ModelShortcut m_modelSC;

    const ptoC_ConnectorOrientation& ConstGetConnectorOrientation() const;

public: // Undocumented - internal stuff
    odb_SequenceConnectorOrientation(const ptoC_PartAttributeListShortcut&,
				     const odb_MeshList& meshList,
				     const odiK_ModelShortcut& modelSC);
};

#endif // odb_Orientations_h
