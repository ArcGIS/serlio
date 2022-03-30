#include "utils/MayaUtilities.h"
#include "utils/MELScriptBuilder.h"

#include <memory>

namespace mu {

int32_t computeSeed(const MFloatPoint& p) {
	float x = p[0];
	float z = p[2];
	int32_t seed = *(int32_t*)(&x);
	seed ^= *(int32_t*)(&z);
	seed %= 714025;
	return seed;
}

int32_t computeSeed(const MFloatPointArray& vertices) {
	MFloatPoint a(0.0, 0.0, 0.0);
	for (unsigned int vi = 0; vi < vertices.length(); vi++) {
		a += vertices[vi];
	}
	a = a / static_cast<float>(vertices.length());
	return computeSeed(a);
}

void statusCheck(const MStatus& status, const char* file, int line) {
	if (MS::kSuccess != status) {
		LOG_ERR << "maya status error at " << file << ":" << line << ": " << status.errorString().asChar() << " (code "
		        << status.statusCode() << ")";
	}
}

std::filesystem::path getWorkspaceRoot(MStatus& status) {
	MELScriptBuilder scriptBuilder;
	scriptBuilder.getWorkspaceDir();

	std::wstring output;
	status = scriptBuilder.executeSync(output);

	if (status == MS::kSuccess) {
		return std::filesystem::path(output).make_preferred();
	}
	else {
		return {};
	}
}

MStatus setEnumOptions(const MObject& node, MFnEnumAttribute& enumAttr,
                       const std::vector<std::wstring>& enumOptions,
                       const std::optional<std::wstring>& customDefaultOption) {
	MStatus stat;
	const MFnDependencyNode fNode(node, &stat);
	if (stat != MStatus::kSuccess)
		return stat;

	const MELVariable melSerlioNode(L"serlioNode");

	MELScriptBuilder scriptBuilder;
	const std::wstring nodeName = fNode.name().asWChar();
	const std::wstring attrName = enumAttr.name().asWChar();
	scriptBuilder.setVar(melSerlioNode, MELStringLiteral(nodeName));
	scriptBuilder.setAttrEnumOptions(melSerlioNode, attrName, enumOptions, customDefaultOption);

	return scriptBuilder.execute();
}
} // namespace mu