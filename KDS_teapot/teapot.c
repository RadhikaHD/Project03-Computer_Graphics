/*
 * Alekhya Kamana, Radhika Dhaipule, & Christian Stith
 * CPSC 6050: Introduction to Graphics
 * Dr. Robert Geist
 * April 23, 2015
 * Project 3: The Teapot Contest!
 * 
 * Compilation:
 * 		gcc teapot.c -lGL -lGLU -lglut -lm -o teapot.out
 * Execution:
 * 		./teapot.out
 * 
 * This project displays a rendered version of the Utah teapot.
 * The image includes environment mapping, texture mapping,
 * shadow-maps, normal mapping, multiple lights, and anti-aliasing.
 * 
 * This code accesses the following files:
 * teapot.frag
 * teapot.vert
 * sky.ppm (source: http://forums.newtek.com/attachment.php?attachmentid=49222&d=1186937601)
 * snow.ppm (source: http://www.photos-public-domain.com/wp-content/uploads/2011/01/shadows-on-snow-texture.jpg)
 * glaz.ppm (source: https://0.s3.envato.com/files/77526815/Paisley-Porcelain590.jpg) 
 * teabump.ppm: (source: https://www.filterforge.com/filters/977-normal.jpg)
 */

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>

#define NUM_SURFACES 2
#define XRES 1280
#define YRES 720

#define VPASSES 20
#define JITTER 0.0005
#define MAX_LENGTH 2000000

#define TEXTURE1 10
#define TEXTURE2 11
#define TEXTURE3 12

#define NORMAL1 20

double X = 0;
double Y = 0;
double Z = 0;

struct point {
	float x, y, z;
};

struct coord {
	float x, y;
};

float* toArray(struct point p) {
	static float vec[3];
	vec[0] = p.x;
	vec[1] = p.y;
	vec[2] = p.z;
	return vec;
}

float* toArrayCoords(struct coord p) {
	static float vec[2];
	vec[0] = p.x;
	vec[1] = p.y;
	return vec;
}

struct face {
	long v[4], n[4], t[4];
};

int mybuf=1;

long numVertices = 0;
struct point vertexArray[MAX_LENGTH];
long numTexCoords = 0;
struct coord texCoordArray[MAX_LENGTH];
long numNormals = 0;
struct point normalArray[MAX_LENGTH];

long numTangents = 0;
struct point tangentArray[MAX_LENGTH];
struct point biTangentArray[MAX_LENGTH];

long numFaces = 0;
struct face faceArray[MAX_LENGTH];

GLfloat indices[MAX_LENGTH];
GLuint sprogram;

GLfloat light0_position[] = { 4, 4.0, -3, 1.0 };
GLfloat light0_direction[] = { -4, -4.0, 3, 1.0};

double genrand() {
	return(((double)(random()+1))/((double)RAND_MAX));
}

struct point cross(struct point u, struct point v) {
	struct point w;
	w.x = u.y*v.z - u.z*v.y;
	w.y = -(u.x*v.z - u.z*v.x);
	w.z = u.x*v.y - u.y*v.x;
	return(w);
}

struct point unit_length(struct point u) {
	double length;
	struct point v;
	length = sqrt(u.x*u.x+u.y*u.y+u.z*u.z);
	v.x = u.x/length;
	v.y = u.y/length;
	v.z = u.z/length;
	return(v);
}

void load_vector( FILE* ifp, struct point array[], long* counter ) {
	fscanf(ifp, "%f", &array[*counter].x);
	fscanf(ifp, "%f", &array[*counter].y);
	fscanf(ifp, "%f", &array[*counter].z);
}

void load_coord( FILE* ifp, struct coord array[], long* counter ) {
	fscanf(ifp, "%f", &array[*counter].x);
	fscanf(ifp, "%f", &array[*counter].y);
}

void load_vector_face( FILE* ifp, struct face array[], long* counter ) {
	int i;
	char slash;
	for ( i=0; i<4; i++ ) {
		fscanf(ifp, "%ld", &array[*counter].v[i]);
		fscanf(ifp, "%c", &slash);
		fscanf(ifp, "%ld", &array[*counter].t[i]);
		fscanf(ifp, "%c", &slash);
		fscanf(ifp, "%ld", &array[*counter].n[i]);
	}
}

