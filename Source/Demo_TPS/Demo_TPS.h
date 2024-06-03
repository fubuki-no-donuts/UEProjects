// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// 自定义的channel会按顺序，这里只定义了一个，所以是ECC_GameTraceChannel1
// 利用宏重新定义，以便可以看出含义
#define ECC_SkeletalMesh ECollisionChannel::ECC_GameTraceChannel1

