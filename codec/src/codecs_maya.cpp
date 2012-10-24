#include <iostream>

#include "prtx/prtspi.h"
#include "encoder/MayaEncoder.h"

#include "codecs_maya.h"


static const int MINIMAL_VERSION_MAJOR = 2012;
static const int MINIMAL_VERSION_MINOR = 1;
static const int MINIMAL_VERSION_BUILD = 0;


extern "C" {


CODECS_MAYA_EXPORTS_API void registerExtensionFactories(prtx::IExtensionManager* manager) {
	log_trace("codecs maya library: registerExtensionFactories begin");
	manager->addFactory(new MayaEncoderFactory());
	log_trace("codecs maya library: registerExtensionFactories done");
}


CODECS_MAYA_EXPORTS_API int getMinimalVersionMajor()
{
	return MINIMAL_VERSION_MAJOR;
}


CODECS_MAYA_EXPORTS_API int getMinimalVersionMinor()
{
	return MINIMAL_VERSION_MINOR;
}


CODECS_MAYA_EXPORTS_API int getMinimalVersionBuild()
{
	return MINIMAL_VERSION_BUILD;
}


}
