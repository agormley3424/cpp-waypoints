// APIAbstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes
#include "../Lua/LuaEnvironment.h"

// Sibling/Children includes
#include "DebugRenderer.h"
#include "PrimeEngine/Scene/TextMesh.h"
#include "PrimeEngine/Scene/LineMesh.h"
#include "PrimeEngine/Scene/TextSceneNode.h"
#include "PrimeEngine/Scene/RootSceneNode.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "PrimeEngine/Scene/MeshManager.h"
#include "PrimeEngine/Scene/MeshInstance.h"

const bool EnableDebugRendering = true;
namespace PE {
namespace Components {

using namespace PE::Events;

PE_IMPLEMENT_CLASS1(DebugRenderer, SceneNode);

// Static member variables
Handle DebugRenderer::s_myHandle;

// Singleton ------------------------------------------------------------------

void DebugRenderer::Construct(PE::GameContext &context, PE::MemoryArena arena)
{
	Handle handle("DebugRenderer", sizeof(DebugRenderer));
	DebugRenderer *pDebugRenderer = new(handle) DebugRenderer(context, arena, handle);
	pDebugRenderer->addDefaultComponents();
	// Singleton
	SetInstanceHandle(handle);
	RootSceneNode::Instance()->addComponent(handle);
}

// Constructor -------------------------------------------------------------
DebugRenderer::DebugRenderer(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself)
: SceneNode(context, arena, hMyself)
, m_lineLists(context, arena, NUM_LineLists)
, m_numFreeing(0)
{
	for (int i = 0; i < NUM_LineLists; ++i)
		m_lineLists.add(Array<float>(context, arena));

	m_numAvaialble = NUM_TextSceneNodes;
	for (int i = 0; i < NUM_TextSceneNodes; i++)
		m_hAvailableSNs[i] = i;

	m_numAvailableLineLists = NUM_LineLists;
	for (int i = 0; i < NUM_LineLists; i++)
		m_availableLineLists[i] = i;
}

