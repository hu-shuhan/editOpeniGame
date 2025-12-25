// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Cluster.h"
#include "Containers/BinaryHeap.h"
#include "Containers/BitArray.h"
#include "Math/Bounds.h"

// Log CRCs to test for deterministic building
#if 0
	#define LOG_CRC( Array ) UE_LOG( LogStaticMesh, Log, TEXT(#Array " CRC %u"), FCrc::MemCrc32( Array.GetData(), Array.Num() * Array.GetTypeSize() ) )
#else
	#define LOG_CRC( Array )
#endif

namespace Nanite
{

struct FClusterGroup
{
	FSphere3f			Bounds;
	FSphere3f			LODBounds;
	float				MinLODError			= 0.0f;
	float				MaxParentLODError	= 0.0f;
	int32				MipLevel			= 0;
	uint32				MeshIndex			= MAX_uint32;
	uint32				AssemblyPartIndex	= MAX_uint32;
	bool				bTrimmed			= false;
	
	uint32				PageIndexStart		= MAX_uint32;
	uint32				PageIndexNum		= 0;
	TArray< uint32 >	Children;
};

struct FAssemblyPartData
{
	uint32 FirstTransform = MAX_uint32;
	uint32 NumTransforms = 0;
};

class FClusterDAG
{
public:
				FClusterDAG() {}
	
	void		AddMesh(
		const FConstMeshBuildVertexView& Verts,
		TArrayView< const uint32 > Indexes,
		TArrayView< const int32 > MaterialIndexes,
		const FBounds3f& VertexBounds,
		const FVertexFormat& VertexFormat );

	void		ReduceMesh( uint32 ClusterRangeStart, uint32 ClusterRangeNum, uint32 MeshIndex );

	FBinaryHeap< float >	FindCut(
		uint32 TargetNumTris,
		float  TargetError,
		uint32 TargetOvershoot,
		TBitArray<>* SelectedGroupsMask ) const;

	TArray< FCluster >		Clusters;
	TArray< FClusterGroup >	Groups;

	TArray< FMatrix44f >		AssemblyTransforms; // flat list of all assembly part transforms
	TArray< FAssemblyPartData >	AssemblyPartData;

	FBounds3f	TotalBounds;

	struct FSettings
	{
		uint32	NumRays				= 1;
		uint32	VoxelLevel			= 0;
		float	RayBackUp			= 0.0f;
		float	MaxEdgeLengthFactor	= 0.0f;
		bool	bPreserveArea		: 1 = false;
		bool	bLerpUVs			: 1 = true;
		bool	bSeparable			: 1 = false;
		bool	bVoxelNDF			: 1 = true;
		bool	bVoxelOpacity		: 1 = false;
	};
	FSettings	Settings;

	bool bHasSkinning		: 1		= false;
	bool bHasTangents		: 1		= false;
	bool bHasColors			: 1		= false;

private:
	void		ReduceGroup(
		FRayTracingScene* RayTracingScene,
		TAtomic< uint32 >& NumClusters,
		TArrayView< uint32 > Children,
		uint32 MaxClusterSize,
		uint32 NumParents,
		int32 GroupIndex,
		uint32 MeshIndex );
};

} // namespace Nanite