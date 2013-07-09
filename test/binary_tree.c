// From: http://en.wikipedia.org/wiki/Recursion_%28computer_science%29#Binary_trees
//
#include <stdio.h>

struct node
{
  int data;            // some integer data
  struct node *left;   // pointer to the left subtree
  struct node *right;  // point to the right subtree
};

// Test if tree_node contains i; return 1 if so, 0 if not.
// __expected:tree_contains(tree_node => $_retval, i => $_retval, tree_node => tree_node)
int tree_contains(struct node* tree_node, int i) {
  if (tree_node == NULL)
    return 0;  // base case
  else if (tree_node->data == i)
    return 1;
  else
    return tree_contains(tree_node->left, i) || tree_contains(tree_node->right, i);
}

// Inorder traversal:
// __expected:tree_print(tree_node => tree_node)
void tree_print(struct node* tree_node) {
  if (tree_node != NULL) {            // base case
    tree_print(tree_node->left);      // go left
    // __define:printf()
    printf("%d ", tree_node->data);   // print the integer followed by a space
    tree_print(tree_node->right);     // go right
  }
}
