void main()
{
#ifdef LOFI_HAS_color
  LOFI_SET_color(LOFI_GET_color(0), 1.0);
#else
  LOFI_SET_color(1.0, 0.0, 0.0, 1.0);
#endif
}