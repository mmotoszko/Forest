

//#include "glew-2.0.0/include/GL/glew.h"
#include <windows.h>
#include "../glm-0.9.8.4/glm/glm.hpp"

#include <SDL.h>
#include <SDL_image.h>
#include <GL/glut.h>
#include <GL/glu.h>
//#include <GL/glext.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <time.h>

//#define GL_BGR 32992

using namespace glm;


class Keyframe
{
public:
    GLfloat time;
    vec3 position;
    vec3 angle;

    Keyframe()
    {
        time = 0;
        position.x = 0;
        position.y = 0;
        position.z = 0;
        angle.x = 0;
        angle.y = 0;
        angle.z = 0;
    }

    Keyframe(GLfloat fTime, GLfloat posX, GLfloat posY, GLfloat posZ, GLfloat angleX, GLfloat angleY, GLfloat angleZ )
    {
        time = fTime;
        position.x = posX;
        position.y = posY;
        position.z = posZ;
        angle.x = angleX;
        angle.y = angleY;
        angle.z = angleZ;
    }
};

class shapeObj
{
public:
    std::vector<fvec3> obj_v;
    std::vector<fvec2> obj_vt;
    std::vector<fvec3> obj_vn;
    int faceCount;
    std::vector< std::vector<vec3> > faces;
    GLfloat* vertices;
    GLfloat* normals;
    GLfloat* textures;
    GLshort* indices;
    char textureName[100];
    GLuint texture;
    shapeObj *parent = NULL;
    std::vector<shapeObj *> children;
    std::vector<Keyframe *> keyframes;
    GLfloat currentFrameM[6] = {0,0,0,0,0,0};
    int keyframeCount = 0;

    void addKeyframe(Keyframe *k)
    {
        for(int i=0; i<keyframeCount; i++)
        {
            if( k->time < keyframes[i]->time )
            {
                std::vector<Keyframe*>::iterator it = keyframes.begin() + i;

                keyframes.insert(it, k);
                return;
            }
        }

        keyframes.push_back(k);
    }

    void setParentObject(shapeObj *p)
    {
        parent = p;
        p->children.push_back(this);
    }

    void clearKeyframes()
    {
        keyframes.clear();
    }
};

GLfloat timeSinceStart;
shapeObj myShape, treeShape, staticTree;
bool shape_loaded = false;
GLuint loadedShape;
GLboolean aPressed = false;
GLboolean dPressed = false;
GLboolean wPressed = false;
GLboolean sPressed = false;
GLboolean cPressed = false;
GLboolean iPressed = false;
GLboolean kPressed = false;
GLboolean jPressed = false;
GLboolean lPressed = false;
GLboolean zPressed = false;
GLboolean fPressed = false;
GLboolean spacePressed = false;
GLboolean jump = false;
GLboolean warp = true;
GLboolean fly = false;
double ms = 15;
double x = 0;
double y = 150;
double z = 0;
double falling = 0;
double gravity = 1;
double xrot = 0;
double yrot = 0;
double zrot = 0;
double mousex = 0;
double mousey = 0;
double mousesens = 12;
GLfloat visibleRange = 0;
GLfloat visibleRangeModifier = 200;
int printVisibility = 0;

SDL_Surface* gScreenSurface = NULL;
SDL_Surface* texturePNG = NULL;
GLuint texture[10];
char textureName[100];
char textureName2[100];

const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1000, 100, 1000, 0 };

const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

GLfloat pointToRectDistance(GLfloat px, GLfloat pz, GLfloat rx, GLfloat rz, GLfloat rwidth, GLfloat rheight)
{
    GLfloat dx = max((double)(abs(px - rx) - rwidth / 2), 0.0);
    GLfloat dz = max((double)(abs(pz - rz) - rheight / 2), 0.0);
    return dx * dx + dz * dz;
}

GLfloat pointToPointDistance(GLfloat x1, GLfloat z1, GLfloat x2, GLfloat z2)
{
    return sqrt( pow(x2 - x1, 2) + pow(z2 - z1, 2) );
}

void drawQuadVertical(GLfloat x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2)
{
    glPushMatrix();
        glTranslated(-x/100, -y/100 - 1, -z/100);
        glBegin(GL_QUADS);
            glVertex3f(x1, y1, z1);
            glVertex3f(x1, y2, z1);
            glVertex3f(x2, y2, z2);
            glVertex3f(x2, y1, z2);
        glEnd();
    glPopMatrix();
}

void drawQuadHorizontal(GLfloat x1, GLfloat z1, GLfloat x2, GLfloat z2, GLfloat y1)
{
    glPushMatrix();
        glTranslated(-x/100, -y/100 - 1, -z/100);
        glBegin(GL_QUADS);
            glVertex3f(x1, y1, z1);
            glVertex3f(x1, y1, z2);
            glVertex3f(x2, y1, z2);
            glVertex3f(x2, y1, z1);
        glEnd();
    glPopMatrix();
}

void drawBox( GLfloat x1, GLfloat z1, GLfloat x2, GLfloat z2, GLfloat y )
{
    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    drawQuadVertical( x1, 0, z1, x1, y, z2 );
    drawQuadVertical( x1, 0, z2, x2, y, z2 );
    drawQuadVertical( x2, 0, z2, x2, y, z1 );
    drawQuadVertical( x2, 0, z1, x1, y, z1 );
    glEnable(GL_LIGHTING);
}

void drawCube(float size)
{
    double s = size/2;

    // FRONT
    glBegin(GL_POLYGON);
        glNormal3f(0,0,-1);
        glTexCoord2f(0,0);
        glVertex3f(  s,  s, -s );
        glTexCoord2f(0,1);
        glVertex3f(  s, -s, -s );
        glTexCoord2f(1,1);
        glVertex3f( -s, -s, -s );
        glTexCoord2f(1,0);
        glVertex3f( -s,  s, -s );
    glEnd();

    // BACK
    glBegin(GL_POLYGON);
        glNormal3f(0,0,1);
        glTexCoord2f(0,0);
        glVertex3f(  s, -s, s );
        glTexCoord2f(0,1);
        glVertex3f(  s,  s, s );
        glTexCoord2f(1,1);
        glVertex3f( -s,  s, s );
        glTexCoord2f(1,0);
        glVertex3f( -s, -s, s );
    glEnd();

    // RIGHT
    glBegin(GL_POLYGON);
        glNormal3f(1,0,0);
        glTexCoord2f(0,0);
        glVertex3f( s, -s, -s );
        glTexCoord2f(0,1);
        glVertex3f( s,  s, -s );
        glTexCoord2f(1,1);
        glVertex3f( s,  s,  s );
        glTexCoord2f(1,0);
        glVertex3f( s, -s,  s );
    glEnd();

    //  LEFT
    glBegin(GL_POLYGON);
        glNormal3f(-1,0,0);
        glTexCoord2f(0,0);
        glVertex3f( -s, -s,  s );
        glTexCoord2f(0,1);
        glVertex3f( -s,  s,  s );
        glTexCoord2f(1,1);
        glVertex3f( -s,  s, -s );
        glTexCoord2f(1,0);
        glVertex3f( -s, -s, -s );
    glEnd();

    //  TOP
    glBegin(GL_POLYGON);
        glNormal3f(0,1,0);
        glTexCoord2f(0,0);
        glVertex3f(  s,  s,  s );
        glTexCoord2f(0,1);
        glVertex3f(  s,  s, -s );
        glTexCoord2f(1,1);
        glVertex3f( -s,  s, -s );
        glTexCoord2f(1,0);
        glVertex3f( -s,  s,  s );
    glEnd();

    //  BOTTOM
    glBegin(GL_POLYGON);
        glNormal3f(0,-1,0);
        glTexCoord2f(0,0);
        glVertex3f(  s, -s, -s );
        glTexCoord2f(0,1);
        glVertex3f(  s, -s,  s );
        glTexCoord2f(1,1);
        glVertex3f( -s, -s,  s );
        glTexCoord2f(1,0);
        glVertex3f( -s, -s, -s );
    glEnd();

    glFlush();
}
//
void displayText( float x, float y, int r, int g, int b, const char *string )
{
	int j = strlen( string );

	glColor3f( r, g, b );
	glRasterPos2f( x, y );
	for( int i = 0; i < j; i++ )
    {
		glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, string[i] );
	}
}


