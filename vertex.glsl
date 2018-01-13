attribute vec4 position;
attribute vec4 normal;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 normalMatrix;
uniform vec2 modelPosition;

varying vec4 varyingNormal;

void main()
{
	varyingNormal = normalize(normalMatrix * normal);
	gl_Position = vec4(modelPosition.x, modelPosition.y, 0.0, 0.0) + projectionMatrix * modelViewMatrix * position;
}