#ifndef PCFG_H
#define PCFG_H

#include <map>
#include <string>
#include <set>
#include <vector>

#include "tree.h"

using std::string;
using std::vector;
using std::map;
using std::multiset;
using std::set;
using std::pair;

typedef string NonTerm;
typedef string POS;
typedef string Token;

// Represents a rule of a probabilistic context free grammar
// left_: left handside of a rule
// right_: right handside of a rule
// Example: "A -> B C"
struct Rule {
  string left_;
  vector<string> right_;

  Rule(string left, string right);
  Rule(string left, string right1, string right2);
};

bool operator==(const Rule& r1, const Rule& r2);
bool operator!=(const Rule& r1, const Rule& r2);
bool operator<(const Rule& r1, const Rule& r2);

// This PCFG implementation is strictly speaking a mix of two PCFGs
// I find it more natural this way to construct MLE constituency trees,
// which involves the POS-tag <-> word mapping (lexicon) and
// the grammar <-> POS-tag tree construction.
// For further info check out "Speech and language processing" by Dan Jurafsky
// Chapter 13 Constituency Parsing
// https://web.stanford.edu/~jurafsky/slp3/
class PCFG {
 public:
  // grammar_probs_:
  // probabilities with wich the nonterminals dissolve following certain rules.
  // grammar_probs_["NP"][Rule("NP","N1","N2")] = 0.2
  // means that "NP" will with 20% chance dissolve binarily in "N1", "N2".
  set<string> non_terminals_;
  set<string> pos_tags_;
  set<string> lexicon_;
  map<Rule, double> grammar_probs_;
  map<Rule, double> lexicon_probs_;

  // Structures to help reverse searching for rules
  // Maps words onto POS-tags that can produce them with a certain probability.
  map<string, vector<pair<string, double> > > reverse_lexicon_;
  // Maps onto NonTerm that can binarily produce the given string pair.
  map<pair<string, string>, vector<pair<string, double> > > reverse_grammar_binary_;
  map<string, vector<pair<string, double> > > reverse_grammar_single_;

  PCFG(set<string>& non_terminals, set<string>& pos_tags,
       set<string>& vocab, map<Rule, double>& lexicon_probs,
       map<Rule, double>& grammar_probs);

  // Searches for all nonterminals that can generate the given nonterminal pair.
  // Returns these NTs with their probability to dissolve to the given pair.
  vector<pair<string, double> > GetGeneratingNonTerms(string left_nt,
                                                      string right_nt);
  // Searches for all nonterminals that can generate the single pos_tag
  // (pos_tag = terminal of the grammar)
  // Returns these NTs with their probability to dissolve to the given POS tag
  vector<pair<string, double> > GetGeneratingNonTerms(string pos_tag);
  // Searches all POS-tags that can generate the given word.
  // Returns those POS-tags with their probability to generate the given word.
  vector<pair<string, double> > GetGeneratingPosTags(string word);

  // Computes the Maximum Likelihood Constituency Tree to produce the given
  // sequence of words(=tokens).
  Tree<string>* ParseSentence(vector<string> tokens);
  void BuildTokenRow(vector<string> const & tokens,
                     vector<vector<pTreeAndProb> >& row);
  void BuildPosTagRow(vector<vector<pTreeAndProb> > const & token_row,
                          vector<vector<pTreeAndProb> >& pos_tag_row);
};


// Inferes a PCFG from the rules of the normalized trees
// Returns a pointer to that PCFG
PCFG InferePCFG(vector<shared_ptr<Tree<string> > >& trees);


// Extracts all rules from the tree t and inserts them either in
// grammar_rules or lexicon_rules
//
// Assumes t is in chomsky normal form (e.g. by calling NormalizeTree)
//
// Grammar rules are all branchings of the form
// Nonterminal -> [Nonterminal]*
// Nonterminal -> POS-tag
// Lexicon rules are all branchings of the form
// POS-tag -> token
void extract_rules(Tree<std::string>* t, 
                   vector<Rule>& grammar_rules, 
                   vector<Rule>& lexicon_rules,
                   set<string>& vocab,
                   set<string>& pos_tags,
                   set<string>& non_terminals,
                   bool simplify_nonterminals = true);
#endif