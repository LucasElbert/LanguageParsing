#ifndef TREE_H
#define TREE_H

#include <stddef.h>
#include <stdio.h>
#include <string>
#include <tuple>
#include <iostream>
#include <vector>
#include <stack>
#include <memory>

using std::vector;
using std::string;
using std::stack;
using std::tuple;
using std::pair;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using std::make_shared;


template <typename T>
class Tree: public std::enable_shared_from_this<Tree<T> > {
 public:
  T value_;
  vector<shared_ptr<Tree<T> > > children_;
  weak_ptr<Tree<T> > parent_;

  Tree() { ; }
  Tree(T value) : value_(value) { ; }
  Tree(T value, weak_ptr<Tree<T> > parent) : value_(value), parent_(parent) { ; }
  

  shared_ptr<Tree<T> > MakeChild(T t);
  void AddChild(shared_ptr<Tree<T> > t);
  void SetChild(int i, shared_ptr<Tree<T> > c) { children_[i] = c; };
  void ClearChildren() { children_.clear(); }
  vector<shared_ptr<Tree<T> > >& GetChildren() { return children_; }
  void release();
  int num_children() { return children_.size(); }
  bool HasChildren() { return children_.size() != 0; }
  bool IsLeaf() { return !HasChildren(); }
  bool IsQuasiLeaf() { 
    return (children_.size() == 1) && (children_[0]->IsTerminal());
  }
  bool IsTerminal() { return IsLeaf(); }
  bool IsPreterminal() {
    return IsQuasiLeaf();
  }
  bool IsRoot() { return parent_ == nullptr; }

  string print(int level=0);
  string BracketString();
};

// Tree pointer and a probability
typedef pair<shared_ptr<Tree<string> >,double> pTreeProb;

// Adds c as a child to this tree. This tree takes (partial) ownership of c!
template<typename T> void Tree<T>::AddChild(shared_ptr<Tree<T> > c){
  children_.push_back(c);
}

// Creates a new tree with value t, ownership lies in 'this'.
// Pointer to the child is returned.
template<typename T> shared_ptr<Tree<T> > Tree<T>::MakeChild(T t){
  auto c = make_shared<Tree<T> >(t, this->weak_from_this());
  children_.push_back(c);
  return c;
}


// Checks whether the tree t has a preterminal child.
// (A child which has a single child which is a leaf)
// e.g. t -> child -> leaf
template<typename T> bool HasPreterminalChild(Tree<T>* t) {
  vector<shared_ptr<Tree<string> > >& children = t->GetChildren();
  for (int i = 0; i < children.size(); i++) {
    if (children[i]->IsPreterminal()) return true;
  }
  return false;
}


/**
 * Replaces t's preterminal children (POS) by dummy nonterminals (_POS).
 *      t                     t
 *    /   \                 /   \
 *  NT    POS     =>      NT    _POS
 *  /\     |              /\      |
 *  ...   word            ...    POS
 *                                |
 *                               word
*/
void ApplyTermRule(Tree<string>* t);

/**
 * Assumes t has more than two children
 * This method keeps the first child and merges all the rest under a new
 * node to make the split at t a binary one.
 *           t                t
 *        /  |  \     =>     /  \
 *      C1  C2  C3         C1   C2&C3
 *                               / \ 
 *                              C2  C3
*/
void ApplyBinarizeRule(Tree<string>* t);

/**
 * Assumes t has a single child which is not a leaf.
 * This function cuts out that single child.
 *         t                      t
 *         c          =>    c1  c2  c3  c4
 *    c1 c2 c3 c4         
 */
void ApplyUnitRule(Tree<string>* t);

/**
 * Transforms a dependency tree into Chomsky Normal Form
 * i.e. it will later only consist of rules (branchings) of the form:
 * NonTerminal -> [NonTerminal]*
 * NonTerminal -> POS-Tag
 * POS-Tag -> token
 */
void NormalizeTree(Tree<string>* t);


/**
 * Builds up a tree from a valid bracket expression
 * Caller must take ownership of the returned tree!
 * That means when it is not needed anymore call the destructor!
 */
shared_ptr<Tree<string> > ParseTree(string s);

/**
 * For simplification we only consider 
 * Returns nonterminal modulo functional label
 * e.g. PP-MOD -> PP
 */
string simplify_nonterminal(string nt);


#endif