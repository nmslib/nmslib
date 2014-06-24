#!/usr/bin/perl -w
my $DataDir=$ENV{"DATA_DIR"};
defined($DataDir) or die("Define the environemnt variable DATA_DIR, which contains data files.");
-d $DataDir or die("DATA_DIR='$DataDir' is not a directory");

my $MaxNumData=1000000;
my $MaxNumQuery=1000;


my $SpaceType = "L2"; 
my $TestSetQty=5;
my $NumPivot=32;
my $BucketSize = 50;
my $alphaLeft=3;
my $alphaRight=3;
my $NN=21;
my $initIndexAttempts=4;
my $indexThreadQty=6; 
my $NumPrefixNeighb=18;
my $NumPivotNeighb=1024;
my $ChunkIndexSize=32768;

my @DataSet       = ("cophir", "sift_texmex_base1m", "final256", "unif64", "wikipedia_lsi128");
my @MaxScanFracs  = (0.025   ,                0.025,       0.1,     0.05,                0.1);
my @MaxLeavesLC   = (150     ,                 130,        100,      600,                100);
my @MinTimesMin   = (4       ,                   4,        4,          1,                  4);

my %Use =           ( "cophir" => 1, "sift_texmex_base1m" => 1, "final256" => 0, "unif64" => 1, "wikipedia_lsi128" => 0);
#my %Use =           ( "cophir" => 0, "sift_texmex_base1m" => 0, "final256" => 0, "unif64" => 1, "wikipedia_lsi128" => 0);

#RunTest(1);
RunTest(10);

# Max 10 million entries
# The source of prime numbers: http://www.mathematical.com/primelist1to100kk.html
# and http://www.bigprimes.net/archive/prime/1001/
sub SelectLSH_H {
  my ($fname, $maxQty) = @_;
  my $qty = defined($maxQty) ? $maxQty: `wc -l $fname`;

  if ($qty <= 1e6) {
    return 2000003;
  }
  if ($qty <= 1.5e6) {
    return 3000017;
  }
  if ($qty <= 2e6) {
    return 4000037;
  }

  die("The set is probably too big to be handled by this computer!");
}

