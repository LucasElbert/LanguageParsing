#include "tree.h"
#include "utils.h"

#include <string>

template <>
string Tree<string>::print(int level) {
  string s(2 + 2 * level, ' ');
  if (!HasChildren()) {
    return s + value_;
  }
  if (IsPreterminal()) {
    return s + value_ + " -> " + children_[0]->value_;
  }

  s = s + value_ + "\n";
  for (int i = 0; i < children_.size(); i++) {
    s += children_[i]->print(level + 1);
    if (i < children_.size() - 1) {
      s += "\n";
    }
  }

  if (level == 0) {
    printf("%s\n", s.c_str());
  }
  return s;
}

template <>
string Tree<string>::BracketString() {
  if (!HasChildren()) {
    return value_;
  }
  if (IsPreterminal()) {
    return value_ + " " + children_[0]->value_;
  }
  string s = value_;
  for (shared_ptr<Tree<string> > c : children_) {
    s += " (" + c->BracketString() + ")";
  }

  return s;
}

void ApplyTermRule(Tree<string>* t) {
  vector<shared_ptr<Tree<string> > >& children = t->children_;
  for (int i = 0; i < t->num_children(); i++) {
    shared_ptr<Tree<string> > child = children[i];
    if (child->IsPreterminal()) {
      auto dummy = make_shared<Tree<string> >("_" + child->value_,
                                              t->weak_from_this());
      dummy->AddChild(child);
      child->parent_ = dummy;
      t->SetChild(i, dummy);
    }
  }
}

void ApplyBinarizeRule(Tree<string>* t) {
  vector<shared_ptr<Tree<string> > >& children = t->children_;

  // Construct dummy
  string dummy_value = "";
  auto dummy = make_shared<Tree<string> >(dummy_value, t->weak_from_this());
  for (int i = 1; i < children.size(); i++) {
    if (i > 1) {
      dummy_value += "&";
    }
    dummy_value += children[i]->value_;
    // Attach child to dummy and increase child's reference counter
    dummy->AddChild(children[i]);
  }
  dummy->value_ = dummy_value;

  // Detach C2, C3, ... from t
  for (int i = children.size() - 1; i >= 1; i--) {
    t->children_.pop_back();
  }

  t->AddChild(dummy);
}

void ApplyUnitRule(Tree<string>* t) {
  shared_ptr<Tree<string> > c = t->children_.back();
  t->ClearChildren();
  vector<shared_ptr<Tree<string> > >& grand_children = c->children_;
  for (shared_ptr<Tree<string> > gc : grand_children) {
    t->AddChild(gc);
  }
}

void NormalizeTree(Tree<string>* t) {
  stack<Tree<string>*> stack_to_normalize;

  // iterate through the tree and normalize the branchings
  // (collapse levels (UNIT), binarize rules (BIN), Insert fake terminals
  // (TERM))
  stack_to_normalize.push(t);
  while (!stack_to_normalize.empty()) {
    Tree<string>* t = stack_to_normalize.top();
    stack_to_normalize.pop();  // This function cuts out this child.

    // 0 Children
    if (t->IsTerminal() || t->IsPreterminal()) {
      continue;
    }

    // 1 Child
    if (t->num_children() == 1) {
      Tree<string>* child = t->children_.back().get();
      if (child->IsPreterminal()) {
        // Child is POS-tag
        continue;
      }
      // UNIT Rule (collapse one level)
      // cut out 'child' from the chain 't -> child -> grand_child'
      ApplyUnitRule(t);
      stack_to_normalize.push(t);
      continue;
    }

    // >= 2 Children.
    // Now it's assumed no child is a terminal because terminals (words) can
    // only
    // be produced by Preterminals (POS-tags) which only have 1 child.
    // So all children are now Preterminals or NonTerminals

    if (HasPreterminalChild(t)) {
      // Some children are Preterminals(=POS-tags)
      // TERM Rule (Wrap non solitary terminals by inserting dummy nonterminals)
      ApplyTermRule(t);
      stack_to_normalize.push(t);
    } else {
      // All children are Nonterminals
      if (t->num_children() == 2) {
        // Perfect binary rule.
        stack_to_normalize.push(t->children_[0].get());
        stack_to_normalize.push(t->children_[1].get());
      } else {
        // > 2 Nonterminal children
        // BINARIZE Rule
        ApplyBinarizeRule(t);
        stack_to_normalize.push(t);
      }
    }
  }
}

shared_ptr<Tree<string> > ParseTree(string s) {
  auto root = make_shared<Tree<string> >("");

  if (s.substr(0, 8) != "( (SENT " || s.substr(s.length() - 1, 1) != ")") {
    return root;
  }
  // strip first and last bracket as they are meaningless
  s = s.substr(2, s.length() - 3);
  root->value_ = "SENT";
  int i = 5;
  char last_stop = ' ';
  shared_ptr<Tree<string> > pcurrent_tree = root;
  while (i < s.length() - 2) {
    int next_i;
    char next_stop;
    string next_text;
    tie(next_i, next_stop, next_text) = ReadUntil(s, i + 1, "()");

    if (next_stop == '(') {
      // create child
      tie(next_i, next_stop, next_text) = ReadUntil(s, next_i + 1, " ");
      next_text = simplify_nonterminal(next_text);
      pcurrent_tree = pcurrent_tree->MakeChild(next_text);
    } else if (next_stop == ')') {
      if (last_stop == ' ') {
        // It looks like "...(NP foofoo)...". Create foofoo terminal
        pcurrent_tree->MakeChild(next_text);
      }
      pcurrent_tree = pcurrent_tree->parent_.lock();
    }

    i = next_i;
    last_stop = next_stop;
  }
  return pcurrent_tree;
}

string simplify_nonterminal(string nt) {
  int i = nt.find('-');
  if (i != string::npos) {
    return nt.substr(0, i);
  }
  return nt;
}
