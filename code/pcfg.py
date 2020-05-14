import copy
import numpy as np

from tree import *


class PCFG():
    '''
    Class representing a Probabilistic Context Free Grammar
    
    It mainly consists of
        A Grammar (Nonterminals, POS-tags, grammar_rules, grammar_probabilities)
        A Lexicon (POS-tags, vocabulary, lexicon_rules, lexicon_probabilities)
    '''
    
    start_symbol = 'SENT'
    
    def __init__(self,
                 lexicon_rules,
                 lexicon_probabilities,
                 grammar_rules,
                 grammar_probabilities,
                 vocab,
                 pos,
                 nonterminals):
        '''
        probabilities should be a dictionary:
        probabilities[left_handside][right_handside] = ...
        '''
        self.lexicon_rules = lexicon_rules
        self.lexicon_probabilities = lexicon_probabilities
        self.grammar_rules = grammar_rules
        self.grammar_probabilities = grammar_probabilities
        self.nonterminals = nonterminals
        self.vocab = vocab
        self.pos = pos
        
        # An index that facilitates searching for rules with given children
        self.left_child_index, self.right_child_index, self.pos_parent_index = self.build_indices(grammar_rules)
    
    def get_lexicon_prob(self, pos, token):
        if pos in self.lexicon_probabilities:
            if token in self.lexicon_probabilities[pos]:
                return self.lexicon_probabilities[pos][token]
            else:
                return None
        print('Error unknown POS')
        assert False
        
    def get_grammar_prob(self, rule):
        return self.grammar_probabilities[rule.str_lhs][rule.str_rhs]
        
    def get_possible_pos_parents(self, pos):
        '''
        For a given pos-tag returns all nonterminals 
        that can directly produce this pos-tag.
        '''
        result = []
        for rule in self.pos_parent_index[pos]:
            left_handside = rule.str_left_handside()
            right_handside = rule.str_right_handside()
            prob = self.grammar_probabilities[left_handside][right_handside]
            result.append((left_handside,prob))
        return result
        
    def get_possible_parents(self, left_child, right_child):
        '''
        Finds all nonterminals NT such that there is a rule (NT -> left_child right_child)
        Returns those NTs with the corresponding probabilities
        '''
        
        if left_child in self.left_child_index and right_child in self.right_child_index:
            possible_parents_left = self.left_child_index[left_child]
            possible_parents_right = self.right_child_index[right_child]
            possible_parent_rules = possible_parents_left.intersection(possible_parents_right)
        else:
            return []
        
        result = []
        right_handside = left_child+' '+right_child
        for rule in possible_parent_rules:
            left_handside = rule.str_left_handside()
            right_handside = rule.str_right_handside()
            prob = self.grammar_probabilities[left_handside][right_handside]
            result.append((left_handside,prob))
        return result
    
    def build_indices(self, rules):
        '''
        Sets up dictionaries to facilitate searching for rules with given children
        
        rules_with_right_child['S'] will yield all rules with right child 'S'.
        Assuming rules are in binary form (lhs -> rhs_0 rhs_1)
        '''    

        rules_with_right_child = dict()
        rules_with_left_child = dict()
        rules_with_terminal = dict()
        for i, rule in enumerate(rules):
            if len(rule.right_handside) > 2:
                print('Invalid rule')
                print(rule)
                assert False
            if len(rule.right_handside) == 1:
                # Nonterminal -> terminal (pos tag)
                pos = rule.right_handside[0]
                if pos not in rules_with_terminal:
                    rules_with_terminal[pos] = set([rule])
                else:
                    rules_with_terminal[pos].add(rule)
            else:
                # Binary rules 'nonterminal -> nonterminal nonterminal'
                left_child, right_child = rule.right_handside

                # create index entries
                if right_child not in rules_with_right_child:
                    rules_with_right_child[right_child] = set([rule])
                else:
                    rules_with_right_child[right_child].add(rule)

                if left_child not in rules_with_left_child:
                    rules_with_left_child[left_child] = set([rule])
                else:
                    rules_with_left_child[left_child].add(rule)

        return rules_with_left_child, rules_with_right_child, rules_with_terminal
        
    
