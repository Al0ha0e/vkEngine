struct DirectionalLight {
    vec4 direction; //xyz
    vec4 color;     //w: intensity
};


layout(set = 0, binding = 1) uniform LightBlockObject {
   DirectionalLight lights[];
} DirectionalLights;