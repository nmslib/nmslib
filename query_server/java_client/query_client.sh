#/bin/bash
tr=`mktemp client.XXX`
echo "mvn compile exec:java -Dexec.mainClass=edu.cmu.lti.oaqa.apps.Client -Dexec.args='$@' " > $tr
. $tr
rm $tr