	// Methods      ------------------------------------------------------------
void DebugRenderer::addDefaultComponents()
{
	Component::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(Events::Event_PRE_GATHER_DRAWCALLS, DebugRenderer::do_PRE_GATHER_DRAWCALLS);
	
	m_currentlyDrawnLineMesh = 0;
	for (int i = 0 ; i < 2; ++i)
	{
		m_hLineMeshes[i] = PE::Handle("LINEMESH", sizeof(LineMesh));
		LineMesh *pLineMesh = new(m_hLineMeshes[i]) LineMesh(*m_pContext, m_arena, m_hLineMeshes[i]);
		pLineMesh->addDefaultComponents();
		
		
		m_pContext->getMeshManager()->registerAsset(m_hLineMeshes[i]);

		// now need to create mesh instances for each that are going to trigger draw
		PE::Handle hInstance("MesInstance", sizeof(MeshInstance));
		MeshInstance *pInstance = new(hInstance) MeshInstance(*m_pContext, m_arena, hInstance);
		pInstance->addDefaultComponents();

		pInstance->initFromRegisteredAsset(m_hLineMeshes[i]);

		addComponent(hInstance);

		pLineMesh->setEnabled(false);

		m_hLineMeshInstances[i] = hInstance;
		pInstance->setEnabled(false);
	}
}
void DebugRenderer::createRootLineMesh()
{
	Vector3 color(0.5f, 0.5f, 0.5f);
	
	const int numSeg = 5;
	const int numPts = (numSeg+1) * 2 * 2 * 2;
	Vector3 linepts[numPts * 2];

	int iPt = 0;
	for (int x = -numSeg; x <= numSeg; ++x)
	{
		Vector3 pos0(x * 1.0f, 0, numSeg * -1.0f);
		Vector3 pos1(x * 1.0f, 0,  (x == 0) ? 0.0f : numSeg *1.0f);

		linepts[iPt++] = pos0; linepts[iPt++] = color;
		linepts[iPt++] = pos1; linepts[iPt++] = color;
	}


	for (int z = -numSeg; z <= numSeg; ++z)
	{
		Vector3 pos0(numSeg * -1.0f, 0, z * 1.0f);
		Vector3 pos1((z == 0) ? 0.0f : numSeg * 1.0f, 0, z * 1.0f);

		linepts[iPt++] = pos0; linepts[iPt++] = color;
		linepts[iPt++] = pos1; linepts[iPt++] = color;
	}

	Matrix4x4 m;
	m.loadIdentity();
	m.importScale(5.0f, 5.0f, 5.0f);

	DebugRenderer::Instance()->createLineMesh(true, m, &linepts[0].m_x, numPts, 0);// send event while the array is on the stack
}


// I think what's happening here is that it takes a pointer to the first float, assuming it's the first in a float of arrays?

//Creates a line mesh with a limited lifespan
// Has transform determines whether its points will be transformed from local to world space
// transform is the world matrix
// pRawData is a pointer leading to an array of floats
// numInRawData is the number of points you're going to have (lines * 2)
// timeToLive determines how long the line mesh is rendered for
// scale increases or decreases the scale of the lines
void DebugRenderer::createLineMesh(bool hasTransform, const Matrix4x4 &transform, float *pRawData, int numInRawData, float timeToLive, float scale /* = 1.0f*/)
{
	if (EnableDebugRendering && m_numAvailableLineLists)
	{
		int index = m_availableLineLists[--m_numAvailableLineLists];
		Array<float> &list = m_lineLists[index];
		

		m_lineListLifetimes[index] = timeToLive;
		int numPoints = 0;
		if (hasTransform)
		{
			numPoints += 3 /*lines*/ * 2 /*points per line*/;
		}

		if (pRawData)
		{
			numPoints += numInRawData;
		}
		
		list.reset(numPoints * 6 /*pos+color*/);

		if (hasTransform)
		{
			Vector3 pos = transform.getPos();
			Vector3 u = pos + transform.getU() * scale;
			Vector3 v = pos + transform.getV() * scale;
			Vector3 n = pos + transform.getN() * scale;

			// Adds two lines of direction in the x direction. One from the untransformed shape, one from the transformed shape
			list.add(pos.m_x, pos.m_y, pos.m_z); list.add(1.f, 0, 0); list.add(u.m_x, u.m_y, u.m_z); list.add(1.f, 0, 0); 
			// Adds two lines of direction in the y direction. One from the untransformed shape, one from the transformed shape
			list.add(pos.m_x, pos.m_y, pos.m_z); list.add(0, 1.f, 0);  list.add(v.m_x, v.m_y, v.m_z); list.add(0, 1.f, 0);
			// Adds two lines of direction in the y direction. One from the untransformed shape, one from the transformed shape
			list.add(pos.m_x, pos.m_y, pos.m_z); list.add(0, 0, 1.f); list.add(n.m_x, n.m_y, n.m_z); list.add(0, 0, 1.f);

			//This doesn't actually transform the other lines. What a gyp!
		}

		if (pRawData)
		{
			for (int i = 0; i < numInRawData; i++)
			{
				Vector3 firstPoint = { pRawData[i * 6], pRawData[i * 6 + 1], pRawData[i * 6 + 2] };
				Vector3 secondPoint = { pRawData[i * 6 + 3], pRawData[i * 6 + 4], pRawData[i * 6 + 5] };

				firstPoint = transform * firstPoint;
				secondPoint = transform * secondPoint;

				list.add(firstPoint.getX());
				list.add(firstPoint.getY());
				list.add(firstPoint.getZ());
				list.add(secondPoint.getX());
				list.add(secondPoint.getY());
				list.add(secondPoint.getZ());
				/*
				list.add(pRawData[i * 6]);
				list.add(pRawData[i * 6 + 1]);
				list.add(pRawData[i * 6 + 2]);
				list.add(pRawData[i * 6 + 3]);
				list.add(pRawData[i * 6 + 4]);
				list.add(pRawData[i * 6 + 5]);*/
			}
		}
	}
	
}
/*
void vecToFloat(Vector3* vecarray, int vecsise, float* floatarr) {

	for (int i = 0, j = 0; i < vecsise; i+=3, ++j) {
		floatarr[j] = vecarray[i].getX();
		floatarr[j+ 1] = vecarray[i].getY();
		floatarr[j+2] = vecarray[i].getZ();
	}
}*/
/*
// I think what's happening here is that it takes a pointer to the first float, assuming it's the first in a float of arrays?
void DebugRenderer::createLineMesh2(bool hasTransform, const Matrix4x4& transform, Vector3* pRawData, int numInRawData, float timeToLive, float scale)
{
	float* floatarr= new float[numInRawData * 3];

	vecToFloat(pRawData, numInRawData, floatarr);
	numInRawData *= 3;

	createLineMesh(hasTransform, transform, floatarr, numInRawData * 3, timeToLive, scale);
	
	}

}*/

void DebugRenderer::createLineMeshForDummies(bool hasTransform, const Matrix4x4& transform, Vector3* pRawData, int numInRawData, float timeToLive, float scale /* = 1.0f*/)
{
	//numInRawData should always be 24 points...
	if (EnableDebugRendering && m_numAvailableLineLists)
	{/**/
		int index = m_availableLineLists[--m_numAvailableLineLists];
		Array<float>& list = m_lineLists[index];


		m_lineListLifetimes[index] = timeToLive;
		int numPoints = 0;
		//if (hasTransform)
		//{
		//	numPoints += 3 /*lines*/ * 2 /*points per line*/;
		//}

		if (pRawData)
		{
			numPoints += numInRawData;
		}

		list.reset(numPoints * 6 /*pos+color*/);

		//if (hasTransform)
		//{
		//	Vector3 pos = transform.getPos();
		//	Vector3 u = pos + transform.getU() * scale;
		//	Vector3 v = pos + transform.getV() * scale;
		//	Vector3 n = pos + transform.getN() * scale;

		//	list.add(pos.m_x, pos.m_y, pos.m_z); list.add(1.f, 0, 0); list.add(u.m_x, u.m_y, u.m_z); list.add(1.f, 0, 0);
		//	list.add(pos.m_x, pos.m_y, pos.m_z); list.add(0, 1.f, 0);  list.add(v.m_x, v.m_y, v.m_z); list.add(0, 1.f, 0);
		//	list.add(pos.m_x, pos.m_y, pos.m_z); list.add(0, 0, 1.f); list.add(n.m_x, n.m_y, n.m_z); list.add(0, 0, 1.f);
		//}

		if (pRawData)
		{
			for (int i = 0; i < 24; i+=2)
			{
				list.add(pRawData[i].getX());
				list.add(pRawData[i].getY());
				list.add(pRawData[i].getZ());
				list.add(pRawData[i + 1].getX());
				list.add(pRawData[i + 1].getY());
				list.add(pRawData[i + 1].getZ());
			}
		}
	}

}

void DebugRenderer::createTextMesh(const char *str, bool isOverlay2D, bool is3D, bool is3DFacedToCamera, bool is3DFacedToCameraLockedYAxis, float timeToLive, Vector3 pos, float scale, int &threadOwnershipMask)
{
	if (EnableDebugRendering && m_numAvaialble)
	{
		int index = m_hAvailableSNs[--m_numAvaialble];
		Handle &h = m_hSNPool[index];
		TextSceneNode *pTextSN = 0;
		if (h.isValid())
		{
			//scene node has already been created
			pTextSN = h.getObject<TextSceneNode>();
			assert(pTextSN->isEnabled() == false); // this SN should never be in the available list if it is enabled
			pTextSN->setSelfAndMeshAssetEnabled(true);
		}
		else
		{
			h = PE::Handle("TEXT_SCENE_NODE", sizeof(TextSceneNode));
			pTextSN = new(h) TextSceneNode(*m_pContext, m_arena, h);
			pTextSN->addDefaultComponents();
			addComponent(h);
		}
		m_lifetimes[index] = timeToLive;
		TextSceneNode::DrawType drawType = TextSceneNode::InWorld;
		if (isOverlay2D)
		{
			drawType = TextSceneNode::Overlay2D;
		
			// modify position to fit [-1,1] coordinates
			pos.m_x = -1.0f + 2.0f * pos.m_x;
			pos.m_y = -1.0f + 2.0f * (1.0f - pos.m_y);
		}
		if (is3DFacedToCamera)
			drawType = TextSceneNode::Overlay2D_3DPos;
		pTextSN->loadFromString_needsRC(str, drawType, threadOwnershipMask);
		pTextSN->m_base.setPos(pos);
		pTextSN->m_scale = scale;
	}	
}

void DebugRenderer::do_PRE_GATHER_DRAWCALLS(Events::Event *pEvt)
{
	// need to check lifetime here and remove whatever is out of time
	Events::Event_PRE_GATHER_DRAWCALLS *pDrawEvent = NULL;
	pDrawEvent = (Events::Event_PRE_GATHER_DRAWCALLS *)(pEvt);

	while (m_numFreeing)
	{
		m_hAvailableSNs[m_numAvaialble++] = m_hFreeingSNs[--m_numFreeing];
	}

	for (int i = 0; i < NUM_TextSceneNodes; i++)
	{
		PE::Handle &h = m_hSNPool[i];
		if (h.isValid())
		{
			TextSceneNode *pTextSN = h.getObject<TextSceneNode>();
			if (pTextSN->isEnabled())
			{
				if (m_lifetimes[i] < 0.0f)
				{
					pTextSN->setSelfAndMeshAssetEnabled(false);
					m_hFreeingSNs[m_numFreeing++] = i;
				}

				m_lifetimes[i] -= 1.0f;
			}
		}
	}

	int totalSize = 0;
	for (int i =0; i < NUM_LineLists; i++)
	{
		Array<float> &list = m_lineLists[i];
		if (list.m_size)
		{
			if (m_lineListLifetimes[i] < 0.0f)
			{
				//remove
				m_availableLineLists[m_numAvailableLineLists++] = i;
				list.m_size = 0;
			}
			m_lineListLifetimes[i] -= 1.0f;
		}
		totalSize += list.m_size;
	}
}


void DebugRenderer::postPreDraw(int &threadOwnershipMask)
{
	// need to generate lines meshes in this method

	//first get size of existing

	// note removing lines is done in other method so here we can assume they are valid
	int totalSize = 0;
	for (int i =0; i < NUM_LineLists; i++)
	{
		Array<float> &list = m_lineLists[i];

		if (list.m_size)
		{
			if (m_lineListLifetimes[i] >= 0.0f)
			{
				totalSize += list.m_size;
			}
		}
	}

	Array<float> vertexData(*m_pContext, m_arena);
	vertexData.reset(totalSize);

	for (unsigned int i = 0; i < NUM_LineLists; i++)
	{
		Array<float> &list = m_lineLists[i];
		if (list.m_size)
		{
			if (m_lineListLifetimes[i] >= 0.0f)
			{
				for (unsigned int iv = 0; iv < list.m_size; iv++)
					vertexData.add(list[iv]);
			}
		}
	}

	LineMesh *pLineMesh = m_hLineMeshes[m_currentlyDrawnLineMesh].getObject<LineMesh>();
	MeshInstance *pLineMeshInstance = m_hLineMeshInstances[m_currentlyDrawnLineMesh].getObject<MeshInstance>();
	// this mesh ahs been submitted to render already. we can disable it here (will nto be submitted on next frame)
	pLineMesh->setEnabled(false);
	pLineMeshInstance->setEnabled(false);

	m_currentlyDrawnLineMesh = (m_currentlyDrawnLineMesh+1)%2;
	pLineMesh = m_hLineMeshes[m_currentlyDrawnLineMesh].getObject<LineMesh>();
	pLineMeshInstance = m_hLineMeshInstances[m_currentlyDrawnLineMesh].getObject<MeshInstance>();
	
	if (vertexData.m_size)
	{
		pLineMesh->loadFrom3DPoints_needsRC(vertexData.getFirstPtr(), totalSize/6, "", threadOwnershipMask);
		pLineMesh->setEnabled(true);
		pLineMeshInstance->setEnabled(true);
	}
	else
	{
		pLineMesh->setEnabled(false);
		pLineMeshInstance->setEnabled(false);
	}
	vertexData.reset(0);
}

}; // namespace Components
}; //namespace PE