bool initSDL()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		success = false;
	}
	else
	{
        //Initialize PNG loading
        int imgFlags = IMG_INIT_PNG;
        if( !( IMG_Init( imgFlags ) & imgFlags ) )
        {
            printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
            success = false;
        }
        else
        {
            //Get window surface
            //gScreenSurface = SDL_GetWindowSurface( gWindow );
        }
	}

	return success;
}


GLuint loadTexture(char* path)
{
    GLuint tex;
    SDL_Surface* textureImage;
    textureImage = IMG_Load(path);
    if(!textureImage)
    {
        printf("Couldn't load %s\n" , path);
        return 1;
    }

    printf("WYMIARY:\n%d x %d\n", textureImage->w, textureImage->h);

    glGenTextures(1 , &tex);
    glBindTexture(GL_TEXTURE_2D , tex);
    printf("pixels: %d\n", textureImage->pixels);

    //non-Linear filtering.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    //Generate the texture.
/*
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureImage->w,
                 textureImage->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 textureImage->pixels);
/**/
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, textureImage->w,
                 textureImage->h, GL_RGBA, GL_UNSIGNED_BYTE,
                 textureImage->pixels);

    glBindTexture(GL_TEXTURE_2D , 0);
// */
    printf("pixels: %d\n", textureImage->pixels);
    printf("tex: %d\n", tex);

    //Free up the memory.
    if(textureImage)
        SDL_FreeSurface(textureImage);

    return tex;
}
//
void loadObj(char* fileName, std::vector<vec3> &v, std::vector<vec2> &vt, std::vector<vec3> &vn, int &faceCount, std::vector< std::vector<vec3> > &faces)
{

    printf("%s\n", fileName);

    FILE *fileOpen;
	fileOpen = fopen(fileName, "r");
	if( !fileOpen )
	{
		printf("Failed to open file\n");
	}

	faceCount = 0;
    std::vector<vec3> temp_face;
    bool f_read = false;

	while( 1 )
    {
        char buffer[100];
        int check = fscanf(fileOpen, "%s", buffer);
        if (check == EOF)
            break; // end of file

        if ( strcmp( buffer, "v" ) == 0 )
        {
            vec3 vertex;
            fscanf(fileOpen, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
            v.push_back(vertex);

            printf("v %f %f %f\n", v.back().x, v.back().y, v.back().z);
        }
        else if ( strcmp( buffer, "vt" ) == 0 )
        {
            vec2 uv;
            fscanf(fileOpen, "%f %f\n", &uv.x, &uv.y );
            vt.push_back(uv);

            printf("vt %f %f \n", vt.back().x, vt.back().y);
        }
        else if ( strcmp( buffer, "vn" ) == 0 )
        {
            vec3 normal;
            fscanf(fileOpen, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
            vn.push_back(normal);

            printf("vn %f %f %f\n", vn.back().x, vn.back().y, vn.back().z);
        }
        else if ( strcmp( buffer, "f" ) == 0  ||  f_read )
        {
            faceCount++;
            printf("f ");
            vec3 verts;
            vec3 temp;
            verts.x = 0;
            verts.y = 0;
            verts.z = 0;

            if(f_read)
            {
                temp[0] = 0;
                temp[1] = 0;
                temp[2] = 0;
                verts.x++;

                std::string buf(buffer);

                std::string token = buf.substr(0, buf.find("/"));
                if( buf.find("/") == -1 )
                    buf.erase(0, -1);
                else
                    buf.erase(0, buf.find("/")+1);
                temp[0] = atoi(token.c_str());

                token = buf.substr(0, buf.find("/"));
                if( buf.find("/") == -1 )
                    buf.erase(0, -1);
                else
                    buf.erase(0, buf.find("/")+1);
                if(strcmp(token.c_str(), ""))
                    temp[1] = atoi(token.c_str());

                token = buf.substr(0, buf.find("\n"));
                if(strcmp(token.c_str(), ""))
                    temp[2] = atoi(token.c_str());

                if(temp[1] == 0  &&  temp[2] == 0)
                    printf("%.0f ", temp[0]);
                else
                if(temp[1] == 0)
                    printf("%.0f//%.0f ", temp[0], temp[2]);
                else
                if(temp[2] == 0)
                    printf("%.0f/%.0f ", temp[0], temp[1]);
                else
                    printf("%.0f/%.0f/%.0f ", temp[0], temp[1], temp[2]);

                temp_face.push_back(temp);
            }

            f_read = false;

            while(fscanf(fileOpen, "%s", buffer) != EOF)
            {
                std::string lastString(buffer);
                lastString = lastString[lastString.size() - 1];

                if(strcmp(lastString.c_str(), "f") == 0)
                {
                    f_read = true;
                    break;
                }

                char lastChar = lastString.c_str()[0];

                if(lastChar < 48  ||  lastChar > 57)
                    break;

                temp[0] = 0;
                temp[1] = 0;
                temp[2] = 0;
                verts.x++;


                std::string buf(buffer);

                std::string token = buf.substr(0, buf.find("/"));
                if( buf.find("/") == -1 )
                    buf.erase(0, -1);
                else
                    buf.erase(0, buf.find("/")+1);
                temp[0] = atoi(token.c_str());

                token = buf.substr(0, buf.find("/"));
                if( buf.find("/") == -1 )
                    buf.erase(0, -1);
                else
                    buf.erase(0, buf.find("/")+1);
                if(strcmp(token.c_str(), ""))
                    temp[1] = atoi(token.c_str());

                token = buf.substr(0, buf.find("\n"));
                if(strcmp(token.c_str(), ""))
                    temp[2] = atoi(token.c_str());


                if(temp[1] == 0  &&  temp[2] == 0)
                    printf("%.0f ", temp[0]);
                else
                if(temp[1] == 0)
                    printf("%.0f//%.0f ", temp[0], temp[2]);
                else
                if(temp[2] == 0)
                    printf("%.0f/%.0f ", temp[0], temp[1]);
                else
                    printf("%.0f/%.0f/%.0f ", temp[0], temp[1], temp[2]);

                temp_face.push_back(temp);
            }

            temp_face.insert(temp_face.begin(), verts);
            faces.push_back(temp_face);
            temp_face.clear();

            printf("\n");
        }
    }

    fclose(fileOpen);}

void loadObjArray(char* fileName, std::vector<vec3> &v, std::vector<vec2> &vt, std::vector<vec3> &vn, int &faceCount, std::vector< std::vector<vec3> > &faces, shapeObj *obj)
{
    int v_over = 0, vt_over = 0, vn_over = 0;
    std::vector<GLfloat> v_check;
    std::vector<GLfloat> vt_check;
    std::vector<GLfloat> vn_check;

    printf("%s\n", fileName);
    std::string name = fileName;
    name.append(".obj");

    FILE *fileOpen;
	fileOpen = fopen(name.c_str(), "r");
	if( !fileOpen )
	{
		printf("Failed to open file\n");
	}

	faceCount = 0;
    std::vector<vec3> temp_face;
    bool f_read = false;

	while( 1 )
    {
        char buffer[100];
        int check = fscanf(fileOpen, "%s", buffer);
        if (check == EOF)
            break; // end of file

        if ( strcmp( buffer, "v" ) == 0 )
        {
            vec3 vertex;
            fscanf(fileOpen, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
            v.push_back(vertex);

            printf("v %f %f %f\n", v.back().x, v.back().y, v.back().z);
        }
        else if ( strcmp( buffer, "vt" ) == 0 )
        {
            vec2 uv;
            fscanf(fileOpen, "%f %f\n", &uv.x, &uv.y );
            vt.push_back(uv);

            printf("vt %f %f \n", vt.back().x, vt.back().y);
        }
        else if ( strcmp( buffer, "vn" ) == 0 )
        {
            vec3 normal;
            fscanf(fileOpen, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
            vn.push_back(normal);

            printf("vn %f %f %f\n", vn.back().x, vn.back().y, vn.back().z);
        }
        else if ( strcmp( buffer, "f" ) == 0  ||  f_read )
        {
            faceCount++;
            printf("f ");
            vec3 verts;
            vec3 temp;
            verts.x = 0;
            verts.y = 0;
            verts.z = 0;

            if(f_read)
            {
                temp[0] = 0;
                temp[1] = -1;
                temp[2] = 0;
                verts.x++;

                std::string buf(buffer);

                std::string token = buf.substr(0, buf.find("/"));
                if( buf.find("/") == -1 )
                    buf.erase(0, -1);
                else
                    buf.erase(0, buf.find("/")+1);
                temp[0] = atoi(token.c_str());

                token = buf.substr(0, buf.find("/"));
                if( buf.find("/") == -1 )
                    buf.erase(0, -1);
                else
                    buf.erase(0, buf.find("/")+1);
                if(strcmp(token.c_str(), ""))
                    temp[1] = atoi(token.c_str());

                token = buf.substr(0, buf.find("\n"));
                if(strcmp(token.c_str(), ""))
                    temp[2] = atoi(token.c_str());

                if(temp[1] == -1 &&  temp[2] == 0)
                    printf("%.0f ", temp[0]);
                else
                if(temp[1] == -1)
                    printf("%.0f//%.0f ", temp[0], temp[2]);
                else
                if(temp[2] == 0)
                    printf("%.0f/%.0f ", temp[0], temp[1]);
                else
                    printf("%.0f/%.0f/%.0f ", temp[0], temp[1], temp[2]);

                temp_face.push_back(temp);
            }

            f_read = false;

            while(fscanf(fileOpen, "%s", buffer) != EOF)
            {
                std::string lastString(buffer);
                lastString = lastString[lastString.size() - 1];

                if(strcmp(lastString.c_str(), "f") == 0)
                {
                    f_read = true;
                    break;
                }

                char lastChar = lastString.c_str()[0];

                if(lastChar < 48  ||  lastChar > 57)
                    break;

                temp[0] = 0;
                temp[1] = -1;
                temp[2] = 0;
                verts.x++;


                std::string buf(buffer);

                std::string token = buf.substr(0, buf.find("/"));
                if( buf.find("/") == -1 )
                    buf.erase(0, -1);
                else
                    buf.erase(0, buf.find("/")+1);
                temp[0] = atoi(token.c_str());

                token = buf.substr(0, buf.find("/"));
                if( buf.find("/") == -1 )
                    buf.erase(0, -1);
                else
                    buf.erase(0, buf.find("/")+1);
                if(strcmp(token.c_str(), ""))
                    temp[1] = atoi(token.c_str());

                token = buf.substr(0, buf.find("\n"));
                if(strcmp(token.c_str(), ""))
                    temp[2] = atoi(token.c_str());


                if(temp[1] == -1  &&  temp[2] == 0)
                    printf("%.0f ", temp[0]);
                else
                if(temp[1] == -1)
                    printf("%.0f//%.0f ", temp[0], temp[2]);
                else
                if(temp[2] == 0)
                    printf("%.0f/%.0f ", temp[0], temp[1]);
                else
                    printf("%.0f/%.0f/%.0f ", temp[0], temp[1], temp[2]);

                temp_face.push_back(temp);
            }

            temp_face.insert(temp_face.begin(), verts);
            faces.push_back(temp_face);
            temp_face.clear();

            printf("\n");
        }
    }

    fclose(fileOpen);

    name = fileName;
    name.append(".mtl");

// load texture file name from .mtl
    char texName[100];
	fileOpen = fopen(name.c_str(), "r");

	if( !fileOpen )
	{
		printf("Failed to open file\n");
	}

	while( 1 )
    {
        char buffer[150];
        int check = fscanf(fileOpen, "%s", buffer);
        if (check == EOF)
            break; // end of file

        if ( strncmp( buffer, "map", 3 ) == 0 )
        {
            fscanf(fileOpen, "%s", texName);
            printf("\nread from .mtl: %s\n", texName);
            break;
        }
    }

	fclose(fileOpen);

	strcpy(obj->textureName, texName);
	printf("\nsaved to shapeObj.textureName: %s\n", obj->textureName);

    GLfloat* vertices;
    GLfloat* normals;
    GLfloat* textures;
    GLshort* indices;

// checking for repeated vertices
    int v_num = 0;
    bool v_new = true;
    std::vector<vec3> v_repeated;

    for(int i=0; i<faceCount; i++)
    {
        for(int j=1; j<4; j++)
        {
            v_new = true;

            for(int k=0; k<i; k++)
            {
                for(int l=1; l<4; l++)
                {
                    if( faces[k][l].x == faces[i][j].x  &&  faces[k][l].y == faces[i][j].y  &&
                        faces[k][l].z == faces[i][j].z  )//&&  ( (k==i && l<j) || (k!=i) ) )
                    {
                        v_new = false;
                        printf("\n unrepeated %d v's, repeated:  %.0f/%.0f/%.0f", v_num, faces[k][l].x, faces[k][l].y, faces[k][l].z);

                        int r_add = true;
                        for(int r=0; r<v_repeated.size(); r++)
                        {
                            if( faces[k][l].x == v_repeated[r].x  &&  faces[k][l].y == v_repeated[r].y  &&  faces[k][l].z == v_repeated[r].z )
                            {
                                r_add = false;
                                break;
                            }
                        }

                        if(r_add)
                        {
                            vec3 rep;
                            rep.x = faces[k][l].x;
                            rep.y = faces[k][l].y;
                            rep.z = faces[k][l].z;
                            v_repeated.push_back(rep);
                        }

                    }
                }
            }

            if(v_new)
            {
                v_num++;
                //printf("\n unrepeated %d v's:  %.0f/%.0f/%.0f", v_num, faces[i][j].x, faces[i][j].y, faces[i][j].z);
            }
        }
    }

    printf("\n faces: %d, unrepeated %d of %d, unique repeated: %d v's\n", faceCount, v_num, faceCount*3, v_repeated.size() );
    //getchar();

    // print duplicate indices
    if(0)
    {
        for(int i=0; i<v_repeated.size(); i++)
        {
            printf( "%.0f/%.0f/%.0f \n", v_repeated[i].x, v_repeated[i].y, v_repeated[i].z );
        }

        getchar();
    }

// constructing index array
// constructing vertex, texture and normal arrays

    indices = new GLshort[faceCount*3];
    vertices = new GLfloat[v_num*3];
    textures = new GLfloat[v_num*3];
    normals  = new GLfloat[v_num*3];
    bool v_add = true;
    int next_ind = 0, found_ind = 0;
    int *rep_check = new int[v_repeated.size()];

    for(int i=0; i<v_repeated.size(); i++)
        rep_check[i] = -1;

    for(int i=0; i<faceCount; i++)
    {
        printf("f ");

        for(int j=0; j<3; j++)
        {
            bool v_add = true;

            for(int k=0; k<v_repeated.size(); k++)
            {
                if( v_repeated[k].x == faces[i][j+1].x  &&  v_repeated[k].y == faces[i][j+1].y  &&
                    v_repeated[k].z == faces[i][j+1].z )
                {
                    found_ind = k;

                    if(rep_check[k] == -1)
                        rep_check[k] = next_ind;
                    else
                        v_add = false;

                    break;
                }
            }

            if(v_add)
            {
                vertices[next_ind*3    ] = v[ (int)faces[i][j+1].x - 1].x;
                vertices[next_ind*3 + 1] = v[ (int)faces[i][j+1].x - 1].y;
                vertices[next_ind*3 + 2] = v[ (int)faces[i][j+1].x - 1].z;

                textures[next_ind*3    ] = vt[ (int)faces[i][j+1].y - 1].x;
                textures[next_ind*3 + 1] = 1 - vt[ (int)faces[i][j+1].y - 1].y;
                textures[next_ind*3 + 2] = 0;

                normals[next_ind*3    ] = vn[ (int)faces[i][j+1].z - 1].x;
                normals[next_ind*3 + 1] = vn[ (int)faces[i][j+1].z - 1].y;
                normals[next_ind*3 + 2] = vn[ (int)faces[i][j+1].z - 1].z;

                indices[i*3 + j] = next_ind;   // coœ nie tak z indeksami
                printf("%d, ", next_ind);
                next_ind++;
            }
            else
            {
                indices[i*3 + j] = rep_check[found_ind];
                printf("%d, ", found_ind);
            }
        }

        printf("\n");
    }

    // print arrays' content
    if(0)
    {
        printf("\nConstructed arrays:\n");
        for(int i=0; i<v_num*3; i++)
        {
            if(i%3 == 0)
                printf("\nv%d: ", i/3);

            printf("%f ", vertices[i]);
        }
        printf("\n");
        for(int i=0; i<v_num*3; i++)
        {
            if(i%3 == 0)
                printf("\nvt%d: ", i/3);

            printf("%f ", textures[i]);
        }
        printf("\n");
        for(int i=0; i<v_num*3; i++)
        {
            if(i%3 == 0)
                printf("\nvn%d: ", i/3);

            printf("%f ", normals[i]);
        }
    }

// save constructed arrays to model structure
    obj->indices = indices;
    obj->normals = normals;
    obj->vertices = vertices;
    obj->textures = textures;
}
//
void drawShape(shapeObj obj)
{
    for(int i = 0; i < obj.faceCount; i++)
    {
        glBegin(GL_POLYGON);

        for(int j = 1; j <= obj.faces[i][0].x; j++)
        {

            if( obj.faces[i][j].z != 0 )
            {
                glNormal3f( obj.obj_vn[ obj.faces[i][j].z - 1 ].x,
                            obj.obj_vn[ obj.faces[i][j].z - 1 ].y,
                            obj.obj_vn[ obj.faces[i][j].z - 1 ].z  );
            }

            if( obj.faces[i][j].y != -1 )
            {
                glTexCoord2f( 1 - obj.obj_vt[ obj.faces[i][j].y - 1 ].x,
                              1 - obj.obj_vt[ obj.faces[i][j].y - 1 ].y  );
            }

            glVertex3f( obj.obj_v[ obj.faces[i][j].x - 1 ].x,
                        obj.obj_v[ obj.faces[i][j].x - 1 ].y,
                        obj.obj_v[ obj.faces[i][j].x - 1 ].z  );
        }

        glEnd();
    }

    glFlush();
}
//
void drawShapeList(shapeObj obj)
{
    if(!shape_loaded)
    {
        shape_loaded = true;
        loadedShape = glGenLists(1);
        glNewList(loadedShape, GL_COMPILE);

        for(int i = 0; i < obj.faceCount; i++)
        {

            glBegin(GL_POLYGON);

            for(int j = 1; j <= obj.faces[i][0].x; j++)
            {
                if(obj.obj_v[ obj.faces[i][j].y - 1 ].x != -1)
                {
                    glTexCoord2f( obj.obj_vt[ obj.faces[i][j].y - 1 ].x,
                                  obj.obj_vt[ obj.faces[i][j].y - 1 ].y  );
                }

                if(obj.obj_v[ obj.faces[i][j].z - 1 ].x != 0)
                {
                    glNormal3f( obj.obj_vn[ obj.faces[i][j].z - 1 ].x,
                                obj.obj_vn[ obj.faces[i][j].z - 1 ].y,
                                obj.obj_vn[ obj.faces[i][j].z - 1 ].z  );
                }

                glVertex3f( obj.obj_v[ obj.faces[i][j].x - 1 ].x,
                            obj.obj_v[ obj.faces[i][j].x - 1 ].y,
                            obj.obj_v[ obj.faces[i][j].x - 1 ].z  );
            }

            glEnd();
        }

        glEndList();
        glFlush();
        glCallList(loadedShape);
    }
    else
        glCallList(loadedShape);
}
//
void drawModelShapeArray(shapeObj *obj, GLfloat px, GLfloat py, GLfloat pz)
{
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    glBindTexture(GL_TEXTURE_2D, obj->texture);

    glPushMatrix();
        glTranslated( -x/100 + px, -y/100 - 1 + py, -z/100 + pz );

        // enable and specify pointers to vertex arrays
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);
        glNormalPointer(GL_FLOAT, 0, obj->normals);
        glTexCoordPointer(3, GL_FLOAT, 0, obj->textures);
        glVertexPointer(3, GL_FLOAT, 0, obj->vertices);

        glDrawElements(GL_TRIANGLES, obj->faceCount*3 * sizeof(GLubyte), GL_UNSIGNED_SHORT, obj->indices);
        //glDrawArrays( GL_POINTS, 0, obj.faceCount*3);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);

    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
}

void drawShapeArray(shapeObj *obj, GLfloat px, GLfloat py, GLfloat pz, GLfloat pxrot, GLfloat pyrot, GLfloat pzrot)
{
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, obj->texture);


    glPushMatrix();

        if(obj->parent == NULL)
        {
            glTranslated( -x/100     + ( obj->currentFrameM[0] + px ),      // was += px before, I'm not sure why but it didn't work that way
                          -y/100 - 1 + ( obj->currentFrameM[1] + py ),      // same here with += py
                          -z/100     + ( obj->currentFrameM[2] + pz )  );   // same here with += pz
        }

        if(obj->parent != NULL)
        {
            glTranslated( -x/100     + obj->parent->currentFrameM[0],
                          -y/100 - 1 + obj->parent->currentFrameM[1],
                          -z/100     + obj->parent->currentFrameM[2]  );

            glRotated(pxrot + obj->parent->currentFrameM[3], 1, 0, 0);
            glRotated(pyrot + obj->parent->currentFrameM[4], 0, 1, 0);
            glRotated(pzrot + obj->parent->currentFrameM[5], 0, 0, 1);

            glTranslated( ( obj->currentFrameM[0] += px ),
                          ( obj->currentFrameM[1] += py ),
                          ( obj->currentFrameM[2] += pz )  );
        }

            glRotated(pxrot + obj->currentFrameM[3], 1, 0, 0);
            glRotated(pyrot + obj->currentFrameM[4], 0, 1, 0);
            glRotated(pzrot + obj->currentFrameM[5], 0, 0, 1);


        // enable and specify pointers to vertex arrays
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);
        glNormalPointer(GL_FLOAT, 0, obj->normals);
        glTexCoordPointer(3, GL_FLOAT, 0, obj->textures);
        glVertexPointer(3, GL_FLOAT, 0, obj->vertices);

        glDrawElements(GL_TRIANGLES, obj->faceCount*3 * sizeof(GLubyte), GL_UNSIGNED_SHORT, obj->indices);
        //glDrawArrays( GL_POINTS, 0, obj.faceCount*3);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);

    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
}

void calculateFrameLinear(shapeObj *obj)
{
    GLfloat fx=0, fy=0, fz=0, fxrot=0, fyrot=0, fzrot=0;

    if( obj->keyframes.size() > 0 )
    {
        GLfloat currentTime = fmod(timeSinceStart/1000 , obj->keyframes.back()->time);

        for(int i=0; i < obj->keyframes.size()-1; i++)
        {
            if( obj->keyframes[i]->time <= currentTime  &&  obj->keyframes[i+1]->time > currentTime )
            {
                GLfloat a = ( obj->keyframes[i+1]->time - currentTime) / ( obj->keyframes[i+1]->time - obj->keyframes[i]->time );

                fx += a*obj->keyframes[i]->position.x + (1-a)*obj->keyframes[i+1]->position.x;
                fy += a*obj->keyframes[i]->position.y + (1-a)*obj->keyframes[i+1]->position.y;
                fz += a*obj->keyframes[i]->position.z + (1-a)*obj->keyframes[i+1]->position.z;
                fxrot += a*obj->keyframes[i]->angle.x + (1-a)*obj->keyframes[i+1]->angle.x;
                fyrot += a*obj->keyframes[i]->angle.y + (1-a)*obj->keyframes[i+1]->angle.y;
                fzrot += a*obj->keyframes[i]->angle.z + (1-a)*obj->keyframes[i+1]->angle.z;

                obj->currentFrameM[0] = fx;
                obj->currentFrameM[1] = fy;
                obj->currentFrameM[2] = fz;
                obj->currentFrameM[3] = fxrot;
                obj->currentFrameM[4] = fyrot;
                obj->currentFrameM[5] = fzrot;

                //printf("%f %f %f %f\n", currentTime, obj->keyframes[i]->time, obj->keyframes[i+1]->time, a );
                break;
            }
        }
    }
}

void calculateFrameBezier(shapeObj *obj)
{
    if( obj->keyframes.size() < 4 )
        return;

    GLfloat fx=0, fy=0, fz=0, fxrot=0, fyrot=0, fzrot=0;
    GLfloat t=0, currentFrameTime=0;
    int currentInd = 0;

    GLfloat currentTime = fmod(timeSinceStart/1000, obj->keyframes.back()->time);

    for(int i=0; i < obj->keyframes.size(); i++)
    {
        if( obj->keyframes[i]->time > currentTime )
        {
            currentInd = i-1;
            break;
        }
    }

    t = ((GLfloat)currentTime - obj->keyframes[currentInd]->time) /
        (obj->keyframes[currentInd+1]->time - obj->keyframes[currentInd]->time);

    vec3 anchor1, anchor2;

    anchor1.x = obj->keyframes[currentInd]->position.x;
    anchor1.y = obj->keyframes[currentInd]->position.y;
    anchor1.z = obj->keyframes[currentInd]->position.z;
    anchor2.x = obj->keyframes[currentInd+1]->position.x;
    anchor2.y = obj->keyframes[currentInd+1]->position.y;
    anchor2.z = obj->keyframes[currentInd+1]->position.z;

    vec3 s1, s2, s3;

    if(currentInd == 0)
    {
        s1.x = (obj->keyframes[obj->keyframes.size()-2]->position.x + obj->keyframes[currentInd]->position.x) / 2;
        s1.y = (obj->keyframes[obj->keyframes.size()-2]->position.y + obj->keyframes[currentInd]->position.y) / 2;
        s1.z = (obj->keyframes[obj->keyframes.size()-2]->position.z + obj->keyframes[currentInd]->position.z) / 2;
    }
    else
    {
        s1.x = (obj->keyframes[currentInd-1]->position.x + obj->keyframes[currentInd]->position.x) / 2;
        s1.y = (obj->keyframes[currentInd-1]->position.y + obj->keyframes[currentInd]->position.y) / 2;
        s1.z = (obj->keyframes[currentInd-1]->position.z + obj->keyframes[currentInd]->position.z) / 2;
    }

    s2.x = (anchor1.x + anchor2.x) / 2;
    s2.y = (anchor1.y + anchor2.y) / 2;
    s2.z = (anchor1.z + anchor2.z) / 2;

    if(currentInd == obj->keyframes.size()-2)
    {
        s3.x = (obj->keyframes[currentInd+1]->position.x + obj->keyframes[1]->position.x) / 2;
        s3.y = (obj->keyframes[currentInd+1]->position.y + obj->keyframes[1]->position.y) / 2;
        s3.z = (obj->keyframes[currentInd+1]->position.z + obj->keyframes[1]->position.z) / 2;
    }
    else
    {
        s3.x = (obj->keyframes[currentInd+1]->position.x + obj->keyframes[currentInd+2]->position.x) / 2;
        s3.y = (obj->keyframes[currentInd+1]->position.y + obj->keyframes[currentInd+2]->position.y) / 2;
        s3.z = (obj->keyframes[currentInd+1]->position.z + obj->keyframes[currentInd+2]->position.z) / 2;
    }

    vec3 is1, is2;

    is1.x = (s1.x + s2.x) / 2;
    is1.y = (s1.y + s2.y) / 2;
    is1.z = (s1.z + s2.z) / 2;
    is2.x = (s2.x + s3.x) / 2;
    is2.y = (s2.y + s3.y) / 2;
    is2.z = (s2.z + s3.z) / 2;

    vec3 v1, v2;

    v1.x = anchor1.x - is1.x;
    v1.y = anchor1.y - is1.y;
    v1.z = anchor1.z - is1.z;
    v2.x = anchor2.x - is2.x;
    v2.y = anchor2.y - is2.y;
    v2.z = anchor2.z - is2.z;

    vec3 control1, control2;

    control1.x = s2.x + v1.x;
    control1.y = s2.y + v1.y;
    control1.z = s2.z + v1.z;
    control2.x = s2.x + v2.x;
    control2.y = s2.y + v2.y;
    control2.z = s2.z + v2.z;

    std::vector<vec3> curve;
    curve.push_back(anchor1);
    curve.push_back(control1);
    curve.push_back(control2);
    curve.push_back(anchor2);

    for(int i=0; i<4; i++)
    {
        int midpow = 1;
        if(i == 1  ||  i == 2)
            midpow = 3;

        fx += pow((1-t), 4-i-1) * pow(t, i) * midpow * curve[i].x;
        fy += pow((1-t), 4-i-1) * pow(t, i) * midpow * curve[i].y;
        fz += pow((1-t), 4-i-1) * pow(t, i) * midpow * curve[i].z;
    }

    if(currentInd == 0  &&  0)
    {
        printf("s1: %f %f %f  s2: %f %f %f\n  s3: %f %f %f\n", s1.x, s1.y, s1.z, s2.x, s2.y, s2.z, s3.x, s3.y, s3.z );
        printf("is1: %f %f %f  is2: %f %f %f\n", is1.x, is1.y, is1.z, is2.x, is2.y, is2.z );
        printf("v1: %f %f %f  v2: %f %f %f\n", v1.x, v1.y, v1.z, v2.x, v2.y, v2.z );
        printf("anchor1: %f %f %f  anchor2: %f %f %f\n", anchor1.x, anchor1.y, anchor1.z, anchor2.x, anchor2.y, anchor2.z );
        printf("control1: %f %f %f  control2: %f %f %f\n", control1.x, control1.y, control1.z, control2.x, control2.y, control2.z );

        printf("%f %f %f %f %f\n", currentTime, obj->keyframes[currentInd]->time, obj->keyframes[currentInd+1]->time, t, obj->keyframes[currentInd+1]->time - obj->keyframes[currentInd]->time);
    }

    GLfloat a = ( obj->keyframes[currentInd+1]->time - currentTime) / ( obj->keyframes[currentInd+1]->time - obj->keyframes[currentInd]->time );

    fxrot += a*obj->keyframes[currentInd]->angle.x + (1-a)*obj->keyframes[currentInd+1]->angle.x;
    fyrot += a*obj->keyframes[currentInd]->angle.y + (1-a)*obj->keyframes[currentInd+1]->angle.y;
    fzrot += a*obj->keyframes[currentInd]->angle.z + (1-a)*obj->keyframes[currentInd+1]->angle.z;

    obj->currentFrameM[0] = fx;
    obj->currentFrameM[1] = fy;
    obj->currentFrameM[2] = fz;
    obj->currentFrameM[3] = fxrot;
    obj->currentFrameM[4] = fyrot;
    obj->currentFrameM[5] = fzrot;

}


class Node
{
private:
    Node *n1, *n2, *n3, *n4;
    GLfloat cX1, cZ1, cX2, cZ2;
    GLfloat rangeX, rangeZ;
    int maxListObjects;
    std::vector<shapeObj*> shapes;
    std::vector<GLfloat> shapeCoords;
    bool visible, initialized;

public:
    Node()
    {
        initialized = false;
    }

    void initialize(GLfloat coordX1, GLfloat coordZ1, GLfloat coordX2, GLfloat coordZ2, int n)
    {
        if(!initialized)
        {
            initialized = true;
            maxListObjects = n;
            visible = false;
            cX1 = coordX1;
            cZ1 = coordZ1;
            cX2 = coordX2;
            cZ2 = coordZ2;
            n1 = NULL;
            n2 = NULL;
            n3 = NULL;
            n4 = NULL;

            if(cX1 < cX2)
                rangeX = cX2 - cX1;
            else
                rangeX = cX1 - cX2;

            if(cZ1 < cZ2)
                rangeZ = cZ2 - cZ1;
            else
                rangeZ = cZ1 - cZ2;
        }
    }

    bool isVisible(GLfloat px, GLfloat py, GLfloat pz, GLfloat range)
    {
        if(initialized)
        {
            if( pointToRectDistance(px, pz, cX1 + rangeX/2, cZ1 + rangeZ/2, rangeX, rangeZ) <= range )
                return true;

            return false;
        }
    }

    bool listIsFull()
    {
        if(initialized)
        {
            if(shapes.size() == maxListObjects)
                return true;

            return false;
        }
    }

    void addShape(shapeObj *object, GLfloat px, GLfloat py, GLfloat pz, GLfloat pxrot, GLfloat pyrot, GLfloat pzrot)
    {
        if(initialized)
        {
            if(n1 == NULL)
            {
                if(!listIsFull())
                {
                    shapes.push_back(object);
                    shapeCoords.push_back(px);
                    shapeCoords.push_back(py);
                    shapeCoords.push_back(pz);
                    shapeCoords.push_back(pxrot);
                    shapeCoords.push_back(pyrot);
                    shapeCoords.push_back(pzrot);
                }
                else
                {
                    addNode();

                    addShape(object, px, py, pz, pxrot, pyrot, pzrot);
                }
            }
            else
            {
                if(px < cX1 + rangeX/2  &&  pz >= cZ1 + rangeZ/2 )
                    n1->addShape(object, px, py, pz, pxrot, pyrot, pzrot);
                else
                if(px < cX1 + rangeX/2  &&  pz < cZ1 + rangeZ/2 )
                    n3->addShape(object, px, py, pz, pxrot, pyrot, pzrot);
                else
                if( px >= cX1 + rangeX/2  &&  pz >= cZ1 + rangeZ/2 )
                    n2->addShape(object, px, py, pz, pxrot, pyrot, pzrot);
                else
                if( px >= cX1 + rangeX/2  &&  pz < cZ1 + rangeZ/2 )
                    n4->addShape(object, px, py, pz, pxrot, pyrot, pzrot);
            }
        }
    }

    void addNode()
    {
        if(initialized)
        {
            // create 4 children nodes
            n1 = new Node();
            n1->initialize(cX1           , cZ1 + rangeZ/2, cX1 + rangeX/2, cZ2           , maxListObjects);
            n2 = new Node();
            n2->initialize(cX1 + rangeX/2, cZ1 + rangeZ/2, cX2           , cZ2           , maxListObjects);
            n3 = new Node();
            n3->initialize(cX1           , cZ1           , cX1 + rangeX/2, cZ1 + rangeZ/2, maxListObjects);
            n4 = new Node();
            n4->initialize(cX1 + rangeX/2, cZ1           , cX2           , cZ2 - rangeZ/2, maxListObjects);

            // add current node's shapes to children nodes
            for(int i = 0; i < maxListObjects; i++)
                addShape(shapes[i], shapeCoords[i*6], shapeCoords[i*6 +1], shapeCoords[i*6 +2], shapeCoords[i*6 +3], shapeCoords[i*6 +4], shapeCoords[i*6 +5] );
        }
    }

    void drawVisible()
    {
        if(initialized)
        {
            if(0)
            {
                if(printVisibility == 0)
                {
                    printf("\n %d: %.3f %.3f %.3f ", isVisible(x, y, z, visibleRange), pointToRectDistance(x, z, cX1, cZ2, rangeX, rangeZ), cX1, cZ2);
                    printVisibility = 10;
                }
            }

            visible = isVisible(x/100, y/100, z/100, visibleRange);

            // draw objects from visible nodes
            if(visible)
            {
                if(n1 != NULL)
                {
                    n1->drawVisible();
                    n2->drawVisible();
                    n3->drawVisible();
                    n4->drawVisible();
                }
                else
                {
                    for(int i=0; i<shapes.size(); i++)
                    {
                        drawShapeArray(shapes[i], shapeCoords[i*6], shapeCoords[i*6 +1], shapeCoords[i*6 +2], shapeCoords[i*6 +3], shapeCoords[i*6 +4], shapeCoords[i*6 +5] );
                    }
                }
            }
        }
    }

    void drawBoundries(GLfloat bHeight)
    {
        if(initialized)
        {
            drawBox(cX1, cZ1, cX2, cZ2, bHeight);

            if(n1 != NULL)
            {
                n1->drawBoundries(bHeight*0.7);
                n2->drawBoundries(bHeight*0.7);
                n3->drawBoundries(bHeight*0.7);
                n4->drawBoundries(bHeight*0.7);
            }
        }
    }

};

Node *forest = new Node();



static void resize(int width, int height)
{
    const float ar = (float) width / (float) height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity() ;
}

static void display(void)
{
    const double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50, 1.77, 0.1, 1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 0.0, 0.1,
              0.0, 0.0, 0.0,
              0.0, 1.0, 0.0);

    glRotated(-xrot,1,0,0);
    glRotated(-yrot,0,1,0);
    glRotated(-zrot,0,0,1);


// clear scene and draw ground
    glColor3d(0.5,0.8,0.5);
    glDisable(GL_LIGHTING);

    drawQuadHorizontal(-3000, -3000, 3000, 3000, 0);

    glEnable(GL_LIGHTING);
//

// draw shapes
    //drawBox(-5, -5, 5, 5, 1);
    drawShapeArray(&treeShape, 0, 0, 0, 0, 0, 0);
    drawShapeArray(&myShape, 5, 1.25, 5, 0, 0, 0);
    //drawShapeArray(&staticTree, 2, 0, 0, 0, 0, 0);

// draw forest structure
    forest->drawVisible();
    forest->drawBoundries(2);

    //displayText(10 ,10, 1, 1, 1, "asdf");

    glFlush();
    glutSwapBuffers();
}

