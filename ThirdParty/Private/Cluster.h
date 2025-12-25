// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StaticMeshResources.h"
#include "Rendering/NaniteResources.h"
#include "Math/Bounds.h"
#include "MeshSimplify.h"
#include "TriangleUtil.h"
#include "NaniteRayTracingScene.h"

#define RAY_TRACE_VOXELS	1

class FGraphPartitioner;

namespace Nanite
{

class FClusterDAG;
class FRayTracingScene;
struct FRayTracingFallbackBuildSettings;

struct FVertexFormat
{
	uint8	NumTexCoords		= 0;
	uint8	NumBoneInfluences	= 0;
	bool	bHasTangents		: 1 = false;
	bool	bHasColors			: 1 = false;

	FORCEINLINE bool Matches( const FVertexFormat& Other )
	{
		return
			NumTexCoords		== Other.NumTexCoords &&
			NumBoneInfluences	== Other.NumBoneInfluences &&
			bHasTangents		== Other.bHasTangents &&
			bHasColors			== Other.bHasColors;
	}

	FORCEINLINE uint32 GetVertSize() const
	{
		return 6 + ( bHasTangents ? 4 : 0 ) + ( bHasColors ? 4 : 0 ) + (NumTexCoords * 2) + (NumBoneInfluences * 2);
	}

	FORCEINLINE uint32 GetColorOffset() const
	{
		return 6 + ( bHasTangents ? 4 : 0 );
	}

	FORCEINLINE uint32 GetUVOffset() const
	{
		return 6 + ( bHasTangents ? 4 : 0 ) + ( bHasColors ? 4 : 0 );
	}

	FORCEINLINE uint32 GetBoneInfluenceOffset() const
	{
		return 6 + (bHasTangents ? 4 : 0) + (bHasColors ? 4 : 0) + (NumTexCoords * 2);
	}
};

struct FMaterialRange
{
	uint32 RangeStart;
	uint32 RangeLength;
	uint32 MaterialIndex;
	TArray<uint8, TInlineAllocator<12>> BatchTriCounts;

	friend FArchive& operator<<(FArchive& Ar, FMaterialRange& Range);
};

struct FStripDesc
{
	uint32 Bitmasks[4][3];
	uint32 NumPrevRefVerticesBeforeDwords;
	uint32 NumPrevNewVerticesBeforeDwords;

	friend FArchive& operator<<(FArchive& Ar, FStripDesc& Desc);
};


class FCluster
{
public:
	FCluster() {}
	FCluster(
		const FConstMeshBuildVertexView& InVerts,
		TArrayView< const uint32 > InIndexes,
		TArrayView< const int32 > InMaterialIndexes,
		const FVertexFormat& InFormat,
		uint32 Begin, uint32 End,
		TArrayView< const uint32 > SortedIndexes,
		TArrayView< const uint32 > SortedTo,
		const FAdjacency& Adjacency );

	FCluster(
		FCluster& SrcCluster,
		uint32 Begin, uint32 End,
		TArrayView< const uint32 > SortedIndexes,
		TArrayView< const uint32 > SortedTo,
		const FAdjacency& Adjacency );

	FCluster( const FClusterDAG& DAG, TArrayView< const uint32 > Children );

	float		Simplify( const FClusterDAG& DAG, uint32 TargetNumTris, float TargetError = 0.0f, uint32 LimitNumTris = 0, const FRayTracingFallbackBuildSettings* RayTracingFallbackBuildSettings = nullptr );
	FAdjacency	BuildAdjacency() const;
	void		Split( FGraphPartitioner& Partitioner, const FAdjacency& Adjacency ) const;
	void		Bound();
	void		Voxelize( FClusterDAG& DAG, FRayTracingScene& RayTracingScene, TArrayView< const uint32 > Children, float VoxelSize );
	void		BuildMaterialRanges();

private:
	uint32		FindVert( uint32 Hash, const float* Vert, FHashTable& HashTable );
	uint32		AddVert( const float* Vert, FHashTable& HashTable );

