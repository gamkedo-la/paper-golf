// Copyright Game Salutes. All Rights Reserved.


#include "Build/BuildCharacteristics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BuildCharacteristics)

FBuildCharacteristics::FBuildCharacteristics() :
	bShipping(UE_BUILD_SHIPPING),
	bDevelopment(UE_BUILD_DEVELOPMENT && !(UE_EDITOR)),
	bTest(UE_BUILD_TEST && !(UE_EDITOR)),
	bEditor(UE_EDITOR),
	bGame(UE_GAME),
	bDrawDebug(!bShipping)
{

}
