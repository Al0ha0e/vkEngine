using System;
using vkEngine.EngineCore;

namespace TestProj;

public sealed class ContactLoggerScript : EntityScript
{
    private RigidBody? rigidBody;
    private Sensor? sensor;

    public ContactLoggerScript(UInt32 entity) : base(entity)
    {
    }

    public override void Start()
    {
        rigidBody = GetComponent<RigidBody>();
        if (rigidBody != null)
        {
            rigidBody.ContactAdded = LogContactAdded;
            rigidBody.ContactPersisted = LogContactPersisted;
            rigidBody.ContactRemoved = LogContactRemoved;
            Console.WriteLine($"ContactLoggerScript registered RigidBody contact callbacks entity={Entity} body={rigidBody.BodyID}.");
            return;
        }

        sensor = GetComponent<Sensor>();
        if (sensor != null)
        {
            sensor.ContactAdded = LogContactAdded;
            sensor.ContactPersisted = LogContactPersisted;
            sensor.ContactRemoved = LogContactRemoved;
            Console.WriteLine($"ContactLoggerScript registered Sensor contact callbacks entity={Entity} body={sensor.BodyID}.");
            return;
        }

        Console.WriteLine($"ContactLoggerScript entity={Entity} has no RigidBody or Sensor.");
    }

    public override void Unload()
    {
        if (rigidBody != null)
        {
            rigidBody.ContactAdded = null;
            rigidBody.ContactPersisted = null;
            rigidBody.ContactRemoved = null;
            rigidBody = null;
        }

        if (sensor != null)
        {
            sensor.ContactAdded = null;
            sensor.ContactPersisted = null;
            sensor.ContactRemoved = null;
            sensor = null;
        }
    }

    private static void LogContactAdded(ContactEvent contact)
    {
        LogContact("ContactAdded", contact);
    }

    private static void LogContactPersisted(ContactEvent contact)
    {
        LogContact("ContactPersisted", contact);
    }

    private static void LogContactRemoved(ContactEvent contact)
    {
        LogContact("ContactRemoved", contact);
    }

    private static void LogContact(string label, ContactEvent contact)
    {
        Console.WriteLine(
            $"{label} self={contact.SelfEntity} other={contact.OtherEntity} selfBody={contact.SelfBodyID} otherBody={contact.OtherBodyID} isSensor={contact.IsSensor} point={contact.SelfPoint} normal={contact.Normal} depth={contact.PenetrationDepth}");
    }
}