static void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 27 :
            exit(0);
            break;

        case 'a':
            aPressed = true;
            break;

        case 'd':
            dPressed = true;
            break;

        case 'w':
            wPressed = true;
            break;

        case 's':
            sPressed = true;
            break;

        case 'c':
            cPressed = true;
            break;

        case ' ':
            spacePressed = true;
            break;

        case 'i':
            iPressed = true;
            break;

        case 'k':
            kPressed = true;
            break;

        case 'j':
            jPressed = true;
            break;

        case 'l':
            lPressed = true;
            break;

        case 'f':
            if(!fly)
            {
                fly = true;
                ms *= 10;
            }
            else
            {
                fly = false;
                ms /= 10;
            }
            break;

        case 'z':
            if(!zPressed)
                zPressed = true;
            else
                zPressed = false;
            break;

    }


    glutPostRedisplay();
}

static void keyboardUp(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 'a':
            aPressed = false;
            break;

        case 'd':
            dPressed = false;
            break;

        case 'w':
            wPressed = false;
            break;

        case 's':
            sPressed = false;
            break;

        case 'c':
            cPressed = false;
            break;

        case ' ':
            spacePressed = false;
            break;

        case 'i':
            iPressed = false;
            break;

        case 'k':
            kPressed = false;
            break;

        case 'j':
            jPressed = false;
            break;

        case 'l':
            lPressed = false;
            break;

    }

    glutPostRedisplay();
}

