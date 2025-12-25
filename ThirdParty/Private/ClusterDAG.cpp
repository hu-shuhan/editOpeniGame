// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClusterDAG.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "GraphPartitioner.h"
#include "BVHCluster.h"
#include "MeshSimplify.h"

namespace Nanite
{

void FClusterDAG::AddMesh(
	const FConstMeshBuildVertexView& Verts,
	TArrayView< const uint32 > Indexes,
	TArrayView< const int32 > MaterialIndexes,
	const FBounds3f& VertexBounds,
	const FVertexFormat& VertexFormat )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Nanite::Build::ClusterTriangles);

	uint32 Time0 = FPlatformTime::Cycles();

	LOG_CRC( Verts );
	LOG_CRC( Indexes );

	bHasSkinning	|= VertexFormat.NumBoneInfluences > 0;
	bHasTangents	|= VertexFormat.bHasTangents;
	bHasColors		|= VertexFormat.bHasColors;

	uint32 NumTriangles = Indexes.Num() / 3;

	FAdjacency Adjacency( Indexes.Num() );
	FEdgeHash EdgeHash( Indexes.Num() );

	auto GetPosition = [ &Verts, &Indexes ]( uint32 EdgeIndex )
	{
		return Verts.Position[ Indexes[ EdgeIndex ] ];
	};

	ParallelFor( TEXT("Nanite.ClusterTriangles.PF"), Indexes.Num(), 4096,
		[&]( int32 EdgeIndex )
		{
			EdgeHash.Add_Concurrent( EdgeIndex, GetPosition );
		} );

	ParallelFor( TEXT("Nanite.ClusterTriangles.PF"), Indexes.Num(), 1024,
		[&]( int32 EdgeIndex )
		{
			int32 AdjIndex = -1;
			int32 AdjCount = 0;
			EdgeHash.ForAllMatching( EdgeIndex, false, GetPosition,
				[&]( int32 EdgeIndex, int32 OtherEdgeIndex )
				{
					AdjIndex = OtherEdgeIndex;
					AdjCount++;
				} );

			if( AdjCount > 1 )
				AdjIndex = -2;

			Adjacency.Direct[ EdgeIndex ] = AdjIndex;
		} );

	FDisjointSet DisjointSet( NumTriangles );

	for( uint32 EdgeIndex = 0, Num = Indexes.Num(); EdgeIndex < Num; EdgeIndex++ )
	{
		if( Adjacency.Direct[ EdgeIndex ] == -2 )
		{
			// EdgeHash is built in parallel, so we need to sort before use to ensure determinism.
			// This path is only executed in the rare event that an edge is shared by more than two triangles,
			// so performance impact should be negligible in practice.
			TArray< TPair< int32, int32 >, TInlineAllocator< 16 > > Edges;
			EdgeHash.ForAllMatching( EdgeIndex, false, GetPosition,
				[&]( int32 EdgeIndex0, int32 EdgeIndex1 )
				{
					Edges.Emplace( EdgeIndex0, EdgeIndex1 );
				} );
			Edges.Sort();	
			
			for( const TPair< int32, int32 >& Edge : Edges )
			{
				Adjacency.Link( Edge.Key, Edge.Value );
			}
		}

		Adjacency.ForAll( EdgeIndex,
			[&]( int32 EdgeIndex0, int32 EdgeIndex1 )
			{
				if( EdgeIndex0 > EdgeIndex1 )
					DisjointSet.UnionSequential( EdgeIndex0 / 3, EdgeIndex1 / 3 );
			} );
	}

	uint32 BoundaryTime = FPlatformTime::Cycles();
	UE_LOG( LogStaticMesh, Log,
		TEXT("Adjacency [%.2fs], tris: %i, UVs %i%s%s"),
		FPlatformTime::ToMilliseconds( BoundaryTime - Time0 ) / 1000.0f,
		Indexes.Num() / 3,
		VertexFormat.NumTexCoords,
		VertexFormat.bHasTangents ? TEXT(", Tangents") : TEXT(""),
		VertexFormat.bHasColors ? TEXT(", Color") : TEXT("") );

#if 0//NANITE_VOXEL_DATA
	FBVHCluster Partitioner( NumTriangles, FCluster::ClusterSize - 4, FCluster::ClusterSize );
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(Nanite::Build::PartitionGraph);

		Partitioner.Build(
			[ &Verts, &Indexes ]( uint32 TriIndex )
			{
				FBounds3f Bounds;
				Bounds  = Verts.Position[ Indexes[ TriIndex * 3 + 0 ] ];
				Bounds += Verts.Position[ Indexes[ TriIndex * 3 + 1 ] ];
				Bounds += Verts.Position[ Indexes[ TriIndex * 3 + 2 ] ];
				return Bounds;
			} );

		check( Partitioner.Ranges.Num() );

		LOG_CRC( Partitioner.Ranges );
	}
