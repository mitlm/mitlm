# estimate-ngram #

## NAME ##

`estimate-ngram` - Estimate an n-gram language model with various smoothing options.

## SYNOPSIS ##

`estimate-ngram [Options] [textfile lmfile]`

## DESCRIPTION ##

Estimates an n-gram language model by cumulating n-gram count statistics from a
raw text file, applying smoothing to distribute counts from observed n-grams to
unseen ones, and building a backoff n-gram language model from counts.  It also
supports perplexity computation and parameter tuning to optimize the development
set perplexity.

Filename argument can be an ASCII file, a compressed file (ending in .Z or .gz),
or '-' to indicate stdin/stdout.

## OPTIONS ##

`--help`
> Print this message.
`--version`
> Print version number.
`--verbose arg (=1)`
> Set verbosity level.
`-o [ --order ] arg (=3)`
> Set the n-gram order of the estimated LM.
`-v [ --read-vocab ] arg`
> Restrict the vocabulary to only words from the specified vocabulary vocabfile.
> n-grams with out of vocabulary words are ignored.
`-t [ --read-text ] arg`
> Cumulate n-gram count statistics from textfile, where each line corresponds to a
> sentence.  Begin and end of sentence tags (<s>, </s>) are automatically added.  Empty
> lines are ignored.
`-c [ --read-count ] arg`
> Read n-gram counts from countsfile.  Using both -read-text and -read-count will combine
> the counts.
`-p [ --read-parameters ] arg`
> Read model parameters from paramfile.
`-s [ --smoothing ] arg (=ModKN)`
> Apply specified smoothing algorithms to all n-gram orders.  See SMOOTHING.
`-`_N_` [ --smoothing`_N_` ] arg`
> Apply specified smoothing algorithms to n-gram order _N_.  Ex. --smoothing1 KN -2 WB.
> See SMOOTHING.
`-d [ --optimize-perplexity ] arg`
> Tune the model parameters to minimize the perplexity of dev text.
`-e [ --evaluate-perplexity ] arg`
> Compute the perplexity of textfile.  This option can be repeated.
`-V [ --write-vocab ] arg`
> Write the LM vocabulary to vocabfile.
`-C [ --write-count ] arg`
> Write n-gram counts to countsfile.
`-D [ --write-binary-count ] arg`
> Write n-gram counts to countsfile in MITLM binary format.
`-L [ --write-lm ] arg`
> Write n-gram language model to lmfile in ARPA backoff text format.
`-B [ --write-binary-lm ] arg`
> Write n-gram language model to lmfile in MITLM binary format.
`-P [ --write-parameters ] arg`
> Write model parameters to paramfile.

## SMOOTHING ##

MITLM currently supports the following smoothing algorithms:

  * `ML` - Maximum likelihood estimation.
  * `KN` - Original interpolated Kneser-Ney smoothing with default parameters estimated from count of count statistics (Kneser and Ney, 95).
  * `ModKN` - Modified interpolated Kneser-Ney smoothing with default parameters estimated from count of count statistics (Chen and Goodman, 1998).
  * `KN`_d_ - Extended interpolated Kneser-Ney smoothing with _d_ discount parameters per n-gram order.  The default parameters are estimated from count of count statistics.  `KN1` and `KN3` are equivalent to `KN` and `ModKN`, respectively.
  * `FixKN` - `KN` using parameters estimated from count statistics.
  * `FixModKN` - `ModKN` using parameters estimated from count statistics.
  * `FixKN`_d_ - `KN`_d_ using parameters estimated from count statistics.

## EXAMPLES ##

Construct a standard trigram model from a text file using modified Kneser-Ney smoothing with default parameters.

> `estimate-ngram -read-text input.txt -smoothing ModKN -write-lm output.lm`

Build a 4-gram model from pre-computed counts using modified Kneser-Ney smoothing, with discount parameters tuned on a development set.

> `estimate-ngram -read-count input.count -smoothing ModKN -optimize-perplexity dev.txt -write-lm output.lm`

Build a trigram model from text using modified Kneser-Ney smoothing with specific parameters.

> `estimate-ngram -read-text input.txt -smoothing ModKN -read-parameters param.txt -write-lm output.lm`

## REFERENCES ##
  * S. Chen and J. Goodman, [An empirical study of smoothing techniques for language modeling](http://research.microsoft.com/~joshuago/tr-10-98.pdf), in _Technical Report TR-10-98_. Computer Science Group, Harvard University, 1998.
  * R. Kneser and H. Ney, [Improved backing-off for m-gram language modeling](http://ieeexplore.ieee.org/xpls/abs_all.jsp?tp=&arnumber=479394), in _Proc. ICASSP_, 1995.