static void mouseMove(int x, int y)
{
    if(!zPressed)
    {
        glutSetCursor(GLUT_CURSOR_NONE);
        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);

        mousex = (double)x  - w/2;
        mousey = (double)y  - h/2;

        yrot -= mousex/100 * mousesens;
        xrot -= mousey/100 * mousesens;


        if(xrot > 90)
            xrot = 90;

        if(xrot < -90)
            xrot = -90;

        if(yrot > 180)
            yrot = -360 + yrot;

        if(yrot < -180)
            yrot = 360 + yrot;

        if( warp )
        {
            warp = false;
            glutWarpPointer(w/2, h/2);
        }
        else
            warp = true;
    }
    else
        glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
}

static void idle(void)
{
    timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
    //printf("%d\n", timeSinceStart);

    //calculateFrameLinear(&myShape);
    calculateFrameBezier(&staticTree);
    calculateFrameBezier(&treeShape);
    calculateFrameBezier(&myShape);

    if(printVisibility > 0)
        printVisibility--;

    if(aPressed)
    {
        if(wPressed ^ sPressed)
        {
            z -= ms/2 * ( -sin(yrot*3.14/180) );
            x -= ms/2 * ( 1 - fabs(yrot)/90 );
        }
        else
        {
            z -= ms * ( -sin(yrot*3.14/180) );
            x -= ms * ( 1 - fabs(yrot)/90 );
        }
    }

    if(dPressed)
    {
        if(wPressed ^ sPressed)
        {
            z += ms/2 * ( -sin(yrot*3.14/180) );
            x += ms/2 * ( 1 - fabs(yrot)/90 );
        }
        else
        {
            z += ms * ( -sin(yrot*3.14/180) );
            x += ms * ( 1 - fabs(yrot)/90 );
        }
    }

    if(wPressed)
    {
        if(aPressed ^ dPressed)
        {
            z -= ms/2 * ( 1 - fabs(yrot)/90 );
            x += ms/2 * ( -sin(yrot*3.14/180) );
        }
        else
        {
            z -= ms * ( 1 - fabs(yrot)/90 );
            x += ms * ( -sin(yrot*3.14/180) );
        }
    }

    if(sPressed)
    {
        if(aPressed ^ dPressed)
        {
            z += ms/2 * ( 1 - fabs(yrot)/90 );
            x -= ms/2 * ( -sin(yrot*3.14/180) );
        }
        else
        {
            z += ms * ( 1 - fabs(yrot)/90 );
            x -= ms * ( -sin(yrot*3.14/180) );
        }
    }

    if(iPressed)
        if(xrot < 90)
            xrot += 2;

    if(kPressed)
        if(xrot > -90)
            xrot -= 2 ;

    if(jPressed)
        yrot += 2;

    if(lPressed)
        yrot -= 2;


    if(yrot > 180)
        yrot = -360 + yrot;

    if(yrot < -180)
        yrot = 360 + yrot;



    if( spacePressed  &&  y <= 150  &&  !fly )
        falling = -15;


    if(fly)
    {
        falling = 0;

        if( spacePressed )
            y += ms/2;

        if( cPressed )// &&  y >= 150 )
        {
            y -= ms/2;
//            if( y < 150 )
//                y = 150;
        }
    }
    else
    {
        y = (y - falling);
        falling += gravity;

        if( y < 150 )
        {
            y = 150;
            falling = 0;
        }
    }

    visibleRange = (visibleRangeModifier + y/100)*(visibleRangeModifier + y/100);


    if(!zPressed)
    {
        //printf("x: %.2f, y: %.2f, z: %.2f", x/100, y/100, z/100);
        //printf(", yrot: %.2f, xrot: %.2f \n", yrot, xrot);
    }

    glutPostRedisplay();
}


