/*
Eric Yang
110379834
Project 05: Water Simulation

This program simulates water and waves that can be created.
Each wave is calculated using a complex equation using sin, cos, and tan

Approach:
    I generated multiple quads and then would manipulate their vertex positions to simulate water.
    The wave size can be controlled to a min and max.

Note:
    This program does not use any shaders as they have not been working on my computer
    This program was done completely using buffers.

Controls:
    Up Button -> Increase wave height (Capped)
    Down Button -> Decrease wave height (Capped)
    Left Button -> Rotate all Left
    Right Button -> Rotate all Right
    Space Button -> Pause wave generation
    'w' -> Move Forward (Capped)
    's' -> Move Backwards (Capped)
    'a' -> Move Left (Capped)
    'd' -> Move Right (Capped)
    'q' -> Move Down (Capped)
    'e' -> Move Up (Capped)


*/
#define GLEW_STATIC
#include<glew.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include<SOIL.h>
#include <string.h>
#include<fstream>
#define PI 3.14159

using namespace std;

bool WireFrame= false;
float i =0;
const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

float xpos =0;
float ypos =0;
float Wwidth,Wheight;

//Shader Code
unsigned int vs, fs, gs, program;
GLint positionAttribute, loc, mvloc, location_time, location_cameraPosition, location_pmvMatrix;
GLfloat modelView[16];
GLfloat projection[16];

//Rotations
float rotX, rotY = 0.0;
//Textures
GLuint tex;
GLuint skyboxTextures[6];
//Misc.
float scale = 1.0;
float time = 0.0;
float waves = 0.0;
bool makeWaves = true;
float waveScale = 3;
float moveX, moveY, moveZ;
float minHeight = -0.01;
/* GLUT callback Handlers */


void readShaderFile(char* fileName, string &str){
    ifstream in(fileName);
    char tmp[1024];

    if(!in.is_open()){
        cout << "The file " << fileName << " is not available "<<endl;
    }
    else{
        while(!in.eof()){
            in.getline(tmp,1024);
            str+=tmp;
            str +='\n';
            //cout<< tmp<<endl;
        }
    }
}

unsigned int loadShader(string &source, unsigned int mode)
{
    unsigned int id;
    char error[1024];

    id = glCreateShader(mode);

    const char *cSrc = source.c_str();

    glShaderSource(id,1,&cSrc,NULL);
    glCompileShader(id);

    glGetShaderInfoLog(id,1024,NULL,error);

    cout<<" Compile Status: \n"<< error<<endl;
    return id;
}

void initShader(char *vName, char *fName, char *gName)
{
    string source = "";

    program = glCreateProgram();

    readShaderFile(vName,source);
    vs = loadShader(source, GL_VERTEX_SHADER);
    source="";

    readShaderFile(gName,source);
    gs = loadShader(source, GL_GEOMETRY_SHADER);
    source="";

    readShaderFile(fName,source);
    fs = loadShader(source, GL_FRAGMENT_SHADER);
    source="";

    glAttachShader(program,vs);
    glAttachShader(program,gs);
    glAttachShader(program,fs);
    glLinkProgram(program);
    glValidateProgram(program);
    //glUseProgram(program);

    location_time   = glGetUniformLocation(program,"time");
    location_pmvMatrix = glGetUniformLocation(program,"projectionViewMatrix");
    location_cameraPosition  = glGetUniformLocation(program,"cameraPosition");
}


void TLoad(char* fileName, GLuint &tex){
    int width, height;
    unsigned char* image;
    glBindTexture(GL_TEXTURE_2D, tex);
    image = SOIL_load_image(fileName,&width,&height,0,SOIL_LOAD_RGBA); // let soil load image
    if(!image) {cout<< "Image not Found"<<endl;}
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,image);
    SOIL_free_image_data(image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
}


