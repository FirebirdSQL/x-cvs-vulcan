Building Vulcan on Mac OS X
---------------------------

The generic posix script - build_vulcan.sh - manages the build.

1/ Open a console prompt on the Vulcan source tree

2/ cd to builds/posix

3/ change permissions (chmod u+x) to make  build_vulcan.sh executable 
   (if necessary)

4/ execute script, making sure to pass the appropriate platform: 

     ./build_vulcan.sh --platform=darwin

5/ Or, for more help try:

     ./build_vulcan.sh --help
  
