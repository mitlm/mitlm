The MIT Language Modeling (MITLM) toolkit is a set of tools designed for the efficient estimation of statistical _n_-gram language models involving iterative parameter estimation.  It achieves much of its efficiency through the use of a compact vector representation of _n_-grams. Details of the data structure and associated algorithms can be found in the following paper.

  * Bo-June (Paul) Hsu and James Glass.  [Iterative Language Model Estimation: Efficient Data Structure & Algorithms](http://people.csail.mit.edu/bohsu/IterativeLanguageModelEstimation2008.pdf).  In Proc. Interspeech, 2008.

Currently, MITLM supports the following features:

  * Smoothing: Modified Kneser-Ney, Kneser-Ney, maximum likelihood
  * Interpolation: Linear interpolation, count merging, generalized linear interpolation
  * Evaluation: Perplexity
  * File formats: ARPA, binary, gzip, bz2

MITLM is available for [download](http://code.google.com/p/mitlm/downloads/list) under the MIT License. It has been built and tested on 32-bit and 64-bit Intel CPUs running Debian Linux 4.0. It currently requires the following:

  * ANSI C++/Fortran compiler (GCC 4.1.2+)

### Acknowledgments ###

The design and implementation of this toolkit benefited significantly from the [SRI Language Modeling Toolkit](http://www.speech.sri.com/projects/srilm/) by Andreas Stolcke.  The project is supported in part by the T-Party Project, a joint research program between MIT CSAIL and Quanta Computer Inc.

©2009 [Bo-June (Paul) Hsu](http://people.csail.mit.edu/bohsu/), [Computer Science and Artificial Intelligence Laboratory](http://www.csail.mit.edu/index.php), [Massachusetts Institute of Technology](http://web.mit.edu).