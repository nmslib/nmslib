#!/usr/bin/perl -w
my $DataDir=$ENV{"DATA_DIR"};
defined($DataDir) or die("Define the environemnt variable DATA_DIR, which contains data files.");
-d $DataDir or die("DATA_DIR='$DataDir' is not a directory");

my $K=1;
my $MaxNumData=500000;
my $MaxNumQuery=1000;
if (0) {
  $MaxNumData=50000;
  $MaxNumQuery=100;
}
my $SpaceType = "L2"; 
my $TestSetQty=5;
my $NumPivot=16;
my $PrefixLen=4;
my $BucketSize = 50;

my @DataSet       = ("colors112", "final128", "unif64", "final16", "final256", "sig10k");
my @Dims          = (112,       128,          64,       16,         256,        1111);
my @MaxScanFracs  = (0.02,      0.02,         0.2,      0.02,       0.02,       0.07);
my @MaxMinCands   = (4000,      16000,        16000,    4000,       16000,      4000);

my %Use = ( "colors112" => 1, "final128" => 1, "unif64" => 1, "final16" => 1, "final256" => 1,"sig10k" => 1 );

for (my $dn = 0; $dn < @DataSet; ++$dn) {
  my $dimension = $Dims[$dn];
  my $Name      = $DataSet[$dn];

  # We can select which methods to execute
  next if (!exists $Use{$Name} || !$Use{$Name});

  my $OutFileDir="ResultsL2/$Name";
  !system("mkdir -p $OutFileDir") or die("Cannot create $OutFileDir");
  my $OutFilePrefix="$OutFileDir/res";

  my @RecallList=   (0.85, 0.7, 0.65, 0.55, 0.44, 0.35, 0.25, 0.15, 0.05);

  my $MaxDbScanFrac = $MaxScanFracs[$dn];
  my $TestQty = 9;
  for (my $i = 0; $i < $TestQty; ++$i) { 
    my $DbScanFrac = $MaxDbScanFrac * ($TestQty - $i) / $TestQty;
    my $MaxLeavesToVisit = 1 << (1 + $TestQty - $i);
    my $MinCand = int($MaxMinCands[$dn] / (1 << $i));
    my $DesiredRecall = $RecallList[$i] or die("Undefined recall for i=$i");
    my $ip1 = $i + 1;
    my $AlphaParams = `head -$ip1 tunning/ResultsL2/OutFile.${Name}.$K|tail -1`;
    chomp $AlphaParams;
    $AlphaParams ne "" or die("Empty AlphaParams"); 

    my $cmd = "../../similarity_search/release/experiment --dataFile $DataDir/$Name.txt --maxNumData $MaxNumData --maxNumQuery $MaxNumQuery --dimension $dimension  --distType float --spaceType $SpaceType --knn $K --testSetQty $TestSetQty --outFilePrefix $OutFilePrefix ";

#--bucketSize $BucketSize ";

    # 1017881 is prime
    $cmd .= " --method lsh_multiprobe:desiredRecall=$DesiredRecall,H=1017881,T=10,L=50,tuneK=$K";
    $cmd .= " --method perm_incsort:numPivot=$NumPivot,dbScanFrac=$DbScanFrac";
    $cmd .= " --method vptree:$AlphaParams,bucketSize=$BucketSize";
    $cmd .= " --method perm_prefix:numPivot=$NumPivot,prefixLength=$PrefixLen,minCandidate=$MinCand";

    RunCmd($i ? 1:0, $cmd);
  }
}

sub RunCmd {
  my ($Append, $cmd) = @_;

  $cmd = "$cmd --appendToResFile $Append 2>&1";

  print "$cmd\n";

  !system($cmd) or die("Cannot execute the command: $cmd");
}
