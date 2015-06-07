#version 120

varying vec3 v_normal;
varying vec2 v_uv;

void main()
{
	vec4 nc = vec4((v_normal+1)/2, 1);
	if (fract(v_uv.x) < 0.1 || fract(v_uv.y) < 0.1) {
		gl_FragColor = nc + vec4(0.3,0.3,0.3,0);
	} else {
		gl_FragColor = nc;
	}
}

