_Syntax for MITLM v0.3 and older can be found at EvaluateNgramV01._

# evaluate-ngram #

## NAME ##

`evaluate-ngram` - Evaluates the performance of an _n_-gram language model.

## SYNOPSIS ##

`evaluate-ngram [Options]`

## DESCRIPTION ##

Evaluates the performance of an _n_-gram language model.  It also supports
various _n_-gram language model conversions, including changes in order,
vocabulary, and file format.

Filename argument can be an ASCII file, a compressed file (ending in .Z or .gz),
or '-' to indicate stdin/stdout.

## OPTIONS ##

| `-h`, `-help`                 | Print this message. |
|:------------------------------|:--------------------|
| `-verbose` (1)                | Set verbosity level. |
| `-o`, `-order` (3)            | Set the _n_-gram order of the estimated LM. |
| `-v`, `-vocab`                | Fix the vocab to only words from the specified file. |
| `-l`, `-lm`                   | Load specified LM.  |
| `-cl`, `-compile-lattices`    | [SLS](SLS.md) Compiles lattices into a binary format. |
| `-ep`, `-eval-perp`           | Compute test set perplexity. |
| `-ew`, `-eval-wer`            | Compute test set lattice word error rate. |
| `-em`, `-eval-margin`         | Compute test set lattice margin. |
| `-wb`, `-write-binary`        | Write LM/counts files in binary format. |
| `-wv`, `-write-vocab`         | Write LM vocab to file. |
| `-wl`, `-write-lm`            | Write ARPA backoff LM to file. |

## EXAMPLES ##

Compute the perplexity of an _n_-gram LM on a text file.

> `evaluate-ngram -lm input.lm -eval-perp test.txt`

Convert a trigram model to a bigram LM.

> `evaluate-ngram -lm input.lm -order 2 -write-lm output.lm`

Convert a binary _n_-gram LM to ARPA text format.

> `evaluate-ngram -lm input.blm -write-lm output.lm`