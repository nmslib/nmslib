#!/bin/bash
. ./lib.sh

cat <<EOF
This data set is a part of the Metric Spaces Library.
If you use it, please, consider citing:

@misc{LibMetricSpace, 
  Author =    {K.~Figueroa and G.{}~Navarro and E.~Ch\'avez}, 
  Keywords =  {Metric Spaces, similarity searching}, 
  Lastchecked = {August 18, 2012}, 
  Note = {Available at 
         {\url{http://www.sisap.org/Metric\_Space\_Library.html}}}, 
  Title = {\mbox{Metric Spaces Library}}, 
  Year =  {2007}
} 

EOF

Download http://boytsov.info/datasets/colors112.txt.gz colors112.txt.gz
CheckMD5 colors112.txt.gz 0f37b63dd93b4856ca364fb843847a59