sub RunTest {
  my ($K) = @_;

  for (my $dn = 0; $dn < @DataSet; ++$dn) {
    my $Name      = $DataSet[$dn];

    # We can select which methods to execute
    next if (!exists $Use{$Name} || !$Use{$Name});

    my $OutFileDir="ResultsL2/$Name";
    !system("mkdir -p $OutFileDir") or die("Cannot create $OutFileDir");
    my $OutFilePrefix="$OutFileDir/res";

    my @RecallList=   (0.95, 0.85, 0.75, 0.65, 0.55, 0.45);

    my $MaxDbScanFrac = $MaxScanFracs[$dn];
    my $TestQty = 6;

    if (1 == 1) {
      if (1==1) {
        # Let's test LSH separately from other methods
        my $DataFile = "$DataDir/$Name.txt";
        my $cmd = "../../similarity_search/release/experiment --dataFile $DataFile --maxNumQuery $MaxNumQuery --distType float --spaceType $SpaceType --knn $K --testSetQty $TestSetQty --outFilePrefix $OutFilePrefix ";
        if (defined($MaxNumData)) { $cmd .= " --maxNumData $MaxNumData "; }

        for (my $i = 0; $i < 2; ++$i) { 
          my $DesiredRecall = $RecallList[$i] or die("Undefined recall for i=$i");
  
          my $H = SelectLSH_H($DataFile, $MaxNumData);
          $cmd .= " --method lsh_multiprobe:desiredRecall=$DesiredRecall,H=$H,T=10,L=50,tuneK=$K";
        }
  
        RunCmd(0, $cmd);
      }
      if (1==1) {
        # Let's test LSH separately from other methods
        my $DataFile = "$DataDir/$Name.txt";
        my $cmd = "../../similarity_search/release/experiment --dataFile $DataFile --maxNumQuery $MaxNumQuery --distType float --spaceType $SpaceType --knn $K --testSetQty $TestSetQty --outFilePrefix $OutFilePrefix ";
        if (defined($MaxNumData)) { $cmd .= " --maxNumData $MaxNumData "; }

        for (my $i = 2; $i < 4; ++$i) { 
          my $DesiredRecall = $RecallList[$i] or die("Undefined recall for i=$i");
  
          my $H = SelectLSH_H($DataFile, $MaxNumData);
          $cmd .= " --method lsh_multiprobe:desiredRecall=$DesiredRecall,H=$H,T=10,L=50,tuneK=$K";
        }
  
        # 1 means that we append results to the previously created file
        RunCmd(1, $cmd);
      }
      if (1==1) {
        # Let's test LSH separately from other methods
        my $DataFile = "$DataDir/$Name.txt";
        my $cmd = "../../similarity_search/release/experiment --dataFile $DataFile --maxNumQuery $MaxNumQuery --distType float --spaceType $SpaceType --knn $K --testSetQty $TestSetQty --outFilePrefix $OutFilePrefix ";
        if (defined($MaxNumData)) { $cmd .= " --maxNumData $MaxNumData "; }

        for (my $i = 4; $i < $TestQty; ++$i) { 
          my $DesiredRecall = $RecallList[$i] or die("Undefined recall for i=$i");
  
          my $H = SelectLSH_H($DataFile, $MaxNumData);
          $cmd .= " --method lsh_multiprobe:desiredRecall=$DesiredRecall,H=$H,T=10,L=50,tuneK=$K";
        }
  
        # 1 means that we append results to the previously created file
        RunCmd(1, $cmd);
      }
    }

    if (1 == 1) {
      my $DataFile = "$DataDir/$Name.txt";
      my $cmd = "../../similarity_search/release/experiment --dataFile $DataFile --maxNumQuery $MaxNumQuery --distType float --spaceType $SpaceType --knn $K --testSetQty $TestSetQty --outFilePrefix $OutFilePrefix ";
      if (defined($MaxNumData)) { $cmd .= " --maxNumData $MaxNumData "; }

      for (my $i = 0; $i < $TestQty; ++$i) { 
        my $DbScanFrac = $MaxDbScanFrac * ($TestQty - $i) / $TestQty / 2.0;
        $cmd .= " --method perm_incsort:numPivot=$NumPivot,dbScanFrac=$DbScanFrac";
      }

      for (my $i = 0; $i < $TestQty; ++$i) { 
        my $DbScanFrac = $MaxDbScanFrac * ($TestQty - $i) / $TestQty;
        $cmd .= " --method perm_vptree:numPivot=$NumPivot,dbScanFrac=$DbScanFrac,alphaLeft=$alphaLeft,alphaRight=$alphaRight";
      }

      for (my $i = 0; $i < $TestQty; ++$i) { 
        my $d = sprintf("%d", $MaxLeavesLC[$dc] / ($TestQty + 1));
        my $maxLC = $MaxLeavesLC[$dc] - $i * $d;
        $cmd .= " --method list_clusters:bucketSize=250,maxLeavesToVisit=$maxLC";
      }

      for (my $i = 0; $i < $TestQty; ++$i) { 
        my $ip1 = $i + 1;
        my $AlphaParams = `head -$ip1 tunning/ResultsL2/OutFile.${Name}.$K|tail -1`;
        chomp $AlphaParams;
        $AlphaParams ne "" or die("Empty AlphaParams"); 
        $cmd .= " --method vptree:$AlphaParams,bucketSize=$BucketSize";
      }

      my @initSearchAttempt = (15,12,9,6,3,1);

      for (my $i = 0; $i < $TestQty; ++$i) { 
        my $ip1 = $i + 1;
  
        my $attempt = $initSearchAttempt[$i];
        defined($attempt) or die("initIndexAttempts are not defined for i=$i");

        $cmd .= " --method small_world_rand:NN=$NN,initIndexAttempts=$initIndexAttempts,initSearchAttempts=$attempt,indexThreadQty=$indexThreadQty";

      }

      for (my $i = 0; $i < $TestQty; ++$i) { 
        my $mt = $MinTimesMin + 2 * $i;
        $cmd .= " --method pivot_neighb_invindx:numPivot=$NumPivotNeighb,numPrefix=$NumPrefixNeighb,useSort=0,invProcAlg=scan,minTimes=$mt,indexThreadQty=$indexThreadQty,chunkIndexSize=$ChunkIndexSize ";
      }

      # 1 means that we append results to the previously created file
      RunCmd(1, $cmd);
    }
  }
}

sub RunCmd {
  my ($Append, $cmd) = @_;

  $cmd = "$cmd --appendToResFile $Append 2>&1";

  print "$cmd\n";

  !system($cmd) or die("Cannot execute the command: $cmd");
}
