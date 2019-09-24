/**
 * Serlio - Esri CityEngine Plugin for Autodesk Maya
 *
 * See https://github.com/esri/serlio for build and usage instructions.
 *
 * Copyright (c) 2012-2019 Esri R&D Center Zurich
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#pragma once

#include "prtModifier/MayaCallbacks.h"
#include "PRTContext.h"

#include "maya/MString.h"
#include "maya/MPxNode.h"
#include "maya/adskDataHandle.h"

#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>


#define PRT_MATERIAL_TYPE_ID 0x8667b

class MaterialColor;

class PRTMaterialNode : public MPxNode {
public:
	PRTMaterialNode(const PRTContext& prtCtx) : mPRTCtx(prtCtx) {}

	static  MStatus     initialize();

	MStatus compute(const MPlug& plug, MDataBlock& data) override;

	static MTypeId  id;
	static  MObject aInMesh;
	static  MObject aOutMesh;

private:
	static void setTexture(MString& mShadingCmd, const std::string& tex, const std::string& target);
	static void setAttribute(MString& mShadingCmd, const std::string& target, const double val);
	static void setAttribute(MString& mShadingCmd, const std::string& target, const double val1, double const val2);
	static void setAttribute(MString& mShadingCmd, const std::string& target, const double val1, double const val2, double const val3);
	static void setAttribute(MString& mShadingCmd, const std::string& target, const MaterialColor& color);
	static void setAttribute(MString& mShadingCmd, const std::string& target, const std::array<double, 2>& val);
	static void setAttribute(MString& mShadingCmd, const std::string& target, const std::array<double, 3>& val);

	static MString sfxFile;

private:
	const PRTContext& mPRTCtx;
}; // class PRTMaterialNode