#else
	FGraphPartitioner Partitioner( NumTriangles, FCluster::ClusterSize - 4, FCluster::ClusterSize );

	{
		TRACE_CPUPROFILER_EVENT_SCOPE(Nanite::Build::PartitionGraph);

		auto GetCenter = [ &Verts, &Indexes ]( uint32 TriIndex )
		{
			FVector3f Center;
			Center  = Verts.Position[ Indexes[ TriIndex * 3 + 0 ] ];
			Center += Verts.Position[ Indexes[ TriIndex * 3 + 1 ] ];
			Center += Verts.Position[ Indexes[ TriIndex * 3 + 2 ] ];
			return Center * (1.0f / 3.0f);
		};
		Partitioner.BuildLocalityLinks( DisjointSet, VertexBounds, MaterialIndexes, GetCenter );

		auto* RESTRICT Graph = Partitioner.NewGraph( NumTriangles * 3 );

		for( uint32 i = 0; i < NumTriangles; i++ )
		{
			Graph->AdjacencyOffset[i] = Graph->Adjacency.Num();

			uint32 TriIndex = Partitioner.Indexes[i];

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
		Graph->AdjacencyOffset[ NumTriangles ] = Graph->Adjacency.Num();

		bool bSingleThreaded = NumTriangles < 5000;

		Partitioner.PartitionStrict( Graph, !bSingleThreaded );
		check( Partitioner.Ranges.Num() );

		LOG_CRC( Partitioner.Ranges );
	}
#endif

	const uint32 OptimalNumClusters = FMath::DivideAndRoundUp< int32 >( Indexes.Num(), FCluster::ClusterSize * 3 );

	uint32 ClusterTime = FPlatformTime::Cycles();
	UE_LOG( LogStaticMesh, Log, TEXT("Clustering [%.2fs]. Ratio: %f"), FPlatformTime::ToMilliseconds( ClusterTime - BoundaryTime ) / 1000.0f, (float)Partitioner.Ranges.Num() / (float)OptimalNumClusters );

	const uint32 BaseCluster = Clusters.Num();
	Clusters.AddDefaulted( Partitioner.Ranges.Num() );

	{
		TRACE_CPUPROFILER_EVENT_SCOPE(Nanite::Build::BuildClusters);
		ParallelFor( TEXT("Nanite.BuildClusters.PF"), Partitioner.Ranges.Num(), 1024,
			[&]( int32 Index )
			{
				auto& Range = Partitioner.Ranges[ Index ];

				Clusters[ BaseCluster + Index ] = FCluster(
					Verts,
					Indexes,
					MaterialIndexes,
					VertexFormat,
					Range.Begin, Range.End,
					Partitioner.Indexes, Partitioner.SortedTo, Adjacency );

				// Negative notes it's a leaf
				Clusters[ BaseCluster + Index ].EdgeLength *= -1.0f;
			});
	}

	uint32 LeavesTime = FPlatformTime::Cycles();
	UE_LOG( LogStaticMesh, Log, TEXT("Leaves [%.2fs]"), FPlatformTime::ToMilliseconds( LeavesTime - ClusterTime ) / 1000.0f );
}

static const uint32 MinGroupSize = 8;
static const uint32 MaxGroupSize = 32;

void FClusterDAG::ReduceMesh( uint32 ClusterRangeStart, uint32 ClusterRangeNum, uint32 MeshIndex )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Nanite::Build::DAG.ReduceMesh);

	if( ClusterRangeNum == 0 )
	{
		return;
	}

	TUniquePtr<FRayTracingScene> RayTracingScene;

#if NANITE_VOXEL_DATA
	if( Settings.bPreserveArea )
	{
		RayTracingScene = MakeUnique< FRayTracingScene >( Clusters, ClusterRangeStart, ClusterRangeNum );
	}
