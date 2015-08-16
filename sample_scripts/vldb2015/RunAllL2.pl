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



my $SpaceType = "L2"; 

my @LargeRecallList =   (0.95, 0.9, 0.85, 0.8, 0.75, 0.7);
my @SmallRecallList =   (0.95, 0.90, 0.85);

my @RecallLists       = (\@LargeRecallList,   \@LargeRecallList, 
                                              \@SmallRecallList,
                                              \@SmallRecallList,
                                              \@SmallRecallList,
                                              \@SmallRecallList,
                                              \@SmallRecallList,
                                              \@SmallRecallList);
                                          
my @DataSetName       = ("cophir",            "sift",
                                              "sift",
                                              "sift",
                                              "sift",
                                              "sift",
                                              "sift",
                                              "sift");
                                      
my @DataSetFile       = ("cophir/full.txt",   "sift_texmex_learn5m.txt",
                                              "sift_texmex_learn5m.txt",
                                              "sift_texmex_learn5m.txt",
                                              "sift_texmex_learn5m.txt",
                                              "sift_texmex_learn5m.txt",
                                              "sift_texmex_learn5m.txt",
                                              "sift_texmex_learn5m.txt");

my @CachePrefix       = ("cophir5M",          "sift5M",
                                              "sift2.5M",
                                              "sift1.25M",
                                              "sift625K",
                                              "sift300K",
                                              "sift150K",
                                              "sift75K");

my @TuneParamFilePrefix=("tunning/ResultsL2/OutFile.cophir", "tunning/ResultsL2/OutFile.sift",
                                                             "tunning/ResultsL2/OutFile.sift",
                                                             "tunning/ResultsL2/OutFile.sift",
                                                             "tunning/ResultsL2/OutFile.sift",
                                                             "tunning/ResultsL2/OutFile.sift",
                                                             "tunning/ResultsL2/OutFile.sift",
                                                             "tunning/ResultsL2/OutFile.sift");

my @MaxNumData        = (5000000,             5000000,
                                              2500000,
                                              1250000,
                                              625000,
                                              300000, 
                                              150000,
                                              75000);

my @NN                = (10,                  13,
                                              13,
                                              13,
                                              13,
                                              13,
                                              13,
                                              13);

my @BucketSize        = (50,                  50,
                                              50,
                                              50,
                                              50,
                                              50,
                                              50,
                                              50);

my @IndexAttempts     = (3,                   3,
                                              3,
                                              3,
                                              3,
                                              3,
                                              3,
                                              3);

my @SearchAttempts    = ([1..5,8,11,15,20,25,30,35,40,45,50], [1..5,8,11,15,20,25,30],
                                              [1..10],
                                              [1..10],
                                              [1..10],
                                              [1..10],
                                              [1..10],
                                              [1..10]);


my @NumPivot       =(2000, 2000,
                      2000,
                      2000,
                      2000,
                      2000,
                      2000,
                      2000
);

my @NumPivotIndex  =(20, 20,
                         20,
                         20,
                         20,
                         20,
                         20,
                         20,
);

my @NumPivotSearch =([4,5,6,7,8,9,10], [4,5,6,7,8,9,10],
 [4,5,6,7,8,9,10],
 [4,5,6,7,8,9,10],
 [4,5,6,7,8,9,10],
 [4,5,6,7,8,9,10],
 [4,5,6,7,8,9,10],
 [4,5,6,7,8,9,10],
);

my @DbScanFrac =([], [],
                     [],
                     [],
                     [],
                     [],
                     [],
                     []
);


my @MaxNumQuery   = (1000,                500,
                                          500,
                                          500,
                                          500,
                                          500,
                                          500,
                                          500);

my @TestSetQty    = (5,                   1,
                                          1,
                                          1,
                                          1,
                                          1,
                                          1,
                                          1);

my @Use           = (1,                   1,
                                          0,
                                          0,
                                          0,
                                          0,
                                          0,
                                          0);


my $DataSetQty = scalar(@DataSetName);

scalar(@RecallLists)  == $DataSetQty or die("RecallList has wrong number of elems");
scalar(@NumPivot)  == $DataSetQty or die("NumPivot has wrong number of elems");
scalar(@NumPivotIndex)  == $DataSetQty or die("NumPivotIndex has wrong number of elems");
scalar(@NumPivotSearch)  == $DataSetQty or die("NumPivotSearch has wrong number of elems");
scalar(@DbScanFrac)  == $DataSetQty or die("DbScanFrac has wrong number of elems");
scalar(@DataSetFile)  == $DataSetQty or die("DataSetFile has wrong number of elems");
scalar(@MaxNumData)   == $DataSetQty or die("MaxNumData has wrong number of elems");
scalar(@MaxNumQuery)  == $DataSetQty or die("MaxNumQuery has wrong number of elems");
scalar(@CachePrefix)  == $DataSetQty or die("CachePrefix has wrong number of elems");
scalar(@Use)          == $DataSetQty or die("Use has wrong number of elems");
scalar(@TestSetQty)   == $DataSetQty or die("TestSetQty has wrong number of elems");
scalar(@NN)           == $DataSetQty or die("NN has wrong number of elems");
scalar(@IndexAttempts)== $DataSetQty or die("IndexAttempts has wrong number of elems");
scalar(@SearchAttempts)== $DataSetQty or die("SearchAttempts has wrong number of elems");

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

    my $OutFileDir="ResultsL2/$Name";
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

    if (1) {
      RunSmallWorld($BinDir, $DataDir, $CacheDir,
             $SpaceType,
             $DataSetFile[$dn], 
             $DataQty, 
             $TestSetQty[$dn], 
             $MaxNumQuery[$dn], 
             $CachePrefix[$dn],
             $K, $OutFilePrefix,
             $NN[$dn],
             $IndexAttempts[$dn],
             $SearchAttempts[$dn]);
    }

    if (1) {
      RunMPLSH($BinDir, $DataDir, $CacheDir,
            $SpaceType,
            $DataSetFile[$dn], 
             $DataQty, 
             $TestSetQty[$dn], 
             $MaxNumQuery[$dn], 
             $CachePrefix[$dn],
             $K, $OutFilePrefix,
             $RecallLists[$dn]);
    }

  }
}

sub RunMPLSH {
  my ($BinDir, $DataDir, $CacheDir,
      $SpaceType,
      $DataNameLoc, 
      $MaxNumDataLoc, $TestSetQtyLoc, $MaxNumQueryLoc, 
      $CachePrefixLoc,
      $KLoc, 
      $OutFilePrefixLoc,
      $RecallListLocRef) = @_;

  defined($BinDir)          or die("undefined BinDir");
  defined($BinDir)          or die("undefined BinDir");
  defined($CacheDir)        or die("undefined CacheDir");
  defined($RecallListLocRef)or die("undefined RecallListLocRef");

  # These two values provide nearly optimal performance on all our data sets.
  my $L = 50; 
  my $T = 10;

  for my $recall (@{$RecallListLocRef}) {
    my $H = $MaxNumDataLoc + 1; # Works really well and is memory efficient

    my @MethArray = ();

    push(@MethArray, " --method lsh_multiprobe:desiredRecall=$recall,H=$H,T=$T,L=$L,tuneK=$KLoc");

    # Run each one in a separate binary, or else we'll run out of memory!
    RunCmd($BinDir, $DataDir, $CacheDir,
      $SpaceType, 
      $DataNameLoc, $MaxNumDataLoc, $TestSetQtyLoc, 
      $MaxNumQueryLoc, 
      $CachePrefixLoc,
      $KLoc, 
      $OutFilePrefixLoc,
      \@MethArray);
  }
}