static void resize(int width, int height)
{
     double Ratio;

     Wwidth = (float)width;
     Wheight = (float)height;

     Ratio= (double)width /(double)height;

    glViewport(0,0,(GLsizei) width,(GLsizei) height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

	gluPerspective (45.0f,Ratio,0.1f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

 }

////////////////////////////////////////////Quad Init

int numQuads = 200; //number of quads we have
const int numVertices = 480000;  //60000 -> 50 | 120000 -> 100 | 480000 -> 200 | 1080000 -> 300   | 1920000 -> 400 | numQuads * numQuads * 12
const int numTexCoords = 320000; //40000 -> 50 | 80000 -> 100  | 320000 -> 200 | 720000 -> 300    | 1280000 -> 400 | numQuads * numQuads * 8
float quadVertices[numVertices];
float quadTexCoords[numTexCoords];
int numIndices = numQuads * numQuads * 6; // Number of indices (two triangles per quad)
GLuint* indices = new GLuint[numIndices];

//Generate all the quads for the waves
void createQuads(){
    for(int i = 0; i < numQuads; i++){
        for(int j = 0; j < numQuads; j++){
            quadVertices[j*12+(i*numQuads*12)] = (j*1.0)/numQuads;
            quadVertices[j*12+(i*numQuads*12)+1] = 0;
            quadVertices[j*12+(i*numQuads*12)+2] = (i*1.0)/numQuads;
            quadVertices[j*12+(i*numQuads*12)+3] = (j*1.0+1.0)/numQuads;
            quadVertices[j*12+(i*numQuads*12)+4] = 0;
            quadVertices[j*12+(i*numQuads*12)+5] = (i*1.0)/numQuads;
            quadVertices[j*12+(i*numQuads*12)+6] = (j*1.0+1.0)/numQuads;
            quadVertices[j*12+(i*numQuads*12)+7] = 0;
            quadVertices[j*12+(i*numQuads*12)+8] = (i*1.0+1.0)/numQuads;
            quadVertices[j*12+(i*numQuads*12)+9] = (j*1.0)/numQuads;
            quadVertices[j*12+(i*numQuads*12)+10] = 0;
            quadVertices[j*12+(i*numQuads*12)+11] = (i*1.0+1.0)/numQuads;

            //Each of the quad has their own texture
//            quadTexCoords[j*8+(i*numQuads*8)]   = 0.0;
//            quadTexCoords[j*8+(i*numQuads*8)+1] = 1.0;
//            quadTexCoords[j*8+(i*numQuads*8)+2] = 1.0;
//            quadTexCoords[j*8+(i*numQuads*8)+3] = 1.0;
//            quadTexCoords[j*8+(i*numQuads*8)+4] = 1.0;
//            quadTexCoords[j*8+(i*numQuads*8)+5] = 0.0;
//            quadTexCoords[j*8+(i*numQuads*8)+6] = 0.0;
//            quadTexCoords[j*8+(i*numQuads*8)+7] = 0.0;

            //All quads share one texture
            quadTexCoords[j*8+(i*numQuads*8)]   = (j*1.0)/numQuads;
            quadTexCoords[j*8+(i*numQuads*8)+1] = (i*1.0+1.0)/numQuads;
            quadTexCoords[j*8+(i*numQuads*8)+2] = (j*1.0+1.0)/numQuads;
            quadTexCoords[j*8+(i*numQuads*8)+3] = (i*1.0+1.0)/numQuads;
            quadTexCoords[j*8+(i*numQuads*8)+4] = (j*1.0+1.0)/numQuads;
            quadTexCoords[j*8+(i*numQuads*8)+5] = (i*1.0)/numQuads;
            quadTexCoords[j*8+(i*numQuads*8)+6] = (j*1.0)/numQuads;
            quadTexCoords[j*8+(i*numQuads*8)+7] = (i*1.0)/numQuads;
        }
    }

    //Generates the indexes -> Not used
    int index = 0;
    for (int i = 0; i < numQuads; i++) {
        for (int j = 0; j < numQuads; j++) {
            // Compute the indices of the four vertices of the quad
            int v1 = i * (numQuads + 1) + j;
            int v2 = v1 + 1;
            int v3 = v2 + (numQuads + 1);
            int v4 = v1 + (numQuads + 1);

            // Add the indices for the two triangles of the quad
            indices[index++] = v1;
            indices[index++] = v2;
            indices[index++] = v3;

            indices[index++] = v1;
            indices[index++] = v3;
            indices[index++] = v4;
        }
    }
}

//Initialize all of the buffers
GLuint quadVBO, quadIBO;

void initQuad() {
    createQuads();

    // Create and bind vertex buffer object
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

    // Upload vertex data to buffer object
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices) + sizeof(quadTexCoords), 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadVertices), quadVertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(quadVertices), sizeof(quadTexCoords), quadTexCoords);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &quadIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(GLuint), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
//Draw all of the quads using glDrawArrays
void drawQuad(){
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIBO);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glEnable(GL_BLEND);

    glVertexPointer(3, GL_FLOAT, 0, (void*)0);
    glTexCoordPointer(2, GL_FLOAT, 0, (void*)(sizeof(quadVertices)));

    glDrawArrays(GL_QUADS, 0, numVertices/3);
