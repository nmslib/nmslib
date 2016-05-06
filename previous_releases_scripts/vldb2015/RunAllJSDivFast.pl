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



my $SpaceType = "jsdivfast";

my @DataSetName       = ("wikipedia_lda_8",         "wikipedia_lda_128");
my @DataSetFile       = ("wikipedia_lda8.txt",      "wikipedia_lda128.txt");
my @CachePrefix       = ("wikipedia_lda_8_jsdiv",   "wikipedia_lda_128_jsdiv");
my @TuneParamFilePrefix=("tunning/ResultsJSFast/OutFile.wikipedia_lda8", "tunning/ResultsJSFast/OutFile.wikipedia_lda128");
my @BucketSize        = (50,                     50);
my @MaxNumData        = (2134995,                   2134995);
my @SmallWorld_NN     = (undef,                     9);
my @SmallWorld_SearchAttempts = (undef,  [1..10]);
my @SmallWorld_IndexAttempts  = (undef,  3);
my @NNDescent_NN      = (25,                        9);
my @NNDescent_SearchAttempts  = ([5, 10, 15, 20, 25, 30, 35, 40], undef);
my @NNDescent_Rho     = (0.5,                       undef);
#my @TestSetQty    = (5, 5);
my @MaxNumQuery   = (200, 200);
my @TestSetQty    = (5, 5);
my @Use           = (1, 1);
my @FlagSmallWorld = (0, 1);
my @FlagNNDescent  = (1, 0);

my @NumPivotIndex  =(32, 32);
my @NumPivot       =(2000, 2000);
my @NumPivotSearch  =([25,27,29,30,32,35,37], [10,13,15,18,20,22,23]);
my @DbScanFrac =([], []);


my $DataSetQty = scalar(@DataSetName);

scalar(@DataSetFile)  == $DataSetQty or die("DataSetFile has wrong number of elems");
scalar(@NumPivot)  == $DataSetQty or die("NumPivot has wrong number of elems");
scalar(@NumPivotIndex)  == $DataSetQty or die("NumPivotIndex has wrong number of elems");
scalar(@NumPivotSearch)  == $DataSetQty or die("NumPivotSearch has wrong number of elems");
scalar(@DbScanFrac)  == $DataSetQty or die("DbScanFrac has wrong number of elems");
scalar(@MaxNumData)   == $DataSetQty or die("MaxNumData has wrong number of elems");
scalar(@MaxNumQuery)  == $DataSetQty or die("MaxNumQuery has wrong number of elems");
scalar(@CachePrefix)  == $DataSetQty or die("CachePrefix has wrong number of elems");
scalar(@Use)          == $DataSetQty or die("Use has wrong number of elems");
scalar(@TestSetQty)   == $DataSetQty or die("TestSetQty has wrong number of elems");
scalar(@SmallWorld_NN)              == $DataSetQty or die("SmallWorld_NN has wrong number of elems");
scalar(@SmallWorld_SearchAttempts)  == $DataSetQty or die("SmallWorld_SearchAttempts has wrong number of elems");
scalar(@NNDescent_NN)               == $DataSetQty or die("NNDescent_NN has wrong number of elems");
scalar(@NNDescent_SearchAttempts)   == $DataSetQty or die("NNDescent_SearchAttempts has wrong number of elems");
scalar(@NNDescent_NN)               == $DataSetQty or die("NNDescent_NN has wrong number of elems");
scalar(@SmallWorld_IndexAttempts)   == $DataSetQty or die("SmallWorld_IndexAttempts has wrong number of elems");

#RunTest(1);
RunTest(10);

sub RunTest {
  my ($K) = @_;

  for (my $dn = 0; $dn < $DataSetQty; ++$dn) {
    my $Name = $DataSetName[$dn];
    my $DataQty = $MaxNumData[$dn];

    if (!$Use[$dn]) {
      print "Skipping $Name K=$K (qty=$DataQty)\n";
      next; 
    }

    my $OutFileDir="ResultsJSDivFast/$Name";
    !system("mkdir -p $OutFileDir") or die("Cannot create $OutFileDir");
    my $OutFilePrefix="$OutFileDir/res";

    if (1) {
      RunNAPP($BinDir, $DataDir, $CacheDir,
             $SpaceType,
             $DataSetFile[$dn], 
             $DataQty, 
             $TestSetQty[$dn], 
             $MaxNumQuery[$dn], 
             $CachePrefix[$dn],
             $K, $OutFilePrefix,
             $NumPivot[$dn],
             $NumPivotIndex[$dn],
             $NumPivotSearch[$dn],
             $DbScanFrac[$dn]
            );
    }

    if (1 && $FlagSmallWorld[$dn]) {
      RunSmallWorld($BinDir, $DataDir, $CacheDir,
             $SpaceType,
             $DataSetFile[$dn], 
             $DataQty, 
             $TestSetQty[$dn], 
             $MaxNumQuery[$dn], 
             $CachePrefix[$dn],
             $K, $OutFilePrefix,
             $SmallWorld_NN[$dn],
             $SmallWorld_IndexAttempts[$dn],
             $SmallWorld_SearchAttempts[$dn]);
    }
    if (1 && $FlagNNDescent[$dn]) {
      RunNNDescent($BinDir, $DataDir, $CacheDir,
             $SpaceType,
             $DataSetFile[$dn], 
             $DataQty, 
             $TestSetQty[$dn], 
             $MaxNumQuery[$dn], 
             $CachePrefix[$dn],
             $K, $OutFilePrefix,
             $NNDescent_NN[$dn],
             $NNDescent_Rho[$dn],
             $NNDescent_SearchAttempts[$dn]);
    }

    if (1) {
      RunVPTree($BinDir, $DataDir, $CacheDir,
             $SpaceType,
             $DataSetFile[$dn], 
             $DataQty, 
             $TestSetQty[$dn], 
             $MaxNumQuery[$dn], 
             $CachePrefix[$dn],
             $K, $OutFilePrefix,
             $TuneParamFilePrefix[$dn],
             $BucketSize[$dn]);
    }
  }
}

