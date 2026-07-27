#ifndef EDITOR_PHYSICS_SHAPE_EDITOR_H
#define EDITOR_PHYSICS_SHAPE_EDITOR_H

#include <physics/shape.hpp>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/MotionProperties.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <memory>

namespace vke_editor
{
    static JPH::ShapeRefC CreateEditorShape(vke_physics::PhyscisShapeType type)
    {
        switch (type)
        {
        case vke_physics::PHYSICS_SHAPE_SPHERE:
            return new JPH::SphereShape(0.5f);
        case vke_physics::PHYSICS_SHAPE_BOX:
            return new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f));
        case vke_physics::PHYSICS_SHAPE_CAPSULE:
            return new JPH::CapsuleShape(0.5f, 0.25f);
        case vke_physics::PHYSICS_SHAPE_CYLINDER:
            return new JPH::CylinderShape(0.5f, 0.25f);
        case vke_physics::PHYSICS_SHAPE_TRIANGLE:
            return new JPH::TriangleShape(JPH::Vec3(0.0f, 0.0f, 0.0f), JPH::Vec3(1.0f, 0.0f, 0.0f), JPH::Vec3(0.0f, 1.0f, 0.0f));
        case vke_physics::PHYSICS_SHAPE_PLANE:
            return new JPH::PlaneShape(JPH::Plane(JPH::Vec3(0.0f, 1.0f, 0.0f), 0.0f));
        default:
            return nullptr;
        }
    }

    static bool IsDynamicBodyShape(vke_physics::PhyscisShapeType type)
    {
        return type == vke_physics::PHYSICS_SHAPE_SPHERE ||
               type == vke_physics::PHYSICS_SHAPE_BOX ||
               type == vke_physics::PHYSICS_SHAPE_CAPSULE ||
               type == vke_physics::PHYSICS_SHAPE_CYLINDER;
    }

    static void ApplyEditorMassProperties(const JPH::BodyCreationSettings &settings, JPH::BodyID bodyID)
    {
        JPH::BodyLockWrite lock(vke_physics::PhysicsManager::GetPhysicsSystem().GetBodyLockInterface(), bodyID);
        if (!lock.Succeeded())
            return;

        JPH::MotionProperties *motionProperties = lock.GetBody().GetMotionPropertiesUnchecked();
        if (motionProperties == nullptr)
            return;

        motionProperties->SetMassProperties(settings.mAllowedDOFs, settings.GetMassProperties());
    }

    static void ApplyEditorShape(std::shared_ptr<vke_physics::PhyscisShape> &shape,
                                 JPH::BodyCreationSettings &settings,
                                 JPH::BodyInterface &bodyInterface,
                                 JPH::BodyID bodyID,
                                 bool updateMassProperties,
                                 vke_physics::PhyscisShapeType type,
                                 const JPH::ShapeRefC &shapeRef)
    {
        if (shape == nullptr)
            shape = std::make_shared<vke_physics::PhyscisShape>(type);

        shape->type = type;
        shape->shapeRef = shapeRef;
        settings.SetShape(shapeRef.GetPtr());

        bodyInterface.SetShape(bodyID, shapeRef.GetPtr(), false, JPH::EActivation::Activate);
        if (updateMassProperties)
            ApplyEditorMassProperties(settings, bodyID);
    }

    static bool DrawPhysicsShapeEditor(const char *id,
                                       std::shared_ptr<vke_physics::PhyscisShape> &shape,
                                       JPH::BodyCreationSettings &settings,
                                       JPH::BodyInterface &bodyInterface,
                                       JPH::BodyID bodyID,
                                       bool updateMassProperties,
                                       bool allowStaticOnlyShapes = true)
    {
        bool changed = false;
        ImGui::PushID(id);

        if (shape == nullptr || shape->shapeRef == nullptr)
            ApplyEditorShape(shape, settings, bodyInterface, bodyID, updateMassProperties, vke_physics::PHYSICS_SHAPE_SPHERE, CreateEditorShape(vke_physics::PHYSICS_SHAPE_SPHERE));

        int shapeType = static_cast<int>(shape->type);
        const char *shapeTypeItems[] = {"Sphere", "Box", "Capsule", "Cylinder", "Triangle", "Plane"};
        const int shapeTypeCount = allowStaticOnlyShapes ? IM_ARRAYSIZE(shapeTypeItems) : 4;
        if (!allowStaticOnlyShapes && !IsDynamicBodyShape(shape->type))
        {
            ApplyEditorShape(shape, settings, bodyInterface, bodyID, updateMassProperties, vke_physics::PHYSICS_SHAPE_BOX, CreateEditorShape(vke_physics::PHYSICS_SHAPE_BOX));
            shapeType = static_cast<int>(shape->type);
            changed = true;
        }

        if (ImGui::Combo("Shape Type", &shapeType, shapeTypeItems, shapeTypeCount))
        {
            shapeType = std::clamp(shapeType, 0, static_cast<int>(vke_physics::PHYSICS_SHAPE_PLANE));
            const auto type = static_cast<vke_physics::PhyscisShapeType>(shapeType);
            ApplyEditorShape(shape, settings, bodyInterface, bodyID, updateMassProperties, type, CreateEditorShape(type));
            changed = true;
        }

        const auto type = shape->type;
        switch (type)
        {
        case vke_physics::PHYSICS_SHAPE_SPHERE:
        {
            auto *sphere = static_cast<const JPH::SphereShape *>(shape->shapeRef.GetPtr());
            float radius = sphere->GetRadius();
            if (ImGui::InputFloat("Radius", &radius, 0.05f, 0.25f, "%.3f"))
            {
                radius = std::max(radius, 0.001f);
                ApplyEditorShape(shape, settings, bodyInterface, bodyID, updateMassProperties, type, new JPH::SphereShape(radius));
                changed = true;
            }
            break;
        }
        case vke_physics::PHYSICS_SHAPE_BOX:
        {
            auto *box = static_cast<const JPH::BoxShape *>(shape->shapeRef.GetPtr());
            const JPH::Vec3 &extent = box->GetHalfExtent();
            float halfExtent[3] = {extent.GetX(), extent.GetY(), extent.GetZ()};
            if (ImGui::InputFloat3("Half Extent", halfExtent, "%.3f"))
            {
                halfExtent[0] = std::max(halfExtent[0], 0.001f);
                halfExtent[1] = std::max(halfExtent[1], 0.001f);
                halfExtent[2] = std::max(halfExtent[2], 0.001f);
                ApplyEditorShape(shape, settings, bodyInterface, bodyID, updateMassProperties, type, new JPH::BoxShape(JPH::Vec3(halfExtent[0], halfExtent[1], halfExtent[2])));
                changed = true;
            }
            break;
        }
        case vke_physics::PHYSICS_SHAPE_CAPSULE:
        {
            auto *capsule = static_cast<const JPH::CapsuleShape *>(shape->shapeRef.GetPtr());
            float halfHeight = capsule->GetHalfHeightOfCylinder();
            float radius = capsule->GetRadius();
            bool valueChanged = ImGui::InputFloat("Half Height", &halfHeight, 0.05f, 0.25f, "%.3f");
            valueChanged |= ImGui::InputFloat("Radius", &radius, 0.05f, 0.25f, "%.3f");
            if (valueChanged)
            {
                halfHeight = std::max(halfHeight, 0.001f);
                radius = std::max(radius, 0.001f);
                ApplyEditorShape(shape, settings, bodyInterface, bodyID, updateMassProperties, type, new JPH::CapsuleShape(halfHeight, radius));
                changed = true;
            }
            break;
        }
        case vke_physics::PHYSICS_SHAPE_CYLINDER:
        {
            auto *cylinder = static_cast<const JPH::CylinderShape *>(shape->shapeRef.GetPtr());
            float halfHeight = cylinder->GetHalfHeight();
            float radius = cylinder->GetRadius();
            bool valueChanged = ImGui::InputFloat("Half Height", &halfHeight, 0.05f, 0.25f, "%.3f");
            valueChanged |= ImGui::InputFloat("Radius", &radius, 0.05f, 0.25f, "%.3f");
            if (valueChanged)
            {
                halfHeight = std::max(halfHeight, 0.001f);
                radius = std::max(radius, 0.001f);
                ApplyEditorShape(shape, settings, bodyInterface, bodyID, updateMassProperties, type, new JPH::CylinderShape(halfHeight, radius));
                changed = true;
            }
            break;
        }
        case vke_physics::PHYSICS_SHAPE_TRIANGLE:
        {
            auto *triangle = static_cast<const JPH::TriangleShape *>(shape->shapeRef.GetPtr());
            const JPH::Vec3 &vertex1 = triangle->GetVertex1();
            const JPH::Vec3 &vertex2 = triangle->GetVertex2();
            const JPH::Vec3 &vertex3 = triangle->GetVertex3();
            float v1[3] = {vertex1.GetX(), vertex1.GetY(), vertex1.GetZ()};
            float v2[3] = {vertex2.GetX(), vertex2.GetY(), vertex2.GetZ()};
            float v3[3] = {vertex3.GetX(), vertex3.GetY(), vertex3.GetZ()};
            bool valueChanged = ImGui::InputFloat3("V1", v1, "%.3f");
            valueChanged |= ImGui::InputFloat3("V2", v2, "%.3f");
            valueChanged |= ImGui::InputFloat3("V3", v3, "%.3f");
            if (valueChanged)
            {
                ApplyEditorShape(shape, settings, bodyInterface, bodyID, updateMassProperties, type, new JPH::TriangleShape(JPH::Vec3(v1[0], v1[1], v1[2]), JPH::Vec3(v2[0], v2[1], v2[2]), JPH::Vec3(v3[0], v3[1], v3[2])));
                changed = true;
            }
            break;
        }
        case vke_physics::PHYSICS_SHAPE_PLANE:
        {
            auto *planeShape = static_cast<const JPH::PlaneShape *>(shape->shapeRef.GetPtr());
            const JPH::Plane &plane = planeShape->GetPlane();
            const JPH::Vec3 &normalValue = plane.GetNormal();
            float normal[3] = {normalValue.GetX(), normalValue.GetY(), normalValue.GetZ()};
            float constant = plane.GetConstant();
            bool valueChanged = ImGui::InputFloat3("Normal", normal, "%.3f");
            valueChanged |= ImGui::InputFloat("Constant", &constant, 0.05f, 0.25f, "%.3f");
            if (valueChanged)
            {
                float length = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
                if (length < 0.0001f)
                {
                    normal[0] = 0.0f;
                    normal[1] = 1.0f;
                    normal[2] = 0.0f;
                    length = 1.0f;
                }
                ApplyEditorShape(shape, settings, bodyInterface, bodyID, updateMassProperties, type, new JPH::PlaneShape(JPH::Plane(JPH::Vec3(normal[0] / length, normal[1] / length, normal[2] / length), constant)));
                changed = true;
            }
            break;
        }
        default:
            break;
        }

        ImGui::PopID();
        return changed;
    }

    static bool EnsureDynamicBodyShape(std::shared_ptr<vke_physics::PhyscisShape> &shape,
                                       JPH::BodyCreationSettings &settings,
                                       JPH::BodyInterface &bodyInterface,
                                       JPH::BodyID bodyID)
    {
        if (shape != nullptr && IsDynamicBodyShape(shape->type))
            return false;

        ApplyEditorShape(shape, settings, bodyInterface, bodyID, true, vke_physics::PHYSICS_SHAPE_BOX, CreateEditorShape(vke_physics::PHYSICS_SHAPE_BOX));
        return true;
    }
}

#endif
