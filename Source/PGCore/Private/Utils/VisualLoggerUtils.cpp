// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#if ENABLE_VISUAL_LOG

#include "Utils/VisualLoggerUtils.h"

#include "PhysicsEngine/BodySetup.h"
#include "Utils/CollisionUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Debug/PGConsoleVars.h"
#include "PGConstants.h"

#include <atomic>

namespace
{
	std::atomic_bool bStartedAutomaticVisualLoggerRecording(false);
}

void PG::VisualLoggerUtils::DrawStaticMeshComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UStaticMeshComponent& Component, const FColor& Color)
{
	const auto Mesh = Component.GetStaticMesh();
	if (!IsValid(Mesh) || !Mesh->GetBodySetup())
	{
		return;
	}

	const auto BodySetup = Mesh->GetBodySetup();
	const auto& AggGeom = BodySetup->AggGeom;

	const auto& Transform = Component.GetComponentTransform();

	// Render all the collision shapes even if multiple types are combined
	if (!AggGeom.ConvexElems.IsEmpty())
	{
		for (const auto& ConvexElm : AggGeom.ConvexElems)
		{
			// Make a copy as we need to transform the position from model to world space
			auto VertexData = ConvexElm.VertexData;
			for (auto& Vertex : VertexData)
			{
				Vertex = Transform.TransformPosition(Vertex);
			}

			Snapshot.AddMesh(
				VertexData,
				ConvexElm.IndexData,
				CategoryName,
				ELogVerbosity::Log,
				Color);
		}
	}
	if (!AggGeom.BoxElems.IsEmpty())
	{
		for (const auto& BoxElem : AggGeom.BoxElems)
		{
			Snapshot.AddBox(
				FBox::BuildAABB(FVector::ZeroVector, FVector{ BoxElem.X, BoxElem.Y, BoxElem.Z }),
				// BoxElem transform includes the center
				(BoxElem.GetTransform() * Transform).ToMatrixWithScale(),
				CategoryName,
				ELogVerbosity::Log,
				Color
			);
		}
	}
	if (!AggGeom.SphereElems.IsEmpty())
	{
		for (const auto& SphereElem : AggGeom.SphereElems)
		{
			Snapshot.AddSphere(
				Transform.TransformPosition(SphereElem.Center),
				SphereElem.Radius,
				CategoryName,
				ELogVerbosity::Log,
				Color
			);
		}
	}
	if (!AggGeom.SphylElems.IsEmpty())
	{
		for (const auto& SphylElm : AggGeom.SphylElems)
		{
			Snapshot.AddCapsule(
				Transform.TransformPosition(SphylElm.Center),
				SphylElm.Length * 0.5f,
				SphylElm.Radius,
				Transform.GetRotation() * SphylElm.Rotation.Quaternion(),
				CategoryName,
				ELogVerbosity::Log,
				Color);
		}
	}
	if (!AggGeom.TaperedCapsuleElems.IsEmpty())
	{
		for (const auto& CapsuleElm : AggGeom.TaperedCapsuleElems)
		{
			Snapshot.AddCapsule(
				Transform.TransformPosition(CapsuleElm.Center),
				(CapsuleElm.Radius0 + CapsuleElm.Radius1) * 0.5f,
				CapsuleElm.Length * 0.5f,
				Transform.GetRotation() * CapsuleElm.Rotation.Quaternion(),
				CategoryName,
				ELogVerbosity::Log,
				Color);
		}
	}
}

void PG::VisualLoggerUtils::StartAutomaticRecording()
{
#if PG_DEBUG_ENABLED

	const bool bStartRecord = bStartedAutomaticVisualLoggerRecording = CAutomaticVisualLoggerRecording.GetValueOnGameThread();

	if (bStartRecord)
	{
		FVisualLogger::Get().SetIsRecordingToFile(true);
	}

#endif
}

void PG::VisualLoggerUtils::StopAutomaticRecording()
{
	if(bStartedAutomaticVisualLoggerRecording)
	{
		FVisualLogger::Get().SetIsRecordingToFile(false);
		bStartedAutomaticVisualLoggerRecording = false;
	}
}
#endif
