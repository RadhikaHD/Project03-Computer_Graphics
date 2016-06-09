/*
 * Alekhya Kamana, Radhika Dhaipule, & Christian Stith
 * CPSC 6050: Introduction to Graphics
 * Dr. Robert Geist
 * April 23, 2015
 * Project 3: The Teapot Contest!
 * 
 * Custom vertex shader. Passes shadowmap and texture coordinate data.
 * 
 */
 
 varying vec3 ec_vnormal, ec_vposition, ec_vtangent, ec_vbitangent;
varying vec4 scoords;
attribute vec3 tangent, bitangent;

void main()
{
	ec_vnormal = gl_NormalMatrix*gl_Normal;
	ec_vposition = gl_ModelViewMatrix*gl_Vertex;
	gl_Position = gl_ProjectionMatrix*gl_ModelViewMatrix*gl_Vertex;
	// This is the result of the simulated render from the light source.
	scoords = gl_TextureMatrix[7]*gl_Vertex;
	
	ec_vtangent = gl_NormalMatrix*tangent;
	ec_vbitangent = gl_NormalMatrix*bitangent;	

	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	vec3 u = ec_vposition;
	vec3 n = ec_vnormal;
	
	vec3 r = reflect( u, n );
	
	float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
	gl_TexCoord[1].s = r.x/m + 0.5;
	gl_TexCoord[1].t = r.y/m + 0.5;
	
}

