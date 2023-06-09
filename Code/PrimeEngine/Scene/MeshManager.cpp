// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "MeshManager.h"
// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/FileSystem/FileReader.h"
#include "PrimeEngine/APIAbstraction/GPUMaterial/GPUMaterialSet.h"
#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PrimeEngine/APIAbstraction/Texture/Texture.h"
#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"
#include "PrimeEngine/APIAbstraction/GPUBuffers/VertexBufferGPUManager.h"
#include "PrimeEngine/../../GlobalConfig/GlobalConfig.h"
#include "PrimeEngine/Scene/DebugRenderer.h"

#include "PrimeEngine/Geometry/SkeletonCPU/SkeletonCPU.h"

#include "PrimeEngine/Scene/RootSceneNode.h"

#include "Light.h"

// Sibling/Children includes

#include "MeshInstance.h"
#include "Skeleton.h"
#include "SceneNode.h"
#include "DrawList.h"
#include "SH_DRAW.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"

#define POSINFINITY std::numeric_limits<PrimitiveTypes::Float32>::max()
#define NEGINFINITY std::numeric_limits<PrimitiveTypes::Float32>::min()

namespace PE {
namespace Components{

PE_IMPLEMENT_CLASS1(MeshManager, Component);
MeshManager::MeshManager(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself)
	: Component(context, arena, hMyself)
	, m_assets(context, arena, 256)
{
}

void MeshManager::getBox(const char* asset, const char* package, int& threadOwnershipMask, float* floatarr, Vector3* maxVerts, bool* hasExtremes) {

	MeshCPU mcpu(*m_pContext, m_arena);
	mcpu.ReadMesh(asset, package, "");
	//Retrieve the mesh's PositionBufferCPU that stores it vertices
//I'm honestly not sure this is the right array for the vertices, but it's divisible by 3. I'm going to assume it is for now
	PositionBufferCPU* p = (PositionBufferCPU*)mcpu.m_hPositionBufferCPU.getObject();
	PrimitiveTypes::Float32* verts = p->m_values.getFirstPtr();


	//OutputDebugStringW(L"Test value");

	PrimitiveTypes::Float32 maxVert[3] = { NEGINFINITY, NEGINFINITY, NEGINFINITY };
	PrimitiveTypes::Float32 minVert[3] = { POSINFINITY, POSINFINITY, POSINFINITY };
	PrimitiveTypes::UInt32 numVerts = p->m_values.m_size; // how much is stored at the moment

	for (int i = 0; i < numVerts; i += 3) {
		maxVert[0] = max(maxVert[0], verts[i]);
		maxVert[1] = max(maxVert[1], verts[i + 1]);
		maxVert[2] = max(maxVert[2], verts[i + 2]);

		minVert[0] = min(minVert[0], verts[i]);
		minVert[1] = min(minVert[1], verts[i + 1]);
		minVert[2] = min(minVert[2], verts[i + 2]);
	}

	Vector3 topForwardLeft = { minVert[0], maxVert[1], minVert[2] };
	Vector3 topForwardRight = { maxVert[0], maxVert[1], minVert[2] };
	Vector3 topBackLeft = { minVert[0], maxVert[1],maxVert[2] };
	Vector3 topBackRight = { maxVert[0], maxVert[1],maxVert[2] };

	Vector3 bottomForwardLeft = { minVert[0], minVert[1],minVert[2] };
	Vector3 bottomForwardRight = { maxVert[0], minVert[1],minVert[2] };
	Vector3 bottomBackLeft = { minVert[0], minVert[1],maxVert[2] };
	Vector3 bottomBackRight = { maxVert[0], minVert[1],maxVert[2] };

	if (!(*hasExtremes)) {
		maxVerts[0] = topForwardLeft;
		maxVerts[1] = topForwardRight;
		maxVerts[2] = topBackLeft;
		maxVerts[3] = topBackRight;
		maxVerts[4] = bottomForwardLeft;
		maxVerts[5] = bottomForwardRight;
		maxVerts[6] = bottomBackLeft;
		maxVerts[7] = bottomBackRight;

		(*hasExtremes) = true;
	}

	Vector3 color = { 255, 255, 255 };

	//The points needed for a box are...
	   //Top: (xMin, zMin, yMax), (xMin, zMax, yMax), (xMax, zMin, yMax), (xMax, zMax, yMax)
	   //Bottom: (xMin, zMin, yMax), 
	/*
	Vector3 posArray[12] = {
		topForwardLeft, topForwardLeft, topForwardRight, topBackLeft,
		topForwardLeft, topForwardRight, topBackLeft, topBackRight,
		bottomForwardLeft, bottomForwardLeft, bottomForwardRight, bottomBackLeft
	};

	Vector3 targetArray[12] = {
		topForwardRight, topBackLeft, topBackRight, topBackRight,
		bottomForwardLeft, bottomForwardRight, bottomBackLeft, bottomBackRight,
		bottomForwardRight, bottomBackLeft, bottomBackRight, bottomBackRight
	};

	Vector3 colorArray[12] = {
		color, color, color, color,
		color, color, color, color,
		color, color, color, color
	};*/

	Vector3 pointArray[48] = {
		topForwardLeft, color, topForwardRight, color, topForwardLeft, color, topBackLeft, color, topForwardRight, color, topBackRight, color, topBackLeft, color, topBackRight, color,
		topForwardLeft, color, bottomForwardLeft, color, topForwardRight, color, bottomForwardRight, color, topBackLeft, color, bottomBackLeft, color, topBackRight, color, bottomBackRight, color,
		bottomForwardLeft, color, bottomForwardRight, color, bottomForwardLeft, color, bottomBackLeft, color, bottomForwardRight, color, bottomBackRight, color, bottomBackLeft, color, bottomBackRight, color
	};

	Vector3 pointArray2[] = {
		topForwardLeft, topForwardRight, topForwardLeft, topBackLeft, topForwardRight, topBackRight, topBackLeft, topBackRight,
		topForwardLeft, bottomForwardLeft, topForwardRight, bottomForwardRight, topBackLeft, bottomBackLeft, topBackRight, bottomBackRight,
		bottomForwardLeft, bottomForwardRight, bottomForwardLeft, bottomBackLeft, bottomForwardRight, bottomBackRight, bottomBackLeft, bottomBackRight

	};

	Vector3 pointArray3[] = {
	topForwardLeft, color, topForwardRight, color//, topForwardLeft, color, topBackLeft, color, topForwardRight, color, topBackRight, color, topBackLeft, color, topBackRight, color,
	//topForwardLeft, color, bottomForwardLeft, color, topForwardRight, color, bottomForwardRight, color, topBackLeft, color, bottomBackLeft, color, topBackRight, color, bottomBackRight, color,
	//bottomForwardLeft, color, bottomForwardRight, color, bottomForwardLeft, color, bottomBackLeft, color, bottomForwardRight, color, bottomBackRight, color, bottomBackLeft, color, bottomBackRight, color

	};

	Vector3 pointArray4[] = {
	Vector3(0,2,0), color, Vector3(20,2,0), color//, topForwardLeft, color, topBackLeft, color, topForwardRight, color, topBackRight, color, topBackLeft, color, topBackRight, color,
	//topForwardLeft, color, bottomForwardLeft, color, topForwardRight, color, bottomForwardRight, color, topBackLeft, color, bottomBackLeft, color, topBackRight, color, bottomBackRight, color,
	//bottomForwardLeft, color, bottomForwardRight, color, bottomForwardLeft, color, bottomBackLeft, color, bottomForwardRight, color, bottomBackRight, color, bottomBackLeft, color, bottomBackRight, color

	};

	Vector3* vecarray = pointArray;
	int vecsize = 48;


	for (int i = 0, j = 0; i < vecsize; ++i, j += 3) {
		floatarr[j] = vecarray[i].getX();
		floatarr[j + 1] = vecarray[i].getY();
		floatarr[j + 2] = vecarray[i].getZ();
	}
}

PE::Handle MeshManager::getAsset(const char *asset, const char *package, int &threadOwnershipMask)
{	/////// This is here for testing debug prints
	//this->getBox(asset, package, threadOwnershipMask);
	OutputDebugStringW(L"OutputDebugStringW Test value\n");
	//std::cout << "cout test value\n" << std::endl;
	//std::cerr << "cerr test value\n" << std::endl;
	OutputDebugStringA("outputdebugstring a test value \n");


	char key[StrTPair<Handle>::StrSize];
	sprintf(key, "%s/%s", package, asset);
	
	int index = m_assets.findIndex(key);
	if (index != -1)
	{
		return m_assets.m_pairs[index].m_value;
	}
	Handle h;

	if (StringOps::endswith(asset, "skela"))
	{
		PE::Handle hSkeleton("Skeleton", sizeof(Skeleton));
		Skeleton *pSkeleton = new(hSkeleton) Skeleton(*m_pContext, m_arena, hSkeleton);
		pSkeleton->addDefaultComponents();

		pSkeleton->initFromFiles(asset, package, threadOwnershipMask);
		h = hSkeleton;
	}
	else if (StringOps::endswith(asset, "mesha"))
	{
		MeshCPU mcpu(*m_pContext, m_arena);
		mcpu.ReadMesh(asset, package, "");
		
		PE::Handle hMesh("Mesh", sizeof(Mesh));
		Mesh *pMesh = new(hMesh) Mesh(*m_pContext, m_arena, hMesh);
		pMesh->addDefaultComponents();

		pMesh->loadFromMeshCPU_needsRC(mcpu, threadOwnershipMask);

#if PE_API_IS_D3D11
		// todo: work out how lods will work
		//scpu.buildLod();
#endif
        // generate collision volume here. or you could generate it in MeshCPU::ReadMesh()
        pMesh->m_performBoundingVolumeCulling = true; // will now perform tests for this mesh

		h = hMesh;
	}


	PEASSERT(h.isValid(), "Something must need to be loaded here");

	RootSceneNode::Instance()->addComponent(h);
	m_assets.add(key, h);
	return h;


}
/*
PE::Handle MeshManager::getAsset(const char* asset, const char* package, int& threadOwnershipMask, float* floatarr)
{	/////// This is here for testing debug prints
	//this->getBox(asset, package, threadOwnershipMask);
	OutputDebugStringW(L"OutputDebugStringW Test value\n");
	std::cout << "cout test value\n" << std::endl;
	std::cerr << "cerr test value\n" << std::endl;
	OutputDebugStringA("outputdebugstring a test value \n");


	char key[StrTPair<Handle>::StrSize];
	sprintf(key, "%s/%s", package, asset);

	int index = m_assets.findIndex(key);
	if (index != -1)
	{
		return m_assets.m_pairs[index].m_value;
	}
	Handle h;

	if (StringOps::endswith(asset, "skela"))
	{
		PE::Handle hSkeleton("Skeleton", sizeof(Skeleton));
		Skeleton* pSkeleton = new(hSkeleton) Skeleton(*m_pContext, m_arena, hSkeleton);
		pSkeleton->addDefaultComponents();

		pSkeleton->initFromFiles(asset, package, threadOwnershipMask);
		h = hSkeleton;
	}
	else if (StringOps::endswith(asset, "mesha"))
	{
		MeshCPU mcpu(*m_pContext, m_arena);
		mcpu.ReadMesh(asset, package, "");

		PE::Handle hMesh("Mesh", sizeof(Mesh));
		Mesh* pMesh = new(hMesh) Mesh(*m_pContext, m_arena, hMesh);
		pMesh->addDefaultComponents();

		pMesh->loadFromMeshCPU_needsRC(mcpu, threadOwnershipMask);

#if PE_API_IS_D3D11
		// todo: work out how lods will work
		//scpu.buildLod();
#endif
		// generate collision volume here. or you could generate it in MeshCPU::ReadMesh()
		pMesh->m_performBoundingVolumeCulling = true; // will now perform tests for this mesh

		////////////////// CUSTOM CODE /////////////////////////////////
		//Retrieve the mesh's PositionBufferCPU that stores it vertices
	//I'm honestly not sure this is the right array for the vertices, but it's divisible by 3. I'm going to assume it is for now
		PositionBufferCPU* p = (PositionBufferCPU*)mcpu.m_hPositionBufferCPU.getObject();
		PrimitiveTypes::Float32* verts = p->m_values.getFirstPtr();


		//OutputDebugStringW(L"Test value");

		PrimitiveTypes::Float32 maxVert[3] = { NEGINFINITY, NEGINFINITY, NEGINFINITY };
		PrimitiveTypes::Float32 minVert[3] = { POSINFINITY, POSINFINITY, POSINFINITY };
		PrimitiveTypes::UInt32 numVerts = p->m_values.m_size; // how much is stored at the moment

		for (int i = 0; i < numVerts; i += 3) {
			maxVert[0] = max(maxVert[0], verts[i]);
			maxVert[1] = max(maxVert[1], verts[i + 1]);
			maxVert[2] = max(maxVert[2], verts[i + 2]);

			minVert[0] = min(minVert[0], verts[i]);
			minVert[1] = min(minVert[1], verts[i + 1]);
			minVert[2] = min(minVert[2], verts[i + 2]);
		}

		Vector3 topForwardLeft = {minVert[0], maxVert[1], minVert[2]};
		Vector3 topForwardRight = { maxVert[0], maxVert[1], minVert[2] };
		Vector3 topBackLeft = { minVert[0], maxVert[1],maxVert[2] };
		Vector3 topBackRight = { maxVert[0], maxVert[1],maxVert[2] };

		Vector3 bottomForwardLeft = { minVert[0], minVert[1],minVert[2] };
		Vector3 bottomForwardRight = { maxVert[0], minVert[1],minVert[2] };
		Vector3 bottomBackLeft = { minVert[0], minVert[1],maxVert[2] };
		Vector3 bottomBackRight = { maxVert[0], minVert[1],maxVert[2] };

		Vector3 color = {255, 255, 255};

		 //The points needed for a box are...
			//Top: (xMin, zMin, yMax), (xMin, zMax, yMax), (xMax, zMin, yMax), (xMax, zMax, yMax)
			//Bottom: (xMin, zMin, yMax), 

		Vector3 pointArray[48] = {
			topForwardLeft, color, topForwardRight, color, topForwardLeft, color, topBackLeft, color, topForwardRight, color, topBackRight, color, topBackLeft, color, topBackRight, color,
			topForwardLeft, color, bottomForwardLeft, color, topForwardRight, color, bottomForwardRight, color, topBackLeft, color, bottomBackLeft, color, topBackRight, color, bottomBackRight, color,
			bottomForwardLeft, color, bottomForwardRight, color, bottomForwardLeft, color, bottomBackLeft, color, bottomForwardRight, color, bottomBackRight, color, bottomBackLeft, color, bottomBackRight, color
		};

		Vector3 pointArray2[] = {
			topForwardLeft, topForwardRight, topForwardLeft, topBackLeft, topForwardRight, topBackRight, topBackLeft, topBackRight,
			topForwardLeft, bottomForwardLeft, topForwardRight, bottomForwardRight, topBackLeft, bottomBackLeft, topBackRight, bottomBackRight,
			bottomForwardLeft, bottomForwardRight, bottomForwardLeft, bottomBackLeft, bottomForwardRight, bottomBackRight, bottomBackLeft, bottomBackRight

		};

		Vector3 pointArray3[] = {
		topForwardLeft, color, topForwardRight, color//, topForwardLeft, color, topBackLeft, color, topForwardRight, color, topBackRight, color, topBackLeft, color, topBackRight, color,
		//topForwardLeft, color, bottomForwardLeft, color, topForwardRight, color, bottomForwardRight, color, topBackLeft, color, bottomBackLeft, color, topBackRight, color, bottomBackRight, color,
		//bottomForwardLeft, color, bottomForwardRight, color, bottomForwardLeft, color, bottomBackLeft, color, bottomForwardRight, color, bottomBackRight, color, bottomBackLeft, color, bottomBackRight, color

		};

		Vector3 pointArray4[] = {
		Vector3(0,2,0), color, Vector3(20,2,0), color//, topForwardLeft, color, topBackLeft, color, topForwardRight, color, topBackRight, color, topBackLeft, color, topBackRight, color,
		//topForwardLeft, color, bottomForwardLeft, color, topForwardRight, color, bottomForwardRight, color, topBackLeft, color, bottomBackLeft, color, topBackRight, color, bottomBackRight, color,
		//bottomForwardLeft, color, bottomForwardRight, color, bottomForwardLeft, color, bottomBackLeft, color, bottomForwardRight, color, bottomBackRight, color, bottomBackLeft, color, bottomBackRight, color

		};

		Vector3* vecarray = pointArray;
		int vecsize = 48;

		
		for (int i = 0, j = 0; i < vecsize; ++i, j += 3) {
			floatarr[j] = vecarray[i].getX();
			floatarr[j + 1] = vecarray[i].getY();
			floatarr[j + 2] = vecarray[i].getZ();
		}
		////////////////////////////////////////////////////////////////

		h = hMesh;
	}


	PEASSERT(h.isValid(), "Something must need to be loaded here");

	RootSceneNode::Instance()->addComponent(h);
	m_assets.add(key, h);
	return h;


}*/
/*
// Returns an array of handles connected to line meshes representing the box
void MeshManager::getBox(const char* asset, const char* package, int& threadOwnershipMask, float* floatarr)
{

	char key[StrTPair<Handle>::StrSize];
	sprintf(key, "%s/%s", package, asset);

	int index = m_assets.findIndex(key);
	assert(index != -1);

	Handle h;

	assert(StringOps::endswith(asset, "mesha"));


	MeshCPU mcpu(*m_pContext, m_arena);

	mcpu.ReadMesh(asset, package, "");


	//Retrieve the mesh's PositionBufferCPU that stores it vertices
	//I'm honestly not sure this is the right array for the vertices, but it's divisible by 3. I'm going to assume it is for now
	PositionBufferCPU* p = (PositionBufferCPU*) mcpu.m_hPositionBufferCPU.getObject();
	PrimitiveTypes::Float32* verts = p->m_values.getFirstPtr();


	//OutputDebugStringW(L"Test value");
	
	PrimitiveTypes::Float32 maxVert[3] = {NEGINFINITY, NEGINFINITY, NEGINFINITY};
	PrimitiveTypes::Float32 minVert[3] = {POSINFINITY, POSINFINITY, POSINFINITY};
	PrimitiveTypes::UInt32 numVerts = p->m_values.m_size; // how much is stored at the moment

	for (int i = 0; i < numVerts; i += 3) {
		maxVert[0] = max(maxVert[0], verts[i]);
		maxVert[1] = max(maxVert[1], verts[i + 1]);
		maxVert[2] = max(maxVert[2], verts[i + 2]);

		minVert[0] = min(minVert[0], verts[i]);
		minVert[1] = min(minVert[1], verts[i + 1]);
		minVert[2] = min(minVert[2], verts[i + 2]);
	}

	Vector3 topForwardLeft = {minVert[0], maxVert[1], minVert[2]};
	Vector3 topForwardRight = {maxVert[0], maxVert[1], minVert[2]};
	Vector3 topBackLeft = {minVert[0], maxVert[1],maxVert[2]};
	Vector3 topBackRight = { maxVert[0], maxVert[1],maxVert[2] };

	Vector3 bottomForwardLeft = { minVert[0], minVert[1],minVert[2] };
	Vector3 bottomForwardRight = { maxVert[0], minVert[1],minVert[2] };
	Vector3 bottomBackLeft = { minVert[0], minVert[1],maxVert[2] };
	Vector3 bottomBackRight = { maxVert[0], minVert[1],maxVert[2] };

	Vector3 color = { 255, 255, 255 };

	// The points needed for a box are...
		//Top: (xMin, zMin, yMax), (xMin, zMax, yMax), (xMax, zMin, yMax), (xMax, zMax, yMax)
		//Bottom: (xMin, zMin, yMax), 

	Vector3 pointArray[] = {
		topForwardLeft, color, topForwardRight, color, topForwardLeft, color, topBackLeft, color, topForwardRight, color, topBackRight, color, topBackLeft, color, topBackRight, color,
		topForwardLeft, color, bottomForwardLeft, color, topForwardRight, color, bottomForwardRight, color, topBackLeft, color, bottomBackLeft, color, topBackRight, color, bottomBackRight, color,
		bottomForwardLeft, color, bottomForwardRight, color, bottomForwardLeft, color, bottomBackLeft, color, bottomForwardRight, color, bottomBackRight, color, bottomBackLeft, color, bottomBackRight, color

	};

	Vector3 pointArray2[] = {
		topForwardLeft, topForwardRight, topForwardLeft, topBackLeft, topForwardRight, topBackRight, topBackLeft, topBackRight,
		topForwardLeft, bottomForwardLeft, topForwardRight, bottomForwardRight, topBackLeft, bottomBackLeft, topBackRight, bottomBackRight,
		bottomForwardLeft, bottomForwardRight, bottomForwardLeft, bottomBackLeft, bottomForwardRight, bottomBackRight, bottomBackLeft, bottomBackRight

	};

	Vector3 pointArray3[] = {
	topForwardLeft, color, topForwardRight, color//, topForwardLeft, color, topBackLeft, color, topForwardRight, color, topBackRight, color, topBackLeft, color, topBackRight, color,
	//topForwardLeft, color, bottomForwardLeft, color, topForwardRight, color, bottomForwardRight, color, topBackLeft, color, bottomBackLeft, color, topBackRight, color, bottomBackRight, color,
	//bottomForwardLeft, color, bottomForwardRight, color, bottomForwardLeft, color, bottomBackLeft, color, bottomForwardRight, color, bottomBackRight, color, bottomBackLeft, color, bottomBackRight, color

	};

	Vector3 pointArray4[] = {
Vector3(0,2,0), color, Vector3(20,2,0), color//, topForwardLeft, color, topBackLeft, color, topForwardRight, color, topBackRight, color, topBackLeft, color, topBackRight, color,
//topForwardLeft, color, bottomForwardLeft, color, topForwardRight, color, bottomForwardRight, color, topBackLeft, color, bottomBackLeft, color, topBackRight, color, bottomBackRight, color,
//bottomForwardLeft, color, bottomForwardRight, color, bottomForwardLeft, color, bottomBackLeft, color, bottomForwardRight, color, bottomBackRight, color, bottomBackLeft, color, bottomBackRight, color

	};
	
	Vector3* vecarray = pointArray;
	int vecsize = 24;


	for (int i = 0, j = 0; i < vecsize; ++i, j += 3) {
		floatarr[j] = vecarray[i].getX();
		floatarr[j + 1] = vecarray[i].getY();
		floatarr[j + 2] = vecarray[i].getZ();
	}
}*/

void MeshManager::registerAsset(const PE::Handle &h)
{
	static int uniqueId = 0;
	++uniqueId;
	char key[StrTPair<Handle>::StrSize];
	sprintf(key, "__generated_%d", uniqueId);
	
	int index = m_assets.findIndex(key);
	PEASSERT(index == -1, "Generated meshes have to be unique");
	
	RootSceneNode::Instance()->addComponent(h);
	m_assets.add(key, h);
}

}; // namespace Components
}; // namespace PE
