// Copyright Epic Games, Inc. All Rights Reserved.

#include "Cluster.h"
#include "GraphPartitioner.h"
#include "Rasterizer.h"
#include "VectorUtil.h"
#include "MatrixUtil.h"
#include "ClusterDAG.h"
#include "NaniteBuilder.h"

namespace Nanite
{

template< bool bHasTangents, bool bHasColors >
void CorrectAttributes( float* Attributes )
{
	float* AttributesPtr = Attributes;

	FVector3f& Normal = *reinterpret_cast< FVector3f* >( AttributesPtr );
	Normal.Normalize();
	AttributesPtr += 3;

	if( bHasTangents )
	{
		FVector3f& TangentX = *reinterpret_cast< FVector3f* >( AttributesPtr );
		AttributesPtr += 3;
	
		TangentX -= ( TangentX | Normal ) * Normal;
		TangentX.Normalize();

		float& TangentYSign = *AttributesPtr++;
		TangentYSign = TangentYSign < 0.0f ? -1.0f : 1.0f;
	}

	if( bHasColors )
	{
		FLinearColor& Color = *reinterpret_cast< FLinearColor* >( AttributesPtr );
		AttributesPtr += 3;

		Color = Color.GetClamped();
	}
}

typedef void (CorrectAttributesFunction)( float* Attributes );

static CorrectAttributesFunction* CorrectAttributesFunctions[ 2 ][ 2 ] =	// [ bHasTangents ][ bHasColors ]
{
	{	CorrectAttributes<false, false>,	CorrectAttributes<false, true>	},
	{	CorrectAttributes<true, false>,		CorrectAttributes<true, true>	}
};

FCluster::FCluster(
	const FConstMeshBuildVertexView& InVerts,
	TArrayView< const uint32 > InIndexes,
	TArrayView< const int32 > InMaterialIndexes,
	const FVertexFormat& InFormat,
	uint32 Begin, uint32 End,
	TArrayView< const uint32 > SortedIndexes,
	TArrayView< const uint32 > SortedTo,
	const FAdjacency& Adjacency )
	: VertexFormat( InFormat )
{
	const uint32 VertSize = GetVertSize();

	GUID = (uint64(Begin) << 32) | End;
	
	NumTris = End - Begin;

	Verts.Reserve( NumTris * VertSize );
	Indexes.Reserve( 3 * NumTris );
	MaterialIndexes.Reserve( NumTris );
	ExternalEdges.Reserve( 3 * NumTris );
	NumExternalEdges = 0;

	check(InMaterialIndexes.Num() * 3 == InIndexes.Num());

	TMap< uint32, uint32 > OldToNewIndex;
	OldToNewIndex.Reserve( NumTris );

	for( uint32 i = Begin; i < End; i++ )
	{
		uint32 TriIndex = SortedIndexes[i];

		for( uint32 k = 0; k < 3; k++ )
		{
			uint32 OldIndex = InIndexes[ TriIndex * 3 + k ];
			uint32* NewIndexPtr = OldToNewIndex.Find( OldIndex );
			uint32 NewIndex = NewIndexPtr ? *NewIndexPtr : ~0u;

			if( NewIndex == ~0u )
			{
				Verts.AddUninitialized( VertSize );
				NewIndex = NumVerts++;
				OldToNewIndex.Add( OldIndex, NewIndex );

				GetPosition( NewIndex ) = InVerts.Position[OldIndex];
				GetNormal( NewIndex ) = InVerts.TangentZ[OldIndex];

				if( VertexFormat.bHasTangents )
				{
					const float TangentYSign = ((InVerts.TangentZ[OldIndex] ^ InVerts.TangentX[OldIndex]) | InVerts.TangentY[OldIndex]);
					GetTangentX( NewIndex ) = InVerts.TangentX[OldIndex];
					GetTangentYSign( NewIndex ) = TangentYSign < 0.0f ? -1.0f : 1.0f;
				}
	
				if( VertexFormat.bHasColors )
				{
					GetColor( NewIndex ) = InVerts.Color[OldIndex].ReinterpretAsLinear();
				}

				if( VertexFormat.NumTexCoords > 0 )
				{
					FVector2f* UVs = GetUVs( NewIndex );
					for( uint32 UVIndex = 0; UVIndex < VertexFormat.NumTexCoords; UVIndex++ )
					{
						UVs[UVIndex] = InVerts.UVs[UVIndex][OldIndex];
					}
				}

				if( VertexFormat.NumBoneInfluences > 0 )
				{
					FVector2f* BoneInfluences = GetBoneInfluences(NewIndex);
					for (uint32 Influence = 0; Influence < VertexFormat.NumBoneInfluences; Influence++)
					{
						BoneInfluences[Influence].X = InVerts.BoneIndices[Influence][OldIndex];
						BoneInfluences[Influence].Y = InVerts.BoneWeights[Influence][OldIndex];
					}
				}

				float* Attributes = GetAttributes( NewIndex );

				// Make sure this vertex is valid from the start
				CorrectAttributesFunctions[ VertexFormat.bHasTangents ][ VertexFormat.bHasColors ]( Attributes );
			}

			Indexes.Add( NewIndex );

			int32 EdgeIndex = TriIndex * 3 + k;
			int32 AdjCount = 0;
			
			Adjacency.ForAll( EdgeIndex,
				[ &AdjCount, Begin, End, &SortedTo ]( int32 EdgeIndex, int32 AdjIndex )
				{
					uint32 AdjTri = SortedTo[ AdjIndex / 3 ];
					if( AdjTri < Begin || AdjTri >= End )
						AdjCount++;
				} );

			ExternalEdges.Add( (int8)AdjCount );
			NumExternalEdges += AdjCount != 0 ? 1 : 0;
		}

		MaterialIndexes.Add( InMaterialIndexes[ TriIndex ] );
	}

	SanitizeVertexData();

	for( uint32 VertexIndex = 0; VertexIndex < NumVerts; VertexIndex++ )
	{
		float* Attributes = GetAttributes( VertexIndex );

		// Make sure this vertex is valid from the start
		CorrectAttributesFunctions[ VertexFormat.bHasTangents ][ VertexFormat.bHasColors ]( Attributes );
	}

	Bound();
}

// Split
FCluster::FCluster(
	FCluster& SrcCluster,
	uint32 Begin, uint32 End,
	TArrayView< const uint32 > SortedIndexes,
	TArrayView< const uint32 > SortedTo,
	const FAdjacency& Adjacency )
	: VertexFormat( SrcCluster.VertexFormat )
	, MipLevel( SrcCluster.MipLevel )
{
	const uint32 VertSize = GetVertSize();

	GUID = Murmur64( { SrcCluster.GUID, (uint64)Begin, (uint64)End } );

	uint32 NumElements = End - Begin;
	check( NumElements <= ClusterSize );
	
	if( SrcCluster.NumTris )
	{
		NumTris = NumElements;
	
		Verts.Reserve( NumElements * VertSize );
		Indexes.Reserve( 3 * NumElements );
		MaterialIndexes.Reserve( NumElements );
		ExternalEdges.Reserve( 3 * NumElements );
		NumExternalEdges = 0;
	
		TMap< uint32, uint32 > OldToNewIndex;
		OldToNewIndex.Reserve( NumTris );
	
		for( uint32 i = Begin; i < End; i++ )
		{
			uint32 TriIndex = SortedIndexes[i];
	
			for( uint32 k = 0; k < 3; k++ )
			{
				uint32 OldIndex = SrcCluster.Indexes[ TriIndex * 3 + k ];
				uint32* NewIndexPtr = OldToNewIndex.Find( OldIndex );
				uint32 NewIndex = NewIndexPtr ? *NewIndexPtr : ~0u;
	
				if( NewIndex == ~0u )
				{
					Verts.AddUninitialized( VertSize );
					NewIndex = NumVerts++;
					OldToNewIndex.Add( OldIndex, NewIndex );
	
					FMemory::Memcpy( &GetPosition( NewIndex ), &SrcCluster.GetPosition( OldIndex ), VertSize * sizeof( float ) );
				}
	
				Indexes.Add( NewIndex );
	
				int32 EdgeIndex = TriIndex * 3 + k;
				int32 AdjCount = SrcCluster.ExternalEdges[ EdgeIndex ];
				
				Adjacency.ForAll( EdgeIndex,
					[ &AdjCount, Begin, End, &SortedTo ]( int32 EdgeIndex, int32 AdjIndex )
					{
						uint32 AdjTri = SortedTo[ AdjIndex / 3 ];
						if( AdjTri < Begin || AdjTri >= End )
							AdjCount++;
					} );
	
				ExternalEdges.Add( (int8)AdjCount );
				NumExternalEdges += AdjCount != 0 ? 1 : 0;
			}
	
			MaterialIndexes.Add( SrcCluster.MaterialIndexes[ TriIndex ] );
		}
	}
	else
	{
		Verts.Reserve( NumElements * VertSize );
		MaterialIndexes.Reserve( NumElements );

		for( uint32 i = Begin; i < End; i++ )
		{
			uint32 BrickIndex = SortedIndexes[i];

			FBrick Brick = SrcCluster.Bricks[ BrickIndex ];
			uint32 NumVoxels = FMath::CountBits( Brick.VoxelMask );

			uint32 OldIndex = Brick.VertOffset;
			uint32 NewIndex = Brick.VertOffset = NumVerts;
			NumVerts += NumVoxels;

			Verts.AddUninitialized( NumVoxels * VertSize );

			FMemory::Memcpy( &GetPosition( NewIndex ), &SrcCluster.GetPosition( OldIndex ), NumVoxels * VertSize * sizeof( float ) );

			Bricks.Add( Brick );
			MaterialIndexes.Add( SrcCluster.MaterialIndexes[ BrickIndex ] );
		}
	}

	Bound();
	check( MaterialIndexes.Num() > 0 );
}

// Merge triangles
FCluster::FCluster( const FClusterDAG& DAG, TArrayView< const uint32 > Children )
	: VertexFormat( DAG.Clusters[ Children[0] ].VertexFormat )
{
	uint32 NumVertsGuess = 0;
	for( uint32 ClusterIndex : Children )
	{
		const FCluster&			Child = DAG.Clusters[ ClusterIndex ];
		const FClusterGroup&	Group = DAG.Groups[ Child.GroupIndex ];

		if( Child.NumTris == 0 )
			continue;

		const bool bIsAssemblyCluster = Group.AssemblyPartIndex != MAX_uint32;

		VertexFormat.NumTexCoords		= FMath::Max( VertexFormat.NumTexCoords,		Child.VertexFormat.NumTexCoords );
		VertexFormat.NumBoneInfluences	= FMath::Max( VertexFormat.NumBoneInfluences,	Child.VertexFormat.NumBoneInfluences );
		VertexFormat.bHasTangents		|= Child.VertexFormat.bHasTangents;
		VertexFormat.bHasColors			|= Child.VertexFormat.bHasColors;

		if (bIsAssemblyCluster)
		{
			const FAssemblyPartData& Part = DAG.AssemblyPartData[Group.AssemblyPartIndex];
			const FBox3f LocalBox(Child.Bounds.Min, Child.Bounds.Max);
			for (const FMatrix44f& Transform : MakeArrayView(&DAG.AssemblyTransforms[Part.FirstTransform], Part.NumTransforms))
			{
				const FBox3f Box = LocalBox.TransformBy(Transform);
				const float MaxScale = Transform.GetScaleVector().GetMax();
				Bounds			+= {.Min = FVector4f(Box.Min, 0.0f), .Max = FVector4f(Box.Max, 0.0f) };
				SurfaceArea		+= Child.SurfaceArea * FMath::Square(MaxScale);
				NumVertsGuess	+= Child.NumVerts;
				NumTris			+= Child.NumTris;
			}
		}
		else
		{
			Bounds			+= Child.Bounds;
			SurfaceArea		+= Child.SurfaceArea;
			NumVertsGuess	+= Child.NumVerts;
			NumTris			+= Child.NumTris;
		}	

		// Can jump multiple levels but guarantee it steps at least 1.
		MipLevel	= FMath::Max( MipLevel,		Child.MipLevel + 1 );
		LODError	= FMath::Max( LODError,		Child.LODError );
		EdgeLength	= FMath::Max( EdgeLength,	Child.EdgeLength );

		GUID = Murmur64( { GUID, Child.GUID } );
	}
	if( NumTris == 0 )
		return;

	const uint32 VertSize = GetVertSize();
	Verts.Reserve( NumVertsGuess * VertSize );
	Indexes.Reserve( 3 * NumTris );
	MaterialIndexes.Reserve( NumTris );
	ExternalEdges.Reserve( 3 * NumTris );

	FHashTable VertHashTable( 1 << FMath::FloorLog2( NumVertsGuess ), NumVertsGuess );

	for( uint32 ClusterIndex : Children )
	{
		const FCluster&			Child = DAG.Clusters[ ClusterIndex ];
		const FClusterGroup&	Group = DAG.Groups[ Child.GroupIndex ];

		if( Child.NumTris == 0 )
			continue;

		const bool bIsAssemblyCluster = Group.AssemblyPartIndex != MAX_uint32;

		if( bIsAssemblyCluster )
		{
			const FAssemblyPartData& Part = DAG.AssemblyPartData[Group.AssemblyPartIndex];
			for(const FMatrix44f& Transform : MakeArrayView(&DAG.AssemblyTransforms[Part.FirstTransform], Part.NumTransforms))
			{
				const FMatrix44f NormalTransform = Transform.RemoveTranslation().Inverse().GetTransposed();
				for( int32 i = 0; i < Child.Indexes.Num(); i++ )
				{
					uint32 NewIndex = TransformAndAddVert( Child, Child.Indexes[i], Transform, NormalTransform, VertHashTable );
					Indexes.Add( NewIndex );
				}

				ExternalEdges.Append( Child.ExternalEdges );
				MaterialIndexes.Append( Child.MaterialIndexes );
			}
		}
		else
		{
			if( VertexFormat.Matches( Child.VertexFormat ) )
			{
				for( int32 i = 0; i < Child.Indexes.Num(); i++ )
				{
					uint32 NewIndex = AddVert( &Child.Verts[ Child.Indexes[i] * VertSize ], VertHashTable );
					Indexes.Add( NewIndex );
				}
			}
			else
			{
				for( int32 i = 0; i < Child.Indexes.Num(); i++ )
				{
					uint32 NewIndex = AddVertMismatched( Child, Child.Indexes[i], VertHashTable );
					Indexes.Add( NewIndex );
				}
			}
			ExternalEdges.Append( Child.ExternalEdges );
			MaterialIndexes.Append( Child.MaterialIndexes );
		}
	}

	FAdjacency Adjacency = BuildAdjacency();
	
	auto GetChildExternalEdgeCount = [ &DAG, &Children ]( int32 ChildIndex, int32& NumInstances )
	{
		const FCluster& Child = DAG.Clusters[ Children[ ChildIndex ] ];
		const FClusterGroup& Group = DAG.Groups[ Child.GroupIndex ];
		NumInstances = Group.AssemblyPartIndex == MAX_uint32 ? 1 : DAG.AssemblyPartData[ Group.AssemblyPartIndex ].NumTransforms;
		return Child.ExternalEdges.Num();
	};
	
	int32 ChildIndex = 0;
	int32 InstanceIndex = 0;
	int32 NumInstances = 0;
	int32 MinIndex = 0;
	int32 MaxIndex = GetChildExternalEdgeCount(0, NumInstances);
	
	for( int32 EdgeIndex = 0; EdgeIndex < ExternalEdges.Num(); EdgeIndex++ )
	{
		if( EdgeIndex >= MaxIndex )
		{
			if (++InstanceIndex == NumInstances)
			{
				InstanceIndex = 0;
				MinIndex = MaxIndex;
				MaxIndex += GetChildExternalEdgeCount(++ChildIndex, NumInstances);
			}
			else
			{
				const int32 NumEdges = MaxIndex - MinIndex;
				MinIndex = MaxIndex;
				MaxIndex = MinIndex + NumEdges;
			}
		}
	
		int32 AdjCount = ExternalEdges[ EdgeIndex ];
	
		Adjacency.ForAll( EdgeIndex,
			[ &AdjCount, MinIndex, MaxIndex ]( int32 EdgeIndex, int32 AdjIndex )
			{
				if( AdjIndex < MinIndex || AdjIndex >= MaxIndex )
					AdjCount--;
			} );
	
		// This seems like a sloppy workaround for a bug elsewhere but it is possible an interior edge is moved during simplifiation to
		// match another cluster and it isn't reflected in this count. Sounds unlikely but any hole closing could do this.
		// The only way to catch it would be to rebuild full adjacency after every pass which isn't practical.
		AdjCount = FMath::Max( AdjCount, 0 );
	
		ExternalEdges[ EdgeIndex ] = (int8)AdjCount;
		NumExternalEdges += AdjCount != 0 ? 1 : 0;
	}

	ensure( NumTris == Indexes.Num() / 3 );
	check( MaterialIndexes.Num() > 0 );
}

float FCluster::Simplify( const FClusterDAG& DAG, uint32 TargetNumTris, float TargetError, uint32 LimitNumTris, const FRayTracingFallbackBuildSettings* RayTracingFallbackBuildSettings )
{
	if( ( TargetNumTris >= NumTris && TargetError == 0.0f ) || LimitNumTris >= NumTris )
	{
		return 0.0f;
	}

	float UVArea[ MAX_STATIC_TEXCOORDS ] = { 0.0f };
	if( VertexFormat.NumTexCoords > 0 )
	{
		for( uint32 TriIndex = 0; TriIndex < NumTris; TriIndex++ )
		{
			uint32 Index0 = Indexes[ TriIndex * 3 + 0 ];
			uint32 Index1 = Indexes[ TriIndex * 3 + 1 ];
			uint32 Index2 = Indexes[ TriIndex * 3 + 2 ];

			FVector2f* UV0 = GetUVs( Index0 );
			FVector2f* UV1 = GetUVs( Index1 );
			FVector2f* UV2 = GetUVs( Index2 );

			for( uint32 UVIndex = 0; UVIndex < VertexFormat.NumTexCoords; UVIndex++ )
			{
				FVector2f EdgeUV1 = UV1[ UVIndex ] - UV0[ UVIndex ];
				FVector2f EdgeUV2 = UV2[ UVIndex ] - UV0[ UVIndex ];
				float SignedArea = 0.5f * ( EdgeUV1 ^ EdgeUV2 );
				UVArea[ UVIndex ] += FMath::Abs( SignedArea );

				// Force an attribute discontinuity for UV mirroring edges.
				// Quadric could account for this but requires much larger UV weights which raises error on meshes which have no visible issues otherwise.
				MaterialIndexes[ TriIndex ] |= ( SignedArea >= 0.0f ? 1 : 0 ) << ( UVIndex + 24 );
			}
		}
	}

	float TriangleSize = FMath::Sqrt( SurfaceArea / (float)NumTris );
	
	FFloat32 CurrentSize( FMath::Max( TriangleSize, THRESH_POINTS_ARE_SAME ) );
	FFloat32 DesiredSize( 0.25f );
	FFloat32 FloatScale( 1.0f );

	// Lossless scaling by only changing the float exponent.
	int32 Exponent = FMath::Clamp( (int)DesiredSize.Components.Exponent - (int)CurrentSize.Components.Exponent, -126, 127 );
	FloatScale.Components.Exponent = Exponent + 127;	//ExpBias
	// Scale ~= DesiredSize / CurrentSize
	float PositionScale = FloatScale.FloatValue;

	for( uint32 i = 0; i < NumVerts; i++ )
	{
		GetPosition(i) *= PositionScale;
	}
	TargetError *= PositionScale;

	uint32 NumAttributes = GetVertSize() - 3;
	float* AttributeWeights = (float*)FMemory_Alloca( NumAttributes * sizeof( float ) );
	float* WeightsPtr = AttributeWeights;

	// Normal
	*WeightsPtr++ = 1.0f;
	*WeightsPtr++ = 1.0f;
	*WeightsPtr++ = 1.0f;

	if( VertexFormat.bHasTangents )
	{
		// Tangent X
		*WeightsPtr++ = 0.0625f;
		*WeightsPtr++ = 0.0625f;
		*WeightsPtr++ = 0.0625f;

		// Tangent Y Sign
		*WeightsPtr++ = 0.5f;
	}

	if( VertexFormat.bHasColors )
	{
		*WeightsPtr++ = 0.0625f;
		*WeightsPtr++ = 0.0625f;
		*WeightsPtr++ = 0.0625f;
		*WeightsPtr++ = 0.0625f;
	}

	// Normalize UVWeights
	for( uint32 UVIndex = 0; UVIndex < VertexFormat.NumTexCoords; UVIndex++ )
	{
		float UVWeight = 0.0f;
		if( DAG.Settings.bLerpUVs )
		{
			float TriangleUVSize = FMath::Sqrt( UVArea[UVIndex] / (float)NumTris );
			TriangleUVSize = FMath::Max( TriangleUVSize, THRESH_UVS_ARE_SAME );
			UVWeight =  1.0f / ( 128.0f * TriangleUVSize );
		}
		*WeightsPtr++ = UVWeight;
		*WeightsPtr++ = UVWeight;
	}

	for (uint32 Influence = 0; Influence < VertexFormat.NumBoneInfluences; Influence++)
	{
		// Set all bone index/weight values to 0.0 so that the closest
		// original vertex to the new position will copy its data wholesale.
		// Similar to the !bLerpUV path, but always used for skinning data.
		float InfluenceWeight = 0.0f;

		*WeightsPtr++ = InfluenceWeight; // Bone index
		*WeightsPtr++ = InfluenceWeight; // Bone weight
	}

	check( ( WeightsPtr - AttributeWeights ) == NumAttributes );

	FMeshSimplifier Simplifier( Verts.GetData(), NumVerts, Indexes.GetData(), Indexes.Num(), MaterialIndexes.GetData(), NumAttributes );

	TMap< TTuple< FVector3f, FVector3f >, int8 > LockedEdges;

	for( int32 EdgeIndex = 0; EdgeIndex < ExternalEdges.Num(); EdgeIndex++ )
	{
		if( ExternalEdges[ EdgeIndex ] )
		{
			uint32 VertIndex0 = Indexes[ EdgeIndex ];
			uint32 VertIndex1 = Indexes[ Cycle3( EdgeIndex ) ];
	
			const FVector3f& Position0 = GetPosition( VertIndex0 );
			const FVector3f& Position1 = GetPosition( VertIndex1 );

			Simplifier.LockPosition( Position0 );
			Simplifier.LockPosition( Position1 );

			LockedEdges.Add( MakeTuple( Position0, Position1 ), ExternalEdges[ EdgeIndex ] );
		}
	}

	Simplifier.SetAttributeWeights( AttributeWeights );
	Simplifier.SetCorrectAttributes( CorrectAttributesFunctions[ VertexFormat.bHasTangents ][ VertexFormat.bHasColors ] );
	Simplifier.SetEdgeWeight( 2.0f );
	Simplifier.SetMaxEdgeLengthFactor( DAG.Settings.MaxEdgeLengthFactor );

	float MaxErrorSqr = Simplifier.Simplify(
		NumVerts, TargetNumTris, FMath::Square( TargetError ),
		0, LimitNumTris, MAX_flt );

	check( Simplifier.GetRemainingNumVerts() > 0 );
	check( Simplifier.GetRemainingNumTris() > 0 );

	if ( RayTracingFallbackBuildSettings && RayTracingFallbackBuildSettings->FoliageOverOcclusionBias > 0.0f )
	{
		Simplifier.ShrinkTriGroupWithMostSurfaceAreaLoss(RayTracingFallbackBuildSettings->FoliageOverOcclusionBias);
	}
#if !NANITE_VOXEL_DATA
	else if( DAG.Settings.bPreserveArea )
		Simplifier.PreserveSurfaceArea();
#endif

	Simplifier.Compact();
	
	Verts.SetNum( Simplifier.GetRemainingNumVerts() * GetVertSize() );
	Indexes.SetNum( Simplifier.GetRemainingNumTris() * 3 );
	MaterialIndexes.SetNum( Simplifier.GetRemainingNumTris() );
	ExternalEdges.Init( 0, Simplifier.GetRemainingNumTris() * 3 );

	NumVerts = Simplifier.GetRemainingNumVerts();
	NumTris = Simplifier.GetRemainingNumTris();

	NumExternalEdges = 0;
	for( int32 EdgeIndex = 0; EdgeIndex < ExternalEdges.Num(); EdgeIndex++ )
	{
		auto Edge = MakeTuple(
			GetPosition( Indexes[ EdgeIndex ] ),
			GetPosition( Indexes[ Cycle3( EdgeIndex ) ] )
		);
		int8* AdjCount = LockedEdges.Find( Edge );
		if( AdjCount )
		{
			ExternalEdges[ EdgeIndex ] = *AdjCount;
			NumExternalEdges++;
		}
	}

	float InvScale = 1.0f / PositionScale;
	for( uint32 i = 0; i < NumVerts; i++ )
	{
		GetPosition(i) *= InvScale;
		Bounds += GetPosition(i);
	}

	for( uint32 TriIndex = 0; TriIndex < NumTris; TriIndex++ )
	{
		// Remove UV mirroring bits
		MaterialIndexes[ TriIndex ] &= 0xffffff;
	}

	return FMath::Sqrt( MaxErrorSqr ) * InvScale;
}

void FCluster::Split( FGraphPartitioner& Partitioner, const FAdjacency& Adjacency ) const
{
	FDisjointSet DisjointSet( NumTris );
	for( int32 EdgeIndex = 0; EdgeIndex < Indexes.Num(); EdgeIndex++ )
	{
		Adjacency.ForAll( EdgeIndex,
			[ &DisjointSet ]( int32 EdgeIndex0, int32 EdgeIndex1 )
			{
				if( EdgeIndex0 > EdgeIndex1 )
					DisjointSet.UnionSequential( EdgeIndex0 / 3, EdgeIndex1 / 3 );
			} );
	}

	auto GetCenter = [ this ]( uint32 TriIndex )
	{
		FVector3f Center;
		Center  = GetPosition( Indexes[ TriIndex * 3 + 0 ] );
		Center += GetPosition( Indexes[ TriIndex * 3 + 1 ] );
		Center += GetPosition( Indexes[ TriIndex * 3 + 2 ] );
		return Center * (1.0f / 3.0f);
	};

	Partitioner.BuildLocalityLinks( DisjointSet, Bounds, MaterialIndexes, GetCenter );

	auto* RESTRICT Graph = Partitioner.NewGraph( NumTris * 3 );

	for( uint32 i = 0; i < NumTris; i++ )
	{
		Graph->AdjacencyOffset[i] = Graph->Adjacency.Num();

		uint32 TriIndex = Partitioner.Indexes[i];

		// Add shared edges
		for( int k = 0; k < 3; k++ )
		{
			Adjacency.ForAll( 3 * TriIndex + k,
				[ &Partitioner, Graph ]( int32 EdgeIndex, int32 AdjIndex )
				{
					Partitioner.AddAdjacency( Graph, AdjIndex / 3, 4 * 65 );
				} );
		}

		Partitioner.AddLocalityLinks( Graph, TriIndex, 1 );
	}
	Graph->AdjacencyOffset[ NumTris ] = Graph->Adjacency.Num();

	Partitioner.PartitionStrict( Graph, false );
}

FAdjacency FCluster::BuildAdjacency() const
{
	FAdjacency Adjacency( Indexes.Num() );
	FEdgeHash EdgeHash( Indexes.Num() );

	for( int32 EdgeIndex = 0; EdgeIndex < Indexes.Num(); EdgeIndex++ )
	{
		Adjacency.Direct[ EdgeIndex ] = -1;

		EdgeHash.ForAllMatching( EdgeIndex, true,
			[ this ]( int32 CornerIndex )
			{
				return GetPosition( Indexes[ CornerIndex ] );
			},
			[&]( int32 EdgeIndex, int32 OtherEdgeIndex )
			{
				Adjacency.Link( EdgeIndex, OtherEdgeIndex );
			} );
	}

	return Adjacency;
}

uint32 FCluster::FindVert( uint32 Hash, const float* Vert, FHashTable& HashTable )
{
	const uint32 VertSize = GetVertSize();
	uint32 Index;
	for( Index = HashTable.First( Hash ); HashTable.IsValid( Index ); Index = HashTable.Next( Index ) )
	{
		uint32 i;
		for( i = 0; i < VertSize; i++ )
		{
			if( Vert[i] != Verts[ Index * VertSize + i ] )
				break;
		}
		if( i == VertSize )
			break;
	}

	return Index;
}

uint32 FCluster::AddVert( const float* Vert, FHashTable& HashTable )
{
	const uint32 VertSize = GetVertSize();
	const FVector3f& Position = *reinterpret_cast< const FVector3f* >( Vert );

	uint32 Hash = HashPosition( Position );
	uint32 NewIndex = FindVert( Hash, Vert, HashTable );
	if( !HashTable.IsValid( NewIndex ) )
	{
		Verts.AddUninitialized( VertSize );
		NewIndex = NumVerts++;
		HashTable.Add( Hash, NewIndex );

		FMemory::Memcpy( &GetPosition( NewIndex ), Vert, VertSize * sizeof( float ) );
	}

	return NewIndex;
}

template<typename TTransformPos, typename TTransformNormal, typename TTransformTangent>
inline uint32 FCluster::AddVertMismatched(
	const FCluster& Other,
	uint32 VertIndex,
	FHashTable& HashTable,
	TTransformPos&& TransformPos,
	TTransformNormal&& TransformNormal,
	TTransformTangent&& TransformTangent )
{
	check( VertexFormat.NumTexCoords		>= Other.VertexFormat.NumTexCoords );
	check( VertexFormat.NumBoneInfluences	>= Other.VertexFormat.NumBoneInfluences );

	const uint32 VertSize = GetVertSize();
	const FVector3f Position = TransformPos( Other.GetPosition(VertIndex) );

	// Create a temporary new vertex that will hold copied and default-initialized data
	const uint32 TempIndex = NumVerts;
	Verts.AddUninitialized(VertSize);
	GetPosition(TempIndex) = Position;
	GetNormal(TempIndex) = TransformNormal(Other.GetNormal(VertIndex));
	if (VertexFormat.bHasTangents)
	{
		GetTangentX(TempIndex)		= Other.VertexFormat.bHasTangents ? TransformTangent(Other.GetTangentX(VertIndex)) : FVector3f(0.0f);
		GetTangentYSign(TempIndex)	= Other.VertexFormat.bHasTangents ? Other.GetTangentYSign(VertIndex) : 1.0f;
	}
	if (VertexFormat.bHasColors)
	{
		GetColor(TempIndex) = Other.VertexFormat.bHasColors ? Other.GetColor(VertIndex) : FLinearColor::White;
	}	
	for (uint32 UVIndex = 0; UVIndex < Other.VertexFormat.NumTexCoords; ++UVIndex)
	{
		GetUVs(TempIndex)[UVIndex] = Other.GetUVs(VertIndex)[UVIndex];
	}
	for (uint32 UVIndex = Other.VertexFormat.NumTexCoords; UVIndex < VertexFormat.NumTexCoords; ++UVIndex)
	{
		GetUVs(TempIndex)[UVIndex] = FVector2f::ZeroVector;
	}
	for (uint32 BoneInfluenceIndex = 0; BoneInfluenceIndex < Other.VertexFormat.NumBoneInfluences; ++BoneInfluenceIndex)
	{
		GetBoneInfluences(TempIndex)[BoneInfluenceIndex] = Other.GetBoneInfluences(VertIndex)[BoneInfluenceIndex];
	}
	for (uint32 BoneInfluenceIndex = Other.VertexFormat.NumBoneInfluences; BoneInfluenceIndex < VertexFormat.NumBoneInfluences; ++BoneInfluenceIndex)
	{
		GetBoneInfluences(TempIndex)[BoneInfluenceIndex] = FVector2f::ZeroVector;
	}

	uint32 Hash = HashPosition( Position );
	uint32 NewIndex = FindVert( Hash, &Verts[ TempIndex * VertSize ], HashTable );
	if( HashTable.IsValid( NewIndex ) )
	{
		// Already exists, remove the temporary
		Verts.SetNumUnsafeInternal( Verts.Num() - VertSize );
		return NewIndex;
	}

	// Doesn't exist, the temporary is officially a new vertex
	NumVerts++;
	HashTable.Add( Hash, TempIndex );
	return TempIndex;
}

uint32 FCluster::AddVertMismatched( const FCluster& Other, uint32 VertIndex, FHashTable& HashTable )
{
	return AddVertMismatched(
		Other,
		VertIndex,
		HashTable,
		[]( const FVector3f& P ) { return P; },
		[]( const FVector3f& N ) { return N; },
		[]( const FVector3f& T ) { return T; }
	);
}

uint32 FCluster::TransformAndAddVert(
	const FCluster& Other,
	uint32 VertIndex,
	const FMatrix44f& Transform,
	const FMatrix44f& NormalTransform,
	FHashTable& HashTable )
{
	return AddVertMismatched(
		Other,
		VertIndex,
		HashTable,
		[&Transform]( const FVector3f& P ) { return Transform.TransformPosition( P ); },
		[&NormalTransform]( FVector3f N )
		{ 
			N = NormalTransform.TransformVector( N );
			N.Normalize();
			return N;
		},
		[&Transform]( FVector3f T )
		{
			T = Transform.TransformVector( T );
			T.Normalize();
			return T;
		}
	);
}

void FCluster::LerpAttributes(
	uint32 VertIndex,
	uint32 TriIndex,
	const FCluster& SrcCluster,
	const FVector3f& Barycentrics )
{
	check( VertexFormat.NumTexCoords		>= SrcCluster.VertexFormat.NumTexCoords );
	check( VertexFormat.NumBoneInfluences	>= SrcCluster.VertexFormat.NumBoneInfluences );

	const uint32 SrcIndex0 = SrcCluster.Indexes[ TriIndex * 3 + 0 ];
	const uint32 SrcIndex1 = SrcCluster.Indexes[ TriIndex * 3 + 1 ];
	const uint32 SrcIndex2 = SrcCluster.Indexes[ TriIndex * 3 + 2 ];

	GetNormal( VertIndex ) =
		SrcCluster.GetNormal( SrcIndex0 ) * Barycentrics[0] +
		SrcCluster.GetNormal( SrcIndex1 ) * Barycentrics[1] +
		SrcCluster.GetNormal( SrcIndex2 ) * Barycentrics[2];

	if( VertexFormat.bHasTangents )
	{
		if( SrcCluster.VertexFormat.bHasTangents )
		{
			GetTangentX( VertIndex ) =
				SrcCluster.GetTangentX( SrcIndex0 ) * Barycentrics[0] +
				SrcCluster.GetTangentX( SrcIndex1 ) * Barycentrics[1] +
				SrcCluster.GetTangentX( SrcIndex2 ) * Barycentrics[2];

			// Need to lerp?
			GetTangentYSign( VertIndex ) =
				SrcCluster.GetTangentYSign( SrcIndex0 ) * Barycentrics[0] +
				SrcCluster.GetTangentYSign( SrcIndex1 ) * Barycentrics[1] +
				SrcCluster.GetTangentYSign( SrcIndex2 ) * Barycentrics[2];
		}
		else
		{
			// TODO
			GetTangentX( VertIndex ) = FVector3f(0.0f);
			GetTangentYSign( VertIndex ) = 1.0f;
		}
	}

	if( VertexFormat.bHasColors )
	{
		if( SrcCluster.VertexFormat.bHasColors )
		{
			GetColor( VertIndex ) =
				SrcCluster.GetColor( SrcIndex0 ) * Barycentrics[0] +
				SrcCluster.GetColor( SrcIndex1 ) * Barycentrics[1] +
				SrcCluster.GetColor( SrcIndex2 ) * Barycentrics[2];
		}
		else
			GetColor( VertIndex ) = FLinearColor::White;
	}

	for( uint32 UVIndex = 0; UVIndex < SrcCluster.VertexFormat.NumTexCoords; UVIndex++ )
	{
		GetUVs( VertIndex )[ UVIndex ] =
			SrcCluster.GetUVs( SrcIndex0 )[ UVIndex ] * Barycentrics[0] +
			SrcCluster.GetUVs( SrcIndex1 )[ UVIndex ] * Barycentrics[1] +
			SrcCluster.GetUVs( SrcIndex2 )[ UVIndex ] * Barycentrics[2];
	}
	for( uint32 UVIndex = SrcCluster.VertexFormat.NumTexCoords; UVIndex < VertexFormat.NumTexCoords; UVIndex++ )
	{
		GetUVs( VertIndex )[ UVIndex ] = FVector2f::ZeroVector;
	}

	// Copy dominant skinning attributes instead of interpolating them
	if( SrcCluster.VertexFormat.NumBoneInfluences > 0 )
	{
		int32 DomCorner = FMath::Max3Index( Barycentrics[0], Barycentrics[1], Barycentrics[2] );
		uint32 DomIndex = SrcCluster.Indexes[ TriIndex * 3 + DomCorner ];
		FMemory::Memcpy( GetBoneInfluences( VertIndex ), SrcCluster.GetBoneInfluences( DomIndex ), SrcCluster.VertexFormat.NumBoneInfluences * sizeof( FVector2f ) );
	}
	for( uint32 InfluenceIndex = SrcCluster.VertexFormat.NumBoneInfluences; InfluenceIndex < VertexFormat.NumBoneInfluences; InfluenceIndex++ )
	{
		GetBoneInfluences( VertIndex )[ InfluenceIndex ] = FVector2f::ZeroVector;
	}
}

void FCluster::Bound()
{
	Bounds = FBounds3f();
	SurfaceArea = 0.0f;
	
	TArray< FVector3f, TInlineAllocator<128> > Positions;
	Positions.SetNum( NumVerts, EAllowShrinking::No );

	for( uint32 i = 0; i < NumVerts; i++ )
	{
		Positions[i] = GetPosition(i);
		Bounds += Positions[i];
	}
	SphereBounds = FSphere3f( Positions.GetData(), Positions.Num() );
	LODBounds = SphereBounds;
	
	float MaxEdgeLength2 = 0.0f;
	for( int i = 0; i < Indexes.Num(); i += 3 )
	{
		FVector3f v[3];
		v[0] = GetPosition( Indexes[ i + 0 ] );
		v[1] = GetPosition( Indexes[ i + 1 ] );
		v[2] = GetPosition( Indexes[ i + 2 ] );

		FVector3f Edge01 = v[1] - v[0];
		FVector3f Edge12 = v[2] - v[1];
		FVector3f Edge20 = v[0] - v[2];

		MaxEdgeLength2 = FMath::Max( MaxEdgeLength2, Edge01.SizeSquared() );
		MaxEdgeLength2 = FMath::Max( MaxEdgeLength2, Edge12.SizeSquared() );
		MaxEdgeLength2 = FMath::Max( MaxEdgeLength2, Edge20.SizeSquared() );

		float TriArea = 0.5f * ( Edge01 ^ Edge20 ).Size();
		SurfaceArea += TriArea;
	}
	EdgeLength = FMath::Sqrt( MaxEdgeLength2 );
}

FORCEINLINE FVector2f UniformSampleDisk( FVector2f E )
{
	float Radius = FMath::Sqrt( E.X );

	float Theta = 2.0f * PI * E.Y;
	float SinTheta, CosTheta;
	FMath::SinCos( &SinTheta, &CosTheta, Theta );
	return Radius * FVector2f( CosTheta, SinTheta );
}

FORCEINLINE FVector3f UniformSampleSphere( FVector2f E )
{
	float CosPhi = 1.0f - 2.0f * E.X;
	float SinPhi = FMath::Sqrt( 1.0f - CosPhi * CosPhi );

	float Theta = 2.0f * PI * E.Y;
	float SinTheta, CosTheta;
	FMath::SinCos( &SinTheta, &CosTheta, Theta );

	return FVector3f(
		SinPhi * CosTheta,
		SinPhi * SinTheta,
		CosPhi );
}

// exp( -0.5 * x^2 / Sigma^2 )
FORCEINLINE FVector2f GaussianSampleDisk( FVector2f E, float Sigma, float Window )
{
	// Scale distribution to set non-unit variance
	// Variance = Sigma^2

	// Window to [-Window, Window] output
	// Without windowing we could generate samples far away on the infinite tails.
	float InWindow = FMath::Exp( -0.5f * FMath::Square( Window / Sigma ) );
					
	// Box-Muller transform
	float Radius = Sigma * FMath::Sqrt( -2.0f * FMath::Loge( (1.0f - E.X) * InWindow + E.X ) );

	float Theta = 2.0f * PI * E.Y;
	float SinTheta, CosTheta;
	FMath::SinCos( &SinTheta, &CosTheta, Theta );
	return Radius * FVector2f( CosTheta, SinTheta );
}

// All Sobol code adapted from PathTracingRandomSequence.ush
uint32 EvolveSobolSeed( uint32& Seed )
{
	// constant from: https://www.pcg-random.org/posts/does-it-beat-the-minimal-standard.html
	const uint32 MCG_C = 2739110765;
	Seed += MCG_C;

	// Generated using https://github.com/skeeto/hash-prospector
	// Estimated Bias ~583
	uint32 Hash = Seed;
	Hash *= 0x92955555u;
	Hash ^= Hash >> 15;
	return Hash;
}

FVector4f LatticeSampler( uint32 SampleIndex, uint32& Seed )
{
	// Same as FastOwenScrambling, but without the final reversebits
	uint32 LatticeIndex = SampleIndex + EvolveSobolSeed( Seed );
	LatticeIndex ^= LatticeIndex * 0x9c117646u;
	LatticeIndex ^= LatticeIndex * 0xe0705d72u;

	// Lattice parameters taken from:
	// Weighted compound integration rules with higher order convergence for all N
	// Fred J. Hickernell, Peter Kritzer, Frances Y. Kuo, Dirk Nuyens
	// Numerical Algorithms - February 2012
	FUintVector4 Result = LatticeIndex * FUintVector4( 1, 364981, 245389, 97823 );

	return (Result >> 8) * 5.96046447754e-08f; // * 2^-24
}

uint32 FastOwenScrambling( uint32 Index, uint32 Seed )
{
	// Laine and Karras / Stratified Sampling for Stochastic Transparency / EGSR 2011
	Index += Seed; // randomize the index by our seed (pushes bits toward the left)
	Index ^= Index * 0x9c117646u;
	Index ^= Index * 0xe0705d72u;
	return ReverseBits( Index );
}

FVector2f SobolSampler( uint32 SampleIndex, uint32& Seed )
{
	// first scramble the index to decorelate from other 4-tuples
	uint32 SobolIndex = FastOwenScrambling( SampleIndex, EvolveSobolSeed( Seed ) );
	// now get Sobol' point from this index
	FUintVector2 Result( SobolIndex );
	// y component can be computed without iteration
	// "An Implementation Algorithm of 2D Sobol Sequence Fast, Elegant, and Compact"
	// Abdalla Ahmed, EGSR 2024
	// See listing (19) in the paper
	// The code is different here because we want the output to be bit-reversed, but
	// the methodology is the same
	Result.Y ^=  Result.Y               >> 16;
	Result.Y ^= (Result.Y & 0xFF00FF00) >>  8;
	Result.Y ^= (Result.Y & 0xF0F0F0F0) >>  4;
	Result.Y ^= (Result.Y & 0xCCCCCCCC) >>  2;
	Result.Y ^= (Result.Y & 0xAAAAAAAA) >>  1;

	// finally scramble the points to avoid structured artifacts
	Result.X = FastOwenScrambling( Result.X, EvolveSobolSeed( Seed ) );
	Result.Y = FastOwenScrambling( Result.Y, EvolveSobolSeed( Seed ) );

	// output as float in [0,1) taking care not to skew the distribution
	// due to the non-uniform spacing of floats in this range
	return (Result >> 8) * 5.96046447754e-08f; // * 2^-24
}

#if 0

// [Loubet and Neyret 2017, "Hybrid mesh-volume LoDs for all-scale pre-filtering of complex 3D assets"]
static void GenerateRay( uint32 SampleIndex, uint32& Seed, FVector3f VoxelCenter, float VoxelSize, FVector3f& Origin, FVector3f& Direction, FVector2f& Time )
{
	Direction = UniformSampleSphere( SobolSampler( SampleIndex, Seed ) );

	Origin = FVector3f( LatticeSampler( SampleIndex, Seed ) ) - 0.5f;
	
	FVector2f Gaussian0 = GaussianSampleDisk( SobolSampler( SampleIndex, Seed ), 0.6f, 1.5f );
	FVector2f Gaussian1 = GaussianSampleDisk( SobolSampler( SampleIndex, Seed ), 0.6f, 1.5f );

	Origin += FVector3f( Gaussian0.X, Gaussian0.Y, Gaussian1.X );
	Origin *= VoxelSize;
	Origin += VoxelCenter;

	Time[0] = 0.0f;
	Time[1] = VoxelSize;
}

#elif 1

static void GenerateRay( uint32 SampleIndex, uint32& Seed, FVector3f VoxelCenter, float VoxelSize, FVector3f& Origin, FVector3f& Direction, FVector2f& Time )
{
	do
	{
		Direction = UniformSampleSphere( SobolSampler( SampleIndex, Seed ) );

		// [ Duff et al. 2017, "Building an Orthonormal Basis, Revisited" ]
		const float Sign = Direction.Z >= 0.0f ? 1.0f : -1.0f;
		const float a = -1.0f / ( Sign + Direction.Z );
		const float b = Direction.X * Direction.Y * a;
	
		FVector3f TangentX( 1.0f +	Sign * a * FMath::Square( Direction.X ), Sign * b, -Sign * Direction.X );
		FVector3f TangentY( b,		Sign + a * FMath::Square( Direction.Y ), -Direction.Y );

		FVector2f Disk = UniformSampleDisk( SobolSampler( SampleIndex, Seed ) );
		Disk *= VoxelSize * 0.5f * UE_SQRT_3;

		Origin  = TangentX * Disk.X;
		Origin += TangentY * Disk.Y;

		// Reject sample if it doesn't hit voxel
		const FVector3f InvDir			= 1.0f / Direction;
		const FVector3f Center			= -Origin * InvDir;
		const FVector3f Extent			= InvDir.GetAbs() * ( VoxelSize * 0.5f );
		const FVector3f MinIntersection = Center - Extent;
		const FVector3f MaxIntersection = Center + Extent;

		Time[0] = MinIntersection.GetMax();
		Time[1] = MaxIntersection.GetMin();
	} while( Time[0] >= Time[1] );

	Origin += VoxelCenter;

	// Force start to zero, negative isn't supported
	Origin += Direction * Time[0];
	Time[1] -= Time[0];
	Time[0] = 0.0f;
}

#else

static void GenerateRay( uint32 SampleIndex, uint32& Seed, FVector3f VoxelCenter, float VoxelSize, FVector3f& Origin, FVector3f& Direction, FVector2f& Time )
{
	//FVector4f Rand = LatticeSampler( SampleIndex, Seed );

	Direction = UniformSampleSphere( SobolSampler( SampleIndex, Seed ) );

	// [ Duff et al. 2017, "Building an Orthonormal Basis, Revisited" ]
	const float Sign = Direction.Z >= 0.0f ? 1.0f : -1.0f;
	const float a = -1.0f / ( Sign + Direction.Z );
	const float b = Direction.X * Direction.Y * a;
	
	FVector3f TangentX( 1.0f +	Sign * a * FMath::Square( Direction.X ), Sign * b, -Sign * Direction.X );
	FVector3f TangentY( b,		Sign + a * FMath::Square( Direction.Y ), -Direction.Y );

	//FVector2f Disk = UniformSampleDisk( FVector2f( Rand.Z, Rand.W ) ) * 0.5f;
	FVector2f Disk = GaussianSampleDisk( SobolSampler( SampleIndex, Seed ), 0.5f, 1.0f );
	Disk *= VoxelSize;

	Origin  = VoxelCenter;
	Origin += TangentX * Disk.X;
	Origin += TangentY * Disk.Y;

	Time[0] = -0.5f * VoxelSize;
	Time[1] = +0.5f * VoxelSize;
}

#endif

static void GenerateRayAligned( uint32 SampleIndex, uint32& Seed, FVector3f VoxelCenter, float VoxelSize, FVector3f& Origin, FVector3f& Direction, FVector2f& Time )
{
	uint32 RandIndex = SampleIndex + EvolveSobolSeed( Seed );
	uint32 Face = RandIndex % 6;
	float Sign = (Face & 1) ? 1.0f : -1.0f;

	const int32 SwizzleZ = Face >> 1;
	const int32 SwizzleX = ( 1 << SwizzleZ ) & 3;
	const int32 SwizzleY = ( 1 << SwizzleX ) & 3;

	FVector2f Sobol = SobolSampler( SampleIndex, Seed );

	Origin = VoxelCenter;
	Origin[ SwizzleX ] += VoxelSize * ( Sobol.X - 1.0f );
	Origin[ SwizzleY ] += VoxelSize * ( Sobol.Y - 1.0f );
	Origin[ SwizzleZ ] -= VoxelSize * 0.5f * Sign;

	Direction[ SwizzleX ] = 0.0f;
	Direction[ SwizzleY ] = 0.0f;
	Direction[ SwizzleZ ] = Sign;

	Time[0] = 0.0f;
	Time[1] = VoxelSize;
}

template< typename T >
struct TSGGX
{
	T	nxx;
	T	nyy;
	T	nzz;

