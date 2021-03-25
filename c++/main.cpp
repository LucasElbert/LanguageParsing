/**
 * Probabilistic Constituency Tree Natural Language Parsing
 *
 * Author: Lucas Elbert
 */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <memory>

#include "pcfg.h"
#include "tree.h"
#include "utils.h"

using namespace std;

shared_ptr<Tree<string> > foo() {
  auto t1 = make_shared<Tree<string> >("T1");
  auto t2 = t1->MakeChild("T1.1");
  shared_ptr<Tree<string> > t4 = t1->MakeChild("T1.2");
  shared_ptr<Tree<string> > t3 = t2->MakeChild("T1.1.1");
  shared_ptr<Tree<string> > t5 = t2->MakeChild("T1.1.2");
  t3->MakeChild("A");
  t5->MakeChild("B");
  t1->print();

  return t1;
}

int main() {
  ifstream infile("../data/sequoia-corpus+fct.mrg_strict");
  string line;
  vector<shared_ptr<Tree<string> > > trees;
  while (getline(infile, line)) {
    shared_ptr<Tree<string> > t = ParseTree(line);
    trees.push_back(t);
    NormalizeTree(t.get());
    //t->print();
    //printf("%s",t->BracketString().c_str());
  }
  printf("\nnumber of trees: %d\n",trees.size());
  printf("InferePCFG\n");

  PCFG pcfg = InferePCFG(trees);
  printf("# vocab: %d\n", pcfg.lexicon_.size());
  printf("# non terms: %d\n", pcfg.non_terminals_.size());
  printf("# pos tags: %d\n", pcfg.pos_tags_.size());


  // Read a new sentence from command line
  line = "Cette exposition nous apprend que une industrie métallurgique existait .";
  vector<string> tokens = split(line, ' ');

  shared_ptr<Tree<string> > t = pcfg.ParseSentence(tokens);
  //DenormalizeTree(&t);
}
