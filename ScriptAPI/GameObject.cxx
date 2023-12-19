#include "GameObject.hxx"
#include "EngineInterface.hxx"

namespace ScriptAPI
{
	bool GameObject::activeInHierarchy(TDS::EntityID entityID)
	{
		return TDS::ecs.getEntityIsEnabled(entityID);
	}

	void GameObject::SetActive(TDS::EntityID entityID, bool status)
	{
		TDS::ecs.setEntityIsEnabled(entityID, status);
		return;
	}

	TDS::EntityID GameObject::GetEntityID()
	{
		return entityID;
	}

	generic <typename T>
	T GameObject::GetComponent()
	{
		System::Type^ type = T::typeid;

		if (type == BoxColliderComponent::typeid)
		{
			return safe_cast<T>(BoxColliderComponent(entityID));
		}
		else if (type == CameraComponent::typeid)
		{
			return safe_cast<T>(CameraComponent(entityID));
		}
		else if (type == CapsuleColliderComponent::typeid)
		{
			return safe_cast<T>(CapsuleColliderComponent(entityID));
		}
		else if (type == NameTagComponent::typeid)
		{
			return safe_cast<T>(NameTagComponent(entityID));
		}
		else if (type == RigidBodyComponent::typeid)
		{
			return safe_cast<T>(RigidBodyComponent(entityID));
		}
		else if (type == SphereColliderComponent::typeid)
		{
			return safe_cast<T>(SphereColliderComponent(entityID));
		}
		else if (type == TransformComponent::typeid)
		{
			return safe_cast<T>(TransformComponent(entityID));
		}
		else if (type == UISpriteComponent::typeid)
		{
			return safe_cast<T>(UISpriteComponent(entityID));
		}
		else if (type == AudioComponent::typeid)
		{
			return safe_cast<T>(AudioComponent(entityID));
		}
		else if (type == GraphicComponent::typeid)
		{
			return safe_cast<T>(GraphicComponent(entityID));
		}

		return safe_cast<T>(EngineInterface::GetScriptReference(entityID, type->FullName));
	}

	void GameObject::SetEntityID(TDS::EntityID id)
	{
		entityID = id;
		transform = TransformComponent(id);
	}
}