	T	nxy;
	T	nxz;
	T	nyz;

	TSGGX&	operator+=( const UE::Math::TVector<T>& Normal )
	{
		// n n^T
		nxx += Normal.X * Normal.X;
		nyy += Normal.Y * Normal.Y;
		nzz += Normal.Z * Normal.Z;
			  
		nxy += Normal.X * Normal.Y;
		nxz += Normal.X * Normal.Z;
		nyz += Normal.Y * Normal.Z;
		
		return *this;
	}

	TSGGX&	operator/=( T a )
	{
		nxx /= a;
		nyy /= a;
		nzz /= a;

		nxy /= a;
		nxz /= a;
		nyz /= a;
		
		return *this;
	}

	void	FitIsotropic( UE::Math::TVector<T>& Center, UE::Math::TVector2<T>& Alpha ) const
	{
		// Diagonalize matrix
		// A = V^T S V
		T A[] =
		{
			nxx, nxy, nxz,
			nxy, nyy, nyz,
			nxz, nyz, nzz
		};
		T V[9];
		T S[3];

		JacobiSVD::EigenSolver3( A, S, V, 1e-8f );

		T Scale[3];
		for( uint32 k = 0; k < 3; k++ )
			Scale[k] = FMath::Sqrt( FMath::Abs( S[k] ) );

		T		MaxRatio = 0.0;
		int32	MaxIndex = 0;
		for( uint32 k = 0; k < 3; k++ )
		{
			const uint32 k0 = k;
			const uint32 k1 = (1 << k0) & 3;

			T Ratio = FMath::Min( Scale[k0], Scale[k1] )
					/ FMath::Max( Scale[k0], Scale[k1] );
			if( MaxRatio < Ratio )
			{
				MaxRatio = Ratio;
				MaxIndex = k;
			}
		}

		const uint32 k0 = MaxIndex;
		const uint32 k1 = (1 << k0) & 3;
		const uint32 k2 = (1 << k1) & 3;

		for( uint32 k = 0; k < 3; k++ )
			Center[k] = V[ 3*k + k2 ];

		Alpha[0] = 0.5f * ( Scale[k0] + Scale[k1] );
		Alpha[1] = Scale[k2];
	}

