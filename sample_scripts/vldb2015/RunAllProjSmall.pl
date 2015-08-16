#!/usr/bin/perl -w
require "./RunCommon.pm";

my $DataDir=$ENV{"DATA_DIR"};
defined($DataDir) or die("Define the environemnt variable DATA_DIR, which contains data files.");
my $CacheDir=$ENV{"CACHE_DIR"};
defined($CacheDir) or die("Define the environemnt variable CACHE_DIR, which contains gold standard cache files.");
-d $DataDir or die("DATA_DIR='$DataDir' is not a directory");
my $BinDir=$ENV{"BIN_DIR"};
defined($BinDir) or die("Define the environemnt variable BIN_DIR, which contains binary files.");
-d $BinDir or die("BIN_DIR='$BinDir' is not a directory");
-f "$BinDir/experiment" or die("Missing file '$BinDir/experiment'");


my @ProjType  = (
                ["rand", "perm", "randrefpt", "fastmap"],
                ["rand", "perm", "randrefpt", "fastmap"],
                ["rand", "perm"],
                ["rand", "perm", "randrefpt", "fastmap"],
                ["perm"],
                ["perm"],
                ["perm"],
                ["perm"],
                ); 
my @IntermDim   = (0, 0, 4096, 0, 0, 0, 0, 0);
my @UseCosine   = (0, 0, 1, 0, 0, 0, 0, 0);

my @ProjDim   = (
                 [8, 16, 32, 64, 128, 256, 512, 1024],
                 [8, 16, 32, 64, 128, 256, 512, 1024],
                 [4, 16, 64, 128, 256, 512, 1024],
                 [4, 16, 64, 128, 256, 512, 1024],
                 [4, 16, 64, 128, 256, 512, 1024],
                 [4, 16, 64, 128, 256, 512, 1024],
                 [4, 16, 64, 128, 256, 512, 1024],
                 [4, 16, 64, 128, 256, 512, 1024],
                );

my @knnAmp    = (
                 [10, 40, 60, 100, 400, 600, 1000, 4000],
                 [10, 40, 60, 100, 400, 600, 1000, 4000],
                 [10, 40, 60, 100, 400, 600, 1000, 4000],
                 [10, 40, 60, 100, 400, 600, 1000, 4000],
                 [10, 40, 60, 100, 400, 600, 1000, 4000],
                 [10, 40, 60, 100, 400, 600, 1000, 4000],
                 [10, 40, 60, 100, 400, 600, 1000, 4000],
                 [10, 40, 60, 100, 400, 600, 1000, 4000],
                );

my @SpaceType = ("l2",
                 "l2",
                 "cosinesimil_sparse_fast",
                 "kldivgenfast",
                 "normleven",
                 "sqfd_heuristic_func:alpha=1",
                 "kldivgenfast",
                 "jsdivfast",
                );

my @DataSetName       = ("cophir",            
                         "sift",
                         "wikipedia",
                         "wikipedia_lda128",
                         "dna32_4",
                         "sqfd20_10k",
                         "wikipedia_lda8",
                         "wikipedia_lda128"
                        );
                                      
my @DataSetFile       = ("cophir/full.txt",   
                         "sift_texmex_learn5m.txt",
                         "wikipedia.txt",
                         "wikipedia_lda128.txt",
                         "StringSpaces/dna5M_32_4.txt",
                         "SQFD/in_20_10k.txt",
                         "wikipedia_lda8.txt",
                         "wikipedia_lda128.txt"
                        );

my @CachePrefix       = ("cophir1M",          
                         "sift1M",
                         "cosinesimil_sparse_1M",
                         "wikipedia_lda1M_128_kldiv",
                         "dna1M_normleven_32_4",
                         "sqfd_20_10k_heuristic_func_alpha=1_1M",
                         "wikipedia_lda1M_8_kldiv",
                         "wikipedia_lda1M_128_jsdiv",
                        );

my @MaxNumData        = (1000000,
                         1000000,
                         1000000,
                         1000000,
                         1000000,
                         1000000,
                         1000000,
                         1000000
                         );

my @MaxNumQuery   = (200,
                     200,
                     200,
                     200,
                     200,
                     200,
                     200,
                     200);

my @TestSetQty    = (1,
                     1,
                     1,
                     1,
                     1,
                     1,
                     1,
                     1);

my @Use           = (0,
                     0,
                     0,
                     0,
                     0,
                     0, 
                     0, 
                     1);

my $DataSetQty = scalar(@DataSetName);