void load_obj(unsigned int shader, char* fname) {
	
	FILE *ifp;
	ifp = fopen(fname, "r");
	int i;
	char lineType[2];
	while ( fscanf(ifp, "%s", lineType) != EOF ) {
		if ( lineType[0] == '#' ) {
			// Line is comment
		}
		else if ( lineType[0] == 'm' ) {
			// Line is material
		}
		else if ( lineType[0] == 'v' ) {
			// Normals
			if ( lineType[1] == 'n' ) {
				load_vector(ifp, normalArray, &numNormals);
				numNormals++;
			}
			// Texcoord
			else if ( lineType[1] == 't' ) {
				load_coord(ifp, texCoordArray, &numTexCoords);
				numTexCoords++;
			}
			// Tangent
			else if ( lineType[1] == 'x' ) {
				load_vector(ifp, tangentArray, &numTangents);
				// numTangents++;
			}
			// Bitangent
			else if ( lineType[1] == 'y' ) {
				load_vector(ifp, biTangentArray, &numTangents);
				numTangents++;
			}
			// Vertex
			else {
				load_vector(ifp, vertexArray, &numVertices);
				numVertices++;
			}
		} else if ( lineType[0] == 'f' ) {
			load_vector_face(ifp, faceArray, &numFaces);
			numFaces++;
		}
		char line[100];
		fgets (line, 100, ifp);
	}
	
	fclose(ifp);
	
	GLint index_tangent = glGetAttribLocation(shader, "tangent");
	GLint index_bitangent = glGetAttribLocation(shader, "bitangent");
	
	for( i=0; i<numFaces; i++ ) {
		int j = 0;
		for (j=0; j<4; j++) {
			// Vertices
			indices[i*12 + j*3 + 0] = vertexArray[faceArray[i].v[j]-1].x;
			indices[i*12 + j*3 + 1] = vertexArray[faceArray[i].v[j]-1].y;
			indices[i*12 + j*3 + 2] = vertexArray[faceArray[i].v[j]-1].z;
			// Texcoords
			indices[numFaces*12 + i*8 + j*2 + 0] = texCoordArray[faceArray[i].t[j]-1].x;
			indices[numFaces*12 + i*8 + j*2 + 1] = texCoordArray[faceArray[i].t[j]-1].y;
			// Normals
			indices[numFaces*20 + i*12 + j*3 + 0] = normalArray[faceArray[i].n[j]-1].x;
			indices[numFaces*20 + i*12 + j*3 + 1] = normalArray[faceArray[i].n[j]-1].y;
			indices[numFaces*20 + i*12 + j*3 + 2] = normalArray[faceArray[i].n[j]-1].z;

			// Shader stuff
			
			int vi = faceArray[i].v[j]-1;
			int ti = faceArray[i].t[j]-1;
			int ni = faceArray[i].n[j]-1;
			glNormal3fv( toArray(normalArray[ni]));
			glTexCoord2fv( toArrayCoords(texCoordArray[ti]));
			glVertexAttrib3fv(index_tangent, toArray(tangentArray[vi]));
			glVertexAttrib3fv(index_bitangent, toArray(biTangentArray[vi]));
			glVertex3fv( toArray(vertexArray[vi]));
			
		}	
	}	
}