int main(int argc, char *argv[])
{
    srand(time(NULL));
    if(!initSDL())
        printf("Cannot initialize SDL!\n");
    glutInit(&argc, argv);
    glutInitWindowSize(1120,630);
    glutInitWindowPosition(10,10);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);

// various objects
    loadObjArray("box", myShape.obj_v, myShape.obj_vt, myShape.obj_vn, myShape.faceCount, myShape.faces, &myShape);
//    loadObjArray("torus", myShape.obj_v, myShape.obj_vt, myShape.obj_vn, myShape.faceCount, myShape.faces, &myShape);
//    loadObjArray("auto", myShape.obj_v, myShape.obj_vt, myShape.obj_vn, myShape.faceCount, myShape.faces, &myShape);
//    loadObjArray("monkey", myShape.obj_v, myShape.obj_vt, myShape.obj_vn, myShape.faceCount, myShape.faces, &myShape);
    loadObjArray("tree", treeShape.obj_v, treeShape.obj_vt, treeShape.obj_vn, treeShape.faceCount, treeShape.faces, &treeShape);
    loadObjArray("tree", staticTree.obj_v, staticTree.obj_vt, staticTree.obj_vn, staticTree.faceCount, staticTree.faces, &staticTree);


    if(0) // print loaded shape vertices, texture vertices, normals, faces
    {
        printf("\n");

        for(unsigned int i = 0; i < myShape.obj_v.size(); i++)
        {
            printf("v %f %f %f\n", myShape.obj_v[i].x, myShape.obj_v[i].y, myShape.obj_v[i].z);
        }
        for(unsigned int i = 0; i < myShape.obj_vt.size(); i++)
        {
            printf("vt %f %f\n", myShape.obj_vt[i].x, myShape.obj_vt[i].y);
        }
        for(unsigned int i = 0; i < myShape.obj_vn.size(); i++)
        {
            printf("vn %f %f %f\n", myShape.obj_vn[i].x, myShape.obj_vn[i].y, myShape.obj_vn[i].z);
        }
        printf("%d faces:\n", myShape.faceCount);
        for(unsigned int i = 0; i < myShape.faces.size(); i++)
        {
            printf("f %.0f ", myShape.faces[i][0].x);

            for(unsigned int j = 1; j < myShape.faces[i].size(); j++)
            {
                printf("%.0f/%.0f/%.0f ", myShape.faces[i][j].x, myShape.faces[i][j].y, myShape.faces[i][j].z);
            }
            printf("\n");
        }
    }

    glutCreateWindow("Forest");

    myShape.texture = loadTexture(myShape.textureName);
    treeShape.texture = loadTexture(treeShape.textureName);
    staticTree.texture = loadTexture(staticTree.textureName);

    myShape.setParentObject(&treeShape);

    //                                   t     x     y     z   xr   yr   zr
    treeShape.addKeyframe(new Keyframe(0.0,    0,    0,    0,   0,   0,   0));
    treeShape.addKeyframe(new Keyframe(1.0,    5,    0,    5,   0,  72,   0));
    treeShape.addKeyframe(new Keyframe(2.0,   -5,    0,    5,   0, 144,   0));
    treeShape.addKeyframe(new Keyframe(3.0,   -5,    0,   -5,   0, 216,   0));
    treeShape.addKeyframe(new Keyframe(4.9,    5,    0,   -5,   0, 288,   0));
    treeShape.addKeyframe(new Keyframe(6.1,    0,    0,    0,   0, 360,   0));

    //                                 t     x     y     z   xr   yr   zr
    myShape.addKeyframe(new Keyframe(0.0,    0,    0,    0,   0,   0,   0));
    myShape.addKeyframe(new Keyframe(0.5,    0,    0,    0,  90,   0,   0));
    myShape.addKeyframe(new Keyframe(1.0,    0,    0,    0, 180,   0,   0));
    myShape.addKeyframe(new Keyframe(1.5,    0,    0,    0, 270,   0,   0));
    myShape.addKeyframe(new Keyframe(2.0,    0,    0,    0, 360,   0,   0));


