sub RunCmd {
  my ($BinDir, $DataDir, $CacheDir,
      $SpaceTypeLoc, 
      $DataNameLoc, $MaxNumDataLoc, $TestSetQtyLoc, 
      $MaxNumQueryLoc, 
      $CachePrefixLoc,
      $KLoc, 
      $OutFilePrefixLoc,
      $MethArrRef) = @_;

  defined($BinDir)          or die("undefined BinDir");
  defined($DataDir)         or die("undefined DataDir");
  defined($CacheDir)        or die("undefined CacheDir");
  defined($SpaceTypeLoc)    or die("undefined SpaceTypeLoc");
  defined($DataNameLoc)     or die("undefined DataNameLoc");
  defined($MaxNumDataLoc)   or die("undefined MaxNumDataLoc");
  defined($TestSetQtyLoc)   or die("undefined TestSetQtyLoc");
  defined($SpaceTypeLoc)    or die("undefined SpaceTypeLoc");
  defined($MaxNumQueryLoc)  or die("undefined MaxNumQueryLoc");
  defined($CachePrefixLoc)  or die("undefined CachePrefixLoc");
  defined($KLoc)            or die("undefined KLoc");
  defined($OutFilePrefixLoc)or die("undefined OutFilePrefixLoc");

  my $DataFile = "$DataDir/$DataNameLoc";
  my $OutFileRep = "${OutFilePrefixLoc}_K=$KLoc.rep";

  print "Report file: $OutFileRep\n";
  my $Append = 0;

  if ( -f "$OutFileRep") {
  # Append only if the output file exists already
    $Append = 1;
  }

  my $cmd = "$BinDir/experiment --dataFile $DataFile  --maxNumData $MaxNumDataLoc --maxNumQuery $MaxNumQueryLoc --distType float --spaceType $SpaceTypeLoc --knn $KLoc --testSetQty $TestSetQtyLoc --outFilePrefix $OutFilePrefixLoc --cachePrefixGS $CacheDir/$CachePrefixLoc ";

  my @MethArr = @{$MethArrRef};

  for my $s (@MethArr) {
    $cmd .= " $s";
  }

  $cmd = "$cmd --appendToResFile $Append 2>&1";

  print "==============================================================\n";
  print "$cmd\n";
  print "==============================================================\n";

  !system($cmd) or die("Cannot execute the command: $cmd");
}

sub RunSmallWorld {
  my ($BinDir, $DataDir, $CacheDir,
      $SpaceTypeLoc,
      $DataNameLoc, 
      $MaxNumDataLoc, $TestSetQtyLoc, $MaxNumQueryLoc, 
      $CachePrefixLoc,
      $KLoc, 
      $OutFilePrefixLoc,
      $NNLoc, $IndexAttemptsLoc, $SearchAttemptsLoc) = @_;

  my $IndexThreadQty=4; 

  defined($BinDir)               or die("undefined BinDir");
  defined($DataDir)              or die("undefined DataDir");
  defined($CacheDir)             or die("undefined CacheDir");
  defined($SpaceTypeLoc)         or die("undefined SpaceTypeLoc");
  defined($NNLoc)                or die("NNLoc undefined!");
  defined($IndexAttemptsLoc)     or die("IndexAttemptsLoc undefined!");
  defined($SearchAttemptsLoc)    or die("SearchAttemptsLoc undefined!");

  my @MethArray = ();

  for my $att (@$SearchAttemptsLoc) {

    push(@MethArray, " --method small_world_rand:NN=$NNLoc,initIndexAttempts=$IndexAttemptsLoc,indexThreadQty=$IndexThreadQty,initSearchAttempts=$att");
  }

  # We should run them all at once, so that we create only index!
  RunCmd($BinDir, $DataDir, $CacheDir,
    $SpaceTypeLoc, 
    $DataNameLoc, $MaxNumDataLoc, $TestSetQtyLoc, 
    $MaxNumQueryLoc, 
    $CachePrefixLoc,
    $KLoc, 
    $OutFilePrefixLoc,
    \@MethArray);
}

