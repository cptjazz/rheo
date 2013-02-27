// __expected:arg_overwrite_1(a => $_retval)
int arg_overwrite_1(int a) {
  if (a)
    return 3;
  else 
    return 4;
}

// __expected:arg_overwrite_2()
int arg_overwrite_2(int a) {
  a = 0;

  if (a)
    return 3;
  else 
    return 4;
}
