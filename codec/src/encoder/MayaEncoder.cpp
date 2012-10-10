/*
 * MayaEncoder.cpp
 *
 *  Created on: Sep 11, 2012
 *      Author: shaegler
 */

#include <iostream>
#include <sstream>

#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "api/prtapi.h"
#include "spi/base/Log.h"
#include "spi/base/IGeometry.h"
#include "spi/base/IShape.h"
#include "spi/base/ILeafIterator.h"
#include "spi/codec/EncodePreparator.h"
#include "spi/extension/ExtensionManager.h"

#include "util/StringUtils.h"

#include "maya/MDataHandle.h"
#include "maya/MStatus.h"
#include "maya/MObject.h"
#include "maya/MDoubleArray.h"
#include "maya/MPointArray.h"
#include "maya/MPoint.h"
#include "maya/MString.h"
#include "maya/MFileIO.h"
#include "maya/MLibrary.h"
#include "maya/MIOStream.h"
#include "maya/MGlobal.h"
#include "maya/MStringArray.h"
#include "maya/MFloatArray.h"
#include "maya/MFloatPoint.h"
#include "maya/MFloatPointArray.h"
#include "maya/MDataBlock.h"
#include "maya/MDataHandle.h"
#include "maya/MIntArray.h"
#include "maya/MDoubleArray.h"
#include "maya/MLibrary.h"
#include "maya/MPlug.h"
#include "maya/MDGModifier.h"
#include "maya/MSelectionList.h"
#include "maya/MDagPath.h"
#include "maya/MFileObject.h"

#include "maya/MFnNurbsSurface.h"
#include "maya/MFnMesh.h"
#include "maya/MFnMeshData.h"
#include "maya/MFnLambertShader.h"
#include "maya/MFnTransform.h"
#include "maya/MFnSet.h"
#include "maya/MFnPartition.h"

#include "encoder/MayaEncoder.h"


#include "util/Timer.h"
#include "util/URIUtils.h"
#include "util/Exception.h"


#define M_CHECK3(_stat_) {if(MS::kSuccess != _stat_) {throw std::runtime_error(StringUtils::printToString("err:%s %d\n", _stat_.errorString().asChar(), _stat_.statusCode()));}}

MayaEncoder::MayaEncoder() {
}


MayaEncoder::~MayaEncoder() {
}

void MayaEncoder::encode(prtspi::IOutputStream* stream, const prtspi::InitialShape** initialShapes, size_t initialShapeCount,
		prtspi::AbstractResolveMapPtr am, const prt::Attributable* options, void* encCxt) {
	//am = am->toFileURIs();

	if(encCxt == 0) throw(RuntimeErrorST(L"encCtxt null!"));

	Timer tim;
	prtspi::Log::trace("MayaEncoder:encode: #initial shapes = %d", initialShapeCount);

	prtspi::EncodePreparator* encPrep = prtspi::EncodePreparator::create();
	for (size_t i = 0; i < initialShapeCount; ++i) {
		prtspi::IGeometry** occluders = 0;
		prtspi::ILeafIterator* li = prtspi::ILeafIterator::create(initialShapes[i], am, occluders, 0);
		for (const prtspi::IShape* shape = li->getNext(); shape != 0; shape = li->getNext()) {
			encPrep->add(initialShapes[i], shape);
//			prtspi::Log::trace(L"encode leaf shape mat: %ls", shape->getMaterial()->getString(L"name"));
		}
	}

	const float t1 = tim.stop();
	tim.start();

	prtspi::IContentArray* geometries = prtspi::IContentArray::create();
	encPrep->createEncodableGeometries(geometries);
	convertGeometry(am, stream, geometries, *((MayaData*)encCxt));
	geometries->destroy();

	encPrep->destroy();

	const float t2 = tim.stop();
	prtspi::Log::info("MayaEncoder::encode() : preparator %f s, encoding %f s, total %f s", t1, t2, t1+t2);

	prtspi::Log::trace("MayaEncoder::encode done.");
}


