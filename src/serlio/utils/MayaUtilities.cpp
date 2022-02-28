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
bool mStringArraysAreEqual(const MStringArray& lhs, const MStringArray& rhs) {
	if (lhs.length() != rhs.length())
		return false;
	uint32_t index;
	for (uint32_t index = 0; index < lhs.length(); index++) {
		if (lhs[index] != rhs[index])
			return false;
	}
	return true;
}
} // namespace mu