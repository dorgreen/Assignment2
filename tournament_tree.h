//
// Created by Dor Green
//


// This is a recursive tree structure. Each node is a mutex with a link to it's two sons (or none)
// the user holds a pointer to the root
// the tree is of depth n-1, where the n-th level is the threads trying to lock the mutex tree
// on the last level, left and right nodes are NULL, and two threads compete for this node's mutex.
// for a depth of N, we'll make (2^N)-1 of these nodes

//struct trnmnt_tree;

typedef struct trnmnt_tree {
    int mutex_id  ; // The mutex representing this node.
    struct trnmnt_tree *left;      // the left son (if non-leaf) or null if it's the last mutex level.
    struct trnmnt_tree *right;     // the right son (if non-leaf) or null if it's the last mutex level.
    int height ;    // DEPTH from trnmnt_tree_alloc() for root.
    // the leafs are the threads, so the last trnmnt_tree level is 1!!!
    // if height == 1, both left and right are NULL, and two threads compete for holding the mutex given by mutex_id
} trnmnt_tree;


trnmnt_tree* trnmnt_tree_alloc(int depth);
int trnmnt_tree_dealloc(trnmnt_tree* tree);
int trnmnt_tree_acquire(trnmnt_tree* tree,int ID);
int trnmnt_tree_release(trnmnt_tree* tree,int ID);
