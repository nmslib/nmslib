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



my $SpaceType = "normleven";

my @DataSetName       = ("dna32_4",
                         "dna32_4",
                         "dna32_4",
                         "dna32_4",
                         "dna32_4",
                         "dna32_4",
                         "dna32_4"
                        );
my @DataSetFile       = ("StringSpaces/dna5M_32_4.txt",
                        "StringSpaces/dna5M_32_4.txt",
                        "StringSpaces/dna5M_32_4.txt",
                        "StringSpaces/dna5M_32_4.txt",
                        "StringSpaces/dna5M_32_4.txt",
                        "StringSpaces/dna5M_32_4.txt",
                        "StringSpaces/dna5M_32_4.txt",
);
my @CachePrefix       = ("dna1M_normleven_32_4",
                         "dna500K_normleven_32_4",
                         "dna250K_normleven_32_4",
                         "dna125K_normleven_32_4",
                         "dna62K_normleven_32_4",
                         "dna31K_normleven_32_4",
                        "dna16K_normleven_32_4"
);
my @MaxNumData        = (1000000,
                        500000,
                        250000,
                        125000,
                        62000,
                        31000,
                        16000
);

my @TuneParamFilePrefix=("tunning/ResultsNormLeven/OutFile.normleven", 
                         "tunning/ResultsNormLeven/OutFile.normleven",
                         "tunning/ResultsNormLeven/OutFile.normleven",
                         "tunning/ResultsNormLeven/OutFile.normleven",
                         "tunning/ResultsNormLeven/OutFile.normleven",
                         "tunning/ResultsNormLeven/OutFile.normleven",
                         "tunning/ResultsNormLeven/OutFile.normleven");
my @BucketSize        = (50,
                         50,
                         50,
                         50,
                         50,
                         50,
                         50);
my @NN                = (30,
                         30,
                         30,
                         30,
                         30,
                         30,
                         30,
                        );
my @Rho               = (0.5,
                         0.5,
                         0.5,
                         0.5,
                         0.5,
                         0.5,
                         0.5,
                        );
my @IndexAttempts     = (3,
                         3,
                         3,
                         3,
                         3,
                         3,
                         3);
my @SearchAttempts    = ([10,15,20,25,30,40,50,60,70,80,90],
                         [6..8,10,12,14,16,18,20,25,30,35,40],
                         [6..8,10,12,14,16,18,20,25,30,35,40],
                         [6..8,10,12,14,16,18,20,25,30,35,40],
                         [6..8,10,12,14,16,18,20,25,30,35,40],
                         [6..8,10,12,14,16,18,20,25,30,35,40],
                         [6..8,10,12,14,16,18,20,25,30,35,40]
                        );



my @NumPivot       =(1000,
                      1000,
                      1000,
                      1000,
                      1000,
                      1000,
                      1000
);

my @NumPivotIndex  =(25, 
                     25,
                     25,
                     25,
                     25,
                     25,
                     25
);

my @NumPivotSearch =(
                    [2, 2, 2, 2, 2, 2],
                    [2, 2, 2, 2, 2, 2],
                    [2, 2, 2, 2, 2, 2],
                    [2, 2, 2, 2, 2, 2],
                    [2, 2, 2, 2, 2, 2],
                    [2, 2, 2, 2, 2, 2],
                    [2, 2, 2, 2, 2, 2],
);

my @DbScanFracNAPP =(
                    [0.0005,0.001,0.0025, 0.005,0.01,0.015],
                    [0.0005,0.001,0.0025, 0.005,0.01,0.015],
                    [0.0005,0.001,0.0025, 0.005,0.01,0.015],
                    [0.0005,0.001,0.0025, 0.005,0.01,0.015],
                    [0.0005,0.001,0.0025, 0.005,0.01,0.015],
                    [0.0005,0.001,0.0025, 0.005,0.01,0.015],
                    [0.0005,0.001,0.0025, 0.005,0.01,0.015],
);

my @NumPivotIncs =(
                    128,
                    128,
                    128,
                    128,
                    128,
                    128,
                    128
);


my @DbScanFracIncs =(
                    [0.01,0.02,0.03,0.04,0.05,0.06],
                    [0.01,0.02,0.03,0.04,0.05,0.06],
                    [0.01,0.02,0.03,0.04,0.05,0.06],
                    [0.01,0.02,0.03,0.04,0.05,0.06],
                    [0.01,0.02,0.03,0.04,0.05,0.06],
                    [0.01,0.02,0.03,0.04,0.05,0.06],
                    [0.01,0.02,0.03,0.04,0.05,0.06]
);

my @NumPivotBinIncs =(
                    256,
                    256,
                    256,
                    256,
                    256,
                    256,
                    256
);

my @BinThreshold =(
                    128,
                    128,
                    128,
                    128,
                    128,
                    128,
                    128
);



my @DbScanFracBinIncs =(
                    [0.004,0.014,0.02,0.03,0.04,0.05],
                    [0.004,0.014,0.02,0.03,0.04,0.05],
                    [0.004,0.014,0.02,0.03,0.04,0.05],
                    [0.004,0.014,0.02,0.03,0.04,0.05],
                    [0.004,0.014,0.02,0.03,0.04,0.05],
                    [0.004,0.014,0.02,0.03,0.04,0.05],
                    [0.004,0.014,0.02,0.03,0.04,0.05],
);


my @MaxNumQuery   = (200,
                      200,
                      200,
                      200,
                      200,
                      200,
                      200
                    );
my @TestSetQty    = (5,
                     1,
                     1,
                     1,
                     1,
                     1,
                     1);
my @Use           = (1,
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
scalar(@DbScanFracBinIncs)  == $DataSetQty or die("DbScanFracBinIncs has wrong number of elems");
scalar(@NumPivotIncs)  == $DataSetQty or die("NumPivotIncs has wrong number of elems");
scalar(@BinThreshold)  == $DataSetQty or die("BinThreshold has wrong number of elems");
scalar(@NumPivotBinIncs)  == $DataSetQty or die("NumPivotBinIncs has wrong number of elems");
scalar(@DataSetFile)  == $DataSetQty or die("DataSetFile has wrong number of elems");
scalar(@MaxNumData)   == $DataSetQty or die("MaxNumData has wrong number of elems");
scalar(@BucketSize)   == $DataSetQty or die("BucketSize has wrong number of elems");
scalar(@TuneParamFilePrefix)== $DataSetQty or die("TuneParamFilePrefix has wrong number of elems");
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

    my $OutFileDir="ResultsNormLeven/$Name";
    !system("mkdir -p $OutFileDir") or die("Cannot create $OutFileDir");
    my $OutFilePrefix="$OutFileDir/res";

    if (1) {
      RunProjIncSortBin($BinDir, $DataDir, $CacheDir,
             $SpaceType,
             $DataSetFile[$dn], 
             $DataQty, 
             $TestSetQty[$dn], 
             $MaxNumQuery[$dn], 
             $CachePrefix[$dn],
             $K, $OutFilePrefix,
             $NumPivotBinIncs[$dn],
             $BinThreshold[$dn],
             $DbScanFracBinIncs[$dn]
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
      RunNNDescent($BinDir, $DataDir, $CacheDir,
             $SpaceType,
             $DataSetFile[$dn], 
             $DataQty, 
             $TestSetQty[$dn], 
             $MaxNumQuery[$dn], 
             $CachePrefix[$dn],
             $K, $OutFilePrefix,
             $NN[$dn],
             $Rho[$dn],
             $SearchAttempts[$dn]);
    }
 }
}

