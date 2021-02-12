#include "pcfg.h"
#include "tree.h"

Rule::Rule(string left, string right) {
  left_ = left;
  right_ = vector<string>{right};
}
Rule::Rule(string left, string right1, string right2) {
  left_ = left;
  right_ = vector<string>{right1, right2};
}

bool operator==(const Rule& r1, const Rule& r2) {
  if (r1.left_ != r2.left_) return false;
  if (r1.right_.size() != r2.right_.size()) return false;
  for (int i = 0; i < r1.right_.size(); i++) {
    if (r1.right_[i] != r2.right_[i]) return false;
  }
  return true;
}

bool operator!=(const Rule& r1, const Rule& r2) { return !(r1 == r2); }

string GetComparisonString(const Rule& r) {
  string s = r.left_;
  for (string right : r.right_) {
    s += right;
  }
  return s;
}

bool operator<(const Rule& r1, const Rule& r2) {
  return GetComparisonString(r1) < GetComparisonString(r2);
}

PCFG::PCFG(set<string>& non_terminals, set<string>& pos_tags,
           set<string>& vocab, map<Rule, double>& lexicon_probs,
           map<Rule, double>& grammar_probs) {
  non_terminals_ = non_terminals;
  pos_tags_ = pos_tags;
  lexicon_ = vocab;
  lexicon_probs_ = lexicon_probs;
  grammar_probs_ = grammar_probs;

  // Construct the reverse lexicon
  // from which POS-tags can each word be produced, and with which probabilitiy?
  for (string word : lexicon_) {
    reverse_lexicon_[word] = vector<pair<string, double> >();
  }
  for (auto const& it : lexicon_probs_) {
    string pos_tag = it.first.left_;
    string word = it.first.right_[0];
    double probability = it.second;
    reverse_lexicon_[word].push_back(
        pair<string, double>(pos_tag, probability));
  }

  // Construct the reverse grammar
  // From which Nonterminals can
  //  - binary Nonterminal pairs
  //  - POS-tags
  // be constructed and with which probability?
  for (auto const& it : grammar_probs_) {
    bool is_binary_rule = it.first.right_.size() == 2;
    bool is_single_rule = it.first.right_.size() == 1;
    // Our grammar is in Chomsky Normal Form, therefore the right handside
    // always has either 2 non terminals or a single pos-tag
    if (is_binary_rule) {
      Rule const& rule = it.first;
      pair<string, string> right =
          pair<string, string>(rule.right_[0], rule.right_[1]);
      double probability = it.second;
      pair<string, double> entry(rule.left_, probability);
      if (reverse_grammar_binary_.count(right) == 0) {
        reverse_grammar_binary_[right] = vector<pair<string, double> >();
      }
      reverse_grammar_binary_[right].push_back(entry);
    } else if (is_single_rule) {
      double probability = it.second;
      string pos_tag = it.first.right_[0];
      string non_term = it.first.left_;
      pair<string, double> entry(non_term, probability);
      if (reverse_grammar_single_.count(pos_tag) == 0) {
        reverse_grammar_single_[pos_tag] = vector<pair<string, double> >();
      }
      reverse_grammar_single_[pos_tag].push_back(entry);
    }
  }
}

vector<pair<string, double> > PCFG::GetGeneratingNonTerms(string left_nt,
                                                          string right_nt) {
  return reverse_grammar_binary_[pair<string,string>(left_nt, right_nt)];
}

vector<pair<string, double> > PCFG::GetGeneratingNonTerms(string pos_tag) {
  return reverse_grammar_single_[pos_tag];
}

vector<pair<string, double> > PCFG::GetGeneratingPosTags(string word) {
  return reverse_lexicon_[word];
}



void PCFG::BuildTokenRow(vector<string> const & tokens,
                         vector<vector<pTreeAndProb> >& row) {
  row.clear();
  // Lowest row is an exception it just contains the raw tokens
  for (int i = 0; i < tokens.size(); i++) {
    auto t = make_shared<Tree<string> >(tokens[i]);
    pTreeAndProb tree_and_prob(t, 1.0);
    row.push_back(vector<pTreeAndProb>({tree_and_prob}));
  }
}

void PCFG::BuildPosTagRow(vector<vector<pTreeAndProb> > const & token_row,
                          vector<vector<pTreeAndProb> >& pos_tag_row) {
  pos_tag_row.clear();
  for (int i = 0; i < token_row.size(); i++) {
    vector<pTreeAndProb> pos_tag_row_cell;
    shared_ptr<Tree<string> > token_tree = token_row[i][0].first;
    string token = token_tree->value_;
    vector<pair<string,double> > pos_tags = GetGeneratingPosTags(token);
    for (pair<string,double> pp : pos_tags) {
      string pos_tag = pp.first;
      auto tree = make_shared<Tree<string> >(pos_tag);
      tree->AddChild(token_tree);
      double probability = pp.second;
      pos_tag_row_cell.push_back(pTreeAndProb(tree, probability));
    }
    pos_tag_row.push_back(pos_tag_row_cell);
  }
}


