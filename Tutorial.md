# MIT Language Modeling Toolkit Tutorial #

The MIT Language Modeling (MITLM) toolkit is a set of tools designed for the efficient estimation of statistical _n_-gram language models involving iterative parameter estimation. In addition to standard language modeling estimation, MITLM support the tuning of smoothing, interpolation, and _n_-gram weighting parameters. In this tutorial, we will describe how to build and tune language models using MITLM. Please refer to the references for additional details on the language modeling techniques presented below.

### Installation ###

To install MITLM, download the latest stable release from http://www.sls.csail.mit.edu/mitlm. Alternatively, you can download the latest source code from the SVN repository.

```
svn checkout http://mitlm.googlecode.com/svn/trunk/ mitlm
```

To build the toolkit, run:

```
cd mitlm
make -j
```

## _N_-gram Language Modeling ##

A statistical _n_-gram language model describes the conditional probability of a word _w_ given its history, described by the _n_ - 1 previous words, _h_. It is typically estimated from a large text corpus. In this tutorial, we will train a language model for lecture transcription from the corresponding course textbook (`Textbook.txt`) and transcripts of generic lectures (`Lectures.txt`). The evaluation data consists of 20 lectures from an introductory computer science class, where we allocate the first 10 as the development set (`CS-Dev.txt`) and use the remaining 10 lectures for the test set (`CS-Test.txt`).

### Estimation & Evaluation ###

We can estimate an _n_-gram language model using EstimateNgram. To gather _n_-gram statistics from the text corpus `Lectures.txt`, estimate the probabilities and backoff weights of the _n_-gram language model, and output the result to `Lectures.lm` in the ARPA backoff _n_-gram format, we run:

```
estimate-ngram -text Lectures.txt -write-lm Lectures.lm
```

In addition to the long argument names, MITLM also provides shorter names consisting of the first letters of each word in the long argument name. Thus, we can also use the following syntax:

```
estimate-ngram -t Lectures.txt -wl Lectures.lm
```

By default, MITLM trains a trigram model (_n_ = 3). We can also explicit specify the order of the _n_-gram model using the `-order` argument.

```
estimate-ngram -order 4 -t Lectures.txt -wl Lectures.4.lm
```

To evaluate the performance of the resulting language model, we can specify the `-eval-perp` argument and compute the perplexity of the specified evaluation set.

```
estimate-ngram -t Lectures.txt -wl Lectures.lm -eval-perp CS-Dev.txt
```

Alternatively, we can use EvaluateNgram to compute the model perplexity.

```
evaluate-ngram -lm Lectures.lm -eval-perp CS-Dev.txt
```

### Count Features ###

We can compute various features from the _n_-gram statistics, such as counts and write them to a file.

```
estimate-ngram -t Lectures.txt \
  -write-counts Lectures.counts \
  -write-eff-counts Lectures.effcounts \
  -write-left-counts Lectures.lcounts \
  -write-right-counts Lectures.rcounts

estimate-ngram -t Textbook.txt \
  -wc Textbook.counts \
  -wec Textbook.effcounts \
  -wlc Textbook.lcounts \
  -wrc Textbook.rcounts
```

We can also train a language model from counts instead of text.

```
estimate-ngram -counts Textbook.counts -wl Textbook.lm
```

### Vocabulary ###

We can output the vocabulary observed in a corpus using the `-write-vocab` argument. Note that we can specify multiple files to the `-text` argument to accumulate _n_-gram statistics from multiple text files.

```
estimate-ngram -t "CS-Dev.txt, CS-Test.txt" -write-vocab CS.vocab
```

By default, MITLM estimates a language model using all the observed vocabulary. In some cases, we may want to estimate the language model over a specific fixed vocabulary. _N_-grams in the corpus that contain an out-of-vocabulary word are ignored. The _n_-gram probabilities are smoothed over all the words in the vocabulary even if they are not observed.

```
estimate-ngram -vocab CS.vocab -t Lectures.txt -wl Lectures.CS.lm
```

