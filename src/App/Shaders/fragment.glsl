#version 330 core

out vec4 out_col;

uniform vec2 resolution;
uniform vec2 offset;
uniform float zoom;
uniform float time;
uniform int maxIterations;


void main()
{
	vec2 original_offset = offset * resolution.y / 2;
	vec2 k = (2.0 * gl_FragCoord.xy - resolution) / resolution.y;
	vec2 uv = k / zoom + offset;

	// Initial values
	vec2 z = vec2(0.0);
	vec2 c = uv;

	int iter = 0;
	float smooth_val = 0.0;

	// Mandelbrot iteration
	for (int i = 0; i < maxIterations; i++)
	{
		iter = i;
		// Complex multiplication: z^2 + c
		z = vec2(
			z.x * z.x - z.y * z.y + c.x,
			2.0 * z.x * z.y + c.y);
		// If |z| > 2, point escapes to infinity
		if (dot(z, z) > 4.0)
		{
			// Smooth coloring
			smooth_val = float(i) - dot(z, z) + 4.0;
			break;
		}
	}

	// Color calculation
	vec3 color;
	if (iter == maxIterations - 1)
	{
		// Inside the Mandelbrot set
		float si = sin(time / 1000);
		color = mix(vec3(0.30, 0.47, 0.8), vec3(0.707, 0.12, 0.9), si * si) / 3;
	}
	else
	{
		float co = cos(time / 1000);
		float t = smooth_val / float(maxIterations);
		color = mix(vec3(0.30, 0.47, 0.8), vec3(0.707, 0.12, 0.9), t + 0.1 * co * co);
	}

	// Add some visual enhancements
	color = mix(color, vec3(1.0), 1.0 - smoothstep(0.0, 0.1, length(z)));

	out_col = vec4(color, 1.0f);
}