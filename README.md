# LanguageParsing
In this project I developed a probabilistic parser for french (or any other) language that is robust to unknown words
based on a PCFG, the Levenshtein distance, word embeddings and a probabilistic CYK-algorithm. The
parser is trained on the SEQUOIA database, reads in a sentence and outputs a predicted parse tree.

In an evaluation on the test set of the SEQUOIA database the system achieved 87.7% measured as POS-tag label accuracy.

## How to run:
    1. Start main.py
    2. Wait a few seconds to learn the PCFG.
    2. Enter a new sentence to be parsed.

## Examples:

1.
```
python ./code/main.py
Enter space separated tokens:
Football en Italiee
parsed as:
( (SENT (NC Football) (PP (P en) (NP (NPP Italie)))))
```
Predicted parse tree:


2.
Input sentence: Financement illégal du RPR
Ground truth:
Prediction:


3.
Input sentence: Charles Pasqua accuse même Jospin d'avoir créé un " cabinet noir " , voué à déstabiliser la droite 
Ground truth:
Prediction:

