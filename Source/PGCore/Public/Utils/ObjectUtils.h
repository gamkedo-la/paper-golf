// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"

#include "Runtime/CoreUObject/Public/UObject/SoftObjectPtr.h"
#include "Runtime/Engine/Classes/Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

#include <concepts>


namespace PG::ObjectUtils
{
	template<std::derived_from<UObject> T>
	bool IsClassDefaultObject(const T* Object);

	template<std::derived_from<UObject> T>
	T* GetClassDefaultObject();

	template<std::derived_from<UActorComponent> T>
	T* FindDefaultComponentByClass(AActor* ActorClassDefault);

    /**
    * Returns whether the specified UObject is exactly the specified type.
    */
    template<std::derived_from<UObject> T>
    bool IsExactTypeOf(const T* Object);

    /**
    * Returns whether the specified UObject is exactly the specified type.
    */
    template<std::derived_from<UObject> T>
    bool IsExactTypeOf(const T& Object);

    template<typename S, std::derived_from<UObject> T>
    bool InitScriptInterface(UObject* Outer, TScriptInterface<S>& ScriptInterface, const TSubclassOf<T>& ObjectClass);

    template<std::derived_from<UInterface> T>
    bool ImplementsInterface(const UObject* Object);

    template<std::derived_from<UInterface> UInterfaceType, typename TInterfaceType>
    TInterfaceType* CastToInterface(UObject* Object);

    template<std::derived_from<UObject> T>
    void LoadObjectAsync(const TSoftObjectPtr<T>& ObjectPtr, TFunction<void(T*)>&& OnObjectLoaded);

    template<std::derived_from<UObject> T>
    void LoadClassAsync(const TSoftClassPtr<T>& ClassPtr, TFunction<void(UClass*)>&& OnClassLoaded);

    namespace Private
    {
        template<typename PtrType, std::derived_from<UObject> CallbackType>
        void LoadAsync(const PtrType& Ptr, TFunction<void(CallbackType*)>&& OnLoaded);
    }
}


#pragma region Template Definitions

template<std::derived_from<UObject> T>
T* PG::ObjectUtils::GetClassDefaultObject()
{
	auto Class = T::StaticClass();
	check(Class);

	auto RawCDO = Class->GetDefaultObject();
	auto CDO = Cast<T>(RawCDO);
	ensureMsgf(CDO, TEXT("CDO is incorrect type for %s -> %s"), *Class->GetName(), RawCDO ? *RawCDO->GetName() : TEXT("NULL"));

	return CDO;
}

template<std::derived_from<UObject> T>
inline void PG::ObjectUtils::LoadObjectAsync(const TSoftObjectPtr<T>& ObjectPtr, TFunction<void(T*)>&& OnObjectLoaded)
{
    Private::LoadAsync(ObjectPtr, MoveTemp(OnObjectLoaded));
}

template<std::derived_from<UObject> T>
inline void PG::ObjectUtils::LoadClassAsync(const TSoftClassPtr<T>& ClassPtr, TFunction<void(UClass*)>&& OnClassLoaded)
{
    Private::LoadAsync(ClassPtr, MoveTemp(OnClassLoaded));
}

template<typename PtrType, std::derived_from<UObject> CallbackType>
void PG::ObjectUtils::Private::LoadAsync(const PtrType& Ptr, TFunction<void(CallbackType*)>&& OnLoaded)
{
    if (!ensure(!Ptr.IsNull()))
    {
        return;
    }

    // Must use this version and not just create an instance of FStreamableManager as then the loading doesn't work when the callback fires!
    auto& StreamableManager = UAssetManager::Get().GetStreamableManager();
    StreamableManager.RequestAsyncLoad(Ptr.ToSoftObjectPath(), [Ptr, OnLoaded = MoveTemp(OnLoaded)]()
    {
        OnLoaded(Ptr.Get());
    });
}

template<std::derived_from<UObject> T>
inline bool PG::ObjectUtils::IsClassDefaultObject(const T* Object)
{
    return Object && 
            ((Object->GetFlags() & (EObjectFlags::RF_ArchetypeObject | EObjectFlags::RF_ClassDefaultObject)) ||
            (Object->GetClass() && Object->GetClass()->GetDefaultObject() == Object));
}

template<std::derived_from<UObject> T>
inline bool PG::ObjectUtils::IsExactTypeOf(const T* Object)
{
    Object&& IsExactTypeOf(*Object);
}

template<std::derived_from<UObject> T>
inline bool PG::ObjectUtils::IsExactTypeOf(const T& Object)
{
    return Object.GetClass() == T::StaticClass();
}

// Adapted from https://forums.unrealengine.com/t/how-to-get-a-component-from-a-classdefaultobject/383881/5
template<std::derived_from<UActorComponent> T>
T* PG::ObjectUtils::FindDefaultComponentByClass(AActor* ActorClassDefault)
{
    if (!ActorClassDefault)
    {
        return nullptr;
    }

    if (auto FoundComponent = ActorClassDefault->FindComponentByClass<T>(); FoundComponent)
    {
        return FoundComponent;
    }

    UClass* InActorClass = ActorClassDefault->GetClass();

    // Check blueprint nodes. Components added in blueprint editor only (and not in code) are not available from
    // CDO.
    const auto RootBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InActorClass);
    const auto InComponentClass = T::StaticClass();

    UClass* ActorClass = InActorClass;

    // Go down the inheritance tree to find nodes that were added to parent blueprints of our blueprint graph.
    do
    {
        UBlueprintGeneratedClass* ActorBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(ActorClass);
        if (!ActorBlueprintGeneratedClass)
        {
            return nullptr;
        }

        const TArray<USCS_Node*>& ActorBlueprintNodes =
            ActorBlueprintGeneratedClass->SimpleConstructionScript->GetAllNodes();

        for (USCS_Node* Node : ActorBlueprintNodes)
        {
            if (Node->ComponentClass->IsChildOf(InComponentClass))
            {
                return Cast<T>(Node->GetActualComponentTemplate(RootBlueprintGeneratedClass));
            }
        }

        ActorClass = Cast<UClass>(ActorClass->GetSuperStruct());

    } while (ActorClass != AActor::StaticClass());

    return nullptr;
}

template<typename S, std::derived_from<UObject> T>
bool PG::ObjectUtils::InitScriptInterface(UObject* Outer, TScriptInterface<S>& ScriptInterface, const TSubclassOf<T>& ObjectClass)
{
    if (!ObjectClass)
    {
        return false;
    }

    auto Object = NewObject<T>(Outer, ObjectClass);
    auto Interface = Cast<S>(Object);

    if (!Object || !Interface)
    {
        return false;
    }

    ScriptInterface.SetObject(Object);
    ScriptInterface.SetInterface(Interface);

    return static_cast<bool>(ScriptInterface);
}

template<std::derived_from<UInterface> T>
inline bool PG::ObjectUtils::ImplementsInterface(const UObject* Object)
{
    // See https://www.stevestreeting.com/2020/11/02/ue4-c-interfaces-hints-n-tips/
    return Object && Object->GetClass() && Object->GetClass()->ImplementsInterface(T::StaticClass());
}

template<std::derived_from<UInterface> UInterfaceType, typename TInterfaceType>
inline TInterfaceType* PG::ObjectUtils::CastToInterface(UObject* Object)
{
    return ImplementsInterface<UInterfaceType>(Object) ? Cast<TInterfaceType>(Object) : nullptr;
}

#pragma endregion Template Definitions
