=== =================================================
== vertex 
void main()
{
  
  gl_Position = GetMVPMatrix() * vec4(LOFI_GET_position(), 1.0);
  LOFI_SET_color(LOFI_GET_color());
}

=== =================================================
== fragment 
#ifdef LOFI_GLSL_330
out vec4 fragColor;
#endif
void main()
{
#ifdef LOFI_GLSL_330
#ifdef LOFI_HAS_color
  fragColor = vec4(LOFI_GET_color(), 1.0);
#else
  fragColor = vec4(0.5,0.5,0.5, 1.0);
#endif
#else
#ifdef LOFI_HAS_color
  gl_FragColor = vec4(LOFI_GET_color(), 1.0);
#else
  gl_FragColor = vec4(0.5,0.5,0.5, 1.0);
#endif
#endif
}