void lights(){
	
	//key
	float light0_diffuse[] = {1.0, 0.8, 0.8, 1.0}; 
	float light0_specular[] = { 2.0, 2.0, 2.0, 0.0 }; 
	float light0_direction[] = { 0.0, -1.0, 0.0, 1.0};
	
	//fill
	float light1_diffuse[] = {0.8, 0.8, 0.8, 1.0}; 
	float light1_specular[] = { 2.25, 2.25, 2.25, 0.0 }; 
	float light1_position[] = { 5.0, -1.2, 0.0, 1.0 };
	float light1_direction[] = { 0.0, 0.0, -1.0, 1.0};

	//back
	float light2_diffuse[] = {1.0, 1.0, 0.8, 1.0}; 
	float light2_specular[] = { 2.25, 2.25, 2.25, 0.0 }; 
	float light2_position[] = { 0.0, 5.0, -5.0, 1.0 };
	float light2_direction[] = { 0.0, -1.0, 1.0, 1.0};

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,1); 
	glLightfv(GL_LIGHT0,GL_DIFFUSE,light0_diffuse); 
	glLightfv(GL_LIGHT0,GL_SPECULAR,light0_specular); 
	glLightf(GL_LIGHT0,GL_SPOT_EXPONENT,1.0); 
	glLightf(GL_LIGHT0,GL_SPOT_CUTOFF,180.0); 
	glLightf(GL_LIGHT0,GL_CONSTANT_ATTENUATION,1.0); 
	glLightf(GL_LIGHT0,GL_LINEAR_ATTENUATION,0.2); 
	glLightf(GL_LIGHT0,GL_QUADRATIC_ATTENUATION,0.01); 
	glLightfv(GL_LIGHT0,GL_POSITION,light0_position);
	glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,light0_direction);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	
	glLightfv(GL_LIGHT1,GL_DIFFUSE,light1_diffuse); 
	glLightfv(GL_LIGHT1,GL_SPECULAR,light1_specular); 
	glLightf(GL_LIGHT1,GL_SPOT_EXPONENT,1.0); 
	glLightf(GL_LIGHT1,GL_SPOT_CUTOFF,180.0); 
	glLightf(GL_LIGHT1,GL_CONSTANT_ATTENUATION,1.0); 
	glLightf(GL_LIGHT1,GL_LINEAR_ATTENUATION,0.2); 
	glLightf(GL_LIGHT1,GL_QUADRATIC_ATTENUATION,0.01); 
	glLightfv(GL_LIGHT1,GL_POSITION,light1_position);
	glLightfv(GL_LIGHT1,GL_SPOT_DIRECTION,light1_direction);
	glEnable(GL_LIGHT1);
	
		
	glLightfv(GL_LIGHT2,GL_DIFFUSE,light2_diffuse); 
	glLightfv(GL_LIGHT2,GL_SPECULAR,light2_specular); 
	glLightf(GL_LIGHT2,GL_SPOT_EXPONENT,1.0);
	glLightf(GL_LIGHT2,GL_SPOT_CUTOFF,180.0); 
	glLightf(GL_LIGHT2,GL_CONSTANT_ATTENUATION,1.0); 
	glLightf(GL_LIGHT2,GL_LINEAR_ATTENUATION,0.2); 
	glLightf(GL_LIGHT2,GL_QUADRATIC_ATTENUATION,0.01); 
	glLightfv(GL_LIGHT2,GL_POSITION,light2_position);
	glLightfv(GL_LIGHT2,GL_SPOT_DIRECTION,light2_direction);
	glEnable(GL_LIGHT2);
	
}


char *read_shader_program(char *filename) {
	FILE *fp;
	char *content = NULL;
	int fd, count;
	fd = open(filename,O_RDONLY);
	count = lseek(fd,0,SEEK_END);
	close(fd);
	content = (char *)calloc(1,(count+1));
	fp = fopen(filename,"r");
	count = fread(content,sizeof(char),count,fp);
	content[count] = '\0';
	fclose(fp);
	return content;
}


unsigned int set_shaders(){
	GLint vertCompiled, fragCompiled;
	char *vs, *fs;
	GLuint v, f, p;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);
	vs = read_shader_program("teapot.vert");
	fs = read_shader_program("teapot.frag");
	glShaderSource(v,1,(const char **)&vs,NULL);
	glShaderSource(f,1,(const char **)&fs,NULL);
	free(vs);
	free(fs); 
	glCompileShader(v);
	glCompileShader(f);
	p = glCreateProgram();
	glAttachShader(p,f);
	glAttachShader(p,v);
	glLinkProgram(p);
	return(p);
}