#endif
	
	uint32 LevelOffset	= ClusterRangeStart;
	
	TAtomic< uint32 > NumClusters( Clusters.Num() );

	bool bFirstLevel = true;

	UE::Tasks::FCancellationToken* CancellationToken = UE::Tasks::FCancellationTokenScope::GetCurrentCancellationToken();
	while( true )
	{
		if (CancellationToken && CancellationToken->IsCanceled())
		{
			return;
		}

		TArrayView< FCluster > LevelClusters( &Clusters[ LevelOffset ], bFirstLevel ? ClusterRangeNum : (Clusters.Num() - LevelOffset) );
		bFirstLevel = false;

		uint32 NumExternalEdges = 0;

		float MinError = +MAX_flt;
		float MaxError = -MAX_flt;
		float AvgError = 0.0f;

		for( FCluster& Cluster : LevelClusters )
		{
			NumExternalEdges	+= Cluster.NumExternalEdges;
			TotalBounds			+= Cluster.Bounds;

			MinError = FMath::Min( MinError, Cluster.LODError );
			MaxError = FMath::Max( MaxError, Cluster.LODError );
			AvgError += Cluster.LODError;
		}
		AvgError /= (float)LevelClusters.Num();

		UE_LOG( LogStaticMesh, Verbose, TEXT("Num clusters %i. Error %.4f, %.4f, %.4f"), LevelClusters.Num(), MinError, AvgError, MaxError );

		uint32 MaxClusterSize = FCluster::ClusterSize;
		if( LevelClusters.Num() < 2 )
		{
			if( LevelClusters[0].NumTris )
			{
				break;
			}
			else if( LevelClusters[0].MaterialIndexes.Num() > 64 )
			{
				MaxClusterSize = 64;
			}
			else if( LevelClusters[0].MaterialIndexes.Num() > 32 )
			{
				MaxClusterSize = 32;
			}
			else
			{
				break;
			}
		}
		
		if( LevelClusters.Num() <= MaxGroupSize )
		{
			TArray< uint32, TInlineAllocator< MaxGroupSize > > Children;

			uint32 NumGroupElements = 0;
			for( FCluster& Cluster : LevelClusters )
			{
				NumGroupElements  += Cluster.MaterialIndexes.Num();
				Children.Add( LevelOffset++ );
			}
			uint32 MaxParents = FMath::DivideAndRoundUp( NumGroupElements, MaxClusterSize * 2 );

			LevelOffset = Clusters.Num();
			Clusters.AddDefaulted( MaxParents );
			Groups.AddDefaulted( 1 );

			ReduceGroup( RayTracingScene.Get(), NumClusters, Children, MaxClusterSize, MaxParents, Groups.Num() - 1, MeshIndex );

			check( LevelOffset < NumClusters );

			// Correct num to atomic count
			Clusters.SetNum( NumClusters, EAllowShrinking::No );

			continue;
		}
		
		struct FExternalEdge
		{
			uint32	ClusterIndex;
			int32	EdgeIndex;
		};
		TArray< FExternalEdge >	ExternalEdges;
		FHashTable				ExternalEdgeHash;
		TAtomic< uint32 >		ExternalEdgeOffset(0);

		// We have a total count of NumExternalEdges so we can allocate a hash table without growing.
		ExternalEdges.AddUninitialized( NumExternalEdges );
		ExternalEdgeHash.Clear( 1 << FMath::FloorLog2( NumExternalEdges ), NumExternalEdges );

		// Add edges to hash table
		ParallelFor( TEXT("Nanite.BuildDAG.PF"), LevelClusters.Num(), 32,
			[&]( uint32 ClusterIndex )
			{
				FCluster& Cluster = LevelClusters[ ClusterIndex ];

				if (CancellationToken && CancellationToken->IsCanceled())
				{
					return;
				}

				for( int32 EdgeIndex = 0; EdgeIndex < Cluster.ExternalEdges.Num(); EdgeIndex++ )
				{
					if( Cluster.ExternalEdges[ EdgeIndex ] )
					{
						uint32 VertIndex0 = Cluster.Indexes[ EdgeIndex ];
						uint32 VertIndex1 = Cluster.Indexes[ Cycle3( EdgeIndex ) ];
	
						const FVector3f& Position0 = Cluster.GetPosition( VertIndex0 );
						const FVector3f& Position1 = Cluster.GetPosition( VertIndex1 );

						uint32 Hash0 = HashPosition( Position0 );
						uint32 Hash1 = HashPosition( Position1 );
						uint32 Hash = Murmur32( { Hash0, Hash1 } );

						uint32 ExternalEdgeIndex = ExternalEdgeOffset++;
						ExternalEdges[ ExternalEdgeIndex ] = { ClusterIndex, EdgeIndex };
						ExternalEdgeHash.Add_Concurrent( Hash, ExternalEdgeIndex );
					}
				}
			});

		if (CancellationToken && CancellationToken->IsCanceled())
		{
			return;
		}

		check( ExternalEdgeOffset == ExternalEdges.Num() );

		TAtomic< uint32 > NumAdjacency(0);

		// Find matching edge in other clusters
		ParallelFor( TEXT("Nanite.BuildDAG.PF"), LevelClusters.Num(), 32,
			[&]( uint32 ClusterIndex )
			{
				FCluster& Cluster = LevelClusters[ ClusterIndex ];

				if (CancellationToken && CancellationToken->IsCanceled())
				{
					return;
				}

				for( int32 EdgeIndex = 0; EdgeIndex < Cluster.ExternalEdges.Num(); EdgeIndex++ )
				{
					if( Cluster.ExternalEdges[ EdgeIndex ] )
					{
						uint32 VertIndex0 = Cluster.Indexes[ EdgeIndex ];
						uint32 VertIndex1 = Cluster.Indexes[ Cycle3( EdgeIndex ) ];
	
						const FVector3f& Position0 = Cluster.GetPosition( VertIndex0 );
						const FVector3f& Position1 = Cluster.GetPosition( VertIndex1 );

						uint32 Hash0 = HashPosition( Position0 );
						uint32 Hash1 = HashPosition( Position1 );
						uint32 Hash = Murmur32( { Hash1, Hash0 } );

						for( uint32 ExternalEdgeIndex = ExternalEdgeHash.First( Hash ); ExternalEdgeHash.IsValid( ExternalEdgeIndex ); ExternalEdgeIndex = ExternalEdgeHash.Next( ExternalEdgeIndex ) )
						{
							FExternalEdge ExternalEdge = ExternalEdges[ ExternalEdgeIndex ];

							FCluster& OtherCluster = LevelClusters[ ExternalEdge.ClusterIndex ];

							if( OtherCluster.ExternalEdges[ ExternalEdge.EdgeIndex ] )
							{
								uint32 OtherVertIndex0 = OtherCluster.Indexes[ ExternalEdge.EdgeIndex ];
								uint32 OtherVertIndex1 = OtherCluster.Indexes[ Cycle3( ExternalEdge.EdgeIndex ) ];
			
								if( Position0 == OtherCluster.GetPosition( OtherVertIndex1 ) &&
									Position1 == OtherCluster.GetPosition( OtherVertIndex0 ) )
								{
									if( ClusterIndex != ExternalEdge.ClusterIndex )
									{
										// Increase it's count
										Cluster.AdjacentClusters.FindOrAdd( ExternalEdge.ClusterIndex, 0 )++;

										// Can't break or a triple edge might be non-deterministically connected.
										// Need to find all matching, not just first.
									}
								}
							}
						}
					}
				}
				NumAdjacency += Cluster.AdjacentClusters.Num();

				// Force deterministic order of adjacency.
				Cluster.AdjacentClusters.KeySort(
					[ &LevelClusters ]( uint32 A, uint32 B )
					{
						return LevelClusters[A].GUID < LevelClusters[B].GUID;
					} );
			});

		if (CancellationToken && CancellationToken->IsCanceled())
		{
			return;
		}

		FDisjointSet DisjointSet( LevelClusters.Num() );

		for( uint32 ClusterIndex = 0; ClusterIndex < (uint32)LevelClusters.Num(); ClusterIndex++ )
		{
			for( auto& Pair : LevelClusters[ ClusterIndex ].AdjacentClusters )
			{
				uint32 OtherClusterIndex = Pair.Key;

				uint32 Count = LevelClusters[ OtherClusterIndex ].AdjacentClusters.FindChecked( ClusterIndex );
				check( Count == Pair.Value );

				if( ClusterIndex > OtherClusterIndex )
				{
					DisjointSet.UnionSequential( ClusterIndex, OtherClusterIndex );
				}
			}
		}

		FGraphPartitioner Partitioner( LevelClusters.Num(), MinGroupSize, MaxGroupSize );

		auto GetCenter = [&]( uint32 Index )
		{
			FBounds3f& Bounds = LevelClusters[ Index ].Bounds;
			return 0.5f * ( Bounds.Min + Bounds.Max );
		};
		Partitioner.BuildLocalityLinks( DisjointSet, TotalBounds, TArrayView< const int32 >(), GetCenter );

		if (CancellationToken && CancellationToken->IsCanceled())
		{
			return;
		}

		auto* RESTRICT Graph = Partitioner.NewGraph( NumAdjacency );

		for( int32 i = 0; i < LevelClusters.Num(); i++ )
		{
			Graph->AdjacencyOffset[i] = Graph->Adjacency.Num();

			uint32 ClusterIndex = Partitioner.Indexes[i];

			for( auto& Pair : LevelClusters[ ClusterIndex ].AdjacentClusters )
			{
				uint32 OtherClusterIndex = Pair.Key;
				uint32 NumSharedEdges = Pair.Value;

				const auto& Cluster0 = Clusters[ LevelOffset + ClusterIndex ];
				const auto& Cluster1 = Clusters[ LevelOffset + OtherClusterIndex ];

				bool bSiblings = Cluster0.GroupIndex != MAX_uint32 && Cluster0.GroupIndex == Cluster1.GroupIndex;

				Partitioner.AddAdjacency( Graph, OtherClusterIndex, NumSharedEdges * ( bSiblings ? 1 : 16 ) + 4 );
			}

			Partitioner.AddLocalityLinks( Graph, ClusterIndex, 1 );
		}
		Graph->AdjacencyOffset[ Graph->Num ] = Graph->Adjacency.Num();

		LOG_CRC( Graph->Adjacency );
		LOG_CRC( Graph->AdjacencyCost );
		LOG_CRC( Graph->AdjacencyOffset );
		
		bool bSingleThreaded = LevelClusters.Num() <= 32;

		Partitioner.PartitionStrict( Graph, !bSingleThreaded );

		LOG_CRC( Partitioner.Ranges );

		uint32 MaxParents = 0;
		for( auto& Range : Partitioner.Ranges )
		{
			uint32 NumGroupElements = 0;
			for( uint32 i = Range.Begin; i < Range.End; i++ )
			{
				// Global indexing is needed in Reduce()
				Partitioner.Indexes[i] += LevelOffset;
				NumGroupElements += Clusters[ Partitioner.Indexes[i] ].MaterialIndexes.Num();
			}
			MaxParents += FMath::DivideAndRoundUp( NumGroupElements, MaxClusterSize * 2 );
		}

		LevelOffset = Clusters.Num();

		Clusters.AddDefaulted( MaxParents );
		Groups.AddDefaulted( Partitioner.Ranges.Num() );

		ParallelFor( TEXT("Nanite.BuildDAG.PF"), Partitioner.Ranges.Num(), 1,
			[&]( int32 PartitionIndex )
			{
				if (CancellationToken && CancellationToken->IsCanceled())
				{
					return;
				}

				auto& Range = Partitioner.Ranges[ PartitionIndex ];

				TArrayView< uint32 > Children( &Partitioner.Indexes[ Range.Begin ], Range.End - Range.Begin );

				uint32 NumGroupElements = 0;
				for( uint32 i = Range.Begin; i < Range.End; i++ )
				{
					NumGroupElements += Clusters[ Partitioner.Indexes[i] ].MaterialIndexes.Num();
				}
				uint32 MaxParents = FMath::DivideAndRoundUp( NumGroupElements, MaxClusterSize * 2 );
				uint32 ClusterGroupIndex = PartitionIndex + Groups.Num() - Partitioner.Ranges.Num();

				ReduceGroup( RayTracingScene.Get(), NumClusters, Children, MaxClusterSize, MaxParents, ClusterGroupIndex, MeshIndex );
			} );

		if (CancellationToken && CancellationToken->IsCanceled())
		{
			return;
		}

		check( LevelOffset < NumClusters );

		// Correct num to atomic count
		Clusters.SetNum( NumClusters, EAllowShrinking::No );

		// Force a deterministic order of the generated parent clusters
		{
			// TODO: Optimize me.
			// Just sorting the array directly seems like the safest option at this stage (right before UE5 final build).
			// On AOD_Shield this seems to be on the order of 0.01s in practice.
			// As the Clusters array is already conservatively allocated, it seems storing the parent clusters in their designated
			// conservative ranges and then doing a compaction pass at the end would be a more efficient solution that doesn't involve sorting.
			
			//uint32 StartTime = FPlatformTime::Cycles();
			TArrayView< FCluster > Parents( &Clusters[ LevelOffset ], Clusters.Num() - LevelOffset );
			Parents.Sort(
				[&]( const FCluster& A, const FCluster& B )
				{
					return A.GUID < B.GUID;
				} );
			//UE_LOG(LogStaticMesh, Log, TEXT("SortTime Adjacency [%.2fs]"), FPlatformTime::ToMilliseconds(FPlatformTime::Cycles() - StartTime) / 1000.0f);
		}
		
	}