	// Linearly filtering SGGX, which is the same as using the auto-correlation matrix (second moments), directly is a decent fit.
	// Reprojecting area to eigenvectors can be better but requires a second pass.

	// Projected area
	// alpha = sqrt( w^T S w )
	// alpha = sqrt( w^T n n^T w )
	// alpha = abs( dot( n, w ) )
};

bool TestCrosshair( FRayTracingScene& RayTracingScene, const FVector3f& VoxelCenter, float VoxelSize, uint32& HitClusterIndex, uint32& HitTriIndex, FVector3f& HitBarycentrics )
{
	FVector2f Time( 0.0f, VoxelSize );
	for( int j = 0; j < 3; j++ )
	{
		FVector3f Origin = VoxelCenter;
		Origin[j] -= 0.5f * VoxelSize;
		FVector3f Direction( 0.0f );
		Direction[j] = 1.0f;

		// TODO use Ray4
		FRay1 Ray = {};
		Ray.SetRay( Origin, Direction, Time );

		RayTracingScene.Intersect1( Ray );
		if( RayTracingScene.GetHit( Ray, HitClusterIndex, HitTriIndex, HitBarycentrics ) )
			return true;
	}
	return false;
}

void FCluster::Voxelize( FClusterDAG& DAG, FRayTracingScene& RayTracingScene, TArrayView< const uint32 > Children, float VoxelSize )
{
	if( DAG.Settings.bVoxelNDF || ( DAG.Settings.bVoxelOpacity && DAG.Settings.NumRays > 1 ) )
		VertexFormat.bHasColors = true;

	for( uint32 ChildIndex : Children )
	{
		const FCluster& Child = DAG.Clusters[ ChildIndex ];

		Bounds			+= Child.Bounds;
		SurfaceArea		+= Child.SurfaceArea;

		VertexFormat.NumTexCoords		= FMath::Max( VertexFormat.NumTexCoords,		Child.VertexFormat.NumTexCoords );
		VertexFormat.NumBoneInfluences	= FMath::Max( VertexFormat.NumBoneInfluences,	Child.VertexFormat.NumBoneInfluences );
		VertexFormat.bHasTangents		|= Child.VertexFormat.bHasTangents;
		VertexFormat.bHasColors			|= Child.VertexFormat.bHasColors;

		// Can jump multiple levels but guarantee it steps at least 1.
		MipLevel	= FMath::Max( MipLevel,		Child.MipLevel + 1 );
		LODError	= FMath::Max( LODError,		Child.LODError );
		EdgeLength	= FMath::Max( EdgeLength,	Child.EdgeLength );

		GUID = Murmur64( { GUID, Child.GUID } );
	}

	const uint32 VertSize = GetVertSize();

	TSet< FIntVector3 >			CandidateVoxelSet;
	TMap< FIntVector3, uint32 > VoxelMap;

	check( VoxelSize > 0.0f );
	const float RcpVoxelSize = 1.0f / VoxelSize;

	for( uint32 ChildIndex : Children )
	{
		const FCluster& Child = DAG.Clusters[ ChildIndex ];

		if( Child.NumTris )
		{
			for( uint32 TriIndex = 0; TriIndex < Child.NumTris; TriIndex++ )
			{
				FVector3f Triangle[3];
				for( int k = 0; k < 3; k++ )
					Triangle[k] = Child.GetPosition( Child.Indexes[ TriIndex * 3 + k ] ) * RcpVoxelSize;
	
				const float* Attributes0 = Child.GetAttributes( Child.Indexes[ TriIndex * 3 + 0 ] );
				const float* Attributes1 = Child.GetAttributes( Child.Indexes[ TriIndex * 3 + 1 ] );
				const float* Attributes2 = Child.GetAttributes( Child.Indexes[ TriIndex * 3 + 2 ] );

			#if RAY_TRACE_VOXELS
				VoxelizeTri26( Triangle,
					[&]( const FIntVector3& Voxel, const FVector3f& Barycentrics )
					{
						CandidateVoxelSet.Add( Voxel );
					} );
			#else
				VoxelizeTri6( Triangle,
					[&]( const FIntVector3& Voxel, const FVector3f& Barycentrics )
					{
						uint32& NewIndex = VoxelMap.FindOrAdd( Voxel, ~0u );
						if( NewIndex == ~0u )
						{
							NewIndex = NumVerts++;
	
							Verts.AddUninitialized( VertSize );
							MaterialIndexes.Add( Child.MaterialIndexes[ TriIndex ] );

							GetPosition( NewIndex ) = ( Voxel + 0.5f ) * VoxelSize;

							LerpAttributes( NewIndex, TriIndex, Child, Barycentrics );
							
							CorrectAttributesFunctions[ VertexFormat.bHasTangents ][ VertexFormat.bHasColors ]( GetAttributes( NewIndex ) );
						}
					} );
			#endif
			}
		}
		else
		{
			for( int32 BrickIndex = 0; BrickIndex < Child.Bricks.Num(); BrickIndex++ )
			{
				int32 MaterialIndex = Child.MaterialIndexes[ BrickIndex ];

				uint32 NumVoxels = FMath::CountBits( Child.Bricks[ BrickIndex ].VoxelMask );
				for( uint32 i = 0; i < NumVoxels; i++ )
				{
					uint32 VertIndex = Child.Bricks[ BrickIndex ].VertOffset + i;
					FVector3f Center = Child.GetPosition( VertIndex ) * RcpVoxelSize;
				#if RAY_TRACE_VOXELS
					float Extent = Child.LODError * 0.5f * RcpVoxelSize;

					FIntVector3 MinVoxel = FloorToInt( Center - Extent );
					FIntVector3 MaxVoxel = FloorToInt( Center + Extent );

					for( int32 z = MinVoxel.Z; z <= MaxVoxel.Z; z++ )
					{
						for( int32 y = MinVoxel.Y; y <= MaxVoxel.Y; y++ )
						{
							for( int32 x = MinVoxel.X; x <= MaxVoxel.X; x++ )
							{
								FIntVector3 Voxel( x, y, z );
								CandidateVoxelSet.Add( Voxel );
							}
						}
					}
				#else
					FIntVector3 Voxel = FloorToInt( Center );

					uint32& NewIndex = VoxelMap.FindOrAdd( Voxel, ~0u );
					if( NewIndex == ~0u )
					{
						NewIndex = NumVerts++;

						Verts.AddUninitialized( VertSize );
						MaterialIndexes.Add( MaterialIndex );

						GetPosition( NewIndex ) = ( FVector3f( Voxel ) + 0.5f ) * VoxelSize;

						// TODO Mixed formats
						uint32 AttrSize = VertSize - 3;
						FMemory::Memcpy( GetAttributes( NewIndex ), Child.GetAttributes( VertIndex ), AttrSize * sizeof( float ) );
					}
				#endif
				}
			}

		#if RAY_TRACE_VOXELS
			for( const FVector3f& Position : Child.ExtraVoxels )
			{
				const FVector3f Center	= Position * RcpVoxelSize;
				const float Extent		= Child.LODError * 0.5f * RcpVoxelSize;

				const FIntVector3 MinVoxel = FloorToInt( Center - Extent );
				const FIntVector3 MaxVoxel = FloorToInt( Center + Extent );

				for( int32 z = MinVoxel.Z; z <= MaxVoxel.Z; z++ )
				{
					for( int32 y = MinVoxel.Y; y <= MaxVoxel.Y; y++ )
					{
						for( int32 x = MinVoxel.X; x <= MaxVoxel.X; x++ )
						{
							CandidateVoxelSet.Add( FIntVector3( x, y, z ) );
						}
					}
				}
			}
		#endif
		}
	}

#if RAY_TRACE_VOXELS

	{
		// Trace bricks
		check( ExtraVoxels.Num() == 0 );
		NumVerts = 0;

		const float RayBackUp = VoxelSize * DAG.Settings.RayBackUp;

		FBinaryHeap< float > CoverageHeap( CandidateVoxelSet.Num(), CandidateVoxelSet.Num() );
		float CoverageSum = 0.0f;
		
		for ( FIntVector Voxel : CandidateVoxelSet )
		{
			FVector3f VoxelCenter = ( Voxel + 0.5f ) * VoxelSize;
			
			uint32 TileID;
			TileID  = FMath::MortonCode3( Voxel.X & 1023 );
			TileID |= FMath::MortonCode3( Voxel.Y & 1023 ) << 1;
			TileID |= FMath::MortonCode3( Voxel.Z & 1023 ) << 2;

			TSGGX< float > NDF = {};

			uint32		HitClusterIndex = 0;
			uint32		HitTriIndex = 0;
			FVector3f	HitBarycentrics;
			uint16		HitCount = 0;
			uint16		RayCount = 0;
			if( DAG.Settings.NumRays > 1 )
			{
				uint32 HitCountDim[3] = {};
				uint32 RayCountDim[3] = {};
				for( uint32 i = 0; i < DAG.Settings.NumRays; i += 16 )
				{
					FRay16 Ray16 = {};
					for( int j = 0; j < 16; j++ )
					{
						// Combine pixel-level and sample-level bits into the sample index (visible structure will be hidden by owen scrambling of the index)
						uint32 SampleIndex = ReverseBits( TileID * DAG.Settings.NumRays + (i + j) );
						uint32 Seed = 0;

						FVector3f Origin;
						FVector3f Direction;
						FVector2f Time;
						if( DAG.Settings.bSeparable )
						{
							GenerateRayAligned( SampleIndex, Seed, VoxelCenter, VoxelSize, Origin, Direction, Time );
						
							Origin -= Direction * VoxelSize;
							Time[1] += VoxelSize * 2.0f;
						}
						else
						{
							GenerateRay( SampleIndex, Seed, VoxelCenter, VoxelSize, Origin, Direction, Time );
						
							Origin -= Direction * RayBackUp;
							Time[1] += RayBackUp;
						}

						Ray16.SetRay( j, Origin, Direction, Time );
					}

					RayTracingScene.Intersect16( Ray16 );
					RayCount += 16;

					for( int j = 0; j < 16; j++ )
					{
						uint32 Dim = FMath::Max3Index( FMath::Abs( Ray16.ray.dir_x[j] ), FMath::Abs( Ray16.ray.dir_y[j] ), FMath::Abs( Ray16.ray.dir_z[j] ) );
						RayCountDim[ Dim ]++;

						if( RayTracingScene.GetHit( Ray16, j, HitClusterIndex, HitTriIndex, HitBarycentrics ) )
						{
							if( DAG.Settings.bSeparable )
							{
								if( Ray16.ray.tfar[j] < VoxelSize ||
									Ray16.ray.tfar[j] > VoxelSize * 2.0f )
								{
									RayCount--;
									RayCountDim[ Dim ]--;
									continue;
								}
							}
							else if( Ray16.ray.tfar[j] < RayBackUp )
							{
								RayCount--;
								continue;
							}

							HitCount++;
							HitCountDim[ Dim ]++;

							// Sample attributes from hit triangle
							FCluster& HitCluster = DAG.Clusters[ HitClusterIndex ];

							FVector3f HitNormal =
								HitCluster.GetNormal( HitCluster.Indexes[ HitTriIndex * 3 + 0 ] ) * HitBarycentrics[0] +
								HitCluster.GetNormal( HitCluster.Indexes[ HitTriIndex * 3 + 1 ] ) * HitBarycentrics[1] +
								HitCluster.GetNormal( HitCluster.Indexes[ HitTriIndex * 3 + 2 ] ) * HitBarycentrics[2];
							HitNormal.Normalize();
							NDF += HitNormal;
						}
					}
				}

				if( DAG.Settings.bSeparable )
				{
					// Force covered if all rays along 1 axis hit something
					if( ( RayCountDim[0] && RayCountDim[0] == HitCountDim[0] ) ||
						( RayCountDim[1] && RayCountDim[1] == HitCountDim[1] ) ||
						( RayCountDim[2] && RayCountDim[2] == HitCountDim[2] ) )
					{
						uint32 Dummy1, Dummy2;
						FVector3f Dummy3;
						if( TestCrosshair( RayTracingScene, VoxelCenter, VoxelSize, Dummy1, Dummy2, Dummy3 ) )
							RayCount = HitCount;
					}
				}
			}
			else
			{
				if( DAG.Settings.bSeparable )
				{
					RayCount++;
					if( TestCrosshair( RayTracingScene, VoxelCenter, VoxelSize, HitClusterIndex, HitTriIndex, HitBarycentrics ) )
						HitCount++;
				}
				else
				{
					FRay1 Ray = {};
					{
						uint32 SampleIndex = ReverseBits( TileID );
						uint32 Seed = 0;

						FVector3f Origin;
						FVector3f Direction;
						FVector2f Time;
						GenerateRay( SampleIndex, Seed, VoxelCenter, VoxelSize, Origin, Direction, Time );
						Ray.SetRay( Origin, Direction, Time );
					}

					RayTracingScene.Intersect1( Ray );
					RayCount++;

					if( RayTracingScene.GetHit( Ray, HitClusterIndex, HitTriIndex, HitBarycentrics ) )
						HitCount++;
				}
			}

			if( HitCount > 0 )
			{
				uint32 NewIndex = NumVerts++;

				VoxelMap.Add( Voxel, NewIndex );

				// Sample attributes from hit triangle
				FCluster& HitCluster = DAG.Clusters[ HitClusterIndex ];

				Verts.AddUninitialized( VertSize );
				MaterialIndexes.Add( HitCluster.MaterialIndexes[ HitTriIndex ] );

				GetPosition( NewIndex ) = VoxelCenter;

				LerpAttributes( NewIndex, HitTriIndex, HitCluster, HitBarycentrics );

				if( DAG.Settings.NumRays > 1 )
				{
					if( DAG.Settings.bVoxelNDF )
					{
						NDF /= HitCount;

						FVector3f AvgNormal;
						FVector2f Alpha;
						NDF.FitIsotropic( AvgNormal, Alpha );

						GetNormal( NewIndex ) = AvgNormal;
						//GetColor( NewIndex ).A = (2.0f / PI) * FMath::Atan2( Alpha.X, Alpha.Y );
						if( Alpha.X > Alpha.Y )
							GetColor( NewIndex ).A = 1.0f - 0.5f * Alpha.Y / Alpha.X;
						else
							GetColor( NewIndex ).A = 0.5f * Alpha.X / Alpha.Y;
					}
					
					float Coverage = (float)HitCount / RayCount;
					CoverageHeap.Add( Coverage, NewIndex );
					CoverageSum += Coverage;

					if( DAG.Settings.bVoxelOpacity )
						GetColor( NewIndex ).B = Coverage;
				}
				else
				{
					if( DAG.Settings.bVoxelNDF )
						GetColor( NewIndex ).A = 0.0f;
				}

				CorrectAttributesFunctions[ VertexFormat.bHasTangents ][ VertexFormat.bHasColors ]( GetAttributes( NewIndex ) );
			}
			else
			{
				// Remember rejected voxels, so their volume still gets sampled at higher levels
				ExtraVoxels.Add( VoxelCenter );
			}
		}
	
	#if 1
		if( DAG.Settings.NumRays > 1 && !DAG.Settings.bVoxelOpacity )
		{
			while( (float)CoverageHeap.Num() > CoverageSum )
			{
				uint32 VertIndex	= CoverageHeap.Top();
				float Coverage		= CoverageHeap.GetKey( VertIndex );
				CoverageHeap.Pop();

				const FVector3f& Position = GetPosition( VertIndex );

				FIntVector3 Voxel = FloorToInt( Position * RcpVoxelSize );
				VoxelMap.Remove( Voxel );

				// Remember rejected voxels, so their volume still gets sampled at higher levels
				ExtraVoxels.Add( Position );

				// Distribute coverage to neighbors
				TArray< uint32, TFixedAllocator<27> > Neighbors;
				for( int32 z = -1; z <= 1; z++ )
				{
					for( int32 y = -1; y <= 1; y++ )
					{
						for( int32 x = -1; x <= 1; x++ )
						{
							uint32* AdjIndex = VoxelMap.Find( Voxel + FIntVector3(x,y,z) );
							if( AdjIndex )
							{
								Neighbors.Add( *AdjIndex );
							}
						}
					}
				}

				Coverage /= (float)Neighbors.Num();
				for( auto AdjIndex : Neighbors )
				{
					float AdjCoverage = CoverageHeap.GetKey( AdjIndex );
					AdjCoverage = 1.0f - ( 1.0f - AdjCoverage ) * ( 1.0f - Coverage );
					CoverageHeap.Update( AdjCoverage, AdjIndex );
				}
			}

			NumVerts = 0;
		
			// Compact remaining
			TArray< float > NewVerts;
			TArray< int32 > NewMaterialIndexes;

			for( auto& Voxel : VoxelMap )
			{
				uint32 OldIndex = Voxel.Value;
				uint32 NewIndex = Voxel.Value = NumVerts++;

				NewVerts.AddUninitialized( VertSize );
				NewMaterialIndexes.Add( MaterialIndexes[ OldIndex ] );

				FMemory::Memcpy( &NewVerts[ NewIndex * VertSize ], &GetPosition( OldIndex ), VertSize * sizeof( float ) );
			}

			Swap( Verts,			NewVerts );
			Swap( MaterialIndexes,	NewMaterialIndexes );
		}
	#endif
	}

	
	
	if( VoxelMap.Num() == 0 )
	{
		// VOXELTODO:	Silly workaround for the case where no voxels are hit by rays.
		//				Solve this properly.

		const FCluster& FirstChild = DAG.Clusters[ Children[ 0 ] ];
		const FVector3f Center = FirstChild.GetPosition( 0 ) * RcpVoxelSize;
		const FIntVector3 Voxel = FloorToInt( Center );

		uint32& NewIndex = VoxelMap.FindOrAdd( Voxel, ~0u );
		NewIndex = 0;

		Verts.AddUninitialized( VertSize );
		MaterialIndexes.Add( FirstChild.MaterialIndexes[ 0 ] );

		GetPosition( 0 ) = ( Voxel + 0.5f ) * VoxelSize;

		const uint32 AttrSize = VertSize - 3;
		FMemory::Memcpy( GetAttributes( 0 ), FirstChild.GetAttributes( 0 ), AttrSize * sizeof(float));
	}
#endif

	check( MaterialIndexes.Num() > 0 );

	VoxelsToBricks( VoxelMap );
}

void FCluster::VoxelsToBricks( TMap< FIntVector3, uint32 >& VoxelMap )
{
	check( Bricks.IsEmpty() );

	const uint32 VertSize = GetVertSize();

	NumVerts = 0;
	
	TArray< float > NewVerts;
	TArray< int32 > NewMaterialIndexes;

	TSet< FIntVector4 > BrickSet;
	for( auto Voxel : VoxelMap )
		BrickSet.FindOrAdd( FIntVector4( Voxel.Key & ~3, MaterialIndexes[ Voxel.Value ] ) );

	TArray< FIntVector4 > SortedBricks = BrickSet.Array();
	SortedBricks.Sort(
		[]( const FIntVector4& A, const FIntVector4& B )
		{
			if( A.W != B.W )
				return A.W < B.W;
			else if( A.Z != B.Z )
				return A.Z < B.Z;
			else if( A.Y != B.Y )
				return A.Y < B.Y;
			else
				return A.X < B.X;
		} );

	for( FIntVector4& Candidate : SortedBricks )
	{
		FBrick Brick;
		Brick.VoxelMask = 0;
		Brick.Position = FIntVector3( Candidate );
		Brick.VertOffset = NumVerts;

		FIntVector3 BrickMin( MAX_int32 );
		bool bBrickValid = false;
		for( uint32 z = 0; z < 4; z++ )
		{
			for( uint32 y = 0; y < 4; y++ )
			{
				for( uint32 x = 0; x < 4; x++ )
				{
					FIntVector3 Voxel = Brick.Position + FIntVector3(x,y,z);
					uint32* VertIndex = VoxelMap.Find( Voxel );
					if( VertIndex && MaterialIndexes[ *VertIndex ] == Candidate.W )
					{
						BrickMin = BrickMin.ComponentMin( Voxel );
						bBrickValid = true;
					}
				}
			}
		}

		if( !bBrickValid )
			continue;	// No voxels left in brick. Skip it.

		Brick.Position = BrickMin;

		uint32 VoxelIndex = 0;
		for( uint32 z = 0; z < 4; z++ )
		{
			for( uint32 y = 0; y < 4; y++ )
			{
				for( uint32 x = 0; x < 4; x++ )
				{
					FIntVector3 Voxel = Brick.Position + FIntVector3(x,y,z);
					uint32* VertIndex = VoxelMap.Find( Voxel );
					if( VertIndex && MaterialIndexes[ *VertIndex ] == Candidate.W )
					{
						Brick.VoxelMask |= 1ull << VoxelIndex;
						VoxelMap.Remove( Voxel );

						uint32 OldIndex = *VertIndex;
						uint32 NewIndex = NumVerts++;

						NewVerts.AddUninitialized( VertSize );

						FMemory::Memcpy( &NewVerts[ NewIndex * VertSize ], &GetPosition( OldIndex ), VertSize * sizeof( float ) );
					}

					VoxelIndex++;
				}
			}
		}

		Bricks.Add( Brick );
		NewMaterialIndexes.Add( Candidate.W );
	}
	check( VoxelMap.IsEmpty() );

	Swap( Verts,			NewVerts );
	Swap( MaterialIndexes,	NewMaterialIndexes );
}

void FCluster::BuildMaterialRanges()
{
	check( MaterialRanges.Num() == 0 );
	check( NumTris * 3 == Indexes.Num() );

	TArray< int32, TInlineAllocator<128> > MaterialElements;
	TArray< int32, TInlineAllocator<64> > MaterialCounts;

	MaterialElements.AddUninitialized( MaterialIndexes.Num() );
	MaterialCounts.AddZeroed( NANITE_MAX_CLUSTER_MATERIALS );

	// Tally up number per material index
	for( int32 i = 0; i < MaterialIndexes.Num(); i++ )
	{
		MaterialElements[i] = i;
		MaterialCounts[ MaterialIndexes[i] ]++;
	}

	// Sort by range count descending, and material index ascending.
	// This groups the material ranges from largest to smallest, which is
	// more efficient for evaluating the sequences on the GPU, and also makes
	// the minus one encoding work (the first range must have more than 1 tri).
	MaterialElements.Sort(
		[&]( int32 A, int32 B )
		{
			int32 IndexA = MaterialIndexes[A];
			int32 IndexB = MaterialIndexes[B];
			int32 CountA = MaterialCounts[ IndexA ];
			int32 CountB = MaterialCounts[ IndexB ];

			if( CountA != CountB )
				return CountA > CountB;

			if( IndexA != IndexB )
				return IndexA < IndexB;

			return A < B;
		} );

	FMaterialRange CurrentRange;
	CurrentRange.RangeStart = 0;
	CurrentRange.RangeLength = 0;
	CurrentRange.MaterialIndex = MaterialElements.Num() > 0 ? MaterialIndexes[ MaterialElements[0] ] : 0;

	for( int32 i = 0; i < MaterialElements.Num(); i++ )
	{
		int32 MaterialIndex = MaterialIndexes[ MaterialElements[i] ];

		// Material changed, so add current range and reset
		if (CurrentRange.RangeLength > 0 && MaterialIndex != CurrentRange.MaterialIndex)
		{
			MaterialRanges.Add(CurrentRange);

			CurrentRange.RangeStart = i;
			CurrentRange.RangeLength = 1;
			CurrentRange.MaterialIndex = MaterialIndex;
		}
		else
		{
			++CurrentRange.RangeLength;
		}
	}

	// Add last triangle to range
	if (CurrentRange.RangeLength > 0)
	{
		MaterialRanges.Add(CurrentRange);
	}

	if( NumTris )
	{
		TArray< uint32 >	NewIndexes;
		TArray< int32 >		NewMaterialIndexes;
	
		NewIndexes.AddUninitialized( Indexes.Num() );
		NewMaterialIndexes.AddUninitialized( MaterialIndexes.Num() );
	
		for( uint32 NewIndex = 0; NewIndex < NumTris; NewIndex++ )
		{
			uint32 OldIndex = MaterialElements[ NewIndex ];
			NewIndexes[ NewIndex * 3 + 0 ] = Indexes[ OldIndex * 3 + 0 ];
			NewIndexes[ NewIndex * 3 + 1 ] = Indexes[ OldIndex * 3 + 1 ];
			NewIndexes[ NewIndex * 3 + 2 ] = Indexes[ OldIndex * 3 + 2 ];
			NewMaterialIndexes[ NewIndex ] = MaterialIndexes[ OldIndex ];
		}
		Swap( Indexes,			NewIndexes );
		Swap( MaterialIndexes,	NewMaterialIndexes );
	}
	else
	{
		const uint32 VertSize = GetVertSize();

		TArray< float >		NewVerts;
		TArray< int32 >		NewMaterialIndexes;
		TArray< FBrick >	NewBricks;
	
		NewVerts.AddUninitialized( Verts.Num() );
		NewMaterialIndexes.AddUninitialized( MaterialIndexes.Num() );
		NewBricks.AddUninitialized( Bricks.Num() );
		NumVerts = 0;

		for( int32 NewIndex = 0; NewIndex < MaterialElements.Num(); NewIndex++ )
		{
			int32 OldIndex = MaterialElements[ NewIndex ];

			NewMaterialIndexes[ NewIndex ] = MaterialIndexes[ OldIndex ];

			FBrick& OldBrick = Bricks[ OldIndex ];
			FBrick& NewBrick = NewBricks[ NewIndex ];

			uint32 NumVoxels = FMath::CountBits( OldBrick.VoxelMask );
			
			NewBrick = OldBrick;
			NewBrick.VertOffset = NumVerts;
			NumVerts += NumVoxels;

			FMemory::Memcpy( &NewVerts[ NewBrick.VertOffset * VertSize ], &GetPosition( OldBrick.VertOffset ), NumVoxels * VertSize * sizeof( float ) );
		}
		Swap( Verts,			NewVerts );
		Swap( MaterialIndexes,	NewMaterialIndexes );
		Swap( Bricks,			NewBricks );
	}
}

static void SanitizeFloat( float& X, float MinValue, float MaxValue, float DefaultValue )
{
	if( X >= MinValue && X <= MaxValue )
		;
	else if( X < MinValue )
		X = MinValue;
	else if( X > MaxValue )
		X = MaxValue;
	else
		X = DefaultValue;
}

static void SanitizeVector( FVector3f& V, float MaxValue, FVector3f DefaultValue )
{
	if ( !(	V.X >= -MaxValue && V.X <= MaxValue &&
			V.Y >= -MaxValue && V.Y <= MaxValue &&
			V.Z >= -MaxValue && V.Z <= MaxValue ) )	// Don't flip condition. This is intentionally written like this to be NaN-safe.
	{
		V = DefaultValue;
	}
}

void FCluster::SanitizeVertexData()
{
	const float FltThreshold = NANITE_MAX_COORDINATE_VALUE;

	for( uint32 VertexIndex = 0; VertexIndex < NumVerts; VertexIndex++ )
	{
		FVector3f& Position = GetPosition( VertexIndex );
		SanitizeFloat( Position.X, -FltThreshold, FltThreshold, 0.0f );
		SanitizeFloat( Position.Y, -FltThreshold, FltThreshold, 0.0f );
		SanitizeFloat( Position.Z, -FltThreshold, FltThreshold, 0.0f );

		FVector3f& Normal = GetNormal( VertexIndex );
		SanitizeVector( Normal, FltThreshold, FVector3f::UpVector );

		if( VertexFormat.bHasTangents )
		{
			FVector3f& TangentX = GetTangentX( VertexIndex );
			SanitizeVector( TangentX, FltThreshold, FVector3f::ForwardVector );

			float& TangentYSign = GetTangentYSign( VertexIndex );
			TangentYSign = TangentYSign < 0.0f ? -1.0f : 1.0f;
		}

		if( VertexFormat.bHasColors )
		{
			FLinearColor& Color = GetColor( VertexIndex );
			SanitizeFloat( Color.R, 0.0f, 1.0f, 1.0f );
			SanitizeFloat( Color.G, 0.0f, 1.0f, 1.0f );
			SanitizeFloat( Color.B, 0.0f, 1.0f, 1.0f );
			SanitizeFloat( Color.A, 0.0f, 1.0f, 1.0f );
		}

		if( VertexFormat.NumTexCoords > 0 )
		{
			FVector2f* UVs = GetUVs( VertexIndex );
			for( uint32 UVIndex = 0; UVIndex < VertexFormat.NumTexCoords; UVIndex++ )
			{
				SanitizeFloat( UVs[ UVIndex ].X, -FltThreshold, FltThreshold, 0.0f );
				SanitizeFloat( UVs[ UVIndex ].Y, -FltThreshold, FltThreshold, 0.0f );
			}
		}

		if( VertexFormat.NumBoneInfluences > 0 )
		{
			FVector2f* BoneInfluences = GetBoneInfluences( VertexIndex );
			for( uint32 Influence = 0; Influence < VertexFormat.NumBoneInfluences; Influence++ )
			{
				SanitizeFloat( BoneInfluences[Influence].X, 0.0f, FltThreshold, 0.0f );
				SanitizeFloat( BoneInfluences[Influence].Y, 0.0f, FltThreshold, 0.0f );
			}
		}
	}
}

FArchive& operator<<(FArchive& Ar, FMaterialRange& Range)
{
	Ar << Range.RangeStart;
	Ar << Range.RangeLength;
	Ar << Range.MaterialIndex;
	Ar << Range.BatchTriCounts;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FStripDesc& Desc)
{
	for (uint32 i = 0; i < 4; i++)
	{
		for (uint32 j = 0; j < 3; j++)
		{
			Ar << Desc.Bitmasks[i][j];
		}
	}
	Ar << Desc.NumPrevRefVerticesBeforeDwords;
	Ar << Desc.NumPrevNewVerticesBeforeDwords;
	return Ar;
}
} // namespace Nanite
