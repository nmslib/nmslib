#!/usr/bin/perl -w
my $DataDir=$ENV{"DATA_DIR"};
defined($DataDir) or die("Define the environemnt variable DATA_DIR, which contains data files.");
-d $DataDir or die("DATA_DIR='$DataDir' is not a directory");

my $MaxNumData=1000000;
my $MaxNumQuery=1000;


my $SpaceType = "KLDivGenFast"; 
my $TestSetQty=5;
my $NumPivot=128;
my $BucketSize = 50;
my $alphaLeft=3;
my $alphaRight=3;
my $NN=21;
my $initIndexAttempts=4;
my $indexThreadQty=6; 
my $NumPrefixNeighb=18;
my $NumPivotNeighb=1024;
my $ChunkIndexSize=32768;

my @DataSet       = ("final16", "final64", "final256");
my @MaxScanFracs  = (0.005    , 0.04     ,    0.05);
my @MaxLeavesLC   = (150      , 150      ,    150);

my %Use =           ( "final16" => 1, "final64" => 1, "final256" => 1);

#RunTest(1);
RunTest(10);

sub RunTest {
  my ($K) = @_;

  for (my $dn = 0; $dn < @DataSet; ++$dn) {
    my $Name      = $DataSet[$dn];

    # We can select which methods to execute
    next if (!exists $Use{$Name} || !$Use{$Name});

    my $OutFileDir="ResultsKL/$Name";
    !system("mkdir -p $OutFileDir") or die("Cannot create $OutFileDir");
    my $OutFilePrefix="$OutFileDir/res";

    my $MaxDbScanFrac = $MaxScanFracs[$dn];
    my $TestQty = 6;

    if (1 == 1) {
      my $DataFile = "$DataDir/$Name.txt";
      my $cmd = "../../similarity_search/release/experiment --dataFile $DataFile --maxNumQuery $MaxNumQuery --distType float --spaceType $SpaceType --knn $K --testSetQty $TestSetQty --outFilePrefix $OutFilePrefix ";
      if (defined($MaxNumData)) { $cmd .= " --maxNumData $MaxNumData "; }

      for (my $i = 0; $i < $TestQty; ++$i) { 
        my $DbScanFrac = $MaxDbScanFrac * ($TestQty - $i) / $TestQty;
        $cmd .= " --method perm_incsort:numPivot=$NumPivot,dbScanFrac=$DbScanFrac";
      }

      for (my $i = 0; $i < $TestQty; ++$i) { 
        my $DbScanFrac = $MaxDbScanFrac * ($TestQty - $i) / $TestQty;
        $cmd .= " --method perm_vptree:numPivot=$NumPivot,dbScanFrac=$DbScanFrac,alphaLeft=$alphaLeft,alphaRight=$alphaRight";
      }

      for (my $i = 0; $i < $TestQty; ++$i) { 
        my $ip1 = $i + 1;
        my $AlphaParams = `head -$ip1 tunning/ResultsKL/OutFile.${Name}.$K|tail -1`;
        chomp $AlphaParams;
        $AlphaParams ne "" or die("Empty AlphaParams"); 
        $cmd .= " --method vptree:$AlphaParams,bucketSize=$BucketSize";
      }

      my @initSearchAttempt = (8,6,4,3,2,1);

      for (my $i = 0; $i < $TestQty; ++$i) { 
        my $DbScanFrac = $MaxDbScanFrac * ($TestQty - $i) / $TestQty;
        my $ip1 = $i + 1;
  
        my $attempt = $initSearchAttempt[$i];
        defined($attempt) or die("initIndexAttempts are not defined for i=$i");

        $cmd .= " --method small_world_rand:NN=$NN,initIndexAttempts=$initIndexAttempts,initSearchAttempts=$attempt,indexThreadQty=$indexThreadQty";

      }

      my @minTimes = (4, 6, 8, 10, 12, 14);
      for (my $i = 0; $i < $TestQty; ++$i) { 
        my $mt = $minTimes[$i];
        $cmd .= " --method pivot_neighb_invindx:numPivot=$NumPivotNeighb,numPrefix=$NumPrefixNeighb,useSort=0,invProcAlg=scan,minTimes=$mt,indexThreadQty=$indexThreadQty,chunkIndexSize=$ChunkIndexSize ";
      }

      # 1 means that we append results to the previously created file
      RunCmd(0, $cmd);
    }
  }
}

sub RunCmd {
  my ($Append, $cmd) = @_;

  $cmd = "$cmd --appendToResFile $Append 2>&1";

  print "$cmd\n";

  !system($cmd) or die("Cannot execute the command: $cmd");
}