				template<typename TTransformPos, typename TTransformNormal, typename TTransformTangent>
	uint32		AddVertMismatched( const FCluster& Other, uint32 VertIndex, FHashTable& HashTable, TTransformPos&& TransformPos, TTransformNormal&& TransformNormal, TTransformTangent&& TransformTangent );
	uint32		AddVertMismatched( const FCluster& Other, uint32 VertIndex, FHashTable& HashTable );

	uint32 TransformAndAddVert(
		const FCluster& Other,
		uint32 VertIndex,
		const FMatrix44f& Transform,
		const FMatrix44f& NormalTransform,
		FHashTable& HashTable
	);

	void		LerpAttributes( uint32 VertIndex, uint32 TriIndex, const FCluster& SrcCluster, const FVector3f& Barycentrics );
	void		VoxelsToBricks( TMap< FIntVector3, uint32 >& VoxelMap );

public:
	uint32				GetVertSize() const;
	FVector3f&			GetPosition( uint32 VertIndex );
	float*				GetAttributes( uint32 VertIndex );
	FVector3f&			GetNormal( uint32 VertIndex );
	FVector3f&			GetTangentX( uint32 VertIndex );
	float&				GetTangentYSign( uint32 VertIndex );
	FLinearColor&		GetColor( uint32 VertIndex );
	FVector2f*			GetUVs( uint32 VertIndex );
	FVector2f*			GetBoneInfluences( uint32 VertIndex );

	const FVector3f&	GetPosition( uint32 VertIndex ) const;
	const float*		GetAttributes( uint32 VertIndex ) const;
	const FVector3f&	GetNormal( uint32 VertIndex ) const;
	const FVector3f&	GetTangentX( uint32 VertIndex ) const;
	const float&		GetTangentYSign( uint32 VertIndex ) const;
	const FLinearColor&	GetColor( uint32 VertIndex ) const;
	const FVector2f*	GetUVs( uint32 VertIndex ) const;
	const FVector2f*	GetBoneInfluences( uint32 VertIndex ) const;

	void				SanitizeVertexData();

	friend FArchive& operator<<(FArchive& Ar, FCluster& Cluster);

	static const uint32	ClusterSize = 128;
	
	FVertexFormat		VertexFormat;

	uint32				NumVerts = 0;
	uint32				NumTris = 0;

	TArray< float >		Verts;
	TArray< uint32 >	Indexes;
	TArray< int32 >		MaterialIndexes;

#if RAY_TRACE_VOXELS
	TArray< FVector3f >	ExtraVoxels;
#endif

	TArray< int8 >		ExternalEdges;
	uint32				NumExternalEdges = 0;

	TArray< uint32 >	ExtendedData;

	struct FBrick
	{
		uint64		VoxelMask;
		FIntVector3	Position;
		uint32		VertOffset;
	};
	TArray< FBrick >	Bricks;

	TMap< uint32, uint32 >	AdjacentClusters;

	FBounds3f	Bounds;
	uint64		GUID = 0;
	uint32		MipLevel = 0;

	FIntVector	QuantizedPosStart		= { 0u, 0u, 0u };
	int32		QuantizedPosPrecision	= 0u;
	FIntVector  QuantizedPosBits		= { 0u, 0u, 0u };

	float		EdgeLength = 0.0f;
	float		LODError = 0.0f;
	float		SurfaceArea = 0.0f;
	
	FSphere3f	SphereBounds;
	FSphere3f	LODBounds;

	uint32		GroupIndex			= MAX_uint32;
	uint32		GroupPartIndex		= MAX_uint32;
	uint32		GeneratingGroupIndex= MAX_uint32;

	TArray<FMaterialRange, TInlineAllocator<4>> MaterialRanges;
	TArray<FIntVector>	QuantizedPositions;