#if RAY_TRACE_VOXELS
	for( FCluster& Cluster : Clusters )
	{
		Cluster.ExtraVoxels.Empty();	// VOXELTODO: Free this earlier
	}
#endif
	
	// Max out root node
	uint32 RootIndex = LevelOffset;
	FClusterGroup RootClusterGroup;
	RootClusterGroup.Children.Add( RootIndex );
	RootClusterGroup.Bounds				= Clusters[ RootIndex ].SphereBounds;
	RootClusterGroup.LODBounds			= FSphere3f( 0 );
	RootClusterGroup.MaxParentLODError	= 1e10f;
	RootClusterGroup.MinLODError		= -1.0f;
	RootClusterGroup.MipLevel			= Clusters[ RootIndex ].MipLevel + 1;
	RootClusterGroup.MeshIndex			= MeshIndex;
	RootClusterGroup.AssemblyPartIndex	= MAX_uint32;
	RootClusterGroup.bTrimmed			= false;
	Clusters[ RootIndex ].GroupIndex = Groups.Num();
	Groups.Add( RootClusterGroup );
}


float InverseLerp(
	float y,
	float x0, float y0,
	float x1, float y1 )
{
	return ( x0 * (y1 - y) - x1 * (y0 - y) ) / ( y1 - y0 );
}

