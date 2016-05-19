#!/usr/bin/perl -w
my $DataDir=$ENV{"DATA_DIR"};
defined($DataDir) or die("Define the environemnt variable DATA_DIR, which contains data files.");
-d $DataDir or die("DATA_DIR='$DataDir' is not a directory");

my $K=1;
my $MaxNumData=100000;
my $MaxNumQuery=1000;

my $SpaceType = "KLDivGenFast"; 
my $TestSetQty=5;
my $NumPivot=16;
my $PrefixLen=4;
my $BucketSize = 50;

my @DataSet       = ("final16", "final32", "final64", "final128", "final256", "sig10k_norm");
my @Dims          = (16, 32, 64, 128, 256, 1111);
my @MaxScanFracs  = (0.01, 0.015, 0.02, 0.025, 0.03, 0.035);
my @MaxMinCands   = (12000, 16000, 20000, 24000, 24000, 1000);

my %Use = ( "final16" => 1, "final128" => 1, "sig10k_norm" => 1 );

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
    my $DbScanFrac = $MaxDbScanFrac / (2 ** $i);
    $DbScanFrac = 0.0002 if $DbScanFrac < 0.0002;
    my $MaxLeavesToVisit = 1 << ($TestQty - $i - 1);
    my $MinCand = int($MaxMinCands[$dn] / (2.4 ** $i));
    $MinCand = 1 if $MinCand < 1;
    my $DesiredRecall = $RecallList[$i] or die("Undefined recall for i=$i");
    my $ip1 = $i + 1;
    my $AlphaParams = `head -$ip1 tunning/ResultsKL/OutFile.${Name}.$K|tail -1`;
    chomp $AlphaParams;
    $AlphaParams ne "" or die("Empty AlphaParams"); 
    my $cmd = "../../similarity_search/release/experiment --dataFile $DataDir/$Name.txt --maxNumData $MaxNumData --maxNumQuery $MaxNumQuery --dimension $dimension  --distType float --spaceType $SpaceType --knn $K --testSetQty $TestSetQty --outFilePrefix $OutFilePrefix ";

    $cmd .= " --method perm_incsort:numPivot=$NumPivot,dbScanFrac=$DbScanFrac ";
    $cmd .= " --method perm_vptree:numPivot=$NumPivot,dbScanFrac=$DbScanFrac,alphaLeft=2,alphaRight=2,bucketSize=$BucketSize";
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