void set_material() {
	float mat_ambient[] = {0.1,0.18725,0.1745,1.0}; 
	float mat_diffuse[] = {0.396,0.74151,0.9102,1.0}; 
	float mat_specular[] = {0.297254,0.30829,0.306678,1.0};
	float mat_shininess[] = {70.8};
	glMaterialfv(GL_FRONT,GL_AMBIENT,mat_ambient);
	glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_diffuse);
	glMaterialfv(GL_FRONT,GL_SPECULAR,mat_specular);
	glMaterialfv(GL_FRONT,GL_SHININESS,mat_shininess);
}


void load_texture(char *filename, unsigned int tid, unsigned int p) {
	FILE *fopen(), *fptr;
	char buf[512];
	int im_size, im_width, im_height, max_color;
	unsigned char *texture_bytes, *parse; 

	fptr=fopen(filename,"r");
	fgets(buf,512,fptr);
	do{
		fgets(buf,512,fptr);
	} while(buf[0]=='#');
	
	parse = strtok(buf," \t");
	im_width = atoi(parse);

	parse = strtok(NULL," \n");
	im_height = atoi(parse);

	fgets(buf,512,fptr);
	parse = strtok(buf," \n");
	max_color = atoi(parse);

	im_size = im_width*im_height;
	texture_bytes = (unsigned char *)calloc(3,im_size);
	fread(texture_bytes,3,im_size,fptr);
	fclose(fptr);

	glActiveTexture(GL_TEXTURE0 + tid - 1);
	glBindTexture(GL_TEXTURE_2D,tid);
	glEnable(GL_TEXTURE_2D);
	
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,im_width,im_height,0,GL_RGB, GL_UNSIGNED_BYTE,texture_bytes);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_REPLACE);
	free(texture_bytes);
	

}

void draw_array() {

	int location;

	set_material();

	// Teapot
	location = glGetUniformLocation(sprogram,"mode");
	glUniform1i(location,1);
	location = glGetUniformLocation(sprogram,"shadowmap");
	glUniform1i(location,7);
	location = glGetUniformLocation(sprogram,"envMap");
	glUniform1i(location,TEXTURE3 - 1);
	location = glGetUniformLocation(sprogram,"mytexture");
	glUniform1i(location,TEXTURE1 - 1);
	location = glGetUniformLocation(sprogram,"mynormalmap");
	glUniform1i(location,NORMAL1 - 1);
	glPushMatrix();
	glTranslatef(0, -1.1, 0);
	glRotated( 80.0, 0.0, 1.0, 0.0 );
	glRotated( 20.0, 1.0, 0.0, 0.0 );
	glClearColor(0,0,0,0.0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDrawArrays(GL_QUADS, 0, numFaces*4);
	glPopMatrix();

	// Ground Plane
	location = glGetUniformLocation(sprogram,"mode");
	glUniform1i(location,2);
	location = glGetUniformLocation(sprogram,"mytexture");
	glUniform1i(location,TEXTURE2 - 1);
	double x=8.0;
	double y = -1.0;
	double z = 8.0;
	glBegin(GL_POLYGON);
		glNormal3f(0, 1, 0);
		glTexCoord2f(1.0f, 0.0f); 		glVertex3f(-x, y, z);
		glTexCoord2f(1.0f, 1.0f); 		glVertex3f(-x, y, -z);
		glTexCoord2f(0.0f, 1.0f); 		glVertex3f(x, y, -z);
		glTexCoord2f(0.0f, 0.0f); 		glVertex3f(x, y, z);
	glEnd();
	
	// Sky
	location = glGetUniformLocation(sprogram,"mode");
	glUniform1i(location,3);
	location = glGetUniformLocation(sprogram,"mytexture");
	glUniform1i(location,TEXTURE3 - 1);
	GLUquadric* qptr=gluNewQuadric();
	gluQuadricTexture(qptr,1);
	gluQuadricOrientation(qptr,GLU_INSIDE);
	glTranslatef(0, -0.7, 0);
	glRotatef(0,0.0,1.0,0.0);
	glRotatef(90,1.0,0.0,0.0);
	gluSphere(qptr,8.0,64,64);
	glPopMatrix();
}


void view_volume(float *ep, float *vp, int jitter) {
	vp[0] += JITTER*genrand();
	vp[1] += JITTER*genrand();
	vp[2] += JITTER*genrand();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0,(float)(XRES)/(float)(YRES),0.1,20.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(ep[0],ep[1],ep[2],vp[0],vp[1],vp[2],0.0,1.0,0.0);
}

void aa_display(float *ep, float *vp) {
	int view_pass;
	glClear(GL_ACCUM_BUFFER_BIT);
	for( view_pass=0; view_pass<VPASSES; view_pass++ ){
		view_volume(ep, vp, 1);
		draw_array();
		glAccum(GL_ACCUM,1.0/(float)(VPASSES));
	}
	glAccum(GL_RETURN,1.0);
	glutSwapBuffers();
}

// Code taken from Dr. Geist's handout
void build_shadowmap() {
	glBindTexture(GL_TEXTURE_2D,1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, XRES, YRES, 0, 
		GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER,1);
	glDrawBuffer(GL_NONE);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,1,0);
	glBindFramebufferEXT(GL_FRAMEBUFFER,0);
}