def normalize_tree(tree):
    '''
    Transforms the tree into a tree obeying the chomsky normal form
    '''
    work_tree = copy.deepcopy(tree)
    # iterate through the tree and normalize the rules (collapse levels (UNIT) and binarize rules (BIN))
    stack_to_normalize = [work_tree]
    
    while len(stack_to_normalize) > 0:
        t = stack_to_normalize.pop()
         
        if t.is_terminal() or t.is_preterminal():
            continue
            
        if len(t.children) == 1:
            child = t.children[0]
            if child.is_terminal() or child.is_preterminal():
                continue
            # UNIT rule (collapse one level)
            t.children = child.children
            for grand_child in t.children:
                grand_child.parent = t
            # restart treating t
            stack_to_normalize.append(t)
            continue
        
        # We have >= 2 children
        children_are_nt = [not (c.is_terminal() or c.is_preterminal()) for c in t.children]
        assert not any([c.is_terminal() for c in t.children])
        # exactly two nonterminal children -> perfect binary rule -> continue
        if all(children_are_nt) and (len(t.children) == 2):
            stack_to_normalize += t.children
            continue
        
        # Some children are not nonterminals
        # TERM
        if not all(children_are_nt):
            # TERM eliminate non solitary terminals by inserting dummy nonterminals
            # Assuming the not nonterminals are POS (not tokens)!
            # Rules of the form   NonTerm -> (NonTerm POS)
            # will be reshaped to NonTerm -> (NonTerm _POS), _POS -> POS with the new nonterminal _POS
            for i in range(len(t.children)):
                if not children_are_nt[i]:
                    child = t.children[i]
                    intermediate_tree = Tree(value='_'+child.value, parent=t, children=[child])
                    child.parent = intermediate_tree
                    t.children[i] = intermediate_tree
            # restart treating t
            stack_to_normalize.append(t)
            continue
        
        # Have more than two children (all are nonterminals)
        # BINARIZE
        # split into first nonterminal vs rest of nonterminals
        children_values = [c.value for c in t.children[1:]]
        new_tree_value = '&'.join(children_values)
        new_tree = Tree(new_tree_value, parent=t, children=t.children[1:])
        t.children = [t.children[0],new_tree]
        # restart treating t
        stack_to_normalize.append(t)
        
    return work_tree


def create_pcfg(trees):
    '''
    Inferes a PCFG from a collection of parse trees
    
    returns the PCFG
    '''
    
    lexicon_rules = set()
    grammar_rules = set()
    pos = set()
    vocab = set()
    nonterminals = set()
    lexicon_rule_counts = {}
    grammar_rule_counts = {}
    
    # count rules in trees
    for t in trees:
        tree = normalize_tree(t)
        tree_lexicon, tree_grammar = tree.all_rules(split_lexicon_grammar=True)
        tree_nts, tree_pos, tree_vocab = tree.extract_values()
        
        # update set of rules, nonterminals, etc.
        pos.update(tree_pos)
        vocab.update(tree_vocab)
        nonterminals.update(tree_nts)
        lexicon_rules.update(tree_lexicon)
        grammar_rules.update(tree_grammar)
        
        # count rules
        for rules, rule_counts in [(tree_lexicon, lexicon_rule_counts),
                                   (tree_grammar, grammar_rule_counts)]:
            for rule in rules:
                # update the counters
                str_lh = rule.str_left_handside()
                str_rh = rule.str_right_handside()
                if str_lh not in rule_counts:
                    rule_counts[str_lh] = {}
                if str_rh not in rule_counts[str_lh]:
                    rule_counts[str_lh][str_rh] = 0.0
                rule_counts[str_lh][str_rh] += 1.0
    
    # Give every POS a small probability to emit an unknown word
    for p in pos:
        r = Rule(p,['<UNK>'])
        lexicon_rules.add(r)
        lexicon_rule_counts[p]['<UNK>'] = 1
    
    # transform counts to probabilities
    for rule_counts in [lexicon_rule_counts, grammar_rule_counts]:
        for str_lh, counts in rule_counts.items():
            total = sum(counts.values())
            for str_rh in counts.keys():
                counts[str_rh] = np.log(counts[str_rh]/total)
    
    pcfg = PCFG(lexicon_rules,
                lexicon_rule_counts,
                grammar_rules,
                grammar_rule_counts,
                vocab,
                pos,
                nonterminals)
    return pcfg
            