void MayaEncoder::convertGeometry(prtspi::AbstractResolveMapPtr am, prtspi::IOutputStream* stream, prtspi::IContentArray* geometries, MayaData& mdata)
{
	static bool SETUPMATERIALS = true;

	prtspi::Log::trace("--- MayaEncoder::convertGeometry begin");

	// maya api tutorial: http://ewertb.soundlinker.com/maya.php


	MFloatPointArray vertices;
	MIntArray        counts;
	MIntArray        connects;

	MFloatArray      tcsU;
	MFloatArray      tcsV;
	MIntArray        tcConnects;

	uint32_t base = 0;
	uint32_t tcBase = 0;
	for(size_t gi = 0, size = geometries->size(); gi < size; ++gi) {
		prtspi::IGeometry* geo = (prtspi::IGeometry*)geometries->get(gi);

		const double* verts = geo->getVertices();
		const size_t vertsCount = geo->getVertexCount();

		for(size_t i = 0; i < vertsCount; ++i)
			vertices.append((float)verts[3*i], (float)verts[3*i+1], (float)verts[3*i+2]);

		const size_t tcsCount = geo->getUVCount();
		if(tcsCount > 0) {
			const double* tcs = geo->getUVs();
			for(size_t i=0; i<tcsCount; i++) {
				tcsU.append((float)tcs[i*2]);
				tcsV.append((float)tcs[i*2+1]);
			}
		}

		for(size_t fi = 0; fi < geo->getFaceCount(); ++fi) {
			const prtspi::IFace* face = geo->getFace(fi);
			counts.append((int)face->getIndexCount());

			const uint32_t* indices = face->getVertexIndices();
			for(size_t vi = 0; vi < face->getIndexCount(); ++vi)
				connects.append(base + indices[vi]);

			if(face->getUVIndexCount() > 0) {
				for(size_t vi = 0; vi < face->getIndexCount(); ++vi)
					tcConnects.append(tcBase + face->getUVIndices()[vi]);
			}
		}

		base   = vertices.length();
		tcBase = tcsU.length();
	}

	// setup mesh
	MStatus stat;
	MDataHandle outputHandle = mdata.mData->outputValue(*mdata.mPlug, &stat);
	M_CHECK3(stat);
	MObject oMesh = outputHandle.asMesh();

	MFnMeshData dataCreator;
	MObject newOutputData = dataCreator.create(&stat);
	M_CHECK3(stat);

	MFnMesh meshFn;
	oMesh = meshFn.create(
		vertices.length(),
		counts.length(),
		vertices,
		counts,
		connects,
		newOutputData,
		&stat);
	M_CHECK3(stat);

	if(SETUPMATERIALS) {
		mdata.mShadingGroups->clear();
		mdata.mShadingRanges->clear();

		MPlugArray plugs;
		bool isConnected = mdata.mPlug->connectedTo(plugs, false, true, &stat);
		M_CHECK3(stat);
		prtspi::Log::trace("plug is connected: %d; %d plugs\n", isConnected, plugs.length());
		if(plugs.length() > 0) {
			// setup uvs + shader connections
			if(tcConnects.length() > 0) {
				MString layerName = "map1";
				stat = meshFn.setUVs(tcsU, tcsV, &layerName);
				M_CHECK3(stat);

				prtspi::Log::trace("tcConnect has size %d",  tcConnects.length());

				int uvInd = 0;
				int curFace = 0;
				for(size_t gi = 0, size = geometries->size(); gi < size; ++gi) {
					prtspi::IGeometry* geo = (prtspi::IGeometry*)geometries->get(gi);

					MString texName;

					prtspi::IMaterial* mat = geo->getMaterial();
					if(mat->getTextureArray(L"diffuseMap")->size() > 0) {
						std::wstring uri(mat->getTextureArray(L"diffuseMap")->get(0)->getName());
						texName = MString(uri.substr(wcslen(URIUtils::SCHEME_FILE)).c_str());
					}

					const int faceCount   = (int)geo->getFaceCount();
					const size_t tcsCount = geo->getUVCount();
					const bool hasUVs     = tcsCount > 0;

					prtspi::Log::trace("Material %d : hasUVs = %d, faceCount = %d, texName = '%s'\n", gi, hasUVs, faceCount, texName.asChar());

					int startFace = curFace;

					if(hasUVs) {
						for(int i=0; i<faceCount; i++) {
							for(int j=0; j<counts[curFace]; j++) {
								stat = meshFn.assignUV(curFace, j,  tcConnects[uvInd++], &layerName);
								M_CHECK3(stat);
							}
							curFace++;
						}
					}
					else curFace += faceCount;

					if(curFace - 1 > startFace) {
						MString cmd("createShadingGroup(\"");
						cmd += texName;
						cmd += "\")";
						MString result = MGlobal::executeCommandStringResult(cmd, false, false, &stat);
						prtspi::Log::trace("mel cmd '%s' executed, result = '%s'", cmd.asChar(), result.asChar());

						mdata.mShadingGroups->append(result);
						mdata.mShadingRanges->append(startFace);
						mdata.mShadingRanges->append(curFace - 1);
					}

					M_CHECK3(stat);
				}
			}
		} // if > 0 connections
	} // if SETUPMATERIALS


	stat = outputHandle.set(newOutputData);
	M_CHECK3(stat);


	prtspi::Log::trace("    mayaVertices.length = %d", vertices.length());
	prtspi::Log::trace("    mayaCounts.length   = %d", counts.length());
	prtspi::Log::trace("    mayaConnects.length = %d", connects.length());
}

void MayaEncoder::unpackRPK(std::wstring rpkPath) {

}

