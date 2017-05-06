#version 130

varying vec2 f_texcoord;
uniform sampler2D mytexture;

void main( void ) {

	vec4 tex_color = texture2D(mytexture, f_texcoord);
	
	if(tex_color[3] == 0.0) {
		discard;
	}

	gl_FragColor = tex_color;

}
