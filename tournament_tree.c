//
// Created by Dor Green on 04/05/2019.
//
#include "types.h"
#include "user.h"
#include "kthread.h"
#include "tournament_tree.h"

trnmnt_tree* trnmnt_tree_alloc(int depth){
    // if depth is too shallow or the tree is bigger than the mutexes we could allow, fail
    // recursion base
    if(depth < 1 || ( depth>1 && depth>>1 > MAX_MUTEXES) )  return 0;

    // try to alloc a mutex for this node
    int muid = 0;
    muid = kthread_mutex_alloc();
    if(muid < 1){
        //printf(1,"Can't alloc mutex!\n"); // TODO DEBUG ONLY
        return 0;
    }

    // succeed creating a mutex, allocate the node
    trnmnt_tree* left = 0;
    trnmnt_tree* right = 0;

    trnmnt_tree* tt = (trnmnt_tree*) malloc(sizeof(trnmnt_tree));

    tt->height = depth;
    tt->mutex_id = muid;

    // RECURSE!
    left = trnmnt_tree_alloc(depth-1);
    right = trnmnt_tree_alloc(depth-1);

    // Should get two (real) trees or this is leaf mutex.
    // If not, dealloc self and recursively dealloc subtrees, if there are any.
    if(depth > 1 && (right == 0 || left == 0)){
        // POSSIBLY INFINITE LOOP with other failing calls from trnmnt_tree_alloc ?
        free(tt);
        kthread_mutex_dealloc(muid);
        if(right != 0) trnmnt_tree_dealloc(right);
        if(left != 0) trnmnt_tree_dealloc(right);
        return 0;
    }

    // successfully allocated everything. commit and return
    tt->left = left;
    tt->right = right;
    return tt;
}

// Dealloc from root to remove the whole tree
// first dealloc children, then self.
int trnmnt_tree_dealloc(trnmnt_tree* tree){
    // Base case: the empty tree is already de-allocated.
    if(tree == 0) return 0;

    int fail = 0;
    // RECURSE!
    if(tree->height > 1){
        fail = fail + trnmnt_tree_dealloc(tree->left);
        fail = fail + trnmnt_tree_dealloc(tree->right);
        if(fail < 0){
            //printf(1,"Can't dealloc children!\n"); // TODO DEBUG ONLY
        }
    }

    // Dealloc this mutex
    fail = fail + kthread_mutex_dealloc(tree->mutex_id);

    // finally, dealloc the tree node
    // FREE IF THE MUTEX WON'T DEALLOCATED?!?!?!?!!
    free(tree);

    // propogate errors, if had any
    if(fail < 0) return -1;
    return 0;
}

// returns the highest (relative) index in the left subtree
int get_middle(int height){
    // {lower value} 0.5 * ( 2^h-1    - 1 )
    return (int) ((2<<(height-1))-1)/2;
}



// return 0 if all good
// -1 on failure
// Basically fancy binary search with aquiring mutex from the leafs up.
int trnmnt_tree_acquire(trnmnt_tree* tree,int ID){
    // the ID is of a thread, that is a leaf in our tree
    // the thread (leaf) is not a part of the actual tree
    // ID is RELATIVE TO TREE HEIGHT, and always between 0 : (2^height) -1
    if(tree == 0 || ID < 0) return -1;

    // BASE CASE: mutex-leaf. has two threads competeting.
    if(tree->height == 1){
        // if ID == 0 it's the left thread
        // if ID == 1 it's the right thread
        kthread_mutex_lock(tree->mutex_id);
        return 0;
    }


    // INDUCTIVE CASE: should we go left or right?
    int mid = get_middle(tree->height); // mid = the highest (relative) index in the left subtree

    if(ID > mid){
        // ID is in the right sub-tree.
        if(tree->right == 0)
            return -1;
        if(trnmnt_tree_acquire(tree->right, ID-mid-1)== -1){
            return -1;
        }
    }
    else{
        // ID is in the left sub-tree.
        if(tree->left == 0)
            return -1;
        if(trnmnt_tree_acquire(tree->left, ID) == -1){
            return -1;
        }
    }

    // By here, we've (recursivly) aqquired all nodes from the bottom up to the given node
    return kthread_mutex_lock(tree->mutex_id);
}



// Release locks in tree from root to leaf
// Get to the root, unlock it, than unlock other nodes in path in that order
// updates ID to be relative ID upon going down the tree
int trnmnt_tree_release(trnmnt_tree* tree,int ID){
    int ans = 0;
    if(tree == 0)
        return -1;

    // Release me
    ans = kthread_mutex_unlock(tree->mutex_id);

    // BASE CASE, made it to leaf-mutex
    if(tree->height == 1) return ans;

    // Now go down
    // INDUCTIVE CASE: should we go left or right?
    int mid = get_middle(tree->height); // mid = the highest (relative) index in the left subtree

    if(ID > mid){
        // ID is in the right sub-tree.
        if(tree->right == 0){
            return -1;
        }

        return trnmnt_tree_release(tree->right, ID-mid-1);
    }
    else{
        // ID is in the left sub-tree.
        // No need to change ID :)
        if(tree->left == 0)
            return -1;
        return trnmnt_tree_release(tree->left, ID);
    }
}
