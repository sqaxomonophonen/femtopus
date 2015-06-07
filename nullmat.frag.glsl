#version 120

varying vec3 v_normal;
varying vec2 v_uv;

void main()
{
	if (fract(v_uv.x) < 0.1 || fract(v_uv.y) < 0.1) {
		gl_FragColor = vec4(1,1,1,1);
	} else {
		gl_FragColor = vec4((v_normal+1)/2, 1);
	}
}

