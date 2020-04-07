=== =================================================
== Simple.Vertex 
void main()
{
    vec3 p = vec3(view * model * vec4(position,1.0));
    gl_Position = projection * vec4(p,1.0);
}

=== =================================================
== Simple.Fragment 
void main()
{
#ifdef LOFI_HAS_color
  LOFI_SET_color(LOFI_GET_color(0), 1.0);
#else
  LOFI_SET_color(1.0, 0.0, 0.0, 1.0);
#endif
}