sub RunNNDescent {
  my ($BinDir, $DataDir, $CacheDir,
      $SpaceTypeLoc,
      $DataNameLoc, 
      $MaxNumDataLoc, $TestSetQtyLoc, $MaxNumQueryLoc, 
      $CachePrefixLoc,
      $KLoc, 
      $OutFilePrefixLoc,
      $NNLoc, $RhoLoc, $SearchAttemptsLoc) = @_;

  my $IndexThreadQty=4; 

  defined($BinDir)               or die("undefined BinDir");
  defined($DataDir)              or die("undefined DataDir");
  defined($CacheDir)             or die("undefined CacheDir");
  defined($SpaceTypeLoc)         or die("undefined SpaceTypeLoc");
  defined($NNLoc)                or die("NNLoc undefined!");
  defined($RhoLoc)               or die("RhoLoc undefined!");
  defined($SearchAttemptsLoc)    or die("SearchAttemptsLoc undefined!");

  my @MethArray = ();

  for my $att (@$SearchAttemptsLoc) {
    # Let's ignore the number of threads here
    push(@MethArray, " --method nndes:NN=$NNLoc,rho=$RhoLoc,initSearchAttempts=$att");
  }

  # We should run them all at once, so that we create only index!
  RunCmd($BinDir, $DataDir, $CacheDir,
    $SpaceTypeLoc, 
    $DataNameLoc, $MaxNumDataLoc, $TestSetQtyLoc, 
    $MaxNumQueryLoc, 
    $CachePrefixLoc,
    $KLoc, 
    $OutFilePrefixLoc,
    \@MethArray);
}

sub RunVPTree {
  my ($BinDir, $DataDir, $CacheDir,
      $SpaceTypeLoc,
      $DataNameLoc, 
      $MaxNumDataLoc, $TestSetQtyLoc, $MaxNumQueryLoc, 
      $CachePrefixLoc,
      $KLoc, 
      $OutFilePrefixLoc,
      $TuneParamFilePrefixLoc,
      $BucketSizeLoc) = @_;

  defined($BinDir)               or die("undefined BinDir");
  defined($DataDir)              or die("undefined DataDir");
  defined($CacheDir)             or die("undefined CacheDir");
  defined($SpaceTypeLoc)         or die("undefined SpaceTypeLoc");
  defined($BucketSizeLoc)        or die("undefined BucketSizeLoc");
  defined($TuneParamFilePrefixLoc)or die("undefined TuneParamFilePrefixLoc");

  my @MethArray = ();

  my $f = "$TuneParamFilePrefixLoc.$KLoc";

  open F, $f or die("Can't open '$f' for reading");

  while(my $addParam = <F>) {
    chomp $addParam;
    if ($addParam ne "") {
      print "Using parameters: $addParam";
      push(@MethArray, " --method vptree:bucketSize=$BucketSizeLoc,$addParam");
    }
  }

  # We should run them all at once, so that we create only index!
  RunCmd($BinDir, $DataDir, $CacheDir,
    $SpaceTypeLoc, 
    $DataNameLoc, $MaxNumDataLoc, $TestSetQtyLoc, 
    $MaxNumQueryLoc, 
    $CachePrefixLoc,
    $KLoc, 
    $OutFilePrefixLoc,
    \@MethArray);
}

sub RunProjIncSortBin {
  my ($BinDir, $DataDir, $CacheDir,
      $SpaceTypeLoc,
      $DataNameLoc, 
      $MaxNumDataLoc, $TestSetQtyLoc, $MaxNumQueryLoc, 
      $CachePrefixLoc,
      $KLoc, 
      $OutFilePrefixLoc,
      $numPivotLoc,
      $binThresholdLoc,
      $dbScanFracLoc) = @_;

  defined($BinDir)               or die("undefined BinDir");
  defined($DataDir)              or die("undefined DataDir");
  defined($CacheDir)             or die("undefined CacheDir");
  defined($SpaceTypeLoc)         or die("undefined SpaceTypeLoc");
  defined($binThresholdLoc)      or die("binThresholdLoc undefined!");
  defined($dbScanFracLoc)        or die("dbScanFracLoc undefined!");
  defined($numPivotLoc)          or die("numPivotLoc undefined!");

  my @MethArray = ();

  for my $dbScanFrac (@$dbScanFracLoc) {
    push(@MethArray, " --method perm_incsort_bin:numPivot=$numPivotLoc,dbScanFrac=$dbScanFrac,binThreshold=$binThresholdLoc");
  }

  # We should run them all at once, so that we create only index!
  RunCmd($BinDir, $DataDir, $CacheDir,
    $SpaceTypeLoc, 
    $DataNameLoc, $MaxNumDataLoc, $TestSetQtyLoc, 
    $MaxNumQueryLoc, 
    $CachePrefixLoc,
    $KLoc, 
    $OutFilePrefixLoc,
    \@MethArray);
}

