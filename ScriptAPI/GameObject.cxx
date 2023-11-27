#include "GameObject.hxx"

namespace ScriptAPI
{
	BoxColliderComponent GameObject::GetBoxColliderComponent()
	{
		return BoxColliderComponent(entityID);
	}
	CameraComponent GameObject::GetCameraComponent()
	{
		return CameraComponent(entityID);
	}
	CapsuleColliderComponent GameObject::GetCapsuleColliderComponent()
	{
		return CapsuleColliderComponent(entityID);
	}
	NameTagComponent GameObject::GetNameTagComponent()
	{
		return NameTagComponent(entityID);
	}
	RigidBodyComponent GameObject::GetRigidBodyComponent()
	{
		return RigidBodyComponent(entityID);
	}
	SphereColliderComponent GameObject::GetSphereColliderComponent()
	{
		return SphereColliderComponent(entityID);
	}

	TransformComponent GameObject::GetTransformComponent()
	{
		return TransformComponent(entityID);
	}

	UISpriteComponent GameObject::GetUISpriteComponent()
	{
		return UISpriteComponent(entityID);
	}

	GraphicComponent GameObject::GetGraphicComponent()
	{
		return GraphicComponent(entityID);
	}

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
			return safe_cast<T>(GetBoxColliderComponent());
		}
		else if (type == CameraComponent::typeid)
		{
			return safe_cast<T>(GetCameraComponent());
		}
		else if (type == CapsuleColliderComponent::typeid)
		{
			return safe_cast<T>(GetCapsuleColliderComponent());
		}
		else if (type == NameTagComponent::typeid)
		{
			return safe_cast<T>(GetNameTagComponent());
		}
		else if (type == RigidBodyComponent::typeid)
		{
			return safe_cast<T>(GetRigidBodyComponent());
		}
		else if (type == SphereColliderComponent::typeid)
		{
			return safe_cast<T>(GetSphereColliderComponent());
		}
		else if (type == TransformComponent::typeid)
		{
			return safe_cast<T>(GetTransformComponent());
		}
		else if (type == UISpriteComponent::typeid)
		{
			return safe_cast<T>(GetUISpriteComponent());
		}
		else if (type == GraphicComponent::typeid)
		{
			return safe_cast<T>(GetGraphicComponent());
		}

		return T();
	}

	void GameObject::SetEntityID(TDS::EntityID id)
	{
		entityID = id;
		transform = TransformComponent(id);
	}
}