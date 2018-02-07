#/bin/bash
tmpn=`mktemp`

for suff in h cc cpp c ; do
  for f in `find . -name *.$suff` ; do
    echo $f
    scripts/update_file_header.py $f $tmpn
    if [ "$?" = "0" ] ; then
      cat $tmpn > $f
    fi
  done
done
rm $tmpn

