from tree import *
from pcfg import *
from oov import *
from cyk import *

from time import time
import numpy as np
import pickle
import sys
 
def parse(tokens, pcfg, embedding_idx, embeddings, simplify):
    
    ## OOV and typo handling ##
    preprocessed_tokens = [get_most_similar_words(t,pcfg.vocab, embedding_idx, embeddings) for t in tokens]
    
    # CYK parsing
    start = time()
    tree, prob = CYK(preprocessed_tokens, pcfg)
    
    if tree is not None and simplify:
        tree.simplify()
        
    return tree, prob
    

def initialize():
    '''
    Trains a PCFG on the first 80% of the SEQUOIA database and
    reads in the polyglot word embeddings
    
    returns 
        Trained PCFG
        List of words in the embeddings
        The embeddings
        A dictionary to convert words to embedding index
    '''
    print('Training pcfg')
    try:
        corpus_path = './data/sequoia-corpus+fct.mrg_strict'
        with open(corpus_path,'r') as f:
            lines = [line.strip() for line in f]
    except IOError:
        print('Sequoia corpus not found')
        print('Expected path: '+corpus_path)
        sys.exit()

    try:
        embedding_path = './data/polyglot-fr.pkl'
        words, embeddings = pickle.load(open(embedding_path, 'rb'), encoding='latin1')
        embedding_idx = dict()
        for i, w in enumerate(words):
            embedding_idx[w] = i
    except IOError:
        print('Polyglot not found')
        print('Expected path: '+embedding_path)
        sys.exit()
        
    lines_train = lines[:int(0.8*len(lines))]
    lines_test = lines[-int(0.1*len(lines)):]
    print('Reading sentences')
    trees = [sentence_to_tree(l) for l in lines_train]
    print('Constructing pcfg')
    
    pcfg = create_pcfg(trees)
    print('Done\n')
    
    return pcfg, words, embeddings, embedding_idx
    

def show_case(pcfg, words, embeddings, embedding_idx):
    corpus_path = './data/sequoia-corpus+fct.mrg_strict'
    with open(corpus_path,'r') as f:
            lines = [line.strip() for line in f]
    lines_test = lines[-int(0.1*len(lines)):]
    
    m = 0
    for i,line in enumerate(lines_test):
        gt_tree = sentence_to_tree(lines_test[i])
        tokens = gt_tree.all_terminals(eliminate_duplicates=False)
        m = max(m,len(tokens))
        continue
        if len(tokens) == 4:
            print('i',i)
            print(line)
            parse_tree,_ = parse(tokens, pcfg, embedding_idx, embeddings, simplify=False)
            if parse_tree is not None:
                ground_truth = gt_tree.extract_token_pos_mapping()
                mapping = parse_tree.extract_token_pos_mapping()
                parse_tree.simplify()
                cor = evaluate_mapping(ground_truth, mapping)
                parsed_string = '( '+parse_tree.to_bracket_str()+')'
                gt_tree.draw().render('gt_tree.gv',view=True)
                parse_tree.draw().render('parse_tree.gv',view=True)
                acc = float(cor)/len(tokens)
                print('acc',acc)
                print(stop)
    print(m)
        
    
if __name__ == "__main__":
        
    pcfg, words, embeddings, embedding_idx = initialize()
    
    line = input('Enter space separated tokens:\n')
    tokens = line.split(' ')
    
    t,p = parse(tokens, pcfg, embedding_idx, embeddings, simplify=True)
    
    if t is None:
        print('Could not parse the sentence')
    else:
        print('parsed as:')
        print('( '+t.to_bracket_str()+')')
        t.draw().view()
