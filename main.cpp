#include "glsupport.h"
#include <glut.h>
#include "matrix4.h"
#include "quat.h"
#include "geometrymaker.h"

GLuint modelviewMatrixUniformLocation;
GLuint projectionMatrixUniformLocation;
GLuint normalMatrixUniformLocation;
GLuint color;
GLuint program;
GLuint vertexBO;
GLuint indexBO;
GLuint positionAttribute;
GLuint normalAttribute;
Matrix4 eyeMatrix;
Quat ZRotation = Quat::makeZRotation(10.0f);
GLint loc;
static int i = 1;

struct VertexPN{
	Cvec3f p, n;
	VertexPN() {}
	VertexPN(float x, float y, float z, float nx, float ny, float nz) : p(x, y, z), n(nx, ny, nz) {}
	VertexPN& operator = (const GenericVertex& v) {
		p = v.pos;
		n = v.normal;
		return *this;
	}
};

struct Transform{
	Quat rotation;
	Cvec3 scale;
	Cvec3 position;
	Transform() : scale(1.0f, 1.0f, 1.0f){
	}
	//return object matrix
	Matrix4 createMatrix(){
		Matrix4 objectMatrix;
		objectMatrix = Matrix4::makeTranslation(position) * quatToMatrix(rotation) * Matrix4::makeScale(scale);
		return objectMatrix;
	}
};

struct Geometry{
	int numIndices;
	void Draw(GLuint positionAttribute, GLuint normalAttribute){
		glBindBuffer(GL_ARRAY_BUFFER, vertexBO);
		glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), (void*)offsetof(VertexPN, p));
		glEnableVertexAttribArray(positionAttribute);

		glVertexAttribPointer(normalAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), (void*)offsetof(VertexPN, n));
		glEnableVertexAttribArray(normalAttribute);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBO);
		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, 0);
	}
};

struct Entity{
	Transform transform;
	Geometry geometry;
	Entity *parent;
	//get the entity's parents' matrixes recursively, remove scale matrixes and multiplied by objectMatrix
	Matrix4 getObjectMatrix(struct Entity *object)	{
		if (object->parent == NULL)
			return (object->transform).createMatrix();
		else
			return getObjectMatrix(object->parent) * inv(Matrix4::makeScale((object->parent->transform).scale)) * (object->transform).createMatrix();
	}
	
	void Draw(Matrix4 &eyeInverse, GLuint positionAttribute, GLuint normalAttribute, GLuint modelviewMatrixUniformLocation, GLuint normalMatrixUniformLocation){
		Matrix4 modelViewMatrix;		
		modelViewMatrix = eyeInverse * getObjectMatrix(this);
		
		Matrix4 normMatrix = normalMatrix(modelViewMatrix);
		GLfloat glmatrix[16];
		modelViewMatrix.writeToColumnMajorMatrix(glmatrix);
		glUniformMatrix4fv(modelviewMatrixUniformLocation, 1, false, glmatrix);

		GLfloat glmatrixNormal[16];
		normMatrix.writeToColumnMajorMatrix(glmatrixNormal);
		glUniformMatrix4fv(normalMatrixUniformLocation, 1, false, glmatrixNormal);

		geometry.Draw(positionAttribute, normalAttribute);
	}
};

Entity primitive1, primitive2, primitive3;

int createPrimitiveVBO(float size){
	int vbLen, ibLen;
	getCubeVbIbLen(vbLen, ibLen);
	std::vector<VertexPN> vtx(vbLen);
	std::vector<unsigned short> idx(ibLen);
	makeCube(size, vtx.begin(), idx.begin());//create a cube specified by the size

	glGenBuffers(1, &vertexBO);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPN) * vtx.size(), vtx.data(), GL_STATIC_DRAW);
	glGenBuffers(1, &indexBO);
	glBindBuffer(GL_ARRAY_BUFFER, indexBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned short) * idx.size(), idx.data(), GL_STATIC_DRAW);
	return ibLen;
}

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	
	primitive1.Draw(inv(eyeMatrix), positionAttribute, normalAttribute, modelviewMatrixUniformLocation, normalMatrixUniformLocation);	//draw primitive1
	primitive2.Draw(inv(eyeMatrix), positionAttribute, normalAttribute, modelviewMatrixUniformLocation, normalMatrixUniformLocation);	//draw primitive2
	primitive3.Draw(inv(eyeMatrix), positionAttribute, normalAttribute, modelviewMatrixUniformLocation, normalMatrixUniformLocation);   //draw primitive3
		
	glutSwapBuffers();	
}