// forest
    visibleRangeModifier = 150;
    int fieldSize = 400;
    int forestSize = 10000;

    forest->initialize( -fieldSize, -fieldSize, fieldSize, fieldSize, 4);
    for(int i=0; i<forestSize; i++)
    {
        forest->addShape(&staticTree, rand()%(fieldSize * 2) - fieldSize, 0,
                                   rand()%(fieldSize * 2) - fieldSize, 0, rand()%180, 0);
    }


    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(mouseMove);
    glutIdleFunc(idle);


    glClearColor(0.5, 0.8, 1, 1);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);

    glutMainLoop();

    return EXIT_SUCCESS;
}






/*
    drawChecker(-8, 0, -8);

    glColor3d(1,0,0);

    glPushMatrix();
        glTranslated(-x/100,-y/100,-z/100 - 16);
        drawCube(2);
    glPopMatrix();

    glColor3d(0,1,0);

    glPushMatrix();
        glTranslated(-x/100,-y/100,-z/100 + 16);
        drawCube(2);
    glPopMatrix();

    glColor3d(0,0,1);

    glPushMatrix();
        glTranslated(-x/100 + 16,-y/100,-z/100);
        drawCube(2);
    glPopMatrix();

    glColor3d(0,1,1);

    glPushMatrix();
        glTranslated(-x/100 - 16,-y/100,-z/100);
        drawCube(2);
    glPopMatrix();

    glColor3d(1,1,0);

    glPushMatrix();
        glTranslated(-x/100,-y/100 + 16,-z/100);
        drawCube(2);
    glPopMatrix();
*/