scalar(@ProjType)     == $DataSetQty or die("ProjType has wrong number of elems");
scalar(@IntermDim)    == $DataSetQty or die("IntermDim has wrong number of elems");
scalar(@UseCosine)    == $DataSetQty or die("UseCosine has wrong number of elems");
scalar(@ProjDim)      == $DataSetQty or die("ProjDim has wrong number of elems");
scalar(@knnAmp)       == $DataSetQty or die("knnAmp has wrong number of elems");
scalar(@SpaceType)    == $DataSetQty or die("SpaceType has wrong number of elems");
scalar(@DataSetFile)  == $DataSetQty or die("DataSetFile has wrong number of elems");
scalar(@DataSetName)  == $DataSetQty or die("DataSetName has wrong number of elems");
scalar(@MaxNumData)   == $DataSetQty or die("MaxNumData has wrong number of elems");
scalar(@MaxNumQuery)  == $DataSetQty or die("MaxNumQuery has wrong number of elems");
scalar(@CachePrefix)  == $DataSetQty or die("CachePrefix has wrong number of elems");
scalar(@Use)          == $DataSetQty or die("Use has wrong number of elems");
scalar(@TestSetQty)   == $DataSetQty or die("TestSetQty has wrong number of elems");

print "# of data sets $DataSetQty\n";

#RunTest(1);
RunTest(10);

sub RunTest {
  my ($K) = @_;

  for (my $dn = 0; $dn < $DataSetQty; ++$dn) {
    my $Name = $DataSetName[$dn];
    my $DataQty = $MaxNumData[$dn];
    my $space = $SpaceType[$dn];

    if (!$Use[$dn]) {
      print "Skipping $Name K=$K (qty=$DataQty)\n";
      next; 
    }

    my $OutFileDir="ResultsProjData/${Name}_$space";
    !system("mkdir -p $OutFileDir") or die("Cannot create $OutFileDir");
    my $OutFilePrefix="$OutFileDir/res";

    for my $pt (@{$ProjType[$dn]}) {
    for my $pd (@{$ProjDim[$dn]}) {
      my $uc = $UseCosine[$dn] && $pt eq "rand" ? 1 :0;
      RunProjIncSortVer0($BinDir, $DataDir, $CacheDir,
                   $space,
                   $DataSetFile[$dn], 
                   $DataQty, 
                   $TestSetQty[$dn], 
                   $MaxNumQuery[$dn], 
                   $CachePrefix[$dn],
                   $K, $OutFilePrefix,
                   $pt, 
                   $IntermDim[$dn], 
                   $uc,
                   $pd, $knnAmp[$dn]
      );
    }
    }

  }
}


sub RunProjIncSortVer0 {
  my ($BinDir, $DataDir, $CacheDir,
      $SpaceTypeLoc,
      $DataNameLoc, 
      $MaxNumDataLoc, $TestSetQtyLoc, $MaxNumQueryLoc, 
      $CachePrefixLoc,
      $KLoc, 
      $OutFilePrefixLoc,
      $projTypeLoc, 
      $intermDimLoc,  # Can be zero for everything than coseine/sparse random projections
      $useCosine,
      $projDimLoc,$knnAmpLoc) = @_;

  defined($BinDir)               or die("undefined BinDir");
  defined($DataDir)              or die("undefined DataDir");
  defined($CacheDir)             or die("undefined CacheDir");
  defined($SpaceTypeLoc)         or die("undefined SpaceTypeLoc");
  defined($projTypeLoc)          or die("projTypeLoc undefined!");
  defined($knnAmpLoc)            or die("knnAmpLoc undefined!");
  defined($projDimLoc)           or die("projDimLoc undefined!");
  defined($useCosine)            or die("useCosine undefined!");
  defined($intermDimLoc)         or die("intermDimLoc undefined!");

  my @MethArray = ();

  for my $knnAmp (@$knnAmpLoc) {
    push(@MethArray, " --method proj_incsort:projType=$projTypeLoc,intermDim=$intermDimLoc,projDim=$projDimLoc,knnAmp=$knnAmp,useCosine=$useCosine");
  }

  # We should run them all at once, so that we create only index!
  RunCmd($BinDir, $DataDir, $CacheDir,
    $SpaceTypeLoc, 
    $DataNameLoc, $MaxNumDataLoc, $TestSetQtyLoc, 
    $MaxNumQueryLoc, 
    $CachePrefixLoc,
    $KLoc, 
    $OutFilePrefixLoc,
    \@MethArray);
}
