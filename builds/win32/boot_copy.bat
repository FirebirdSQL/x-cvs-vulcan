cd ..\..\src
cp burp\backup.boot burp\backup.cpp >nul
cp burp\restore.boot burp\restore.cpp >nul
cp dsql\array.boot dsql\array.cpp >nul
cp dsql\blob.boot dsql\blob.cpp >nul
cp gpre\gpre_meta.boot gpre\gpre_meta.cpp >nul
cp include\gen\autoconfig_msvc.h include\gen\autoconfig.h
cd ..
mkdir Vulcan
cd builds\win32
cp Vulcan.sln.template ..\..\Vulcan\Vulcan.sln




