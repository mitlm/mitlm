# evaluate-ngram #

## NAME ##

`evaluate-ngram` - Evaluates the performance of an n-gram language model.

## SYNOPSIS ##

`evaluate-ngram [Options]`

## DESCRIPTION ##

Evaluates the performance of an n-gram language model.  It also supports various
n-gram language model conversions, including changes in order, vocabulary,
and file format.

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
`-l [ --read-lm ] arg`
> Reads n-gram language model in either ARPA or binary format.
`-e [ --evaluate-perplexity ] arg`
> Compute the perplexity of textfile.  This option can be repeated.
`-V [ --write-vocab ] arg`
> Write the LM vocabulary to vocabfile.
`-L [ --write-lm ] arg`
> Write the interpolated n-gram language model to lmfile in ARPA backoff text format.
`-B [ --write-binary-lm ] arg`
> Write the interpolated n-gram language model to lmfile in MITLM binary format.

## EXAMPLES ##

Compute the perplexity of a n-gram LM on a text file.

> `evaluate-ngram -read-lm input.lm -evaluate-perplexity test.txt`

Convert a trigram model to a bigram LM.

> `evaluate-ngram -read-lm input.lm -order 2 -write-lm output.lm`

Convert a binary n-gram LM to ARPA text format.

> `evaluate-ngram -read-lm input.blm -write-lm output.lm`