	FStripDesc		StripDesc;
	TArray<uint8>	StripIndexData;
};

FORCEINLINE uint32 FCluster::GetVertSize() const
{
	return VertexFormat.GetVertSize();
}

FORCEINLINE FVector3f& FCluster::GetPosition( uint32 VertIndex )
{
	return *reinterpret_cast< FVector3f* >( &Verts[ VertIndex * GetVertSize() ] );
}

FORCEINLINE const FVector3f& FCluster::GetPosition( uint32 VertIndex ) const
{
	return *reinterpret_cast< const FVector3f* >( &Verts[ VertIndex * GetVertSize() ] );
}

FORCEINLINE float* FCluster::GetAttributes( uint32 VertIndex )
{
	return &Verts[ VertIndex * GetVertSize() + 3 ];
}

FORCEINLINE const float* FCluster::GetAttributes( uint32 VertIndex ) const
{
	return &Verts[ VertIndex * GetVertSize() + 3 ];
}

FORCEINLINE FVector3f& FCluster::GetNormal( uint32 VertIndex )
{
	return *reinterpret_cast< FVector3f* >( &Verts[ VertIndex * GetVertSize() + 3 ] );
}

FORCEINLINE const FVector3f& FCluster::GetNormal( uint32 VertIndex ) const
{
	return *reinterpret_cast< const FVector3f* >( &Verts[ VertIndex * GetVertSize() + 3 ] );
}

FORCEINLINE FVector3f& FCluster::GetTangentX( uint32 VertIndex )
{
	return *reinterpret_cast< FVector3f* >( &Verts[ VertIndex * GetVertSize() + 6 ] );
}

FORCEINLINE const FVector3f& FCluster::GetTangentX( uint32 VertIndex ) const
{
	return *reinterpret_cast< const FVector3f* >( &Verts[ VertIndex * GetVertSize() + 6 ] );
}

FORCEINLINE float& FCluster::GetTangentYSign( uint32 VertIndex )
{
	return *reinterpret_cast< float* >( &Verts[ VertIndex * GetVertSize() + 9 ] );
}

FORCEINLINE const float& FCluster::GetTangentYSign( uint32 VertIndex ) const
{
	return *reinterpret_cast< const float* >( &Verts[ VertIndex * GetVertSize() + 9 ] );
}

FORCEINLINE FLinearColor& FCluster::GetColor( uint32 VertIndex )
{
	return *reinterpret_cast< FLinearColor* >( &Verts[ VertIndex * GetVertSize() + VertexFormat.GetColorOffset() ] );
}

FORCEINLINE const FLinearColor& FCluster::GetColor( uint32 VertIndex ) const
{
	return *reinterpret_cast< const FLinearColor* >( &Verts[ VertIndex * GetVertSize() + VertexFormat.GetColorOffset() ] );
}

FORCEINLINE FVector2f* FCluster::GetUVs( uint32 VertIndex )
{
	return reinterpret_cast< FVector2f* >( &Verts[ VertIndex * GetVertSize() + VertexFormat.GetUVOffset() ] );
}

FORCEINLINE const FVector2f* FCluster::GetUVs( uint32 VertIndex ) const
{
	return reinterpret_cast< const FVector2f* >( &Verts[ VertIndex * GetVertSize() + VertexFormat.GetUVOffset() ] );
}

FORCEINLINE FVector2f* FCluster::GetBoneInfluences( uint32 VertIndex )
{
	return reinterpret_cast< FVector2f* >( &Verts[ VertIndex * GetVertSize() + VertexFormat.GetBoneInfluenceOffset() ] );
}

FORCEINLINE const FVector2f* FCluster::GetBoneInfluences( uint32 VertIndex ) const
{
	return reinterpret_cast< const FVector2f* >( &Verts[ VertIndex * GetVertSize() + VertexFormat.GetBoneInfluenceOffset() ] );
}

} // namespace Nanite