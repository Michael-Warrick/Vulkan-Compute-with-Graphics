#version 460
#define PI 3.14159265358979323846

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 projection;
}ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoords;

// Structure to hold particle data
struct Particle {
    vec3 position;
    vec3 velocity;
};

layout(set = 0, binding = 2) buffer ParticleBuffer {
    Particle computeParticles[];
};

// Reimplementation of glm::translate()
mat4 translate(mat4 matrix, vec3 translation) {
    mat4 translationMatrix = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        translation.x, translation.y, translation.z, 1.0
    );
    return translationMatrix * matrix;
}


// Degrees to radians helper function
float degreesToRadians(float angleInDegrees) {
    return angleInDegrees * 3.14159265358979323846 / 180.0;
}

// Reimplementation of glm::rotate()
mat4 rotate(mat4 model, float angleInRadians, vec3 axis) {
    // Normalize the axis vector
    vec3 normAxis = normalize(axis);
    
    // Compute sine and cosine of the angle
    float c = cos(angleInRadians);
    float s = sin(angleInRadians);
    float t = 1.0 - c;
    
    // Extract the axis components
    float x = normAxis.x;
    float y = normAxis.y;
    float z = normAxis.z;
    
    // Compute the rotation matrix
    mat4 rotation = mat4(
        t * x * x + c,      t * x * y - s * z,  t * x * z + s * y,  0.0,
        t * x * y + s * z,  t * y * y + c,      t * y * z - s * x,  0.0,
        t * x * z - s * y,  t * y * z + s * x,  t * z * z + c,      0.0,
        0.0,                0.0,                0.0,                1.0
    );
    
    // Apply the rotation to the model matrix
    return rotation * model;
}

void main() 
{
    // float frequency = 0.5;
    // float amplitude = 0.1;

    // float sineWave = sin(ubo.deltaTime * frequency * 2.0 * 3.14159265358979323846);
    // vec3 translation = vec3(0.0, amplitude * sineWave, 0.0);
    // vec3 rotationAxis = vec3(0.0, 1.0, 0.0);

    mat4 transformedModel;
    transformedModel = translate(ubo.model, computeParticles[0].position);
    // transformedModel = rotate(transformedModel, (ubo.deltaTime * 0.4) * degreesToRadians(90.0), rotationAxis);
    gl_Position = ubo.projection * ubo.view * transformedModel * vec4(inPosition, 1.0);

    fragColor = inColor;
    fragTexCoords = inTexCoords;
}