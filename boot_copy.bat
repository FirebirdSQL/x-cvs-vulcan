cd src
type burp\backup.boot >burp\backup.cpp 
type burp\restore.boot >burp\restore.cpp
type dsql\blob.boot >dsql\blob.cpp
type dsql\array.boot >dsql\array.cpp
type gpre\gpre_meta.boot >gpre\gpre_meta.cpp
type include\gen\autoconfig_msvc.h >include\gen\autoconfig.h
cd ..
mkdir Vulcan
copy Vulcan.sln.template Vulcan\Vulcan.sln