float InverseLerp(
	float y,
	float x0, float y0,
	float x1, float y1,
	float x2, float y2 )
{
	// Inverse quadratic interpolation
#if 0
	float a = (y0 - y) * (x1 - x0) * (y1 - y2);
	float b = (y1 - y) * (x1 - x2) * (x1 - x0) * (y2 - y0);
	float c = (y2 - y) * (x1 - x2) * (y0 - y1);
				
	return x1 + b / (a + c);
#else
	return
		(y - y1) * (y - y2) * x0 / ( (y0 - y1) * (y0 - y2) ) +
		(y - y2) * (y - y0) * x1 / ( (y1 - y2) * (y1 - y0) ) +
		(y - y0) * (y - y1) * x2 / ( (y2 - y0) * (y2 - y1) );
#endif
}

// Brent's method
template< typename FuncType >
float BrentRootFind(
	float y, float Tolerance,
	float xA, float yA,
	float xB, float yB,
	float xGuess, bool bInitialGuess,
	int32 MaxIter,
	FuncType&& Func )
{
	if( FMath::Abs( yA - y ) < FMath::Abs( yB - y ) )
	{
		Swap( xA, xB );
		Swap( yA, yB );
	}

	float xC = xA;
	float yC = yA;
	float xD = xA;

	bool bBisection = true;

	for( int32 i = 0; i < MaxIter; i++ )
	{
		if( FMath::Abs( xB - xA ) < SMALL_NUMBER ||
			FMath::Abs( yB - y ) <= Tolerance )
			break;

		if( yC != yA && yC != yB )
		{
			xGuess = InverseLerp(
				y,
				xA, yA,
				xB, yB,
				xC, yC );
		}
		else if( !bInitialGuess )
		{
			xGuess = InverseLerp(
				y,
				xA, yA,
				xB, yB );
		}
		bInitialGuess = false;

		if( bBisection )
		{
			bBisection =
				FMath::Abs( xGuess - xB ) >= 0.5f * FMath::Abs( xB - xC ) ||
				FMath::Abs( xB - xC ) < SMALL_NUMBER;
		}
		else
		{
			bBisection =
				FMath::Abs( xGuess - xB ) >= 0.5f * FMath::Abs( xC - xD ) ||
				FMath::Abs( xC - xD ) < SMALL_NUMBER;
		}

		// Outside of interval
		if( ( xGuess - ( 0.75f * xA + 0.25f * xB ) ) * ( xGuess - xB ) >= 0.0f )
			bBisection = true;

		if( bBisection )
			xGuess = 0.5f * ( xA + xB );

		float yGuess = Func( xGuess );

		xD = xC;
		xC = xB;
		yC = yB;

		if( ( yA - y ) * ( yGuess - y ) < 0.0f )
		{
			xB = xGuess;
			yB = yGuess;
		}
		else
		{
			xA = xGuess;
			yA = yGuess;
		}

		if( FMath::Abs( yA - y ) < FMath::Abs( yB - y ) )
		{
			Swap( xA, xB );
			Swap( yA, yB );
		}
	}

	return xB;
}

