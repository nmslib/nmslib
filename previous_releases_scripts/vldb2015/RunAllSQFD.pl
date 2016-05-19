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



my $SpaceType = "sqfd_heuristic_func:alpha=1"; 

my @DataSetName       = ("sqfd10_10k",                              "sqfd20_10k",
                         "sqfd20_10k",
                         "sqfd20_10k",
                         "sqfd20_10k",
                         "sqfd20_10k",
                         "sqfd20_10k",
                         "sqfd20_10k");
my @DataSetFile       = ("SQFD/in_10_10k.txt",                      "SQFD/in_20_10k.txt",
                         "SQFD/in_20_10k.txt",
                         "SQFD/in_20_10k.txt",
                         "SQFD/in_20_10k.txt",
                         "SQFD/in_20_10k.txt",
                         "SQFD/in_20_10k.txt",
                         "SQFD/in_20_10k.txt");
my @CachePrefix       = ("sqfd_10_10k_heuristic_func_alpha=1_1M",   "sqfd_20_10k_heuristic_func_alpha=1_1M",
                         "sqfd_20_10k_heuristic_func_alpha=1_500k",
                         "sqfd_20_10k_heuristic_func_alpha=1_250k",
                         "sqfd_20_10k_heuristic_func_alpha=1_125k",
                         "sqfd_20_10k_heuristic_func_alpha=1_60k",
                         "sqfd_20_10k_heuristic_func_alpha=1_30k",
                         "sqfd_20_10k_heuristic_func_alpha=1_15k");
my @TuneParamFilePrefix=("tunning/ResultsSQFD_10_10k/OutFile.sqfd", "tunning/ResultsSQFD_20_10k/OutFile.sqfd",
                         "tunning/ResultsSQFD_20_10k/OutFile.sqfd",
                         "tunning/ResultsSQFD_20_10k/OutFile.sqfd",
                         "tunning/ResultsSQFD_20_10k/OutFile.sqfd",
                         "tunning/ResultsSQFD_20_10k/OutFile.sqfd",
                         "tunning/ResultsSQFD_20_10k/OutFile.sqfd",
                         "tunning/ResultsSQFD_20_10k/OutFile.sqfd");
my @BucketSize        = (5,                     5,
                         5,
                         5,
                         5,
                         5,
                         5,
                         5);
my @MaxNumData        = (1000000,                                   1000000,
                         500000,
                         250000,
                         125000,
                         60000,
                         30000,
                         15000);
my @NN                = (11,                                        11,
                         11,
                         11,
                         11,
                         11,
                         11,
                         11
);
my @IndexAttempts     = (3,                                         3,
                         3,
                         3,
                         3,
                         3,
                         3,
                         3);
my @SearchAttempts    = ([3..7,9,11,13],                            [3..7,9,11,13],
                         [3..7,9,11,13],
                         [3..7,9,11,13],
                         [3..7,9,11,13],
                         [3..7,9,11,13],
                         [3..7,9,11,13],
                         [3..7,9,11,13]);






my @NumPivot       =(500, 500,
                      500,
                      500,
                      500,
                      500,
                      500,
                      500
);

my @NumPivotIndex  =(25, 25,
                     25,
                     25,
                     25,
                     25,
                     25,
                     25
);

my @NumPivotSearch =(
                    [9, 9, 9, 9, 9, 9], [9, 9, 9, 9, 9, 9],
                    [9, 9, 9, 9, 9, 9],
                    [9, 9, 9, 9, 9, 9],
                    [9, 9, 9, 9, 9, 9],
                    [9, 9, 9, 9, 9, 9],
                    [9, 9, 9, 9, 9, 9],
                    [9, 9, 9, 9, 9, 9]
);

my @DbScanFracNAPP =(
                  [0.001, 0.003, 0.005, 0.007, 0.01, 0.015], [0.0002, 0.0004, 0.0006, 0.0008, 0.001, 0.003],
                  [0.0004, 0.0006, 0.0008, 0.001, 0.003, 0.005],
                  [0.002, 0.003, 0.004, 0.005,  0.007, 0.015],
                  [0.002, 0.003, 0.004, 0.005,  0.007, 0.015],
                  [0.005, 0.01,  0.015, 0.02,  0.025,  0.03],
                  [0.005, 0.01,  0.015, 0.02,  0.025,  0.03],
                  [0.005, 0.01,  0.015, 0.02,  0.025,  0.03]
);

my @NumPivotIncs =(
                    128, 128,
                    128,
                    128,
                    128,
                    128,
                    128,
                    128
);


