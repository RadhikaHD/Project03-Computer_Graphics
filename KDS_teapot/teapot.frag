/*
 * Alekhya Kamana, Radhika Dhaipule, & Christian Stith
 * CPSC 6050: Introduction to Graphics
 * Dr. Robert Geist
 * April 23, 2015
 * Project 3: The Teapot Contest!
 * 
 * Custom fragment shader. Renders teapot, ground plane, and sky sphere
 * in respective modes.
 * 
 */
 
varying vec3 ec_vnormal, ec_vposition, ec_vtangent, ec_vbitangent;
varying vec4 scoords;
uniform sampler2D envMap, mynormalmap, mytexture, shadowmap;
uniform int mode;

float getShadow() {
	
	float depthsample, clarity;
	vec4 pccoords = scoords/scoords.w;
	depthsample = texture2D(shadowmap, pccoords.st).z;
	clarity = 1.0;
	if(depthsample < (pccoords.z + 0.0045)) clarity = 0.5;
	return clarity;
}



vec4 env_light() {
	return texture2D( envMap, vec2(gl_TexCoord[1].s, 1.0 - gl_TexCoord[1].t));
}


vec4 norms(int normal_map, int spec) {

	vec4 sum = vec4(0, 0, 0, 1.0);

	for ( int i=0; i<3; i++) {
	
		mat3 tform;
		vec3 P, N, L, V, H, tcolor, mapN;
		vec4 diffuse_color = gl_FrontMaterial.diffuse; 
		vec4 specular_color = gl_FrontMaterial.specular; 
		float shininess = gl_FrontMaterial.shininess;
		float pi = 3.14159;

		tform = mat3(ec_vtangent, ec_vbitangent, ec_vnormal);
		P = ec_vposition;
		N = normalize(ec_vnormal);
		L = normalize(gl_LightSource[i].position - P);
		V = normalize(-P);
		H = normalize(L+V);
				
		if ( normal_map) {
			mapN = vec3(texture2D(mynormalmap, gl_TexCoord[0].st));
			mapN.xy = 2.0 * mapN.xy - vec2(1.0, 1.0);
			N = normalize(tform * normalize(mapN));
		}
				
		tcolor = vec3(texture2D(mytexture, gl_TexCoord[0].st));
		diffuse_color = vec4(tcolor, 1.0) * max( dot(N, L), 0.0 );
		diffuse_color *= gl_LightSource[i].diffuse;


		if ( spec ) {

			specular_color = gl_FrontMaterial.specular*pow( max( dot( H, N ), 0.0 ), shininess);
			specular_color *= ( shininess+2.0)/(8.0*pi);
			specular_color *= gl_LightSource[i].specular;

			specular_color *= pow(max(dot(H,N),0.0),shininess);
			float normalize = ( shininess + 2.0 ) / (8.0*3.14159);
			specular_color *= normalize;
			diffuse_color += specular_color;
		}

	
		sum += diffuse_color;	
	}
	sum /= 3.0;
	return sum;

}

void teapot() {
	vec4 environment = env_light();
	vec4 surface = norms(1, 1);
	gl_FragColor = 0.5*environment + 0.5*surface;
	gl_FragColor *= getShadow();
}

void plane() {
	vec4 surface = norms(0, 1);
	gl_FragColor = surface;
	gl_FragColor *= getShadow();

}

void sky() {
	vec4 surface = norms(0, 0);
	gl_FragColor = surface;
}



void main() {
	if ( mode == 1 ) {
		teapot();
	} else if ( mode == 2 ) {
		plane();
	} else if ( mode == 3 ) {
		sky();
	}
	
	gl_FragColor *= 1.2;
	
}
