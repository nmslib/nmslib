#!/usr/bin/perl -w
my $DataDir=$ENV{"DATA_DIR"};
defined($DataDir) or die("Define the environemnt variable DATA_DIR, which contains data files.");
-d $DataDir or die("DATA_DIR='$DataDir' is not a directory");

my $K=1;
my $MaxNumData=500000;
my $MaxNumQuery=1000;
if (0) {
  $MaxNumData=10000;
  $MaxNumQuery=100;
}
my $TestSetQty=1;
my $NumPivot=16;
my $PrefixLen=4;
my $BucketSize = 50;

my @DataSet       = ("final16", "final32", "final64", "final128", "final256", "sig10k");
my @Dims          = (16, 32, 64, 128, 256, 1111);

my %Use = ( "final16" => 1, "final32" => 1, "final128" => 1, "final256" => 1, "sig10k" => 1 );

for my $SpaceType ("KLDivGenSlow", "KLDivGenFast") {
for (my $dn = 0; $dn < @DataSet; ++$dn) {
  my $dimension = $Dims[$dn];
  my $Name      = $DataSet[$dn];

  # We can select which methods to execute
  next if (!exists $Use{$Name} || !$Use{$Name});

  my $OutFileDir="ResultsExact_$SpaceType/$Name";
  !system("mkdir -p $OutFileDir") or die("Cannot create $OutFileDir");
  my $OutFilePrefix="$OutFileDir/res";


  my $cmd = "../../similarity_search/release/experiment --dataFile $DataDir/$Name.txt --maxNumData $MaxNumData --maxNumQuery $MaxNumQuery --dimension $dimension  --distType float --spaceType $SpaceType --knn $K --testSetQty $TestSetQty --outFilePrefix $OutFilePrefix ";

  $cmd .= " --method bbtree:bucketSize=$BucketSize";
  RunCmd(0, $cmd);
}
}


sub RunCmd {
  my ($Append, $cmd) = @_;

  $cmd = "$cmd --appendToResFile $Append 2>&1";

  print "$cmd\n";

  !system($cmd) or die("Cannot execute the command: $cmd");
}
