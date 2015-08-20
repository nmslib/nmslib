This directory contains the sample script [sample_run.sh](sample_run.sh) as well as scripts to re-produce most of our results. To run the sample script without drawing plots type:
```
./sample_run.sh 0
```
To run sample experiments **AND** plot performance plots, type:
```
./sample_run.sh 1
```
Note that plots generating software requires Python, Latex, and PGF.

Scripts to reproduce our previous results are in subdirectories [sisap2013](sisap2013), [nips2013](nips2013), [da2014](smallworld_Apr2014), and [vldb2015](vldb2015).
Notes on reproducibility:
* All scripts except for **vldb2015** should use binaries from [the previous release](https://github.com/searchivarius/NonMetricSpaceLib/releases/tag/v1.0).
* The current release contain implementations of proximity graphs that are actually more efficient than those used for vldb2015 paper. 

In the case of light-weight distance, e.g., L2, the new implementations are about 3 times as efficient as the old ones. However,
in the case of expensive distances the difference is much less. In particular, in the case of the normalized Levenshtein distance, the improvement is only 30%. Nevertheless, the old versions are still present under the names ``small_world_rand_old`` and ``nndes_old``.

However, the provided vldb2015 scripts will use **newer versions by default**, because it does not affect the big picture and conclusions
of the paper.
