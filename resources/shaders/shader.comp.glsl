#version 460

struct PhysicsObject {
    vec3 position;
    vec4 rotation;
    vec3 velocity;
    vec3 angularVelocity;
    float radius;
    float mass;
    float elasticity;
    float momentOfInertia;
};

layout(binding = 0) uniform ParameterUBO {
    float physicsTimeStep;
} ubo;

layout(std140, binding = 1) readonly buffer PhysicsObjectSSBOIn {
   PhysicsObject objectsIn[];
};

layout(std140, binding = 2) buffer PhysicsObjectSSBOOut {
   PhysicsObject objectsOut[];
};

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

bool isCollidingSphereWithPlane(inout PhysicsObject sphere) {
    if ((sphere.position.y - sphere.radius) <= 0.0) {
        return true;
    }

    return false;
}

bool isCollidingSphereWithSphere(inout PhysicsObject sphereOne, inout PhysicsObject sphereTwo) {
    // Squaring radii to avoid calling sqrt()
    float sumRadiiSquared = (sphereOne.radius + sphereTwo.radius) * (sphereOne.radius + sphereTwo.radius);
    float distanceSquared = (sphereTwo.position.x - sphereOne.position.x) * (sphereTwo.position.x - sphereOne.position.x)
                          + (sphereTwo.position.y - sphereOne.position.y) * (sphereTwo.position.y - sphereOne.position.y)
                          + (sphereTwo.position.z - sphereOne.position.z) * (sphereTwo.position.z - sphereOne.position.z);

    return distanceSquared <= sumRadiiSquared;
}

void resolveCollisionSphereWithPlane(inout PhysicsObject sphere) {
    const float planeFrictionCoefficient = 0.5;

    sphere.velocity.y = -sphere.velocity.y * sphere.elasticity;
    sphere.position.y = 0.0 + sphere.radius;

    vec3 tangentialVelocity = vec3(sphere.velocity.x, 0.0, sphere.velocity.z);
    vec3 frictionImpulse = -planeFrictionCoefficient * tangentialVelocity;

    sphere.velocity += frictionImpulse;
}

void resolveCollisionSphereWithSphere(inout PhysicsObject sphereOne, inout PhysicsObject sphereTwo) {
    vec3 normalDirection = sphereOne.position - sphereTwo.position;
    vec3 normalizedNormalDirection = normalize(normalDirection);

    // Calculate relative velocity
    vec3 relativeVelocity = sphereOne.velocity - sphereTwo.velocity;

    // Calculate velocity along normal
    float velocityAlongNormal = dot(relativeVelocity, normalizedNormalDirection);

    // If spheres are moving apart, no need to resolve the collision
    if (velocityAlongNormal > 0.0) {
        return;
    }

    // Calculate combined Coefficient of Restitution
    float e = min(sphereOne.elasticity, sphereTwo.elasticity);

    // Calculate impulse scalar
    float j = -(1.0 + e) * velocityAlongNormal;
    j /= (1.0 / sphereOne.mass) + (1.0 / sphereTwo.mass);

    // Apply impulse to spheres
    vec3 impulse = j * normalizedNormalDirection;
    sphereOne.velocity += impulse / sphereOne.mass;
    sphereTwo.velocity -= impulse / sphereTwo.mass;
}

void main() {
    const vec3 gravity = vec3(0.0, -9.81, 0.0);

    uint index = gl_GlobalInvocationID.x;
    PhysicsObject objectIn = objectsIn[index];

    objectsOut[index].velocity = objectIn.velocity + gravity * ubo.physicsTimeStep;
    objectsOut[index].position = objectIn.position + objectsOut[index].velocity * ubo.physicsTimeStep;
    objectsOut[index].radius = objectIn.radius;
    objectsOut[index].mass = objectIn.mass;
    objectsOut[index].elasticity = objectIn.elasticity;

    if (isCollidingSphereWithPlane(objectsOut[index])) {
        resolveCollisionSphereWithPlane(objectsOut[index]);
    }

    for (uint i = 0; i < objectsIn.length(); ++i) {
        if (i != index) {
            if (isCollidingSphereWithSphere(objectsOut[index], objectsOut[i])) {
                resolveCollisionSphereWithSphere(objectsOut[index], objectsOut[i]);
            }
        }
    }
}