/**
Constructs the Maximum likelihood constituency tree to produce the
token sequence.
Implements a variant of the probabilistic CYK Algorithm.
Bottom Up DP
Algorithm overview:
We fill a table where cell (y,x) contains trees that can dissolve to
create tokens (t[x],...,t[x+y-1])
*/
Tree<string>* PCFG::ParseSentence(vector<string> tokens) { 
  vector<vector<vector<pTreeAndProb> > > table(tokens.size());

  // Lowest row simply contains the tokens wraped in trees
  // BuildTokenRow(tokens, table[0]);

  // Second lowest row contains Pos-Tags that generate the tokens
  // BuildPosTagRow(table[0], table[1]);

  // Third lowest row contains NonTerminals that generate the Pos-Tags
  // By unitary rules
  
  // Higher rows contain non terminals that appropriately generate symbols in
  // lower rows
  for (int level=3; level < tokens.size()+2; level++) {
    ;
  }


  
  return nullptr;
  

  
  
  for (int length=2; length <= tokens.size(); length++) {
    for(int offset=0; offset <= tokens.size()-length; offset++) {
      // Fill cell (y,x), i.e. non terms that can create tokens t_x,...,t_(x+y)
    }
    // Non-terms on level i that can create non-terms on level i+1
  }
  
}

void extract_rules(Tree<std::string>* t, vector<Rule>& grammar_rules,
                   vector<Rule>& lexicon_rules, set<string>& vocab,
                   set<string>& pos_tags, set<string>& non_terminals,
                   bool simplify_nonterminals) {
  // DFS, convert each node (except leaves) to a rule
  stack<Tree<std::string>*> stack;
  stack.push(t);
  while (!stack.empty()) {
    t = stack.top();
    stack.pop();

    if (t->IsPreterminal()) {
      // POS-tag -> token
      string pos_tag = simplify_nonterminal(t->value_);
      string token = t->children_.back()->value_;
      lexicon_rules.push_back(Rule(pos_tag, token));
      pos_tags.insert(pos_tag);
      vocab.insert(token);
      continue;
    }
    if (t->num_children() == 1) {
      // Nonterminal -> POS-tag
      string nt = t->value_;
      if (simplify_nonterminals) nt = simplify_nonterminal(nt);
      string pos_tag = t->children_[0]->value_;
      grammar_rules.push_back(Rule(nt, pos_tag));
      non_terminals.insert(nt);
      stack.push(t->children_[0].get());
      continue;
    } else {
      // Binary nonterminal rule
      // NT -> NT_A NT_B
      string left = t->value_;
      if (simplify_nonterminals) left = simplify_nonterminal(left);
      string right1 = t->children_[0]->value_;
      string right2 = t->children_[1]->value_;
      grammar_rules.push_back(Rule(left, right1, right2));
      non_terminals.insert(left);
      stack.push(t->children_[0].get());
      stack.push(t->children_[1].get());
    }
  }
}

/**
 * Restructures the rules in a way that gives for each rule the observed
 * frequency that the left handside of the rule has dissolved following this
 * rule
 * Returns a map string -> vector<pair(Rule, double)>
 * e.g. If rules contains only the three rules(A -> B C), (A -> B E), (A -> B C)
 * The return would be {(A -> B C) -> 0.667, (A -> B E) -> 0.333}
 */
void CountsToProbabilities(vector<Rule>& rules, map<Rule, double>& out) {
  map<string, int> left_handside_count;
  map<Rule, int> rule_count;

  for (Rule r : rules) {
    if (left_handside_count.find(r.left_) == left_handside_count.end()) {
      left_handside_count[r.left_] = rule_count[r];
    } else {
      left_handside_count[r.left_] += rule_count[r];
    }
    if (rule_count.find(r) == rule_count.end()) {
      rule_count[r] = 1;
    } else {
      rule_count[r]++;
    }
  }

  for (Rule r : rules) {
    out[r] = ((double)rule_count[r]) / left_handside_count[r.left_];
  }
}

PCFG InferePCFG(vector<shared_ptr<Tree<string> > >& trees) {
  set<string> non_terminals;
  set<string> pos_tags;
  set<string> vocab;

  vector<Rule> grammar_rules;
  vector<Rule> lexicon_rules;

  for (shared_ptr<Tree<string> > t : trees) {
    extract_rules(t.get(), grammar_rules, lexicon_rules, vocab, pos_tags,
                  non_terminals);
  }

  // To give every pos tag the possibility to emit an unknown word
  // we add the artificial observation pos -> <UNK> for every pos tag
  for (string pos : pos_tags) {
    lexicon_rules.push_back(Rule(pos, "<UNK>"));
  }

  map<Rule, double> lexicon_rule_probabilities;
  map<Rule, double> grammar_rule_probabilities;
  CountsToProbabilities(lexicon_rules, lexicon_rule_probabilities);
  CountsToProbabilities(grammar_rules, grammar_rule_probabilities);

  PCFG pcfg(non_terminals, pos_tags, vocab, lexicon_rule_probabilities,
            grammar_rule_probabilities);
  return pcfg;
}