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
my $SpaceType = "KLDivGenFast"; 
my $TestSetQty=5;
my $NumPivot=16;
my $PrefixLen=4;
my $BucketSize = 50;

my @DataSet       = ("final8", "final16", "final32", "final64", "final128", "final256", "sig10k");
my @Dims          = (8       , 16       , 32       , 64       , 128       , 256       , 1111);
my @MaxScanFracs  = (0.015   , 0.02     , 0.03     , 0.04     , 0.05      , 0.06      , 0.07);
my @MaxMinCands   = (16000   , 16000    , 16000    , 16000    , 16000     , 16000     , 2000);

my %Use = ( "final8" => 1, "final16" => 1, "final32" => 1, "final128" => 1, "final256" => 1, "sig10k" => 1 );

for (my $dn = 0; $dn < @DataSet; ++$dn) {
  my $dimension = $Dims[$dn];
  my $Name      = $DataSet[$dn];

  # We can select which methods to execute
  next if (!exists $Use{$Name} || !$Use{$Name});

  my $OutFileDir="ResultsKL/$Name";
  !system("mkdir -p $OutFileDir") or die("Cannot create $OutFileDir");
  my $OutFilePrefix="$OutFileDir/res";

  my @RecallList=   (0.95, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2);

  my $MaxDbScanFrac = $MaxScanFracs[$dn];
  my $TestQty = 9;
  for (my $i = 0; $i < $TestQty; ++$i) { 
    my $DbScanFrac = $MaxDbScanFrac * ($TestQty - $i) / $TestQty;
    my $MaxLeavesToVisit = 1 << (1 + $TestQty - $i);
    my $MinCand = int($MaxMinCands[$dn] / (1 << $i));
    my $DesiredRecall = $RecallList[$i] or die("Undefined recall for i=$i");
    my $ip1 = $i + 1;
    my $AlphaParams = `head -$ip1 tunning/ResultsKL/OutFile.${Name}.$K|tail -1`;
    chomp $AlphaParams;
    $AlphaParams ne "" or die("Empty AlphaParams"); 

    my $cmd = "../../similarity_search/release/experiment --dataFile $DataDir/$Name.txt --maxNumData $MaxNumData --maxNumQuery $MaxNumQuery --dimension $dimension  --distType float --spaceType $SpaceType --knn $K --testSetQty $TestSetQty --outFilePrefix $OutFilePrefix ";

    $cmd .= " --method perm_incsort:numPivot=$NumPivot,dbScanFrac=$DbScanFrac ";
    $cmd .= " --method vptree:$AlphaParams,bucketSize=$BucketSize";
    $cmd .= " --method bbtree:maxLeavesToVisit=$MaxLeavesToVisit,bucketSize=$BucketSize";
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

