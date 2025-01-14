#ifndef iGameVTKWriter_h
#define iGameVTKWriter_h


/*
Temporarily, this is done serially by putting it into a fixed buffer.
In the future, this may be parallelized, and the function will return
an array of buffers. However, the order of different blocks must be maintained
before merging the buffers into m_Buffers.
*/
/*Now we have used parallelism, but only for file processing. 
File output and file processing are serial.
*/

#include "iGameFileWriter.h"
#include "iGameVTKAbstractReader.h"

IGAME_NAMESPACE_BEGIN

class VTKWriter : public FileWriter {
public:
	I_OBJECT(VTKWriter);

	// Factory method to create a new instance of VTKWriter.
	static Pointer New() { return new VTKWriter; }

	// Override to generate the necessary buffers for VTK file output.
	bool GenerateBuffers() override;


	//Write the VTK header information to the buffer.
	const void WriteHeaderToBuffer();

	// Write the point data to the buffer for output.
	const void WritePointsToBuffer(Points::Pointer );

	// Write the cell (element connectivity) data to the buffer.
	const void WriteCellsToBuffer(CellArray::Pointer Cells);

	// Write the cell types to the buffer (indicating the types of cells, e.g., tetrahedrons, hexahedrons).
	const void WriteCellsTypeToBuffer( );

	// Write the point attributes (e.g., scalars, vectors) to the buffer.
	const void WritePointsAttributesToBuffer(AttributeSet::Pointer );

	// Write the cell attributes (e.g., scalars, vectors) to the buffer.
	const void WriteCellsAttributesToBuffer(AttributeSet::Pointer  );

	// Write the attributes (e.g., scalars, vectors) to the buffer.
	const void WriteAttributesToBuffer(ElementArray<AttributeSet::Attribute>::Pointer);

	// Write an array of data (e.g., point data, cell data) to the buffer.
	const void WriteArrayToBuffer(  ArrayObject::Pointer data);

	// Write the dimension size of the structured mesh to the buffer.
	const void WriteDimensionSizeToBuffer( );

	// Write data in an unstructured mesh format to the file.
	const void WriteWithUnstructuredMeshType();

	// Write data in a structured mesh format to the file.
	const void WriteWithStructuredMeshType();

	// Write data in a surface mesh format to the file.
	const void WriteWithSurfaceMeshType();

	//Generate attribute header to write
	std::string GenerateAttributeHeader(AttributeSet::Attribute attribute);


protected:
	// Default constructor for VTKWriter.
	VTKWriter() = default;

	// Destructor for VTKWriter.
	~VTKWriter() override = default;

	// Pointer to a surface mesh object to be written.
	SurfaceMesh::Pointer m_SurfaceMesh{ nullptr };

	// Pointer to an unstructured mesh object to be written.
	UnstructuredMesh::Pointer m_UnstructuredMesh{ nullptr };

	// Pointer to a structured mesh object to be written.
	StructuredMesh::Pointer m_StructuredMesh{ nullptr };
};


IGAME_NAMESPACE_END
#endif