//    glDrawArrays(GL_TRIANGLES, 0, numVertices);
//    glDrawElements(GL_QUADS, numIndices, GL_UNSIGNED_INT, (void*)(0));

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//This manages the waves to make it look randomized. The smallestValue is used to move the skybox bottom wall
void createWaves(float time){
    float smallestValue = 100;
    for(int i = 0; i < numQuads; i++){
        for(int j = 0; j < numQuads; j++){
            // Create four waves with different frequencies and amplitudes
            //Vertex 1
            float x = (j*1.0) / numQuads;
            float z = (i*1.0) / numQuads;
            float y = sin(x * 10.0 + time) * 0.025 + cos(z * 10.0 + time * 1.5) * 0.015
                    + sin((x + z) * 20.0 + time * 0.5) * 0.01 + cos((x - z) * 15.0 + time * 2.0) * 0.02;
            //Vertex 2
            x = (j*1.0+1) / numQuads;
            z = (i*1.0) / numQuads;
            float y2 = sin(x * 10.0 + time) * 0.025 + cos(z * 10.0 + time * 1.5) * 0.015
                    + sin((x + z) * 20.0 + time * 0.5) * 0.01 + cos((x - z) * 15.0 + time * 2.0) * 0.02;
            //Vertex 3
            x = (j*1.0+1) / numQuads;
            z = (i*1.0+1) / numQuads;
            float y3 = sin(x * 10.0 + time) * 0.025 + cos(z * 10.0 + time * 1.5) * 0.015
                    + sin((x + z) * 20.0 + time * 0.5) * 0.01 + cos((x - z) * 15.0 + time * 2.0) * 0.02;
            //Vertex 4
            x = (j*1.0+0.01) / numQuads;
            z = (i*1.0+1) / numQuads;
            float y4 = sin(x * 10.0 + time) * 0.025 + cos(z * 10.0 + time * 1.5) * 0.015
                    + sin((x + z) * 20.0 + time * 0.5) * 0.01 + cos((x - z) * 15.0 + time * 2.0) * 0.02;

            //Generate the "y" of all verticies
            y = y/waveScale;
            y2 = y2/waveScale;
            y3 = y3/waveScale;
            y4 = y4/waveScale;
            quadVertices[j*12+(i*numQuads*12)+1] = y;
            quadVertices[j*12+(i*numQuads*12)+4] = y2;
            quadVertices[j*12+(i*numQuads*12)+7] = y3;
            quadVertices[j*12+(i*numQuads*12)+10] = y4;
            //Update the skybox bottom wall
            if(y < smallestValue)
                smallestValue = y;
            if(y2 < smallestValue)
                smallestValue = y2;
            if(y3 < smallestValue)
                smallestValue = y3;
            if(y4 < smallestValue)
                smallestValue = y4;
        }
    }
    minHeight = smallestValue;
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadVertices), quadVertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void skybox(){
    glDisable(GL_LIGHTING);
    glPushMatrix();
        glScalef(50, 50, 50);
        //front wall
        glBindTexture(GL_TEXTURE_2D, skyboxTextures[0]);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, 1.0, 1.0);
            glTexCoord2f(1.0, 0.0); glVertex3f(1.0, 1.0, 1.0);
            glTexCoord2f(1.0, 1.0); glVertex3f(1.0, -1.0, 1.0);
            glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, -1.0, 1.0);
        glEnd();

        //back wall
        glBindTexture(GL_TEXTURE_2D, skyboxTextures[1]);
        glBegin(GL_QUADS);
            glTexCoord2f(1.0, 0.0); glVertex3f(-1.0, 1.0, -1.0);
            glTexCoord2f(1.0, 1.0); glVertex3f(-1.0, -1.0, -1.0);
            glTexCoord2f(0.0, 1.0); glVertex3f(1.0, -1.0, -1.0);
            glTexCoord2f(0.0, 0.0); glVertex3f(1.0, 1.0, -1.0);
        glEnd();

        //top wall
        glBindTexture(GL_TEXTURE_2D, skyboxTextures[2]);
        glBegin(GL_QUADS);
            glTexCoord2f(1.0, 0.0); glVertex3f(1.0, 1.0, -1.0);
            glTexCoord2f(1.0, 1.0); glVertex3f(1.0, 1.0, 1.0);
            glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, 1.0, 1.0);
            glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, 1.0, -1.0);
        glEnd();

        //bottom wall
        glBindTexture(GL_TEXTURE_2D, tex);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, minHeight*4, 1.0);
            glTexCoord2f(1.0, 0.0); glVertex3f(1.0, minHeight*4, 1.0);
            glTexCoord2f(1.0, 1.0); glVertex3f(1.0, minHeight*4, -1.0);
            glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, minHeight*4, -1.0);
        glEnd();

        //left wall
        glBindTexture(GL_TEXTURE_2D, skyboxTextures[5]);
        glBegin(GL_QUADS);
            glTexCoord2f(1.0, 0.0); glVertex3f(1.0, 1.0, -1.0);
            glTexCoord2f(1.0, 1.0); glVertex3f(1.0, -1.0, -1.0);
            glTexCoord2f(0.0, 1.0); glVertex3f(1.0, -1.0, 1.0);
            glTexCoord2f(0.0, 0.0); glVertex3f(1.0, 1.0, 1.0);
        glEnd();

        //right wall
        glBindTexture(GL_TEXTURE_2D, skyboxTextures[4]);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, 1.0, -1.0);
            glTexCoord2f(1.0, 0.0); glVertex3f(-1.0, 1.0, 1.0);
            glTexCoord2f(1.0, 1.0); glVertex3f(-1.0, -1.0, 1.0);
            glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, -1.0, -1.0);
        glEnd();
    glPopMatrix();

