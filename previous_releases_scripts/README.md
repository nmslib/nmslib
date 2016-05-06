Scripts to reproduce our previous results are in subdirectories [sisap2013](sisap2013), [nips2013](nips2013), [da2014](smallworld_Apr2014), and [vldb2015](vldb2015).
Notes on reproducibility:
* All scripts except for **vldb2015** should use binaries  [version 1.0](https://github.com/searchivarius/NonMetricSpaceLib/releases/tag/v1.0).
* **vldb2015** should use use binaries [version 1.1](https://github.com/searchivarius/NonMetricSpaceLib/releases/tag/v1.1). 

Note that release 1.1 contains implementations of proximity graphs that are actually more efficient than those used for vldb2015 paper. In the case of light-weight distance, e.g., L2, the new implementations are about 3 times as efficient as the old ones. 

However, in the case of expensive distances the difference is much less. In particular, in the case of the normalized Levenshtein distance, the improvement is only 30%. Nevertheless, the old versions are still present under the names ``small_world_rand_old`` and ``nndes_old``.

The provided vldb2015 scripts will use **newer versions by default**, because it does not affect the big picture and conclusions of the paper.
