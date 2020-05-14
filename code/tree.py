import re
from graphviz import Digraph

class Rule():
    '''
    Class representing a rule of a context free grammar
    '''

    def __init__(self, left_handside, right_handside):
        '''
        Input:  
            left_handside is a nonterminal (string)
            right_handside is a list (list of strings)
        '''
        self.left_handside = left_handside
        self.right_handside = right_handside
        self.my_str = self.to_str()
        self.my_hash = self.to_hash()
        self.str_lhs = str(self.left_handside)
        self.str_rhs = ' '.join(self.right_handside)
        
    def __str__(self):
        return self.my_str
    
    def __hash__(self):
        return self.my_hash
    
    def to_hash(self):
        return self.to_str().__hash__()
    
    def to_str(self):
        s = str(self.left_handside)
        s += ' -->'
        for rh in self.right_handside:
            s += ' '+str(rh)
        return s
    
    def str_left_handside(self):
        return self.str_lhs
    
    def str_right_handside(self):
        return self.str_rhs
    
    def __eq__(self, other):
        return self.left_handside == other.left_handside and self.right_handside == other.right_handside
    
    def __neq__(self, other):
        return not self.__eq__(other)
    

    
class Tree():
    '''
    Basic Tree class to represent a context free grammar parse tree
    '''
    
    def __init__(self, value=None, parent=None, children=[]):
        self.children = children.copy()
        self.value = value    
        self.parent = parent
    
    def is_terminal(self):
        return not self.has_children()
    
    def has_children(self):
        return len(self.children) != 0
    
    def is_non_terminal(self):
        return not self.is_terminal()
    
    def is_preterminal(self):
        # A preterminal node corresponds to a POS-tag
        return (len(self.children) == 1) and self.children[0].is_terminal()
    
    def add_child(self, child_tree):
        self.children.append(child_tree)
        child_tree.parent = self
    
    def get_child(self, number):
        return self.children[number]

    def is_root(self):
        return self.parent is None
    
    def to_string(self, level=0):
        s = self.value+'\n'
        for child in self.children:
            s += '     '*(level+1)+child.to_string(level+1)
        return s
    
    def __str__(self):
        return self.to_string(level=0)
    
    def to_bracket_str(self):
        '''
        Produces the bracket representation of the tree.
        '''
        if self.is_preterminal():
            return '('+self.value+' '+self.children[0].value+')'
        
        s = str(self.value)
        for c in self.children:
            s += ' '+c.to_bracket_str()
        s = '('+s+')'
        return s
    
    def all_terminals(self, eliminate_duplicates=True):
        terminals = []
        stack = [self]
        while len(stack) > 0:
            t = stack.pop()
            if t.is_terminal():
                terminals.append(t.value)
            else:
                stack += t.children[::-1]
        if eliminate_duplicates:
            return list(set(terminals))
        return terminals
    
    def all_nonterminals(self, eliminate_duplicates=True):
        nts = []
            
        stack = [self]
        while len(stack) > 0:
            t = stack.pop()
            if t.is_non_terminal():
                nts.append(t.value)
                stack += t.children
        
        if eliminate_duplicates:
            return list(set(nts))
        return nts
    
    def all_preterminals(self, eliminate_duplicates=True):
        pts = []
        stack = [self]
        while len(stack) > 0:
            t = stack.pop()
            if len(t.children) == 1 and t.children[0].is_terminal():
                pts.append(t.value)
            else:
                stack += t.children
        if eliminate_duplicates:
            return list(set(pts))
        return pts
    
    def extract_values(self, eliminate_duplicates = True):
        '''
        Extracts nonterminals, pos-tags and tokens from the tree
        '''
        nonterminals = []
        pos = []
        vocab = []
        
        stack = [self]
        while len(stack) > 0:
            t = stack.pop()
            if t.is_terminal():
                vocab.append(t.value)
            elif t.is_preterminal():
                pos.append(t.value)
                stack += t.children
            else:
                nonterminals.append(t.value)
                stack += t.children
        
        if eliminate_duplicates:
            nonterminals = list(set(nonterminals))
            pos = list(set(pos))
            vocab = list(set(vocab))
        return nonterminals, pos, vocab
    
    def extract_token_pos_mapping(self):
        '''
        Extracts the token -> POS-tag mapping given by the tree
        '''
        
        mapping = []
        stack = [self]
        while len(stack) > 0:
            t = stack.pop()
            if t.is_preterminal():
                pos = t.value
                token = t.children[0].value
                mapping.append((token,pos))
            else:
                stack += t.children[::-1]
        return mapping
    
    def all_rules(self, eliminate_duplicates=True, split_lexicon_grammar=False):
        '''
        Assuming all rules have the form
        - Nonterminal -> [Nonterminal]*
        - Nonterminal -> terminal
        '''
        rules = []
        grammar_rules = []
        lexicon_rules = []
        
        # DFS traversal. Each node with its children represents a rule
        stack = [self]
        while len(stack) > 0:
            t = stack.pop()
            if t.is_terminal():
                continue
            
            left_handside = t.value
            right_handside = [c.value for c in t.children]
            rule = Rule(left_handside, right_handside)
            if all([c.is_terminal() for c in t.children]):
                lexicon_rules.append(rule)
                if len(t.children) > 1:
                    print('Each POS-Tag is only allowed to produce one token')
                assert len(t.children) == 1
            else:
                grammar_rules.append(rule)
                stack += t.children

        # Return rules
        if eliminate_duplicates:
            grammar_rules = list(set(grammar_rules))
            lexicon_rules = list(set(lexicon_rules))
        
        if split_lexicon_grammar:
            return lexicon_rules, grammar_rules
        else:
            rules = grammar_rules + lexicon_rules
            if eliminate_duplicates:
                return list(set(rules))
            return rules
    
    def max_degree(self, exclude_root=True):
        max_degree = len(self.children)
        if exclude_root and self.is_root():
            max_degree = 0
        for c in self.children:
            max_degree = max(max_degree, c.get_max_degree())
        return max_degree
    
    def draw(self):
        dot = Digraph()
        node_count = 1
        dot.node(str(node_count),self.value)
        stack = [(self,1)]

        while len(stack) > 0:
            t, node_id = stack.pop()

            for c in t.children:
                node_count += 1
                c_node_id = node_count
                dot.node(str(c_node_id),c.value)
                dot.edge(str(node_id),str(c_node_id))
                if not c.is_terminal():
                    stack.append((c,c_node_id))
        return dot
        
    def simplify(self):
        '''
        Undoes the binarization and terminal elimination that was
        produced during the grammar normalization
        (unitary rule elimination can not be undone)
        
        Makes the tree easier to read for humans
        '''
        
        if self.is_terminal():
            return
        stack = [self]
        while len(stack) > 0:
            t = stack.pop()
            
            # Undo binarization '&'
            # and terminal elimination '_'
            new_children = []
            collapsed_a_level = False
            for c in t.children:
                if ('&' in c.value) or ('_' == c.value[0]):
                    new_children += c.children
                    for gc in c.children:
                        gc.parent = t
                    collapsed_a_level = True
                else:
                    new_children.append(c)
            t.children = new_children
            
            if collapsed_a_level:
                stack.append(t)
                continue
            else:
                stack += t.children
            
            
                
