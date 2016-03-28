_Syntax for MITLM v0.3 and older can be found at InterpolateNgramV01._

# interpolate-ngram #

## NAME ##

`interpolate-ngram` - Constructs a statically interpolated n-gram language model from component _n_-gram models.

## SYNOPSIS ##

`interpolate-ngram [Options]`

## DESCRIPTION ##

Interpolates multiple _n_-gram models by computing appropriate interpolation
weights from optional features and constructing a statically interpolated
_n_-gram model.  Parameters can be optionally tuned to optimize development set
performance.

Filename argument can be an ASCII file, a compressed file (ending in .Z or .gz),
or '-' to indicate stdin/stdout.

## OPTIONS ##

| `-h`, `-help`                 | Print this message. |
|:------------------------------|:--------------------|
| `-verbose` (1)                | Set verbosity level. |
| `-o`, `-order` (3)            | Set the _n_-gram order of the estimated LM. |
| `-v`, `-vocab`                | Fix the vocab to only words from the specified file. |
| `-u`, `-unk`                  | Replace all out of vocab words with `<unk>`. |
| `-l`, `-lm`                   | Interpolate specified LM files. |
| `-t`, `-text`                 | Interpolate models trained from text files. |
| `-c`, `-counts`               | Interpolate models trained from counts files. |
| `-s`, `-smoothing` (`ModKN`)  | Specify smoothing algorithms.  (`ML`, `FixKN`, `FixModKN`, `FixKN`_d_, `KN`, `ModKN`, `KN`_d_) |
| `-wf`, `-weight-features`     | Specify _n_-gram weighting features. |
| `-i`, `-interpolation` (`LI`) | Specify interpolation mode.  (`LI`, `CM`, `GLI`) |
| `-if`, `-interpolation-features`  | Specify interpolation features. |
| `-tpo`, `-tie-param-order` (true) | Tie parameters across _n_-gram order. |
| `-tpl`, `-tie-param-lm` (false)   | Tie parameters across LM components. |
| `-p`, `-params`               | Set initial model params. |
| `-oa`, `-opt-alg` (`Powell`)  | Specify optimization algorithm.  (`Powell`, `LBFGS`, `LBFGSB`) |
| `-op`, `-opt-perp`            | Tune params to minimize dev set perplexity. |
| `-ow`, `-opt-wer`             | Tune params to minimize lattice word error rate. |
| `-om`, `-opt-margin`          | Tune params to minimize lattice margin. |
| `-wb`, `-write-binary`        | Write LM/counts files in binary format. |
| `-wp`, `-write-params`        | Write tuned model params to file. |
| `-wv`, `-write-vocab`         | Write LM vocab to file. |
| `-wl`, `-write-lm`            | Write ARPA backoff LM to file. |
| `-ep`, `-eval-perp`           | Compute test set perplexity. |
| `-ew`, `-eval-wer`            | Compute test set lattice word error rate. |
| `-em`, `-eval-margin`         | Compute test set lattice margin. |


## INTERPOLATION ##

MITLM currently supports the following interpolation algorithms:

  * `LI` - Interpolates the component models using linear interpolation.  Constructs a model consisting of the union of all observed _n_-grams, with probabilities equal to the weighted sum of the component _n_-gram probabilities, using backoff, if necessary.  Backoff weights are computed to normalize the model.  In linear interpolation, the weight of each component is a fixed constant.
  * `CM` - Interpolates the component models using count merging (Bacchiani et al., 2006; Hsu, 2007).  Unlike linear interpolation, the weight of each component depends on the _n_-gram history count for that component.
  * `GLI` - Interpolates the component models using generalized linear interpolation (Hsu, 2007).  Unlike count merging, the weight of each component depends on arbitrary features of the _n_-gram history, specified through the `-interpolation-features` option.

## REFERENCES ##

  * M. Bacchiani, M. Riley, B. Roark, and R. Sproat, [MAP adaptation of stochastic grammars](http://dx.doi.org/10.1016/j.csl.2004.12.001), _Computer Speech & Language_, vol. 20, no. 1, pp. 41-68, 2006.
  * B. Hsu, [Generalized linear interpolation of language models](http://people.csail.mit.edu/bohsu/GeneralizedLinearInterpolationOfLanguageModels2007.pdf), in _Proc. ASRU_, 2007.
  * F. Jelinek and R. Mercer, Interpolated estimation of Markov source parameters from sparse data, in _Proc. Workshop on Pattern Recognition in Practice_, 1980.