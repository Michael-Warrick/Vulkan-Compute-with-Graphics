#version 460

struct Particle {
    vec3 position;
    vec3 velocity;
};

layout(binding = 0) uniform ParameterUBO {
    float physicsTimeStep;
} ubo;

layout(std140, binding = 1) readonly buffer ParticleSSBOIn {
   Particle particlesIn[];
};

layout(std140, binding = 2) buffer ParticleSSBOOut {
   Particle particlesOut[];
};

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

void main() {
    const vec3 gravity = vec3(0.0, -9.81, 0.0);
    const float damping = 0.8;

    uint index = gl_GlobalInvocationID.x;
    Particle particleIn = particlesIn[index];

    // Apply gravity
    vec3 newVelocity = particleIn.velocity + gravity * ubo.physicsTimeStep;

    // Update position based on velocity
    vec3 newPosition = particleIn.position + newVelocity * ubo.physicsTimeStep;

    // Check if the particle hits the ground
    if (newPosition.y <= 0.0) {
        // Flip the y velocity and position to be above the ground
        newVelocity.y = -newVelocity.y * damping;
        newPosition.y = 0.0;
    }

    particlesOut[index].position = newPosition;
    particlesOut[index].velocity = newVelocity;
}