Sometimes, we would like to explicitly model the probability of out-of-vocabulary words by introducing a special `<unk>` word into the vocabulary. Out-of-vocabulary words in the corpus are effectively replaced with this special `<unk>` token before _n_-grams counts are cumulated. With this option, we can estimate the transition probabilities of _n_-grams involving out-of-vocabulary words.

```
estimate-ngram -v CS.vocab -unk true -t Lectures.txt -wl Lectures.CS.unk.lm
```

### Smoothing ###

By default, MITLM uses modified Kneser-Ney smoothing (`ModKN`). We can specify different smoothing algorithms using the `-smoothing` argument.

```
estimate-ngram -t Lectures.txt -smoothing FixKN -wl Lectures.FixKN.lm
```

We can also specify the smoothing algorithm for each _n_-gram order individually using a comma-delimited list. In the following example 1-grams and 3-grams are smoothed using `FixKN`, while 2-grams are smoothed using `FixModKN`.

```
estimate-ngram -t Lectures.txt -s "FixKN,FixModKN,FixKN" \
  -wl Lectures.MixedSmoothing.lm
```

Currently, MITLM supports the following smoothing algorithms:
  * `ML` - Maximum likelihood. No smoothing is performed.
  * `FixKN`, `FixModKN`, `FixKN`_n_ - Kneser-Ney smoothing with fixed discount parameters estimated from count statistics. We can extend Kneser-Ney smoothing to arbitrary number of discount parameters using the `FixKN`_n_ syntax. Thus, `FixKN` = `FixKN1` and `FixModKN`= `FixKN3`.
  * `KN`, `ModKN`, `KN`_n_ - Equivalent Kneser-Ney smoothing with tunable discount parameters.

### Optimization ###

Some model configurations, such as the default `ModKN` smoothing, have tunable parameters. In these cases, we can tune the model parameters using numerical optimization to minimize the perplexity of a development set.

```
estimate-ngram -t Lectures.txt -opt-perp CS-Dev.txt -wl Lectures.opt.lm
```

By default, Powell's method is used for numerical optimization. However, depending on the model configuration, other optimization techniques may yield faster convergence.

```
estimate-ngram -t Lectures.txt -op CS-Dev.txt -opt-alg LBFGSB \
  -wl Lectures.opt-LBFGSB.lm
```

The following lists the currently supported optimization algorithms:
  * `Powell` - Powell's method
  * `LBFGS` - Limited-memory BFGS
  * `LBFGSB` - Limited-memory BFGS with bounded parameters.

### Interpolation ###

In scenarios where we have multiple partially-matched training corpora, we can build individual language models from each corpus and combine the resulting _n_-gram models using linear interpolation using InterpolateNgram. Linear interpolation introduces the component model weights as tunable parameters. We can tune the parameters to minimize the development set perplexity.

```
interpolate-ngram -lm "Lectures.lm, Textbook.lm" -interpolation LI \
  -op CS-Dev.txt -wl Lectures+Textbook.LI.lm
```

### Joint Optimization ###

Interpolated language models can also be trained directly from training text corpora. In this configuration, individual language models are estimated from the specified training corpora and smoothed using the specified smoothing algorithm. For smoothing algorithms that introduce tunable parameters, MITLM will tune the smoothing and interpolation parameters jointly.

```
interpolate-ngram -text "Lectures.txt, Textbook.txt" -smoothing KN -i LI \
  -op CS-Dev.txt -wl Lectures+Textbook.KN.LI.lm
```

### Advanced Interpolation ###

In addition to linear interpolation, MITLM also supports more sophisticated interpolation techniques such as count merging and generalized linear interpolation, requiring additional features to be specified (see Features). For count merging, the features default to the effective counts of each component.

```
interpolate-ngram -l "Lectures.lm, Textbook.lm" -i CM -op CS-Dev.txt \
  -wl Lectures+Textbook.CM.lm

interpolate-ngram -l "Lectures.lm, Textbook.lm" -i CM \
  -interpolation-features "log:sumhist:%s.effcounts" -op CS-Dev.txt \
  -wl Lectures+Textbook.CM.lm
```

