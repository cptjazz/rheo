// From: http://en.wikipedia.org/wiki/Binary_search_algorithm
//

#define KEY_NOT_FOUND -1

// __expected:midpoint(min => $_retval, max => $_retval)
int midpoint(int min, int max) {
  return (min + max) / 2;
}

// __expected:binary_search_recursive(imin => $_retval, imax => $_retval, key => $_retval, A => $_retval)
int binary_search_recursive(int A[], int key, int imin, int imax)
{
  // test if array is empty
  if (imax < imin)
    // set is empty, so return value showing not found
    return KEY_NOT_FOUND;
  else
  {
    // calculate midpoint to cut set in half
    int imid = midpoint(imin, imax);
    // three-way comparison
    if (A[imid] > key)
      // key is in lower subset
      return binary_search_recursive(A, key, imin, imid - 1);
    else if (A[imid] < key)
      // key is in upper subset
      return binary_search_recursive(A, key, imid + 1, imax);
    else
      // key has been found
      return imid;
  }
}

// __expected:binary_search_iterative(imin => $_retval, imax => $_retval, key => $_retval, A => $_retval)
int binary_search_iterative(int A[], int key, int imin, int imax)
{
  // continue searching while [imin,imax] is not empty
  while (imax >= imin)
  {
    /* calculate the midpoint for roughly equal partition */
    int imid = midpoint(imin, imax);

    // determine which subarray to search
    if (A[imid] < key)
      // change min index to search upper subarray
      imin = imid + 1;
    else if (A[imid] > key)
      // change max index to search lower subarray
      imax = imid - 1;
    else
      // key found at index imid
      return imid;
  }

  // key not found
  return KEY_NOT_FOUND;
}
