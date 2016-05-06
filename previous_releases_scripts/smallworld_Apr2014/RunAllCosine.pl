#!/usr/bin/perl -w
my $DataDir=$ENV{"DATA_DIR"};
defined($DataDir) or die("Define the environemnt variable DATA_DIR, which contains data files.");
-d $DataDir or die("DATA_DIR='$DataDir' is not a directory");

my $MaxNumQuery= 1000;

#my @maxDataQty = (800000);
my @maxDataQty = (12500, 25000, 50000, 100000, 200000, 400000, 800000, 1600000, 3200000);

my $SpaceType = "cosinesimil_sparse_fast"; 
# Let it be just one
my $TestSetQty=5;
my $NumPivot=128;
my $NumPivotNeighb=1024;
my $NumPrefixNeighb=18;
my $BucketSize = 50;
my $NN=15;
my $initIndexAttempts=4;
my $indexThreadQty=4; 
my $alphaLeft=3;
my $alphaRight=3;
my $ChunkIndexSize=32768;

my @DataSet       = ( "wikipedia");
my @MaxScanFracs  = (        0.2);
my @MaxLeavesLC   = ( 500);

my %Use = ( "wikipedia" => 1);

for (my $t = 0; $t < @maxDataQty; ++$t) {
  #RunTest(1, $maxDataQty[$t]);
  RunTest(10, $maxDataQty[$t]);

}

sub RunCmd {
  my ($Append, $cmd) = @_;

  $cmd = "$cmd --appendToResFile $Append 2>&1";

  print "$cmd\n";

  !system($cmd) or die("Cannot execute the command: $cmd");
}

sub RunTest {
  my ($K,  $MaxNumData) = @_;

  for (my $dn = 0; $dn < @DataSet; ++$dn) {
    my $Name      = $DataSet[$dn];

    # We can select which methods to execute
    next if (!exists $Use{$Name} || !$Use{$Name});

    my $OutFileDir="ResultsCosineSimil/${Name}_$MaxNumData";
    !system("mkdir -p $OutFileDir") or die("Cannot create $OutFileDir");
    my $OutFilePrefix="$OutFileDir/res";

    my $DataFile = "$DataDir/$Name.txt";
    my $cmd = "../../similarity_search/release/experiment --dataFile $DataFile --maxNumQuery $MaxNumQuery --distType float --spaceType $SpaceType --knn $K --testSetQty $TestSetQty --outFilePrefix $OutFilePrefix ";
    if (defined($MaxNumData)) { $cmd .= " --maxNumData $MaxNumData "; }

    my $MaxDbScanFrac = $MaxScanFracs[$dn];
    my $TestQty = 6;

    for (my $i = 0; $i < $TestQty; ++$i) { 
      my $d = sprintf("%d", $MaxLeavesLC[$dn] / ($TestQty + 1));
      my $bs = sprintf("%d", 400);
      my $maxLC = $MaxLeavesLC[$dn] - $i * $d;
      $cmd .= " --method list_clusters:bucketSize=$bs,maxLeavesToVisit=$maxLC";
    }

    for (my $i = 0; $i < $TestQty; ++$i) { 
      my $DbScanFrac = $MaxDbScanFrac * ($TestQty - $i) / $TestQty;
      my $ip1 = $i + 1;

      $cmd .= " --method perm_vptree:numPivot=$NumPivot,dbScanFrac=$DbScanFrac,alphaLeft=$alphaRight,alphaRight=$alphaRight";
    }

    for (my $i = 0; $i < $TestQty; ++$i) { 
      my $DbScanFrac = $MaxDbScanFrac * ($TestQty - $i) / $TestQty;
      my $ip1 = $i + 1;

      $cmd .= " --method perm_incsort:numPivot=$NumPivot,dbScanFrac=$DbScanFrac";
    }

    my @initSearchAttempt = (23,19,15,11,7,3);

    for (my $i = 0; $i < $TestQty; ++$i) { 
      my $DbScanFrac = $MaxDbScanFrac * ($TestQty - $i) / $TestQty;
      my $ip1 = $i + 1;
  
      my $attempt = $initSearchAttempt[$i];
      defined($attempt) or die("initIndexAttempts are not defined for i=$i");

      $cmd .= " --method small_world_rand:NN=$NN,initIndexAttempts=$initIndexAttempts,initSearchAttempts=$attempt,indexThreadQty=$indexThreadQty";

    }

    my @minTimes = (1, 2, 3, 4, 5, 6);
    for (my $i = 0; $i < $TestQty; ++$i) { 
      my $mt = $minTimes[$i];
      $cmd .= " --method pivot_neighb_invindx:numPivot=$NumPivotNeighb,numPrefix=$NumPrefixNeighb,useSort=0,invProcAlg=scan,minTimes=$mt,indexThreadQty=$indexThreadQty,chunkIndexSize=$ChunkIndexSize ";
    }

    RunCmd(0, $cmd);
  }
}