void init() {
	glClearDepth(0.0f);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GREATER);
	glReadBuffer(GL_BACK);

	program = glCreateProgram();
	readAndCompileShader(program, "vertex.glsl", "fragment.glsl");
	glUseProgram(program);

	positionAttribute = glGetAttribLocation(program, "position");
	normalAttribute = glGetAttribLocation(program, "normal");

	modelviewMatrixUniformLocation = glGetUniformLocation(program, "modelViewMatrix");
	projectionMatrixUniformLocation = glGetUniformLocation(program, "projectionMatrix");
	normalMatrixUniformLocation = glGetUniformLocation(program, "normalMatrix");

	color = glGetUniformLocation(program, "uColor");
	loc = glGetUniformLocation(program, "modelPosition");

	eyeMatrix = eyeMatrix.makeTranslation(Cvec3(0.0, 0.0, 60.0));

	Matrix4 projectionMatrix;
	projectionMatrix = projectionMatrix.makeProjection(45.0, 1.0, -0.1, -100.0);
	GLfloat glmatrixProjection[16];
	projectionMatrix.writeToColumnMajorMatrix(glmatrixProjection);
	glUniformMatrix4fv(projectionMatrixUniformLocation, 1, false, glmatrixProjection);

	primitive1.parent = NULL;//primitive1 don't have parent
	primitive2.parent = &primitive1;//set primitive1 as primitive2's parent
	primitive3.parent = &primitive2;//set primitive2 as primitive3's parent

	primitive1.transform.scale = Cvec3(1.0, 3.0, 1.0);//set scale
	primitive1.transform.position = Cvec3(-9.0, -5.0, 0.0);//set position
	primitive1.geometry.numIndices = createPrimitiveVBO(6.0);//create primitive1
	primitive2.transform.scale = Cvec3(3.0, 0.5, 1.0);
	primitive2.transform.position = Cvec3(9.0, 5.0, 0.0);
	primitive2.geometry.numIndices = createPrimitiveVBO(8.0);
	primitive3.transform.scale = Cvec3(0.5, 2.0, 1.0);
	primitive3.transform.position = Cvec3(10.0, -3.0, 0.0);
	primitive3.geometry.numIndices = createPrimitiveVBO(8.0);	

	glUniform3f(color, 1.0, 0.0, 0.0);//set object's color using the uColor uniform

	glDisableVertexAttribArray(positionAttribute);
	glDisableVertexAttribArray(normalAttribute);
}

void reshape(int w, int h) {
	glViewport(0, 0, w, h);
}

void idle(void) {
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{		
	switch (key) {
	case 'a':
		//rotate primitive1 around Z axis counterclockwise by 10 degree when key a is pressed
		primitive1.transform.rotation = primitive1.transform.rotation * ZRotation;
		glutPostRedisplay();
		break;
	case 's':
		//rotate primitive2 around Z axis counterclockwise by 10 degree when key s is pressed
		primitive2.transform.rotation = primitive2.transform.rotation * ZRotation;
		glutPostRedisplay();
		break;
	case 'd':
		//rotate primitive3 around Z axis counterclockwise by 10 degree when key d is pressed
		primitive3.transform.rotation = primitive3.transform.rotation * ZRotation;
		glutPostRedisplay();
		break;	
	case 'z':
		//move objects along X axis when key z is pressed using a shader uniform in the vertex shader
		if (loc != -1)
		{
			glUniform2f(loc, 5.0 * i, 0.0);
			i++;
		}
		glutPostRedisplay();
		break;
	default:
		break;
	}
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(500, 500);
	glutCreateWindow("CS-6533");

	glewInit();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIdleFunc(idle);
	glutKeyboardUpFunc(keyboard);

	init();
	glutMainLoop();
	return 0;
}

