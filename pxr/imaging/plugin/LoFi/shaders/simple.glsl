=== =================================================
== vertex 
void main()
{
  /*
  gl_Position = GetMVPMatrix() * vec4(LOFI_GET_position(), 1.0);
  LOFI_SET_color(LOFI_GET_color());
  */
  mat4 viewModel = GetViewMatrix() * GetModelMatrix();
  vec4 p = viewModel * vec4(LOFI_GET_position(), 1.0);
  vec4 n = viewModel * vec4(LOFI_GET_normal(), 0.0);
  gl_Position = GetProjectionMatrix() * p;
  LOFI_SET_color(LOFI_GET_color());
  LOFI_SET_position(p.xyz);
  LOFI_SET_normal(n.xyz);
}

=== =================================================
== fragment 
void main()
{
#ifdef LOFI_HAS_color
LOFI_SET_result(vec4(LOFI_GET_color(),1.0));
#else
LOFI_SET_result(vec4(1.0,0.0,0.0,1.0));
#endif

}