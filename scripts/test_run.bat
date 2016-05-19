@echo off
rem A nice scripting guide is at http://steve-jansen.github.io/guides/windows-batch-scripting/index.html
setlocal ENABLEEXTENSIONS
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

set THREAD_QTY=%2

if  "%THREAD_QTY%" == "" (
  echo "Specify the number of test threads (2d arg)"
  exit /B 1
)

set TEST_SET_QTY=2
set QUERY_QTY=500
set RESULT_FILE=output_file
set K=10
set SPACE=l2
set LOG_FILE_PREFIX=log_%K%
set BIN_DIR=..\similarity_search\x64\Release\experiment
set GS_CACHE_DIR=gs_cache
if not exist %GS_CACHE_DIR% (
  mkdir %GS_CACHE_DIR%
)
set CACHE_PREFIX_GS=%GS_CACHE_DIR%/test_run.tq=%THREAD_QTY%

del /Q %RESULT_FILE%*
del /Q %LOG_FILE_PREFIX%*

set ARG_PREFIX=-s %SPACE% -i %DATA_FILE% -g %CACHE_PREFIX_GS% -b %TEST_SET_QTY% -Q  %QUERY_QTY% -k %K%  --outFilePrefix %RESULT_FILE% --threadTestQty %THREAD_QTY% 

set LN=1

rem exit /B exit only the function, not the script and there seems to be no good standard way to exit the script
call :do_run 0 "-m napp -c numPivot=512,numPivotIndex=64 -t numPivotSearch=40 -t numPivotSearch=42 -t numPivotSearch=44 -t numPivotSearch=46 -t numPivotSearch=48"
if /I "%ERRORLEVEL%" NEQ "0" (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)
call :do_run 1 "-m sw-graph -c NN=10 -t efSearch=10 -t efSearch=20 -t efSearch=40 -t efSearch=80 -t efSearch=160 -t efSearch=240"
if /I "%ERRORLEVEL%" NEQ "0" (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)
call :do_run 1 "-m hnsw -c M=10 -t efSearch=10 -t efSearch=20 -t efSearch=40 -t efSearch=80 -t efSearch=160 -t efSearch=240"
if /I "%ERRORLEVEL%" NEQ "0" (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)
call :do_run 1 "-m vptree -c tuneK=%K%,bucketSize=50,desiredRecall=0.99,chunkBucket=1 "
if /I "%ERRORLEVEL%" NEQ "0" (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)
call :do_run 1 "-m vptree -c tuneK=%K%,bucketSize=50,desiredRecall=0.975,chunkBucket=1 "
if /I "%ERRORLEVEL%" NEQ "0" (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)
call :do_run 1 "-m vptree -c tuneK=%K%,bucketSize=50,desiredRecall=0.95,chunkBucket=1 "
if /I "%ERRORLEVEL%" NEQ "0" (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)
call :do_run 1 "-m vptree -c tuneK=%K%,bucketSize=50,desiredRecall=0.925,chunkBucket=1 "
if /I "%ERRORLEVEL%" NEQ "0" (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)
call :do_run 1 "-m vptree -c tuneK=%K%,bucketSize=50,desiredRecall=0.9,chunkBucket=1 "
if /I "%ERRORLEVEL%" NEQ "0" (
  echo "====================================="
  echo "Failure!"
  exit /B 1
)

echo "Finished successfully!"

exit /B 0

:do_run

set DO_APPEND=%1
set ADD_ARG=%~2
set ARGS=%ARG_PREFIX% %ADD_ARG%

if "%DO_APPEND%" == "1" (
  set APPEND_FLAG= -a  
)

set cmd=%BIN_DIR%\experiment.exe %ARGS% %APPEND_FLAG% -l %LOG_FILE_PREFIX%.%LN%
echo Command to execute:
echo %cmd%
%cmd%

if /I "%ERRORLEVEL%" NEQ "0" (
  echo "====================================="
  echo "Command failed: %cmd%
  echo "====================================="
  exit /B 1
)


set /A LN=LN+1


exit /B 0