Generalized linear interpolation subsumes both linear interpolation and count merging by computing the interpolation weights from multiple, arbitrary features over the _n_-gram histories.

```
interpolate-ngram -l "Lectures.lm, Textbook.lm" -i GLI \
  -if "log:sumhist:%s.effcounts" -op CS-Dev.txt \
  -wl Lectures+Textbook.GLI.lm
```

We can also specify multiple and independent interpolation features for each component. For example, the following configuration applies log count and squared log count features to `Lectures` and only log count features to `Textbook`.

```
interpolate-ngram -l "Lectures.lm, Textbook.lm" -i GLI \
  -if "log:sumhist:%s.effcounts,pow2:log1p:sumhist:%s.effcounts; log:sumhist:%s.counts" \
  -op CS-Dev.txt -wl Lectures+Textbook.GLI.MixedFeatures.lm
```

By default, the parameters for generalized linear interpolation are tied across parameter order, but independent across mixture components. We can modify the default behavior using the arguments `-tie-param-order` and `-tie-param-lm`.

```
interpolate-ngram -l "Lectures.lm, Textbook.lm" -i GLI \
  -if "log:sumhist:%s.effcounts" -op CS-Dev.txt \
  -tie-param-order 0 -tie-param-lm 1 -wl Lectures+Textbook.GLI.tpl.lm
```

### _N_-gram Weighting ###

To reduce the effect out-of-domain _n_-grams have on the resulting language model, we can apply _n_-gram weighting to the LM estimation. _N_-gram weighting computes the weights for each observed _n_-gram from potentially multiple features over the _n_-grams.

```
estimate-ngram -t Lectures.txt -s FixModKN \
  -weight-features "entropy:%s.txt" -op CS-Dev.txt -wl Lectures.weight.lm

interpolate-ngram -t "Lectures.txt, Textbook.txt" -s FixModKN \
  -wf "entropy:%s.txt " -op CS-Dev.txt -wl Lectures+Textbook.weight.lm
```

### Features ###

Features for both generalized linear interpolation and _n_-gram weighting can be specified using the same string format. The format is roughly as follows:


> LM<sub>1</sub>Feat<sub>1</sub>,...,LM<sub>1</sub>Feat<sub>_N_1</sub>; LM_<sub>2</sub>Feat<sub>1</sub>,...,LM<sub>2</sub>Feat<sub>_N_2</sub>; ... LM_<sub><i>L</i></sub>Feat<sub><i>NL</i></sub>


If features are only specified for one LM component, all remaining LM components will share the same features. Features can be parameterized by the basename of the LM component using `%s`. For example, when interpolating `Lectures.lm` and `Textbook.lm`, the following feature strings are equivalent:

```
entropy:%s.txt
entropy:%s.txt; entropy:%s.txt
entropy:Lectures.txt; entropy:Textbook.txt
```

We specify each individual feature LM<sub><i>l</i></sub>Feat<sub><i>f</i></sub> as a string. In general, we can load features from any _n_-gram format text file by simply specifying the file. We can also compute various features from the segmented text corpus using the appropriate prefix (`freq:%s.txt`, `entropy:%s.txt`). Finally, we can apply various functions to the features by applying various function prefixes. Unobserved _n_-grams are assigned the value 0. The following is a list of the currently supported function prefixes:
  * `log` - log(_f_(_hw_))
  * `log1p` - log(_f_(_hw_)+1)
  * `pow2` - (_f_(_hw_))<sup>2</sup>
  * `pow3` - (_f_(_hw_))<sup>3</sup>
  * `sumhist` - SUM<sub>w</sub>_f_(_hw_)

### Binary I/O ###

While text file formats are useful as cross-platform representations, they are less efficient than binary representations. Instead of writing language models in ARPA backoff _n_-gram text format, we can also output the language model in more compact binary format for faster loading/saving.