def simplified_nonterminal(nt):
    '''
    Returns nonterminal modulo functional label
    e.g. PP-MOD -> PP
    '''
    i = nt.find('-')
    if i != -1:
        return nt[:i]
    return nt
        
    
    
def sentence_to_tree(sentence):
    '''
    Transforms a bracket form into a tree structure
        
        Assumes the sentence was constructed by a CFG with rules of the form
        - Nonterminal -> [Nonterminal]*
        - Nonterminal -> terminal
        Assumes sentence starts with '( (SENT ('
    
    Returns the tree
    '''
    
    assert sentence[:9] == '( (SENT ('
    
    delimiter_indices = [m.start() for m in re.finditer('[()]', sentence)]
    
    trees = []
    current_tree = Tree(value='SENT')
    # iterate over all '(' and ')' positions in the sentence
    for i, didx in enumerate(delimiter_indices):
        # skip the '( (SENT ' part
        if i < 2:
            continue
        
        delimiter = sentence[didx]
        # create a new subtree
        if delimiter == '(':
            nonterminal_end = sentence.find(' ',didx)
            nonterminal = sentence[didx+1:nonterminal_end]
            nonterminal = simplified_nonterminal(nonterminal)
            next_tree = Tree(value=nonterminal, parent=current_tree)
            current_tree.add_child(next_tree)
            current_tree = next_tree
        # close the subtree, go up to the parent
        elif delimiter == ')':
            previous_didx = delimiter_indices[i-1]
            previous_delimiter = sentence[previous_didx]
            # Need to generate a terminal
            if previous_delimiter == '(':
                terminal_beginning = sentence.find(' ',previous_didx)+1
                terminal = sentence[terminal_beginning:didx]
                next_tree = Tree(value=terminal, parent=current_tree)
                current_tree.add_child(next_tree)
            # Go one level up
            if not current_tree.is_root():
                current_tree = current_tree.parent
    return current_tree