/*
GLfloat vertices2[] = { 1, 1, 1,  -1, 1, 1,  -1,-1, 1,   1,-1, 1,   // v0,v1,v2,v3 (front)
                        1, 1, 1,   1,-1, 1,   1,-1,-1,   1, 1,-1,   // v0,v3,v4,v5 (right)
                        1, 1, 1,   1, 1,-1,  -1, 1,-1,  -1, 1, 1,   // v0,v5,v6,v1 (top)
                       -1, 1, 1,  -1, 1,-1,  -1,-1,-1,  -1,-1, 1,   // v1,v6,v7,v2 (left)
                       -1,-1,-1,   1,-1,-1,   1,-1, 1,  -1,-1, 1,   // v7,v4,v3,v2 (bottom)
                        1,-1,-1,  -1,-1,-1,  -1, 1,-1,   1, 1,-1 }; // v4,v7,v6,v5 (back)

// normal array
GLfloat normals2[]  = { 0, 0, 1,   0, 0, 1,   0, 0, 1,   0, 0, 1,   // v0,v1,v2,v3 (front)
                        1, 0, 0,   1, 0, 0,   1, 0, 0,   1, 0, 0,   // v0,v3,v4,v5 (right)
                        0, 1, 0,   0, 1, 0,   0, 1, 0,   0, 1, 0,   // v0,v5,v6,v1 (top)
                       -1, 0, 0,  -1, 0, 0,  -1, 0, 0,  -1, 0, 0,   // v1,v6,v7,v2 (left)
                        0,-1, 0,   0,-1, 0,   0,-1, 0,   0,-1, 0,   // v7,v4,v3,v2 (bottom)
                        0, 0,-1,   0, 0,-1,   0, 0,-1,   0, 0,-1 }; // v4,v7,v6,v5 (back)

// color array
GLfloat colors2[]   = { 1, 1, 1,   1, 1, 0,   1, 0, 0,   1, 0, 1,   // v0,v1,v2,v3 (front)
                        1, 1, 1,   1, 0, 1,   0, 0, 1,   0, 1, 1,   // v0,v3,v4,v5 (right)
                        1, 1, 1,   0, 1, 1,   0, 1, 0,   1, 1, 0,   // v0,v5,v6,v1 (top)
                        1, 1, 0,   0, 1, 0,   0, 0, 0,   1, 0, 0,   // v1,v6,v7,v2 (left)
                        0, 0, 0,   0, 0, 1,   1, 0, 1,   1, 0, 0,   // v7,v4,v3,v2 (bottom)
                        0, 0, 1,   0, 0, 0,   0, 1, 0,   0, 1, 1 }; // v4,v7,v6,v5 (back)

// index array of vertex array for glDrawElements() & glDrawRangeElement()
GLubyte indices2[]  = { 0, 1, 2,   2, 3, 0,      // front
                       4, 5, 6,   6, 7, 4,      // right
                       8, 9,10,  10,11, 8,      // top
                      12,13,14,  14,15,12,      // left
                      16,17,18,  18,19,16,      // bottom
                      20,21,22,  22,23,20 };    // back
*/