sub RunProjIncSort {
  my ($BinDir, $DataDir, $CacheDir,
      $SpaceTypeLoc,
      $DataNameLoc, 
      $MaxNumDataLoc, $TestSetQtyLoc, $MaxNumQueryLoc, 
      $CachePrefixLoc,
      $KLoc, 
      $OutFilePrefixLoc,
      $projTypeLoc, 
      $intermDimLoc,  # Can be zero for everything than coseine/sparse random projections
      $useCosine,
      $projDimLoc,$dbScanFracLoc) = @_;

  defined($BinDir)               or die("undefined BinDir");
  defined($DataDir)              or die("undefined DataDir");
  defined($CacheDir)             or die("undefined CacheDir");
  defined($SpaceTypeLoc)         or die("undefined SpaceTypeLoc");
  defined($projTypeLoc)          or die("projTypeLoc undefined!");
  defined($dbScanFracLoc)            or die("dbScanFracLoc undefined!");
  defined($projDimLoc)           or die("projDimLoc undefined!");
  defined($useCosine)            or die("useCosine undefined!");
  defined($intermDimLoc)         or die("intermDimLoc undefined!");

  my @MethArray = ();

  for my $dbScanFrac (@$dbScanFracLoc) {
    push(@MethArray, " --method proj_incsort:projType=$projTypeLoc,intermDim=$intermDimLoc,projDim=$projDimLoc,dbScanFrac=$dbScanFrac,useCosine=$useCosine");
  }

  # We should run them all at once, so that we create only index!
  RunCmd($BinDir, $DataDir, $CacheDir,
    $SpaceTypeLoc, 
    $DataNameLoc, $MaxNumDataLoc, $TestSetQtyLoc, 
    $MaxNumQueryLoc, 
    $CachePrefixLoc,
    $KLoc, 
    $OutFilePrefixLoc,
    \@MethArray);
}

sub RunMIFile {
  my ($BinDir, $DataDir, $CacheDir,
      $SpaceTypeLoc,
      $DataNameLoc, 
      $MaxNumDataLoc, $TestSetQtyLoc, $MaxNumQueryLoc, 
      $CachePrefixLoc,
      $KLoc, 
      $OutFilePrefixLoc,
      $numPivot,
      $numPivotIndex,
      $numPivotSearch,
      $maxPosDiff,
      $dbScanFracLoc
      ) = @_;

  defined($BinDir)               or die("undefined BinDir");
  defined($DataDir)              or die("undefined DataDir");
  defined($CacheDir)             or die("undefined CacheDir");
  defined($SpaceTypeLoc)         or die("undefined SpaceTypeLoc");
  defined($numPivot)             or die("numPivot undefined!");
  defined($numPivotIndex)        or die("numPivotIndex undefined!");
  defined($numPivotSearch)       or die("numPivotSearch undefined!");
  defined($maxPosDiff)           or die("maxPosDiff undefined!");
  defined($dbScanFracLoc)        or die("dbScanFracLoc undefined!");

  my @MethArray = ();

  for my $dbScanFrac (@$dbScanFracLoc) {
    push(@MethArray, "  --method mi-file:numPivot=$numPivot,numPivotIndex=$numPivotIndex,numPivotSearch=$numPivotSearch,maxPosDiff=$maxPosDiff,dbScanFrac=$dbScanFrac");
  }

  # We should run them all at once, so that we create only index!
  RunCmd($BinDir, $DataDir, $CacheDir,
    $SpaceTypeLoc, 
    $DataNameLoc, $MaxNumDataLoc, $TestSetQtyLoc, 
    $MaxNumQueryLoc, 
    $CachePrefixLoc,
    $KLoc, 
    $OutFilePrefixLoc,
    \@MethArray);
}

