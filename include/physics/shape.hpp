#ifndef PHYSICS_SHAPE_H
#define PHYSICS_SHAPE_H

#include <physics/physics.hpp>
#include <asset.hpp>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/TriangleShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>

namespace vke_physics
{
    enum PhyscisShapeType
    {
        PHYSICS_SHAPE_SPHERE,
        PHYSICS_SHAPE_BOX,
        PHYSICS_SHAPE_CAPSULE,
        PHYSICS_SHAPE_CYLINDER,
        PHYSICS_SHAPE_TRIANGLE,
        PHYSICS_SHAPE_PLANE,

    };

    class PhyscisShape
    {
    public:
        PhyscisShapeType type;
        JPH::ShapeRefC shapeRef;

        PhyscisShape() : shapeRef(nullptr) {}
        PhyscisShape(PhyscisShapeType type) : type(type), shapeRef(nullptr) {}

        PhyscisShape(nlohmann::json &json) : type(json["type"])
        {
            switch (type)
            {
            case PHYSICS_SHAPE_SPHERE:
                shapeRef = new JPH::SphereShape((float)json["radius"]);
                break;
            case PHYSICS_SHAPE_BOX:
            {
                auto &extent = json["halfExtent"];
                shapeRef = new JPH::BoxShape(JPH::Vec3Arg(extent[0], extent[1], extent[2]));
            }
            break;
            case PHYSICS_SHAPE_CAPSULE:
                shapeRef = new JPH::CapsuleShape((float)json["halfHeight"], (float)json["radius"]);
                break;
            case PHYSICS_SHAPE_CYLINDER:
                shapeRef = new JPH::CylinderShape((float)json["halfHeight"], (float)json["radius"]);
                break;
            case PHYSICS_SHAPE_TRIANGLE:
            {
                auto &v1 = json["v1"];
                auto &v2 = json["v2"];
                auto &v3 = json["v3"];
                shapeRef = new JPH::TriangleShape(JPH::Vec3Arg(v1[0], v1[1], v1[2]),
                                                  JPH::Vec3Arg(v2[0], v2[1], v2[2]),
                                                  JPH::Vec3Arg(v3[0], v3[1], v3[2]));
            }
            break;
            case PHYSICS_SHAPE_PLANE:
            {
                auto &normal = json["normal"];
                JPH::Plane plane(JPH::Vec3Arg(normal[0], normal[1], normal[2]), json["c"]);
                shapeRef = new JPH::PlaneShape(plane);
            }
            break;
            default:
                break;
            }
        }

        std::string ToJSON()
        {
            if (shapeRef == nullptr)
                return "{}";
            std::string ret = "{ \"type\": " + std::to_string((int)type);
            switch (type)
            {
            case PHYSICS_SHAPE_SPHERE:
            {
                JPH::SphereShape *shape = (JPH::SphereShape *)shapeRef.GetPtr();
                ret += ", \"radius\": " + std::to_string(shape->GetRadius());
            }
            break;
            case PHYSICS_SHAPE_BOX:
            {
                JPH::BoxShape *shape = (JPH::BoxShape *)shapeRef.GetPtr();
                const JPH::Vec3 &extent = shape->GetHalfExtent();
                ret += ", \"halfExtent\": [" + std::to_string(extent.GetX()) + ", " + std::to_string(extent.GetY()) + ", " + std::to_string(extent.GetZ()) + "]";
            }
            break;
            case PHYSICS_SHAPE_CAPSULE:
            {
                JPH::CapsuleShape *shape = (JPH::CapsuleShape *)shapeRef.GetPtr();
                ret += ", \"radius\": " + std::to_string(shape->GetRadius()) + ", \"halfHeight\": " + std::to_string(shape->GetHalfHeightOfCylinder());
            }
            break;
            case PHYSICS_SHAPE_CYLINDER:
            {
                JPH::CylinderShape *shape = (JPH::CylinderShape *)shapeRef.GetPtr();
                ret += ", \"radius\": " + std::to_string(shape->GetRadius()) + ", \"halfHeight\": " + std::to_string(shape->GetHalfHeight());
            }
            break;
            case PHYSICS_SHAPE_TRIANGLE:
            {
                JPH::TriangleShape *shape = (JPH::TriangleShape *)shapeRef.GetPtr();
                const JPH::Vec3 &v1 = shape->GetVertex1();
                const JPH::Vec3 &v2 = shape->GetVertex2();
                const JPH::Vec3 &v3 = shape->GetVertex3();
                ret += ", \"v1\": [" + std::to_string(v1.GetX()) + ", " + std::to_string(v1.GetY()) + ", " + std::to_string(v1.GetZ()) + "]";
                ret += ", \"v2\": [" + std::to_string(v2.GetX()) + ", " + std::to_string(v2.GetY()) + ", " + std::to_string(v2.GetZ()) + "]";
                ret += ", \"v3\": [" + std::to_string(v3.GetX()) + ", " + std::to_string(v3.GetY()) + ", " + std::to_string(v3.GetZ()) + "]";
            }
            break;
            case PHYSICS_SHAPE_PLANE:
            {
                JPH::PlaneShape *shape = (JPH::PlaneShape *)shapeRef.GetPtr();
                const JPH::Plane &plane = shape->GetPlane();
                const JPH::Vec3 &normal = plane.GetNormal();
                ret += ", \"normal\": [" + std::to_string(normal.GetX()) + ", " + std::to_string(normal.GetY()) + ", " + std::to_string(normal.GetZ()) + "]";
                ret += ", \"c\":" + std::to_string(plane.GetConstant());
            }
            break;
            default:
                break;
            }
            ret += " }";
            return ret;
        }
    };
}
#endif