/*
//  Create checkerboard texture
#define checkImageWidth 64
#define checkImageHeight 64
static GLubyte checkImage[checkImageHeight][checkImageWidth][4];

static GLuint texName;


void makeCheckImage()
{
    int i, j, c;

    for (i = 0; i < checkImageHeight; i++) {
        for (j = 0; j < checkImageWidth; j++) {
            c = ((((i&0x8)==0)^((j&0x8))==0))*255;
            checkImage[i][j][0] = (GLubyte) c;
            checkImage[i][j][1] = (GLubyte) c;
            checkImage[i][j][2] = (GLubyte) c;
            checkImage[i][j][3] = (GLubyte) 255;
        }
    }
}

void initChecker()
{
    glClearColor (0.0, 0.0, 0.0, 0.0);
    //glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);

    makeCheckImage();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &texName);
    glBindTexture(GL_TEXTURE_2D, texName);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, checkImageWidth,
                checkImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                checkImage);
}

void drawChecker(int posx, int posy, int posz)
{
    //successfully drawn texture
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glBindTexture(GL_TEXTURE_2D, texName);

    glPushMatrix();
        glTranslated(-x/100 + posx, -y/100 + posy, -z/100 + posz);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0); glVertex3f(-2.0, -1.0, 0.0);
        glTexCoord2f(0.0, 1.0); glVertex3f(-2.0, 1.0, 0.0);
        glTexCoord2f(1.0, 1.0); glVertex3f(0.0, 1.0, 0.0);
        glTexCoord2f(1.0, 0.0); glVertex3f(0.0, -1.0, 0.0);

        glTexCoord2f(0.0, 0.0); glVertex3f(1.0, -1.0, 0.0);
        glTexCoord2f(0.0, 1.0); glVertex3f(1.0, 1.0, 0.0);
        glTexCoord2f(1.0, 1.0); glVertex3f(2.41421, 1.0, -1.41421);
        glTexCoord2f(1.0, 0.0); glVertex3f(2.41421, -1.0, -1.41421);
        glEnd();
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}
*/


