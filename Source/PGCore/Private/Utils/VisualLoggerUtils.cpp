// Copyright Game Salutes. All Rights Reserved.

#if ENABLE_VISUAL_LOG

#include "Utils/VisualLoggerUtils.h"

#include "PhysicsEngine/BodySetup.h"
#include "Utils/CollisionUtils.h"
#include "VisualLogger/VisualLogger.h"

void PG::VisualLoggerUtils::DrawStaticMeshComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UStaticMeshComponent& Component)
{
	const auto Mesh = Component.GetStaticMesh();
	if (!IsValid(Mesh) || !Mesh->GetBodySetup())
	{
		return;
	}

	const auto BodySetup = Mesh->GetBodySetup();
	const auto& AggGeom = BodySetup->AggGeom;

	const auto& Transform = Component.GetComponentTransform();

	if (!AggGeom.ConvexElems.IsEmpty())
	{
		for (const auto& ConvexElm : AggGeom.ConvexElems)
		{
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
				FColor::Blue);
		}
	}
	else if (!AggGeom.BoxElems.IsEmpty())
	{
		Snapshot.AddElement(
			PG::CollisionUtils::GetAABB(Component),
			Transform.ToMatrixNoScale(),
			CategoryName,
			ELogVerbosity::Log,
			FColor::Blue);
	}
	else if (!AggGeom.SphereElems.IsEmpty())
	{
		const auto& Bounds = Component.Bounds;

		Snapshot.AddSphere(
			Bounds.Origin,
			Bounds.SphereRadius,
			CategoryName,
			ELogVerbosity::Log,
			FColor::Blue);
	}
	else if (!AggGeom.SphylElems.IsEmpty())
	{
		for (const auto& SphylElm : AggGeom.SphylElems)
		{
			Snapshot.AddCapsule(
				Transform.TransformPosition(SphylElm.Center),
				SphylElm.Length * 0.5f,
				SphylElm.Radius,
				SphylElm.Rotation.Quaternion() * Transform.GetRotation(),
				CategoryName,
				ELogVerbosity::Log,
				FColor::Blue);
		}
	}
	else if (!AggGeom.TaperedCapsuleElems.IsEmpty())
	{
		for (const auto& CapsuleElm : AggGeom.TaperedCapsuleElems)
		{
			Snapshot.AddCapsule(
				Transform.TransformPosition(CapsuleElm.Center),
				(CapsuleElm.Radius0 + CapsuleElm.Radius1) * 0.5f,
				CapsuleElm.Length * 0.5f,
				CapsuleElm.Rotation.Quaternion() * Transform.GetRotation(),
				CategoryName,
				ELogVerbosity::Log,
				FColor::Blue);
		}
	}
}

#endif
