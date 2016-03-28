_Syntax for MITLM v0.3 and older can be found at EstimateNgramV01._

# estimate-ngram #

## NAME ##

`estimate-ngram` - Estimate an _n_-gram language model with various smoothing options.

## SYNOPSIS ##

```
estimate-ngram [Options]
```

## DESCRIPTION ##

Estimates an _n_-gram language model by cumulating _n_-gram count statistics,
smoothing observed counts, and building a backoff _n_-gram model.  Parameters
can be optionally tuned to optimize development set performance.

Filename argument can be an ASCII file, a compressed file (ending in `.Z` or `.gz`),
or '-' to indicate stdin/stdout.

## OPTIONS ##

| `-h`, `-help`                 | Print this message. |
|:------------------------------|:--------------------|
| `-verbose` (1)                | Set verbosity level. |
| `-o`, `-order` (3)            | Set the _n_-gram order of the estimated LM. |
| `-v`, `-vocab`                | Fix the vocab to only words from the specified file. |
| `-u`, `-unk`                  | Replace all out of vocab words with `<unk>`. |
| `-t`, `-text`                 | Add counts from text files. |
| `-c`, `-counts`               | Add counts from counts files. |
| `-s`, `-smoothing` (`ModKN`)  | Specify smoothing algorithms.  (`ML`, `FixKN`, `FixModKN`, `FixKN`_d_, `KN`, `ModKN`, `KN`_d_) |
| `-wf`, `-weight-features`     | Specify _n_-gram weighting features. |
| `-p`, `-params`               | Set initial model params. |
| `-oa`, `-opt-alg` (`Powell`)  | Specify optimization algorithm.  (`Powell`, `LBFGS`, `LBFGSB`) |
| `-op`, `-opt-perp`            | Tune params to minimize dev set perplexity. |
| `-ow`, `-opt-wer`             | Tune params to minimize lattice word error rate. |
| `-om`, `-opt-margin`          | Tune params to minimize lattice margin. |
| `-wb`, `-write-binary`        | Write LM/counts files in binary format. |
| `-wp`, `-write-params`        | Write tuned model params to file. |
| `-wv`, `-write-vocab`         | Write LM vocab to file. |
| `-wc`, `-write-counts`        | Write _n_-gram counts to file. |
| `-wec`, `-write-eff-counts`   | Write effective _n_-gram counts to file. |
| `-wlc`, `-write-left-counts`  | Write left-branching _n_-gram counts to file. |
| `-wrc`, `-write-right-counts` | Write right-branching _n_-gram counts to file. |
| `-wl`, `-write-lm`            | Write ARPA backoff LM to file. |
| `-ep`, `-eval-perp`           | Compute test set perplexity. |
| `-ew`, `-eval-wer`            | Compute test set lattice word error rate. |
| `-em`, `-eval-margin`         | Compute test set lattice margin. |


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

> `estimate-ngram -text input.txt -smoothing FixModKN -write-lm output.lm`

Build a 4-gram model from pre-computed counts using modified Kneser-Ney smoothing, with discount parameters tuned on a development set.

> `estimate-ngram -order 4 -counts input.counts -smoothing ModKN -opt-perp dev.txt -write-lm output.lm`