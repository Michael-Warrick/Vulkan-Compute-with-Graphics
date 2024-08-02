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

layout(set = 0, binding = 2) buffer PhysicsObjectBuffer {
    PhysicsObject physicsObjects[];
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
    return angleInDegrees * PI / 180.0;
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

mat4 quatToMat4(vec4 quat) {
    float quatXX = quat.x * quat.x;
    float quatYY = quat.y * quat.y;
    float quatZZ = quat.z * quat.z;

    float quatXZ = quat.x * quat.z;
    float quatXY = quat.x * quat.y;
    float quatYZ = quat.y * quat.z;

    float quatWX = quat.w * quat.x;
    float quatWY = quat.w * quat.y;
    float quatWZ = quat.w * quat.z;

    mat4 resultMatrix = mat4(1.0);
    resultMatrix[0][0] = 1.0 - 2.0 * (quatYY + quatZZ);
    resultMatrix[0][1] = 2.0 * (quatXY + quatWZ);
    resultMatrix[0][2] = 2.0 * (quatXZ - quatWY);

    resultMatrix[1][0] = 2.0 * (quatXY - quatWZ);
    resultMatrix[1][1] = 1.0 - 2.0 * (quatXX + quatZZ);
    resultMatrix[1][2] = 2.0 * (quatYZ + quatWX);

    resultMatrix[2][0] = 2.0 * (quatXZ + quatWY);
    resultMatrix[2][1] = 2.0 * (quatYZ - quatWX);
    resultMatrix[2][2] = 1.0 - 2.0 * (quatXX + quatYY);

    return resultMatrix;
}

mat4 scale(mat4 matrix, float scalingFactor) {
    mat4 scalingMatrix = mat4(
        scalingFactor, 0.0, 0.0, 0.0,
        0.0, scalingFactor, 0.0, 0.0,
        0.0, 0.0, scalingFactor, 0.0,
        0.0, 0.0, 0.0, 1.0
    );

    return scalingMatrix * matrix;
}

void main() 
{
    uint instanceIndex = gl_InstanceIndex;
    
    mat4 transformedModel = scale(ubo.model, (physicsObjects[instanceIndex].radius * 2) / 0.23);
    transformedModel = translate(transformedModel, physicsObjects[instanceIndex].position);

    mat4 rotationMatrix = quatToMat4(physicsObjects[instanceIndex].rotation);
    transformedModel = transformedModel * rotationMatrix;

    gl_Position = ubo.projection * ubo.view * transformedModel * vec4(inPosition, 1.0);

    fragColor = inColor;
    fragTexCoords = inTexCoords;
}