```
estimate-ngram -t Lectures.txt -write-binary true -write-lm Lectures.blm
```

Binary files can be used as input in place of text file equivalent everywhere.

```
evaluate-ngram -lm Lectures.blm -ep CS-Dev.txt
```

Binary file representations are also available for counts.

## SLS Usage ##

The MITLM toolkit can be found at `/usr/sls/mitlm/current/`. This section describes SLS-specific usages of the toolkit that are not yet ready for public consumption. SLS-specific scripts can be found at `/usr/sls/mitlm/scripts/`.

### Recognition ###

In this section, we will build a SUMMIT recognizer from an ARPA backoff _n_-gram model and perform recognition over a lecture.

We start with a basic recognizer configuration for lecture transcription.

```
cp -r /usr/sls/mitlm/recognizer lecture_recognizer
cd lecture_recognizer
```

After initializing the environment (in `bash`), we build the recognition FSTs from the language model.

```
export SLS_HOME=/usr/sls/Galaxy/debcurrent/sls
source /usr/local/initializations/sls.bashrc
export SRILM=/usr/local/srilm
export PATH=$SRILM/bin:$SRILM/bin/i686:/usr/sls/mitlm/sctk-2.2.2/bin:/usr/sls/mitlm/current:/usr/sls/mitlm/scripts:$PATH

cp ../Lectures+Textbook.lm lectures.lm
build_recognizer_from_lm.cmd lectures
```

We perform distributed speech recognition by creating a `.espec` file and running `asr_batch`.

```
echo "corpus: /s/lectures/corpus/lectures.corpus" > lectures.espec
echo "set: <6.001-1986-L01>" >> lectures.espec
echo "line: -tag <tag>; -waveform_in ; -ref_words {<orthography>};" >> lectures.espec
mkdir results
asr_batch -sctk -basename results/Lectures+Textbook
```

### Lattice Rescoring ###

When evaluating multiple language models, it is often more efficient to perform lattice rescoring to compute the word error rate instead of re-running speech recognition over the entire dataset. We can significantly speed up the evaluation by computing the recognition lattices, removing the language model weights, and compiling the lattices into an efficient binary form.

```
build_lattices.cmd lectures 6.001-1986-L01 results/Lectures+Textbook
```

The above script performs speech recognition using the lectures models on the `6.001-1986-L01` dataset and compiles the resulting lattices to `results/Lectures+Textbook.lattices.bin`. Using this lattice, we can quickly evaluate the performance of other language models with the same _n_-gram structure as follows:

```
evaluate-ngram -lm ../Lectures+Textbook.GLI.lm \
  -eval-wer results/Lectures+textbook.lattices.bin
```

## References ##
  * Bo-June (Paul) Hsu. Language Modeling in Limited-Data Domains. _PhD thesis_, Massachusetts Institute of Technology, 2009.
  * Bo-June (Paul) Hsu and James Glass. [Iterative language model estimation: Efficient data structure & algorithms](http://people.csail.mit.edu/bohsu/IterativeLanguageModelEstimation2008.pdf). In _Proc. Interspeech_, Brisbane, Australia, 2008.
  * Bo-June (Paul) Hsu. [Generalized linear interpolation of language models](http://people.csail.mit.edu/bohsu/GeneralizedLinearInterpolationOfLanguageModels2007.pdf). In _Proc. ASRU_, Kyoto, Japan, 2007.
  * Bo-June (Paul) Hsu and James Glass. [N-gram weighting: Reducing training data mismatch in cross-domain language model estimation](http://people.csail.mit.edu/bohsu/NgramWeighting2008.pdf). In _Proc. EMNLP_, Honolulu, Hawaii, USA, 2008.
  * Bo-June (Paul) Hsu and James Glass. [Language model parameter estimation using user transcriptions](http://people.csail.mit.edu/bohsu/ParameterEstimation.pdf). In _Proc. ICASSP_, Taipei, Taiwan, 2009.