//    glEnable(GL_LIGHTING);
}


static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(0,0,10,0.0,0.0,0.0,0.0,1.0,1000.0);

    if(WireFrame)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);		//Draw Our Mesh In Wireframe Mesh
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);		//Toggle WIRE FRAME

//    glUseProgram(program);
    // your code here
//    glDisable(GL_LIGHTING);

    glTranslatef(-moveX, -moveY, -moveZ);
    glPushMatrix();
        glRotatef(rotX, 0, 1, 0);
        glRotatef(rotY, 1, 0, 0);
        skybox();
        glScalef(100, 200, 100);
        glTranslatef(-0.5, 0.01, -0.5);
        if(makeWaves)
            createWaves(waves);
        drawQuad();
        glScalef(1, 1, 1);
    glPopMatrix();

    glTranslatef(moveX, moveY, moveZ);

//   if(location_time !=-1)glUniform1f(location_time,time);
//	if(mvloc !=-1)glUniformMatrix4fv(mvloc,1,GL_FALSE, modelView);
//	if(ploc !=-1)glUniformMatrix4fv(ploc,1,GL_FALSE,projection);

    //glutSolidTeapot(2.0);
    glutSwapBuffers();
}


static void key(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 27 :
        case 'p':
            exit(0);
            break;
        case 'r':
            WireFrame =!WireFrame;
	       break;
        case 'w':
            if(moveZ > -15)
            moveZ -= 0.1;
            break;
        case 's':
            if(moveZ < 15)
            moveZ += 0.1;
            break;
        case 'a':
            if(moveZ > -15)
            moveX -= 0.1;
            break;
        case 'd':
            if(moveX < 15)
            moveX += 0.1;
            break;
        case 'q':
            if(moveY < 15)
            moveY += 0.1;
            break;
        case 'e':
            if(moveY > 5)
                moveY -= 0.1;
            break;


        case ' ':
            makeWaves = !makeWaves;
            break;
    }
}

