@echo off
rem A nice scripting guide is at http://steve-jansen.github.io/guides/windows-batch-scripting/index.html
setlocal ENABLEEXTENSIONS 
SETLOCAL ENABLEDELAYEDEXPANSION
set me=%~n0
set parent=%~dp0

%
set DATA_FILE=%1

if "%DATA_FILE%" == "" (
  echo "Specify a test file (1st arg)"
  exit /B 1
)

if not exist "%DATA_FILE%" (
  echo "The following data file doesn't exist: %DATA_FILE%"
  exit /B 1
)

set SPACE=%2

if "%SPACE%" == "" (
  echo "Specify the space (2d arg)"
  exit /B 1
)

set THREAD_QTY=%3

if  "%THREAD_QTY%" == "" (
  echo "Specify the number of test threads (3d arg)"
  exit /B 1
)

echo "Data file %DATA_FILE% Space: %SPACE% # of threads %THREAD_QTY%"

set TEST_SET_QTY=2
set QUERY_QTY=500
set RESULT_FILE=output_file
set K=10
set LOG_FILE_PREFIX=log_%K%
set BIN_DIR=..\similarity_search\Release\
set GS_CACHE_DIR=gs_cache
if not exist %GS_CACHE_DIR% (
  mkdir %GS_CACHE_DIR%
)
set CACHE_PREFIX_GS=!GS_CACHE_DIR!\test_run.sp=!SPACE!_tq=!THREAD_QTY!

del /Q !RESULT_FILE!*
del /Q !LOG_FILE_PREFIX!*
del /Q !CACHE_PREFIX_GS!*
echo del /Q !RESULT_FILE!*
echo del /Q !LOG_FILE_PREFIX!*
echo del /Q !CACHE_PREFIX_GS!*

set COMMON_ARGS=-s %SPACE% -i %DATA_FILE% -g %CACHE_PREFIX_GS% -b %TEST_SET_QTY% -Q  %QUERY_QTY%   --outFilePrefix %RESULT_FILE% --threadTestQty %THREAD_QTY% -k %K%

set ERRORLEVEL=0
set LN=1

rem exit /B exit only the function, not the script and there seems to be no good standard way to exit the script
call :do_run 0 "napp" "-c numPivot=512,numPivotIndex=64 " 0 "-t numPivotSearch=40 -t numPivotSearch=42 -t numPivotSearch=44 -t numPivotSearch=46 -t numPivotSearch=48" "napp_%SPACE%.index" 
if ERRORLEVEL 1 (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)
call :do_run 1 "sw-graph" "-c NN=10 " 0 "-t efSearch=10 -t efSearch=20 -t efSearch=40 -t efSearch=80 -t efSearch=160 -t efSearch=240" "sw-graph_%SPACE%.index" 
if ERRORLEVEL 1 (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)
set HNSW_INDEX="hnsw_%SPACE%.index"
call :do_run 1 "hnsw" "-c M=10 " 1 "-t efSearch=10 -t efSearch=20 -t efSearch=40 -t efSearch=80 -t efSearch=160 -t efSearch=240" %HNSW_INDEX% 
if ERRORLEVEL 1 (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)
call :do_run 1 "vptree" "-c tuneK=%K%,bucketSize=50,desiredRecall=0.99,chunkBucket=1 "  0
if ERRORLEVEL 1 (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)
call :do_run 1 "vptree" "-c tuneK=%K%,bucketSize=50,desiredRecall=0.95,chunkBucket=1 " 0
if ERRORLEVEL 1 (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)
call :do_run 1 "vptree" "-c tuneK=%K%,bucketSize=50,desiredRecall=0.9,chunkBucket=1 "  0
if ERRORLEVEL 1 (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)

echo "Finished successfully!"

exit /B 0

:do_run

set DO_APPEND=%~1

if "%DO_APPEND%" == "" (
  echo "Specify DO_APPEND (1st arg of the function do_run)"
  exit /B 1
)
if "%DO_APPEND%" == "1" (
  set APPEND_FLAG= -a 1 
)

set METHOD_NAME=%~2
if "%METHOD_NAME%" == "" (
  echo "Specify METHOD_NAME (2d arg of the function do_run)"
  exit /B 1
)
set INDEX_ARGS=%~3
if "%INDEX_ARGS%" == "" (
  echo "Specify INDEX_ARGS (3d arg of the function do_run)"
  exit /B 1
)
set RECALL_ONLY=%~4 
if "%RECALL_ONLY%" == "" ( 
  echo "Specify the RECALL_ONLY flag (4th argument of the function do_run)" 
  exit /B 1
)
set QUERY_ARGS=%~5
set INDEX_NAME=%~6

echo "Method name %METHOD_NAME% Index name: %INDEX_NAME%"

if "%INDEX_NAME%" == "" (
  set CMD=%BIN_DIR%\experiment.exe %COMMON_ARGS% -m %METHOD_NAME% %APPEND_FLAG% %INDEX_ARGS% %QUERY_ARGS% -l %LOG_FILE_PREFIX%.%LN% --recallOnly %RECALL_ONLY%
  echo "Command to execute: !CMD!"
  !CMD!

  if ERRORLEVEL 1 (
    echo "====================================="
    echo "Command failed: !CMD!
    echo "====================================="
    exit /B 1
  )

  echo ""
) else (
  set CMD=%BIN_DIR%\experiment.exe %COMMON_ARGS% -m %METHOD_NAME% %APPEND_FLAG% %INDEX_ARGS% -l %LOG_FILE_PREFIX%_index.%LN% -S %INDEX_NAME% --recallOnly %RECALL_ONLY%
  echo "Command to execute: !CMD!"
  !CMD!

  if ERRORLEVEL 1 (
    echo "====================================="
    echo "Command failed: !CMD!
    echo "====================================="
    exit /B 1
  )
  set CMD=%BIN_DIR%\experiment.exe %COMMON_ARGS% -m %METHOD_NAME% %APPEND_FLAG% %QUERY_ARGS% -l %LOG_FILE_PREFIX%_index.%LN% -L %INDEX_NAME% --recallOnly %RECALL_ONLY% 
  echo "Command to execute: !CMD!"
  !CMD!

  if ERRORLEVEL 1 (
    echo "====================================="
    echo "Command failed: !CMD!
    echo "====================================="
    exit /B 1
  )
  echo ""
)

set /A LN=LN+1

exit /B 0