template< typename FPartitioner, typename FPartitionFunc >
bool SplitCluster( FCluster& Merged, TArray< FCluster >& Clusters, TAtomic< uint32 >& NumClusters, uint32 MaxClusterSize, uint32& NumParents, uint32& ParentStart, uint32& ParentEnd, FPartitionFunc&& PartitionFunc )
{
	if( Merged.MaterialIndexes.Num() <= (int32)MaxClusterSize )
	{
		ParentEnd = ( NumClusters += 1 );
		ParentStart = ParentEnd - 1;

		Clusters[ ParentStart ] = Merged;
		Clusters[ ParentStart ].Bound();
		return true;
	}
	else if( NumParents > 1 )
	{
		check( MaxClusterSize == FCluster::ClusterSize );

		FAdjacency Adjacency = Merged.BuildAdjacency();
		FPartitioner Partitioner( Merged.MaterialIndexes.Num(), MaxClusterSize - 4, MaxClusterSize );
		PartitionFunc( Partitioner, Adjacency );

		if( Partitioner.Ranges.Num() <= (int32)NumParents )
		{
			NumParents = Partitioner.Ranges.Num();
			ParentEnd = ( NumClusters += NumParents );
			ParentStart = ParentEnd - NumParents;

			int32 Parent = ParentStart;
			for( auto& Range : Partitioner.Ranges )
			{
				Clusters[ Parent ] = FCluster( Merged, Range.Begin, Range.End, Partitioner.Indexes, Partitioner.SortedTo, Adjacency );
				Parent++;
			}

			return true;
		}
	}

	return false;
}

