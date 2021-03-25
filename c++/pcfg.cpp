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
  // Which POS-tags can each word be produced from, and with which probabilitiy?
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
  // From which NonTerminals can other NonTerminals or POS-Tags be generated 
  // and with which probability?
  for (auto const& it : grammar_probs_) {
    bool is_binary_rule = it.first.right_.size() == 2;
    bool is_single_rule = it.first.right_.size() == 1;
    // Our grammar is in Chomsky Normal Form, therefore the right handside
    // always has either 2 NonTerminals or a single PosTag
    if (is_binary_rule) {
      Rule const& rule = it.first;
      auto generated = pair<string, string>(rule.right_[0], rule.right_[1]);
      NonTerm generated_left = rule.right_[0];
      NonTerm generated_right = rule.right_[1];
      double probability = it.second;
      pair<Rule, double> entry(rule, probability);

      if (reverse_grammar_binary_.count(generated) == 0) {
        reverse_grammar_binary_[generated] = vector<pair<Rule, double> >();
      }
      if (generate_left.count(generated_left) == 0) {
        generate_left[generated_left] = vector<pair<Rule, double> >();
      }
      if (generate_right.count(generated_right) == 0) {
        generate_right[generated_right] = vector<pair<Rule, double> >();
      }
      
      generate_right[generated_right].push_back(entry);
      generate_left[generated_left].push_back(entry);
      reverse_grammar_binary_[generated].push_back(entry);
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

vector<pair<Rule, double> > PCFG::GetGeneratingNonTerms(string left_nt,
                                                          string right_nt) {
  return reverse_grammar_binary_[pair<string,string>(left_nt, right_nt)];
}

vector<pair<string, double> > PCFG::GetGeneratingNonTerms(string pos_tag) {
  return reverse_grammar_single_[pos_tag];
}

vector<pair<string, double> > PCFG::GetGeneratingPosTags(string word) {
  return reverse_lexicon_[word];
}

void print (pTreeProb pTP) {
  printf("(%s, %f) ", pTP.first->value_.c_str(), pTP.second);
}

void printRow(ParseTableRow& row) {
  printf("size: %i\n", row.size());
  for (int i = 0; i < row.size(); i++) {
    printf("Cell %i, size: %i\n", i, row[i].size());
    for (int j = 0; j < row[i].size(); j++) {
      print(row[i][j]);
    }
    printf("\n");
  }
}

ParseTableRow PCFG::BuildTokenRow(vector<string> const & tokens) {
  ParseTableRow row;
  // Lowest row simply contains the tokens wraped in a tree
  for (int i = 0; i < tokens.size(); i++) {
    auto t = make_shared<Tree<string> >(tokens[i]);
    pTreeProb tree_and_prob(t, 1.0);
    row.push_back(vector<pTreeProb>({tree_and_prob}));
  }
  return row;
}

ParseTableRow PCFG::BuildUnitaryParentRow(
                              ParseTableRow const & children_row,
                              SYMBOL_TYPE children_type){

  ParseTableRow parents_row;
  for (vector<pTreeProb> const & children_cell : children_row) {
    // map instead of vector to avoid duplicates
    map<string, pTreeProb> parents_cell;
    for (pTreeProb child_and_prob : children_cell) {

      shared_ptr<Tree<string> > child_tree = child_and_prob.first;
      string child_symbol = child_tree->value_;
      vector<pair<string,double> > parent_symbols;
      
      if (children_type==POS_TAG) {
        parent_symbols = GetGeneratingNonTerms(child_symbol);
      } else if (children_type==TOKEN) {
        parent_symbols = GetGeneratingPosTags(child_symbol);
      }

      for (pair<string,double> ps : parent_symbols) {
        string parent_symbol = ps.first;
        double probability = ps.second * child_and_prob.second;
        printf("%.3e = %.3e * %.3e\n", probability, ps.second, child_and_prob.second);
        auto parent_tree = make_shared<Tree<string> >(parent_symbol);
        parent_tree->AddChild(child_tree);
        
        // Only save one tree per symbol as we search the max likelihood tree
        bool new_symbol = parents_cell.find(parent_symbol) == parents_cell.end();
        bool higher_probability = false;
        if (!new_symbol) {
          higher_probability = parents_cell[parent_symbol].second < probability;
        }
        if (new_symbol || higher_probability) {
          parents_cell[parent_symbol] = pTreeProb(parent_tree, probability);
        }
      }
    }
    vector<pTreeProb> parents_cell_;
    for (auto it = parents_cell.begin(); it != parents_cell.end(); ++it) {
      pTreeProb ptb = it->second;
      parents_cell_.push_back(ptb);
    }
    parents_row.push_back(parents_cell_);
  }
  return parents_row;
}

/**
 * Creates a new tree from the 
 */
pTreeProb PCFG::BuildParentTree(NonTerm generator_symbol,
                                double generation_prob,
                                pTreeProb left_child,
                                pTreeProb right_child) {

  shared_ptr<Tree<string> > left_tree = left_child.first;
  double left_prob = left_child.second;
  
  shared_ptr<Tree<string> > right_tree = right_child.first;
  double right_prob = right_child.second;

  auto new_tree = make_shared<Tree<string> >(generator_symbol);
  new_tree->AddChild(left_tree);
  left_tree->parent_ = new_tree->weak_from_this();
  new_tree->AddChild(right_tree);
  right_tree->parent_ = new_tree->weak_from_this();
  double new_prob = generation_prob * left_prob * right_prob;
  printf("%.3e = %.3e * %.3e * %.3e\n", new_prob, generation_prob, left_prob, right_prob);
  return pTreeProb(new_tree, new_prob);
}

/**
 * Creates new trees from all PCFG known rules "NT -> (A,B)" with
 * A a root symbol of a tree in 'left' and
 * B a root symbol of a tree in 'right'
 * 
 * Returns a vector of all those trees (with multiplied probabilities)
 * 
 * If a NonTerminal has several generation possibilities only the one with
 * highest probability is returned
 */
vector<pTreeProb> PCFG::BuildParentTrees(vector<pTreeProb> left, 
                                         vector<pTreeProb> right) {
  // Find symbols that can generate tree symbols in left and right
  // Mapping is a bit complicated because the symbol search is string based
  // but the cells contain tree structures. So we need a mapping 
  // from strings to Trees.
  map<string,pTreeProb> left_targets;
  std::set<pair<Rule, double> > generators_left;
  for (pTreeProb ptp : left) {
    string node_symbol = ptp.first->value_;
    left_targets[node_symbol] = ptp;
    auto generators = generate_left[node_symbol];
    generators_left.insert(generators.begin(), generators.end());
  }
  map<string, pTreeProb> right_targets;
  std::set<pair<Rule, double> > generators_right;
  for (pTreeProb ptp : right) {
    string node_symbol = ptp.first->value_;
    right_targets[node_symbol] = ptp;
    auto generators = generate_right[node_symbol];
    generators_right.insert(generators.begin(), generators.end());
  }

  vector<pair<Rule, double> > generators;
  std::set_intersection(generators_left.begin(), generators_left.end(),
                        generators_right.begin(), generators_right.end(),
                        std::back_inserter(generators));
  
  map<NonTerm, pTreeProb> parent_trees;
  // Create a new tree for every found generator
  for (pair<Rule, double> generator : generators) {
    NonTerm generator_symbol = generator.first.left_;
    double generator_probability = generator.second;
    Rule rule = generator.first;
    pTreeProb left_child = left_targets[rule.right_[0]];
    pTreeProb right_child = right_targets[rule.right_[1]];
    pTreeProb parent = BuildParentTree(generator_symbol,
                                       generator_probability,
                                       left_child, 
                                       right_child);
    // Only save one tree per symbol as we search the max likelihood tree
    bool new_symbol = parent_trees.find(generator_symbol) == parent_trees.end();
    bool higher_probability = false;
    if (!new_symbol) {
      higher_probability = parent_trees[generator_symbol].second < parent.second;
    }
    if (new_symbol || higher_probability) {
      parent_trees[generator_symbol] = parent;
    }
  }

  vector<pTreeProb> parent_trees_;
  for (auto it = parent_trees.begin(); it != parent_trees.end(); ++it) {
    pTreeProb ptb = it->second;
    parent_trees_.push_back(ptb);
  }

  return parent_trees_;
}

/**
 * Generates the next row in a valid CYK table
 * 
 * Table is expected to be a CYK table that stores possible symbols to generate tokens
 * table[0] stores the tokens itself. table[1] the POS-Tags that can generate
 * the tokens. All cells above S_(start,end) stores symbols that can eventually
 * generate token_start, ..., token_end
 * 
 * 
 * * ------- GENERATE THE NEXT TOP ROW -----
 *   ...             ...        ...   
 * table[4] ->  |  S_(0,2)  |  S_(1,3)   |  S_(2,4)   |
*  table[3] ->  |  S_(0,1)  |  S_(1,2)   |  S_(2,3)   |  S_(3,4)   |
 * table[2] ->  |  S_(0,0)  |  S_(1,1)   |  S_(2,2)   |  S_(3,3)   |  S_(4,4)   |
 * table[1] ->  |  POS-Tags |  POS-Tags  |  POS-Tags  |  POS-Tags  |  POS-Tags  | 
 * table[0] ->  |  token_0  |  token_1   |  token_2   |  token_3   |  token_4   |   
 */
ParseTableRow PCFG::BuildBinaryParentRow(vector<ParseTableRow> const & table) {
  ParseTableRow row;
  int n_cells = table.back().size()-1;
  int generation_length = table.size()-1;
  int sentence_length = table[0].size();
  printf("n_cells %i\n generation_length %i\n", n_cells, generation_length);
  for (int start = 0; start <= sentence_length - generation_length; start++) {
    // Create and fill the cell S_(start, end)
    int end = start+generation_length-1;
    map<NonTerm, pTreeProb> cell;
    // Find Rules of the form (N -> (A,B)) with 
    // A in S_(start,left_end) and B in S_(left_end+1,end)
    for (int left_end = start; left_end < end; left_end++ ){
      int left_length = left_end - start+1;
      int right_length = end-left_end;
      vector<pTreeProb> const & left_cell = table[left_length+1][start];
      vector<pTreeProb> const & right_cell = table[right_length+1][left_end+1];
      
      vector<pTreeProb> parent_trees = BuildParentTrees(left_cell, right_cell);
      // Store all new or new most likely trees (only most likely cuz MLE)
      for (pTreeProb ptb : parent_trees) {
        NonTerm parent_symbol = ptb.first->value_;
        bool new_symbol = cell.find(parent_symbol) == cell.end();
        bool higher_probability = false;
        if (!new_symbol) {
          higher_probability = cell[parent_symbol].second < ptb.second;
        }
        if (new_symbol || higher_probability) {
          cell[parent_symbol] = ptb;
        }
      }
    }
    // Convert map to vector, only needed the map to keep maxlikelihood only
    vector<pTreeProb> cell_;
    for (auto it = cell.begin(); it != cell.end(); ++it) {
      pTreeProb ptb = it->second;
      cell_.push_back(ptb);
    }
    row.push_back(cell_);
  }
  return row;
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
shared_ptr<Tree<string> > PCFG::ParseSentence(vector<string> tokens) { 
  printf("\n%i tokens\n", tokens.size());
  vector<ParseTableRow> table;

  // Lowest row simply contains the tokens wraped in trees
  // (Later add spelling correction)
  table.push_back(BuildTokenRow(tokens));

  // Second lowest row contains Pos-Tags that generate the tokens
  table.push_back(BuildUnitaryParentRow(table[0], TOKEN));
  
  // Third lowest row contains NonTerminals that generate the Pos-Tags
  // By unitary rules
  table.push_back(BuildUnitaryParentRow(table[1], POS_TAG));
  
  // Higher rows contain non terminals that generate the symbols in
  // lower rows. Elements of the lowest row (table[2]) generate single POS-Tags,
  // which generate single tokens.
  // Elements on level x eventually dissolve into x-1 tokens
  for (int level=3; level <= tokens.size()+1; level++) {
    printf("table[%i]\n", level);
    table.push_back(BuildBinaryParentRow(table));
  }

  int count_trees = 0;
  for (ParseTableRow row : table) {
    for (vector<pTreeProb> cell : row) {
      count_trees += cell.size();
    }
  }
  printf("num_trees: %i\n",count_trees);
  
  pTreeProb mle = GetMostLikely(table.back()[0]);
  printf("%.6e\n",mle.second);
  return mle.first;
}

pTreeProb PCFG::GetMostLikely(vector<pTreeProb> ptbs) {
  pTreeProb best;
  double best_probability = -1;
  for (pTreeProb ptb : ptbs) {
    if (ptb.second > best_probability) {
      best = ptb;
      best_probability = ptb.second;
    }
  }
  return best;
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
      left_handside_count[r.left_] = 1;
    } else {
      left_handside_count[r.left_]++;
    }
    if (rule_count.find(r) == rule_count.end()) {
      rule_count[r] = 1;
    } else {
      rule_count[r]++;
    }
  }

  for (Rule r : rules) {
    double prob = ((double)rule_count[r]) / left_handside_count[r.left_];
    out[r] = prob;
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