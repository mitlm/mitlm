# interpolate-ngram #

## NAME ##

`interpolate-ngram` - Constructs a statically interpolated n-gram language model from component n-gram models.

## SYNOPSIS ##

`interpolate-ngram [Options] [lmfile1 lmfile2 ...]`

## DESCRIPTION ##

Takes multiple n-gram language models, computes appropriate interpolation
weights from optional features, and constructs a statically interpolated
n-gram model.  It also supports perplexity computation and parameter tuning
to optimize the development set perplexity.

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
`-l [ --read-lms ] arg`
> Reads component LMs from specified lmfiles in either ARPA or binary format.
`-f [ --read-features ] arg`
> Reads n-gram feature vectors from specified featurefiles, where each line contains an
> n-gram and its feature value.  Since interpolation weights are computed on features of
> the n-gram history, it is sufficient to have features only up to order n - 1.
`-p [ --read-parameters ] arg`
> Read model parameters from paramfile.
`-i [ --interpolation ] arg (=LI)`
> Apply specified interpolation algorithms to all n-gram orders.  See INTERPOLATION.
`-d [ --optimize-perplexity ] arg`
> Tune the model parameters to minimize the perplexity of dev text.
`-e [ --evaluate-perplexity ] arg`
> Compute the perplexity of textfile.  This option can be repeated.
`-V [ --write-vocab ] arg`
> Write the LM vocabulary to vocabfile.
`-L [ --write-lm ] arg`
> Write the interpolated n-gram language model to lmfile in ARPA backoff text format.
`-B [ --write-binary-lm ] arg`
> Write the interpolated n-gram language model to lmfile in MITLM binary format.
`-P [ --write-parameters ] arg`
> Write model parameters to paramfile.

## INTERPOLATION ##

MITLM currently supports the following interpolation algorithms:

  * `LI` - Interpolates the component models using linear interpolation.  Constructs a model consisting of the union of all observed n-grams, with probabilities equal to the weighted sum of the component n-gram probabilities, using backoff, if necessary.  Backoff weights are computed to normalize the model.  In linear interpolation, the weight of each component is a fixed constant.
  * `CM` - Interpolates the component models using count merging (Bacchiani et al., 2006; Hsu, 2007).  Unlike linear interpolation, the weight of each component depends on the n-gram history count for that component.
  * `GLI` - Interpolates the component models using generalized linear interpolation (Hsu, 2007).  Unlike count merging, the weight of each component depends on arbitrary features of the n-gram history, specified through the `--read-features` option.

## REFERENCES ##

  * M. Bacchiani, M. Riley, B. Roark, and R. Sproat, [MAP adaptation of stochastic grammars](http://dx.doi.org/10.1016/j.csl.2004.12.001), _Computer Speech & Language_, vol. 20, no. 1, pp. 41-68, 2006.
  * B. Hsu, [Generalized linear interpolation of language models](http://people.csail.mit.edu/bohsu/GeneralizedLinearInterpolationOfLanguageModels2007.pdf), in _Proc. ASRU_, 2007.
  * F. Jelinek and R. Mercer, Interpolated estimation of Markov source parameters from sparse data, in _Proc. Workshop on Pattern Recognition in Practice_, 1980.