void FClusterDAG::ReduceGroup( FRayTracingScene* RayTracingScene, TAtomic< uint32 >& NumClusters, TArrayView< uint32 > Children, uint32 MaxClusterSize, uint32 NumParents, int32 GroupIndex, uint32 MeshIndex )
{
	check( GroupIndex >= 0 );

	bool bAnyTriangles = false;
	bool bAllTriangles = true;

	TArray< FSphere3f, TInlineAllocator< MaxGroupSize > > Children_LODBounds;
	TArray< FSphere3f, TInlineAllocator< MaxGroupSize > > Children_SphereBounds;

	float ChildMinLODError = MAX_flt;
	float ChildMaxLODError = 0.0f;
	for( uint32 Child : Children )
	{
		FCluster& Cluster = Clusters[ Child ];

		bAnyTriangles = bAnyTriangles || Cluster.NumTris > 0;
		bAllTriangles = bAllTriangles && Cluster.NumTris > 0;

		bool bLeaf = Cluster.EdgeLength < 0.0f;
		float LODError = Cluster.LODError;

		// Force monotonic nesting.
		Children_LODBounds.Add( Cluster.LODBounds );
		Children_SphereBounds.Add( Cluster.SphereBounds );
		ChildMinLODError = FMath::Min( ChildMinLODError, bLeaf ? -1.0f : LODError );
		ChildMaxLODError = FMath::Max( ChildMaxLODError, LODError );

		Cluster.GroupIndex = GroupIndex;
		Groups[ GroupIndex ].Children.Add( Child );
		check( Groups[ GroupIndex ].Children.Num() <= NANITE_MAX_CLUSTERS_PER_GROUP_TARGET );
	}
	
	FSphere3f ParentLODBounds( Children_LODBounds.GetData(), Children_LODBounds.Num() );
	FSphere3f ParentBounds( Children_SphereBounds.GetData(), Children_SphereBounds.Num() );

	uint32 ParentStart = 0;
	uint32 ParentEnd = 0;

	FCluster Merged;
	float SimplifyError = MAX_flt;

	bool bVoxels = false;
#if NANITE_VOXEL_DATA
	FCluster& FirstCluster = Clusters[ Children[0] ];
	bVoxels = !bAllTriangles || Settings.bPreserveArea;
#endif

	uint32 TargetClusterSize = MaxClusterSize - 2;
	if( bAllTriangles )
	{
		uint32 TargetNumTris = NumParents * TargetClusterSize;

	#if NANITE_VOXEL_DATA
		if( !bVoxels ||
			Settings.VoxelLevel == 0 ||
			Settings.VoxelLevel > FirstCluster.MipLevel + 1 )
	#endif
		{
			Merged = FCluster( *this, Children );
			SimplifyError = Merged.Simplify( *this, TargetNumTris );
		}
	}

#if NANITE_VOXEL_DATA
	if( bVoxels )
	{
		uint32 TotalVerts = 0;
		float SurfaceArea = 0.0f;
		for( uint32 Child : Children )
		{
			TotalVerts	+= Clusters[ Child ].NumVerts;
			SurfaceArea	+= Clusters[ Child ].SurfaceArea;
		}

		int32 TargetNumBricks = NumParents * MaxClusterSize;
		//uint32 TargetNumVoxels = TargetNumBricks * 16;
		uint32 TargetNumVoxels = ( TotalVerts * 3 ) / 4;

		float VoxelSize = FMath::Sqrt( SurfaceArea / TargetNumVoxels );
		VoxelSize *= 0.75f;

		VoxelSize = FMath::Max( VoxelSize, ChildMaxLODError );

	#if 0
		// Round to pow2
		// = exp2( floor( log2(x) + 0.5 ) )
		FFloat32 VoxelSizeF( VoxelSize * UE_SQRT_2 );
		VoxelSizeF.Components.Mantissa = 0;
		VoxelSize = VoxelSizeF.FloatValue;
	#endif

		float EstimatedVoxelSize = VoxelSize;

		while( VoxelSize < SimplifyError )
		{
			FCluster Voxelized;
			Voxelized.Voxelize( *this, *RayTracingScene, Children, VoxelSize );

			if( Voxelized.NumVerts		< TargetNumVoxels &&
				Voxelized.Bricks.Num()	< TargetNumBricks )
			{
				bool bSplitSuccess = SplitCluster< FBVHCluster >( Voxelized, Clusters, NumClusters, MaxClusterSize, NumParents, ParentStart, ParentEnd,
					[ &Voxelized ]( FBVHCluster& Partitioner, FAdjacency& Adjacency )
					{
						Partitioner.Build(
							[ &Voxelized ]( uint32 VertIndex )
							{
								FBounds3f Bounds;
								Bounds = FVector3f( Voxelized.Bricks[ VertIndex ].Position );
								return Bounds;
							} );
					} );

			#if RAY_TRACE_VOXELS
				if( Voxelized.NumTris == 0 )
				{
					// Distribute extra voxels to closest clusters
					for( const FVector3f& Position : Voxelized.ExtraVoxels )
					{	
						float BestDistance = MAX_flt;
						uint32 BestParentIndex = 0xFFFFFFFFu;
						for( uint32 ParentIndex = ParentStart; ParentIndex < ParentEnd; ParentIndex++ )
						{
							FVector3f BoundsCenter = Clusters[ ParentIndex ].Bounds.GetCenter();
							float Distance = ( Position - BoundsCenter ).GetAbsMax();
							if( Distance < BestDistance )
							{
								BestDistance = Distance;
								BestParentIndex = ParentIndex;
							}
						}

						Clusters[ BestParentIndex ].ExtraVoxels.Add( Position );
					}
				}
			#endif

				check( bSplitSuccess );
				break;
			}

			VoxelSize *= 1.1f;
		}

		if( VoxelSize < SimplifyError )
			SimplifyError = VoxelSize;
		else
			bVoxels = false;
	}
#endif

	if( !bVoxels )
	{
		check( bAllTriangles );

		while(1)
		{
			bool bSplitSuccess = SplitCluster< FGraphPartitioner >( Merged, Clusters, NumClusters, MaxClusterSize, NumParents, ParentStart, ParentEnd,
				[ &Merged ]( FGraphPartitioner& Partitioner, FAdjacency& Adjacency )
				{
					Merged.Split( Partitioner, Adjacency );
				} );

			if( bSplitSuccess )
				break;

			TargetClusterSize -= 2;
			if( TargetClusterSize <= MaxClusterSize / 2 )
				break;

			uint32 TargetNumTris = NumParents * TargetClusterSize;

			// Start over from scratch. Continuing from simplified cluster screws up ExternalEdges and LODError.
			Merged = FCluster( *this, Children );
			SimplifyError = Merged.Simplify( *this, TargetNumTris );
		}
	}

	float ParentMaxLODError = FMath::Max( ChildMaxLODError, SimplifyError );

	// Force parents to have same LOD data. They are all dependent.
	for( uint32 Parent = ParentStart; Parent < ParentEnd; Parent++ )
	{
		Clusters[ Parent ].LODBounds			= ParentLODBounds;
		Clusters[ Parent ].LODError				= ParentMaxLODError;
		Clusters[ Parent ].GeneratingGroupIndex = GroupIndex;
	}

	Groups[ GroupIndex ].Bounds				= ParentBounds;
	Groups[ GroupIndex ].LODBounds			= ParentLODBounds;
	Groups[ GroupIndex ].MinLODError		= ChildMinLODError;
	Groups[ GroupIndex ].MaxParentLODError	= ParentMaxLODError;
	Groups[ GroupIndex ].MipLevel			= Clusters[ Children[0] ].MipLevel;
	Groups[ GroupIndex ].MeshIndex			= MeshIndex;
	Groups[ GroupIndex ].AssemblyPartIndex	= MAX_uint32;
	Groups[ GroupIndex ].bTrimmed			= false;
}