my @DbScanFracIncs =(
                    [0.02,0.04,0.06,0.08,0.1,0.02], [0.003,0.005,0.007,0.010,0.015,0.02,0.25],
                    [0.002,0.004,0.006,0.008,0.01,0.02],
                    [0.004,0.006,0.008,0.01,0.02, 0.04],
                    [0.004,0.006,0.008,0.01,0.02, 0.04],
                    [0.004,0.006,0.008,0.01,0.02, 0.04],
                    [0.004,0.006,0.008,0.01,0.02, 0.04],
                    [0.004,0.006,0.008,0.01,0.02, 0.04]
);

my @NumPivotOmedrank =(
                    32, 32,
                    32,
                    32,
                    32,
                    32,
                    32,
                    32
);


my @DbScanFracOmedrank =(
                    [0.02,0.04,0.06,0.08,0.1,0.02], [0.001,0.003,0.005,0.007,0.009,0.011],
                    [0.002,0.004,0.006,0.008,0.01,0.02],
                    [0.004,0.006,0.008,0.01,0.02, 0.04],
                    [0.004,0.006,0.008,0.01,0.02, 0.04],
                    [0.004,0.006,0.008,0.01,0.02, 0.04],
                    [0.004,0.006,0.008,0.01,0.02, 0.04],
                    [0.004,0.006,0.008,0.01,0.02, 0.04]
);



my @MaxNumQuery   = (200, 200,
                         200,
                         200,
                         200,
                         200,
                         200,
                         200);
my @TestSetQty    = (5,   5,
                          1,
                          1,
                          1,
                          1,
                          1,
                          1);
my @Use           = (0,   1,
                          0,
                          0,
                          0,
                          0,
                          0,
                          0);

my $DataSetQty = scalar(@DataSetName);

scalar(@DataSetFile)  == $DataSetQty or die("DataSetFile has wrong number of elems");
scalar(@NumPivot)  == $DataSetQty or die("NumPivot has wrong number of elems");
scalar(@NumPivotIndex)  == $DataSetQty or die("NumPivotIndex has wrong number of elems");
scalar(@NumPivotSearch)  == $DataSetQty or die("NumPivotSearch has wrong number of elems");
scalar(@DbScanFracNAPP)  == $DataSetQty or die("DbScanFracNAPP has wrong number of elems");
scalar(@DbScanFracIncs)  == $DataSetQty or die("DbScanFracIncs has wrong number of elems");
scalar(@NumPivotIncs)  == $DataSetQty or die("NumPivotIncs has wrong number of elems");
scalar(@NumPivotOmedrank)  == $DataSetQty or die("NumPivotOmedrank has wrong number of elems");
scalar(@DbScanFracOmedrank)  == $DataSetQty or die("DbScanFracOmedrank has wrong number of elems");
scalar(@MaxNumData)   == $DataSetQty or die("MaxNumData has wrong number of elems");
scalar(@MaxNumQuery)  == $DataSetQty or die("MaxNumQuery has wrong number of elems");
scalar(@BucketSize)   == $DataSetQty or die("BucketSize has wrong number of elems");
scalar(@TuneParamFilePrefix)== $DataSetQty or die("TuneParamFilePrefix has wrong number of elems");
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

    my $OutFileDir="ResultsSQFD/$Name";
    !system("mkdir -p $OutFileDir") or die("Cannot create $OutFileDir");
    my $OutFilePrefix="$OutFileDir/res";

    if (0) {
      RunOmedrank($BinDir, $DataDir, $CacheDir,
             $SpaceType,
             $DataSetFile[$dn], 
             $DataQty, 
             $TestSetQty[$dn], 
             $MaxNumQuery[$dn], 
             $CachePrefix[$dn],
             $K, $OutFilePrefix,
             "fastmap",  0,
             $NumPivotOmedrank[$dn],
             $NumPivotOmedrank[$dn],
             0.5,
             $DbScanFracOmedrank[$dn]
            );
    }
 
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
             $DbScanFracNAPP[$dn]
            );
    }

    if (1) {
      RunProjIncSort($BinDir, $DataDir, $CacheDir,
             $SpaceType,
             $DataSetFile[$dn], 
             $DataQty, 
             $TestSetQty[$dn], 
             $MaxNumQuery[$dn], 
             $CachePrefix[$dn],
             $K, $OutFilePrefix,
             "perm",  0, 0,
             $NumPivotIncs[$dn],
             $DbScanFracIncs[$dn]
            );
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

