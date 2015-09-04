#!/bin/bash

DF=dist_corr
mkdir -p $DF
cd $DF

SampleQty=200
SampleQueryKNN=200
MaxNumData=1000000
#MaxNumData=50000
knn=1000
#repeatQty=2
repeatQty=10

# The function creates the PDF first
# Then, it converts the PDF to PS and back to ensure that fonts are embedded.
# The problem is the Latex function pdf() doesn't embed fonts.
function plot_and_embed {
  inp="$1"
  outp="$2"
  PLOT_SCRIPT=../print_proj_plot2.R
  $PLOT_SCRIPT "$inp" tmp_ttt.pdf
  pdftops tmp_ttt.pdf
  ps2pdf14 -dPDFSETTINGS=/prepress tmp_ttt.ps "$outp"
}


DimTrunc=64

RUN_BENCH_PROJ=0

if [ "$RUN_BENCH_PROJ" = "1" ] ; then
  DataDir=$DATA_DIR
  if [ "$DataDir" = "" ] ; then
    echo "Define the environemnt variable DATA_DIR, which contains data files."
    exit 1
  fi
  BinDir=$BIN_DIR
  if [ "$BinDir" = "" ] ; then
    echo "Define the environemnt variable BIN_DIR, which contains binary files."
    exit 1
  fi
fi

for sd in 8 128 ; do
  for ProjDim in 64 ; do
    for type in perm ; do
      if [ "$RUN_BENCH_PROJ" = "1" ] ; then
        $BinDir/bench_projection -s jsdivfast --projSpaceType l2 --distType float -i $DATA_DIR/wikipedia_lda${sd}.txt  -o wikipedia_lda${sd}_jsdiv_${type}${ProjDim}_corr.tsv -p ${type}  --projDim $ProjDim --maxNumData $MaxNumData  --sampleRandPairQty $SampleQty --sampleKNNQueryQty $SampleQueryKNN --sampleKNNTotalQty $SampleQty --knn $knn --repeat $repeatQty --binThreshold $DimTrunc
      fi

      plot_and_embed wikipedia_lda${sd}_jsdiv_${type}${ProjDim}_corr.tsv wikipedia_lda${sd}_jsdiv_${type}${ProjDim}_corr.pdf
    done
  done

  for ProjDim in 64 ; do
    for type in perm ; do
      if [ "$RUN_BENCH_PROJ" = "1" ] ; then
        $BinDir/bench_projection -s kldivgenfast --projSpaceType l2 --distType float -i $DATA_DIR/wikipedia_lda${sd}.txt  -o wikipedia_lda${sd}_kldiv_${type}${ProjDim}_corr.tsv -p ${type}  --projDim $ProjDim --maxNumData $MaxNumData  --sampleRandPairQty $SampleQty --sampleKNNQueryQty $SampleQueryKNN --sampleKNNTotalQty $SampleQty --knn $knn --repeat $repeatQty --binThreshold $DimTrunc
      fi

      plot_and_embed wikipedia_lda${sd}_kldiv_${type}${ProjDim}_corr.tsv wikipedia_lda${sd}_kldiv_${type}${ProjDim}_corr.pdf
    done
  done
done

for ProjDim in 64 ; do
  for type in rand perm ; do
    if [ "$RUN_BENCH_PROJ" = "1" ] ; then
      $BinDir/bench_projection -s l2 --projSpaceType l2 --distType float -i $DATA_DIR/sift_texmex_learn5m.txt -o sift_l2_${type}${ProjDim}_corr.tsv -p ${type}  --projDim $ProjDim --maxNumData $MaxNumData  --sampleRandPairQty $SampleQty --sampleKNNQueryQty $SampleQueryKNN --sampleKNNTotalQty $SampleQty --knn $knn --repeat $repeatQty --binThreshold $DimTrunc
    fi

    plot_and_embed sift_l2_${type}${ProjDim}_corr.tsv sift_l2_${type}${ProjDim}_corr.pdf
  done
done

for ProjDim in 64 ; do
  for type in perm ; do
    if [ "$RUN_BENCH_PROJ" = "1" ] ; then
      $BinDir/bench_projection -s normleven --projSpaceType l2 --distType float -i $DATA_DIR/dna5M_32_4.txt  -o dna_32_4_normleven_${type}${ProjDim}_corr.tsv -p ${type}  --projDim $ProjDim --maxNumData $MaxNumData  --sampleRandPairQty $SampleQty --sampleKNNQueryQty $SampleQueryKNN --sampleKNNTotalQty $SampleQty --knn $knn --repeat $repeatQty --binThreshold $DimTrunc
    fi

    plot_and_embed dna_32_4_normleven_${type}${ProjDim}_corr.tsv dna_32_4_normleven_${type}${ProjDim}_corr.pdf
  done
done

for ProjDim in 64 ; do
  for type in perm ; do
    if [ "$RUN_BENCH_PROJ" = "1" ] ; then
      $BinDir/bench_projection -s cosinesimil_sparse_fast --projSpaceType l2 --distType float -i $DATA_DIR/wikipedia.txt -o cosinesimil_wiki_${type}${ProjDim}_corr.tsv -p ${type}  --projDim $ProjDim --maxNumData $MaxNumData  --sampleRandPairQty $SampleQty --sampleKNNQueryQty $SampleQueryKNN --sampleKNNTotalQty $SampleQty --knn $knn --repeat $repeatQty --binThreshold $DimTrunc
    fi

    plot_and_embed cosinesimil_wiki_${type}${ProjDim}_corr.tsv cosinesimil_wiki_${type}${ProjDim}_corr.pdf
  done
done

for ProjDim in 64 ; do
  for type in rand ; do
    if [ "$RUN_BENCH_PROJ" = "1" ] ; then
      $BinDir/bench_projection -s cosinesimil_sparse_fast --projSpaceType cosinesimil --distType float -i $DATA_DIR/wikipedia.txt -o cosinesimil_wiki_${type}${ProjDim}_corr.tsv -p ${type}  --projDim $ProjDim --maxNumData $MaxNumData  --sampleRandPairQty $SampleQty --sampleKNNQueryQty $SampleQueryKNN --sampleKNNTotalQty $SampleQty --knn $knn --repeat $repeatQty --binThreshold $DimTrunc --intermDim 4096
    fi

    plot_and_embed cosinesimil_wiki_${type}${ProjDim}_corr.tsv cosinesimil_wiki_${type}${ProjDim}_corr.pdf
  done
done