FBinaryHeap< float > FClusterDAG::FindCut(
	uint32 TargetNumTris,
	float  TargetError,
	uint32 TargetOvershoot,
	TBitArray<>* SelectedGroupsMask ) const
{
	const FClusterGroup&	RootGroup = Groups.Last();
	const FCluster&			RootCluster = Clusters[ RootGroup.Children[0] ];

	bool bHitTargetBefore = false;

	float MinError = RootCluster.LODError;

	TBitArray<> VisitedGroups;
	VisitedGroups.Init(false, Groups.Num());
	VisitedGroups[Groups.Num() - 1] = true;
	
	FBinaryHeap< float > Heap;
	Heap.Add( -RootCluster.LODError, RootGroup.Children[0] );

	uint32 CurNumTris = RootCluster.NumTris;

	while( true )
	{
		// Grab highest error cluster to replace to reduce cut error
		const uint32 ClusterIndex = Heap.Top();
		const FCluster& Cluster = Clusters[ ClusterIndex ];
		const FClusterGroup& Group = Groups[ Cluster.GroupIndex ];
		const uint32 NumInstances = Group.AssemblyPartIndex == MAX_uint32 ? 1u : AssemblyPartData[ Group.AssemblyPartIndex ].NumTransforms;

		if( Cluster.MipLevel == 0 )
			break;
		if( Cluster.GeneratingGroupIndex == MAX_uint32 )
			break;

		bool bHitTarget = CurNumTris > TargetNumTris || MinError < TargetError;

		// Overshoot the target by TargetOvershoot number of triangles. This allows granular edge collapses to better minimize error to the target.
		if( TargetOvershoot > 0 && bHitTarget && !bHitTargetBefore )
		{
			TargetNumTris = CurNumTris + TargetOvershoot;
			bHitTarget = false;
			bHitTargetBefore = true;
		}

		if( bHitTarget && Cluster.LODError < MinError )
			break;
		
		Heap.Pop();
		CurNumTris -= Cluster.NumTris * NumInstances;

		check( Cluster.LODError <= MinError );
		MinError = Cluster.LODError;

		if (VisitedGroups[Cluster.GeneratingGroupIndex])
		{
			continue;
		}
		VisitedGroups[Cluster.GeneratingGroupIndex] = true;

		const FClusterGroup& NextGroup = Groups[ Cluster.GeneratingGroupIndex ];
		const uint32 NextNumInstances = NextGroup.AssemblyPartIndex == MAX_uint32 ? 1u : AssemblyPartData[ NextGroup.AssemblyPartIndex ].NumTransforms;

		for( uint32 Child : NextGroup.Children )
		{
			if( !Heap.IsPresent( Child ) )
			{
				const FCluster& ChildCluster = Clusters[ Child ];

				check( ChildCluster.MipLevel < Cluster.MipLevel );
				check( ChildCluster.LODError <= MinError );
				Heap.Add( -ChildCluster.LODError, Child );
				CurNumTris += ChildCluster.NumTris * NextNumInstances;
			}
		}

		// TODO: Nanite-Assemblies: Double-check this. I think we have to handle the case where we cross the threshold from the mip tail
		// into the lower mips of assembly parts. I believe it's possible otherwise to get into a situation where some mip tail clusters
		// that were generated by assembly parts are still present on the heap and now overlap with an instanced, higher LOD of the part.
		// Maybe this can be solved simply by detecting when we're crossing that threshold here and removing all clusters from the heap
		// whose generating group == NextGroup like this? Not sure if it covers all cases though.
		if (Group.AssemblyPartIndex == MAX_uint32 && NextGroup.AssemblyPartIndex != MAX_uint32)
		{
			for (int32 OtherGroupIndex = 0; OtherGroupIndex < Groups.Num(); ++OtherGroupIndex)
			{
				const FClusterGroup& OtherGroup = Groups[OtherGroupIndex];
				if (OtherGroup.MipLevel < Group.MipLevel)
				{
					// Skip over higher mip groups
					continue;
				}
				
				for (uint32 OtherClusterIndex : OtherGroup.Children)
				{
					const FCluster& OtherCluster = Clusters[OtherClusterIndex];
					if (Heap.IsPresent(OtherClusterIndex) &&
						OtherCluster.GeneratingGroupIndex == Cluster.GeneratingGroupIndex)
					{
						Heap.Remove(OtherClusterIndex);
						CurNumTris -= OtherCluster.NumTris;
					}
				}
			}
		}
	}

	if (SelectedGroupsMask)
	{
		*SelectedGroupsMask = MoveTemp(VisitedGroups);
	}

	return Heap;
}

} // namespace Nanite