sub RunNAPP {
  my ($BinDir, $DataDir, $CacheDir,
      $SpaceTypeLoc,
      $DataNameLoc, 
      $MaxNumDataLoc, $TestSetQtyLoc, $MaxNumQueryLoc, 
      $CachePrefixLoc,
      $KLoc, 
      $OutFilePrefixLoc,
      $numPivot,
      $numPivotIndex,
      $numPivotSearchLoc,
      $dbScanFracLoc
      ) = @_;

  my $indexThreadQty=4; 

  defined($BinDir)               or die("undefined BinDir");
  defined($DataDir)              or die("undefined DataDir");
  defined($CacheDir)             or die("undefined CacheDir");
  defined($SpaceTypeLoc)         or die("undefined SpaceTypeLoc");
  defined($numPivot)             or die("numPivot undefined!");
  defined($numPivotIndex)        or die("numPivotIndex undefined!");
  defined($numPivotSearchLoc)    or die("numPivotSearchLoc undefined!");
  defined($dbScanFracLoc)        or die("dbScanFracLoc undefined!");

  #dbScanFracLoc can have undef values
  #scalar(@$dbScanFracLoc) == scalar(@$numPivotSearchLoc) or die("# of numPivotSearch not equal to # of dbScanFrac");

  my @MethArray = ();

  my $len = @$numPivotSearchLoc;
  for (my $i = 0; $i < $len; ++$i) {
    my $dbScanFrac = $dbScanFracLoc->[$i];
    my $numPivotSearch = $numPivotSearchLoc->[$i];
    my $useSort = 0;
    if (defined($dbScanFrac) && $dbScanFrac > 0) {
      $useSort=1;
      defined($numPivotSearch) or die("numPivotSearch is not defined for dbScanFrac=$dbScanFrac");
    } else {
      $dbScanFrac = 0;
    }

    push(@MethArray, "  --method napp:numPivot=$numPivot,numPivotIndex=$numPivotIndex,numPivotSearch=$numPivotSearch,indexThreadQty=$indexThreadQty,useSort=$useSort,dbScanFrac=$dbScanFrac");
  }

  # We should run them all at once, so that we create only index!
  RunCmd($BinDir, $DataDir, $CacheDir,
    $SpaceTypeLoc, 
    $DataNameLoc, $MaxNumDataLoc, $TestSetQtyLoc, 
    $MaxNumQueryLoc, 
    $CachePrefixLoc,
    $KLoc, 
    $OutFilePrefixLoc,
    \@MethArray);
}

sub RunOmedrank {
  my ($BinDir, $DataDir, $CacheDir,
      $SpaceTypeLoc,
      $DataNameLoc, 
      $MaxNumDataLoc, $TestSetQtyLoc, $MaxNumQueryLoc, 
      $CachePrefixLoc,
      $KLoc, 
      $OutFilePrefixLoc,
      $projType,
      $intermDim,
      $numPivot,
      $numPivotSearch,
      $minFreq,
      $dbScanFracLoc
      ) = @_;

  defined($BinDir)               or die("undefined BinDir");
  defined($DataDir)              or die("undefined DataDir");
  defined($CacheDir)             or die("undefined CacheDir");
  defined($SpaceTypeLoc)         or die("undefined SpaceTypeLoc");
  defined($projType)             or die("projType undefined!");
  defined($intermDim)            or die("intermDim undefined!");
  defined($numPivot)             or die("numPivot undefined!");
  defined($numPivotSearch)       or die("numPivotSearch undefined!");
  defined($minFreq)              or die("minFreq undefined!");
  defined($dbScanFracLoc)        or die("dbScanFracLoc undefined!");

  my @MethArray = ();

  for my $dbScanFrac (@$dbScanFracLoc) {
    push(@MethArray, "  --method omedrank:projType=$projType,intermDim=$intermDim,numPivot=$numPivot,numPivotSearch=$numPivotSearch,dbScanFrac=$dbScanFrac");
  }

  # We should run them all at once, so that we create only index!
  RunCmd($BinDir, $DataDir, $CacheDir,
    $SpaceTypeLoc, 
    $DataNameLoc, $MaxNumDataLoc, $TestSetQtyLoc, 
    $MaxNumQueryLoc, 
    $CachePrefixLoc,
    $KLoc, 
    $OutFilePrefixLoc,
    \@MethArray);
}


1;
