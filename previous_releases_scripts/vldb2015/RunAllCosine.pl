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



my $SpaceType = "cosinesimil_sparse_fast"; 

my @DataSetName       = ("wikipedia",
                         "wikipedia",
                         "wikipedia",
                         "wikipedia",
                         "wikipedia",
                         "wikipedia",
                         "wikipedia",
                         "wikipedia"
                        );
my @DataSetFile       = ("wikipedia.txt",
                         "wikipedia.txt",
                         "wikipedia.txt",
                         "wikipedia.txt",
                         "wikipedia.txt",
                         "wikipedia.txt",
                         "wikipedia.txt",
                         "wikipedia.txt"
                        );
my @CachePrefix       = ("cosinesimil_sparse_4M",
                         "cosinesimil_sparse_2M",
                         "cosinesimil_sparse_1M",
                         "cosinesimil_sparse_500k",
                         "cosinesimil_sparse_250k",
                         "cosinesimil_sparse_125k",
                         "cosinesimil_sparse_60k",
                         "cosinesimil_sparse_30k"
                        );
my @MaxNumData        = (4000000,
                         2000000,
                         1000000,
                         500000,
                         250000,
                         125000,
                         60000,
                         30000
                        );
my @NN                = (17,
                         17,
                         17,
                         17,
                         17,
                         17,
                         17,
                         17
                        );
my @IndexAttempts     = (3,
                        3,
                        3,
                        3,
                        3,
                        3,
                        3,
                        3
                        );
my @SearchAttempts    = ([1,3,6,10,15,20,30,40,50,60,70,80,90,100],
                        [1..10,15,20,25,30,35,40],
                        [1..10,15,20,25,30,35,40],
                        [1..10,15,20,25,30,35,40],
                        [1..10,15,20,25,30,35,40],
                        [1..10,15,20,25,30,35,40],
                        [1..10,15,20,25,30,35,40],
                        [1..10,15,20,25,30,35,40]
                        );

my @NumPivotIndex  =(32, 
                     32,
                     32,
                     32,
                     32,
                     32,
                     32,
                     32,
);

my @NumPivot       =(1000, 
                      1000,
                      1000,
                      1000,
                      1000,
                      1000,
                      1000,
                      1000
);

my @NumPivotSearch  =([1..6], 
                     [1..6],
                     [1..6],
                     [1..6],
                     [1..6],
                     [1..6],
                     [1..6],
                     [1..6],
);

my @DbScanFrac =([], 
                 [],
                     [],
                     [],
                     [],
                     [],
                     [],
                     []
);


my @MaxNumQuery   = (500,
                     200,
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
                     1,
                     1
                    );
my @Use           = (1,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0
                    );


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

    my $OutFileDir="ResultsCosine/$Name";
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
  }
}

