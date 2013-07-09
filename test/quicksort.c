// From: http://p2p.wrox.com/visual-c/66347-quick-sort-c-code.html
// 

// __expected:q_sort(left => numbers, right => numbers, numbers => numbers)
void q_sort(int numbers[], int left, int right)
{
  int pivot, l_hold, r_hold;

  l_hold = left;
  r_hold = right;
  pivot = numbers[left];

  while (left < right)
  {
    while ((numbers[right] >= pivot) && (left < right))
      right--;

    if (left != right)
    {
      numbers[left] = numbers[right];
      left++;
    }

    while ((numbers[left] <= pivot) && (left < right))
      left++;

    if (left != right)
    {
      numbers[right] = numbers[left];
      right--;
    }
  }

  numbers[left] = pivot;
  pivot = left;
  left = l_hold;
  right = r_hold;

  if (left < pivot)
    q_sort(numbers, left, pivot - 1);
  if (right > pivot)
    q_sort(numbers, pivot + 1, right);
}
