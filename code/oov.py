import numpy as np

def get_levenshtein_close_words(vocab, word):
    '''
    Returns all words from the vocabulary that have a Damerau-levenshtein distance < 3
    '''
    close_words = []
    word = list(word)
    word_chars = set(word)
    
    for v in vocab:
        v = list(v)
        # fast criteria to discard most of the words
        if abs(len(word)-len(v)) > 2:
            continue
        if len(word_chars.symmetric_difference(v)) > 4:
            continue
        
        # costly levenshtein computation
        dist = dl_distance(word,v)
        if dist < 3:
            close_words.append((''.join(v),dist))
    return close_words


def dl_distance(a,b):
    '''
    Damerau-levenshtein distance a.k.a. Optimal string alignment distance.
    '''
    
    a = list(a)
    b = list(b)
    dist = np.zeros((len(a)+1,len(b)+1),dtype=int)
    
    for i in range(len(a)+1):
        dist[i, 0] = i
    for j in range(len(b)+1):
        dist[0, j] = j
    
    for i in range(1,len(a)+1):
        for j in range(1,len(b)+1):
            if a[i-1] == b[j-1]:
                substitution_cost = 0
            else:
                substitution_cost = 1
            dist[i, j] = np.min([dist[i-1, j] + 1,     # deletion
                               dist[i, j-1] + 1,     # insertion
                               dist[i-1, j-1] + substitution_cost])  # substitution
            if (i > 1) and (j > 1) and (a[i-1] == b[j-1-1]) and (a[i-1-1] == b[j-1]):
                dist[i, j] = min(dist[i, j], dist[i-2, j-2] + 1)  # transposition
                    
    return dist[len(a), len(b)]


def cosine_sim(a,b):
    return abs(np.sum((a/np.linalg.norm(a))*(b/np.linalg.norm(b))))


def get_most_similar_words(word, lexicon_vocab, embedding_idx, embeddings):
    '''
    Defines the OOV and typo handling module
    For a word it returns a list of possible correction attempts
    '''
    p = 0.0 #log(1.0) probability
    
    # Word is known (easy case)
    if word in lexicon_vocab:
        return [(word,p)]
    
    # Find close levenshtein words
    levenshtein_close_words = get_levenshtein_close_words(lexicon_vocab, word)
    #levenshtein_close_words = sorted(levenshtein_close_words, key=lambda x:x[1])
    if len(levenshtein_close_words) > 0:
        sim_words = []
        for sim_w, dist in levenshtein_close_words:
            sim_words.append((sim_w,p))
        return sim_words

    # Try to use words from the embeddings (closest three)
    if word in embedding_idx:
        word_emb = embeddings[embedding_idx[word]]
        closest_words = []
        closest_cos_sims = []
        for lex_word in lexicon_vocab:
            if lex_word in embedding_idx:
                lex_word_emb = embeddings[embedding_idx[lex_word]]
                cos_sim = cosine_sim(word_emb,lex_word_emb)
                closest_words.append((lex_word,p))
                closest_cos_sims.append(cos_sim)
                if len(closest_words) > 3:
                    i = np.argmin(closest_cos_sims)
                    del closest_words[i]
                    del closest_cos_sims[i]
        return closest_words

    # Completely unknown word -> special token '<UNK>'
    return [('<UNK>',p)]
