from tree import *
from pcfg import *
from oov import *

from itertools import product
from time import time


def cyk_fill_one_cell(cells, subseq_len, start_idx, pcfg):
    '''
    Fills the cell cells[start_idx][subseq_len] as in the CYK algorithm.
    
    That means: it will afterwards contain those symbols A that can be split into
    A --> B C such that there is some number s<subseq_len with
        B in cells[start_idx][s] and C in cells[start_idx+s][subseq_len-s]
        
    Assumes that cells[start_idx][s] for all s < subseq_len are already filled correctly
    '''
    
    best_parents = dict()
    
    # loop splitting possibilities
    for len_left_tree in range(1,max(2,subseq_len)):
        len_right_tree = subseq_len - len_left_tree
        left_cell = cells[start_idx][len_left_tree]
        right_cell = cells[start_idx+len_left_tree][len_right_tree]
        prod_len = len(left_cell)*len(right_cell)
        
        # Find rules (A --> B C) with B in left cell, C in right cell
        
        # Check for runtime optimization
        if prod_len > len(pcfg.grammar_rules):
            # loop over all grammar rules
            for rule in pcfg.grammar_rules:
                if len(rule.right_handside) != 2:
                    continue
                if rule.right_handside[0] in left_cell and rule.right_handside[1] in right_cell:
                    left_tree, left_prob = left_cell[rule.right_handside[0]]
                    right_tree, right_prob = right_cell[rule.right_handside[1]]
                    nonterm = rule.left_handside
                    this_prob = left_prob + right_prob + pcfg.get_grammar_prob(rule)
                    # If new or better parent
                    if (nonterm not in best_parents) or (best_parents[nonterm][1] < this_prob):
                        parent_tree = Tree(value=nonterm, children=[left_tree,right_tree])
                        best_parents[nonterm] = (parent_tree,this_prob)
        else:
            # loop over cartesian product of left and right cell
            for left_symbol, (left_tree, left_prob) in left_cell.items():
                for right_symbol, (right_tree, right_prob) in right_cell.items():
                    for (nonterm, prob) in pcfg.get_possible_parents(left_symbol,right_symbol):
                        this_prob = left_prob + right_prob + prob
                        if (nonterm not in best_parents) or (best_parents[nonterm][1] < this_prob):
                            parent_tree = Tree(value=nonterm, children=[left_tree,right_tree])
                            best_parents[nonterm] = (parent_tree,this_prob)
    cells[start_idx][subseq_len] = best_parents
    
def CYK(tokens, pcfg):
    ## CYK Algorithm ##
        
    ### Create empty CYK 'Matrix'/dictionary ###
    # cell[i][l] will contain trees that can derive words/tokens[i:i+l] (and the corresponding derivation probability)
    cells = dict()
    for start_idx in range(len(tokens)):
        cells[start_idx] = {}
        for length in range(1,len(tokens)-start_idx+1):
            cells[start_idx][length] = []
    
    ### Create POS-row (lowest row of the CYK-Matrix) ###
    # The POS-row contains POS-tags that can create the sentence's tokens
    # This is handled separately since the Pos -> token rules are not binary
    pos_row = []
    for i in range(len(tokens)):
        pos_row.append([])
        # loop POS-tags to create a token/word
        for pos in pcfg.pos:
            best_tree = None
            best_prob = None
            # loop possible word(corrections) at that position
            for token, token_prob in tokens[i]:
                emission_prob = pcfg.get_lexicon_prob(pos,token)
                if emission_prob is not None:
                    tree_prob = token_prob + emission_prob
                    if best_prob is None or best_prob < tree_prob:
                        token_tree = Tree(value=token, parent=None)
                        pos_tree = Tree(value=pos, children=[token_tree])
                        best_tree = pos_tree
                        best_prob = tree_prob
            if best_tree is not None:
                pos_row[i].append((best_tree,best_prob))
    
    ### Dynamic programming, build up the CYK-Matrix ###
    for subseq_len in range(1,len(tokens)+1):
        for start_idx in range(0,len(tokens)-subseq_len+1):
            # Fill cells[start_idx][seq_len]
            best_parents = dict()
            # Bottom row is a special case cuz terminals are created by unit-rules.
            if subseq_len == 1:
                for pos_tree,pos_prob in pos_row[start_idx]:
                    ppp = pcfg.get_possible_pos_parents(pos_tree.value)
                    for (nonterm, prob) in pcfg.get_possible_pos_parents(pos_tree.value):
                        this_prob = pos_prob + prob
                        if (nonterm not in best_parents) or (best_parents[nonterm][1] < this_prob):
                            parent_tree = Tree(value=nonterm, children=[pos_tree])
                            best_parents[nonterm] = (parent_tree,this_prob)
                cells[start_idx][subseq_len] = best_parents
            else:
                cyk_fill_one_cell(cells, subseq_len, start_idx, pcfg)
    
    ### Return most probable tree starting with the start symbol ###
    if pcfg.start_symbol in cells[0][len(tokens)]:
        return cells[0][len(tokens)][pcfg.start_symbol]
    
    # No parse tree found
    return None, None