void Specialkeys(int key, int x, int y)
{
    switch(key){
        case GLUT_KEY_UP:
            if(waveScale > 1.5)
                waveScale -= 0.1;
            break;
        case GLUT_KEY_DOWN:
            if(waveScale < 15)
                waveScale += 0.1;
            break;
        case GLUT_KEY_RIGHT:
            rotX += 5;
            break;
        case GLUT_KEY_LEFT:
            rotX -= 5;
            break;
    }
}

static void idle(void)
{
    // Use parametric equation with t increment for xpos and y pos
    // Don't need a loop
    if(makeWaves)
        waves += 0.01;
    glutPostRedisplay();
}



void mouse(int btn, int state, int x, int y){

    float scale = 100*(Wwidth/Wheight);

    switch(btn){
        case GLUT_LEFT_BUTTON:

        if(state==GLUT_DOWN){

               // get new mouse coordinates for x,y
               // use scale to match right
            }
            break;
    }
     glutPostRedisplay();
};



static void init(void)
{
    glewInit();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);                 // assign a color you like

    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);

    glEnable(GL_DEPTH_TEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glShadeModel(GL_SMOOTH);

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);

    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //Shaders
//    initShader("V.vs","F.fs","G.gs");
//    glEnable(GL_TEXTURE_2D);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenTextures(1, &tex);//Skybox
//    TLoad("images/water.png", tex);
//    TLoad("images/water2.png", tex);
    TLoad("images/water3.png", tex);
//    TLoad("images/water4.png", tex);

    glGenTextures(6, skyboxTextures);
    TLoad("images/front.png", skyboxTextures[0]);
    TLoad("images/back.png", skyboxTextures[1]);
    TLoad("images/top.png", skyboxTextures[2]);
    TLoad("images/bottom.png", skyboxTextures[3]);
    TLoad("images/left.png", skyboxTextures[4]);
    TLoad("images/right.png", skyboxTextures[5]);
    initQuad();
    moveX = 0;
    moveY = 10;
    moveZ = 0;
}

/* Program entry point */

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);

    glutInitWindowSize(1024, 720);
    glutInitWindowPosition(400, 50);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

    glutCreateWindow("Project Assignment 5");
    init();
    glutReshapeFunc(resize);
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutKeyboardFunc(key);
    glutSpecialFunc(Specialkeys);

    glutIdleFunc(idle);

    glutMainLoop();



//    glDetachShader(program,vs);
//    glDetachShader(program,gs);
//    glDetachShader(program,fs);
//    glDeleteShader(vs);
//    glDeleteShader(fs);
//    glDeleteShader(gs);
//    glDeleteShader(program);
    return EXIT_SUCCESS;
}