// Code taken from Dr. Geist's handout
void save_matrix(float *ep, float *vp) {
	glMatrixMode(GL_TEXTURE); 
	// This must match the unit used in the vertex shader.
	glActiveTexture(GL_TEXTURE7);
	glLoadIdentity();
	glTranslatef(0.0,0.0,-0.005);
	glScalef(0.5,0.5,0.5);
	glTranslatef(1.0,1.0,1.0);
	gluPerspective(45.0,(float)(XRES)/(float)(YRES),0.1,20.0);
	gluLookAt(ep[0],ep[1],ep[2],vp[0],vp[1],vp[2],0.0,1.0,0.0);
}

// Code taken from Dr. Geist's handout
void do_stuff() {
	float eyepoint[3], viewpoint[3];
	int k;
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.8,0.6,0.62,1.0);
	glBindFramebufferEXT(GL_FRAMEBUFFER,1); 
	glUseProgram(0);
	for(k=0;k<3;k++){ 
		eyepoint[k] = light0_position[k]; 
		viewpoint[k] = light0_direction[k]+light0_position[k];
	}
	
	view_volume(eyepoint,viewpoint, 0);

	lights(); 
	aa_display(eyepoint,viewpoint);
	glBindFramebufferEXT(GL_FRAMEBUFFER,0); 

	save_matrix(eyepoint,viewpoint);

	glUseProgram(sprogram);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D,1);

	// Draw scene from the intended eye point, complete with shadows. 
	eyepoint[0] = 1.0; eyepoint[1] = 2.0; eyepoint[2] = -5.0;
	viewpoint[0] = 0.0; viewpoint[1] = 0.0; viewpoint[2] = 0.0;
	view_volume(eyepoint,viewpoint, 0);
	lights(); 
	aa_display(eyepoint,viewpoint);
	glutSwapBuffers();
}

int main(int argc, char **argv) {
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE|GLUT_ACCUM);

	glutInitWindowSize(XRES,YRES);
	glutInitWindowPosition(300,50);
	glutCreateWindow("Beautiful Teapot");
	sprogram=set_shaders();

	load_obj(sprogram, "teapot.605.obj");

	load_texture( "glaz.ppm", TEXTURE1, sprogram);
	load_texture( "snow.ppm", TEXTURE2, sprogram);
	load_texture( "sky.ppm", TEXTURE3, sprogram);
	
	load_texture( "teabump.ppm", NORMAL1, sprogram);
	glActiveTexture(GL_TEXTURE0);

	build_shadowmap();

	glBindBuffer(GL_ARRAY_BUFFER, mybuf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexPointer(3, GL_FLOAT, 3*sizeof(GLfloat), NULL+0);
	glTexCoordPointer(2, GL_FLOAT, 2*sizeof(GLfloat), NULL+numFaces*3*4*sizeof(GLfloat));
	glNormalPointer(GL_FLOAT, 3*sizeof(GLfloat), NULL+numFaces*(3*4+2*4)*sizeof(GLfloat));
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glutDisplayFunc(do_stuff);
	glutMainLoop();
	return 0;
}
